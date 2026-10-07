#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
KickDuckAudioProcessor::KickDuckAudioProcessor()
     : apvts (*this, nullptr, "Parameters", {
          std::make_unique<juce::AudioParameterFloat> ("mix", "Mix", juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 1.0f),
          std::make_unique<juce::AudioParameterFloat> ("maxDuck", "Max Duck", juce::NormalisableRange<float> (0.0f, 24.0f, 0.1f), 12.0f),
          std::make_unique<juce::AudioParameterFloat> ("outGain", "Out Gain", juce::NormalisableRange<float> (-12.0f, 12.0f, 0.1f), 0.0f),
          std::make_unique<juce::AudioParameterFloat> ("scHpf", "SC HPF", juce::NormalisableRange<float> (20.0f, 2000.0f, 1.0f, 0.3f), 80.0f),
          std::make_unique<juce::AudioParameterBool> ("bypass", "Bypass", false)
     })
{
    mixParam = apvts.getRawParameterValue ("mix");
    maxDuckParam = apvts.getRawParameterValue ("maxDuck");
    outGainParam = apvts.getRawParameterValue ("outGain");
    scHpfParam = apvts.getRawParameterValue ("scHpf");
    bypassParam = apvts.getRawParameterValue ("bypass"); // Теперь указывает на bool

    scHpfChain.get<0>().coefficients = juce::dsp::IIR::Coefficients<float>::makeHighPass (48000.0, 80.0f);
    scopeData.resize (scopeFifo.getTotalSize());
}

// ... (деструктор и остальные методы JUCE boilerplate остаются прежними) ...

void KickDuckAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::dsp::ProcessSpec spec { sampleRate, static_cast<juce::uint32> (samplesPerBlock), 2 };
    scHpfChain.prepare (spec);
    bypassGain.prepare (spec);
    outputGain.prepare (spec);

    bypassGain.setRampDurationSeconds (0.02);
    outputGain.setRampDurationSeconds (0.02);
}

void KickDuckAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    
    // Clear extra channels
    for (int i = getTotalNumInputChannels(); i < getTotalNumOutputChannels(); ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // --- 1. Clickless Bypass ---
    bool bypassState = bypassParam->load();
    if (bypassState && bypassGain.getTargetValue() > 0.0f)
        bypassGain.setTargetValue (0.0f);
    else if (!bypassState && bypassGain.getTargetValue() < 1.0f)
        bypassGain.setTargetValue (1.0f);

    // --- 2. Режим KICK ---
    if (currentMode.load() == DuckMode::KICK)
    {
        if (auto* playHead = getPlayHead())
        {
            juce::AudioPlayHead::CurrentPositionInfo posInfo;
            playHead->getCurrentPosition (posInfo);
            if (!posInfo.isPlaying || std::abs (posInfo.editOriginTime - lastPlayheadSample) > getSampleRate() * 0.1)
            {
                phaseAccumulator = fmod (posInfo.editOriginTime * posInfo.bpm / 60.0, 1.0) * 2.0 * juce::MathConstants<double>::pi;
                wasPlaying = false;
            }
            else if (posInfo.isPlaying && !wasPlaying)
            {
                phaseAccumulator = 0.0;
            }
            wasPlaying = posInfo.isPlaying;
            lastPlayheadSample = posInfo.editOriginTime;
        }
    }

    // --- 3. Сглаживание ---
    targetMix = mixParam->load();
    currentMix += 0.05f * (targetMix - currentMix);
    targetGain = juce::Decibels::decibelsToGain (outGainParam->load());
    currentGain += 0.05f * (targetGain - currentGain);

    if (std::abs (targetHpfFreq - scHpfParam->load()) > 1.0f)
    {
        targetHpfFreq = scHpfParam->load();
        auto newCoeffs = juce::dsp::IIR::Coefficients<float>::makeHighPass (getSampleRate(), targetHpfFreq);
        scHpfChain.get<0>().coefficients = newCoeffs;
    }

    // --- 4. Обработка ---
    juce::dsp::AudioBlock<float> block (buffer);
    
    if (getBusCount (true) > 1)
    {
        if (auto* scBus = getBusBuffer (buffer, true, 1)) // Возвращает AudioBuffer<float>*
        {
            juce::dsp::AudioBlock<float> sidechainBlock (*scBus);
            scHpfChain.process (juce::dsp::ProcessContextReplacing<float> (sidechainBlock));
        }
    }
    
    juce::dsp::AudioBlock<float> mainBlock (block);

    // [ВАША ЛОГИКА ДАКИНГА ЗДЕСЬ]
    // ... (применение currentMix/maxDuck к mainBlock) ...

    // --- 5. Кроссфейд ---
    if (currentMode.load() != targetMode.load() && !isFading)
    {
        isFading = true;
        fadeCounter = 0;
    }
    if (isFading)
    {
        fadeCounter++;
        float fadePos = (float)fadeCounter / (float)fadeLengthSamples;
        if (fadePos >= 1.0f) { isFading = false; currentMode.store (targetMode.load()); }
    }

    // --- 6. Применение выхода ---
    outputGain.setGainLinear (currentGain);
    juce::dsp::ProcessContextReplacing<float> context (block);
    outputGain.process (context);
    bypassGain.process (context);

    // --- 7. ОСЦИЛЛОГРАФ (ИСПРАВЛЕНО) ---
    float maxLevel = 0.0f;
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
    {
        auto* channelData = buffer.getReadPointer(channel);
        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            float absSample = std::abs(channelData[i]);
            if (absSample > maxLevel)
                maxLevel = absSample;
        }
    }

    maxLevel = juce::jmin(1.0f, maxLevel);
    float dbValue = juce::Decibels::gainToDecibels(maxLevel, -100.0f);
    float normalized = (dbValue + 100.0f) / 100.0f;

    if (scopeFifo.getFreeSpace() > 0)
    {
        // ИСПРАВЛЕНО: используем ScopedWrite
        juce::AbstractFifo::ScopedWrite write (scopeFifo, 1);
        scopeData[write.startIndex1] = normalized;
    }
}
