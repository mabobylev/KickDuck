#include "PluginProcessor.h"
#include "PluginEditor.h"

KickDuckAudioProcessor::KickDuckAudioProcessor()
    : AudioProcessor (BusesProperties()
        .withInput  ("Input",     juce::AudioChannelSet::stereo(), true)
        .withOutput ("Output",    juce::AudioChannelSet::stereo(), true)
        .withInput  ("Sidechain", juce::AudioChannelSet::stereo(), false)),
      apvts (*this, nullptr, "PARAMS", createLayout())
{
    pIn    = apvts.getRawParameterValue ("input");
    pThr   = apvts.getRawParameterValue ("threshold");
    pRatio = apvts.getRawParameterValue ("ratio");
    pAtk   = apvts.getRawParameterValue ("attack");
    pRel   = apvts.getRawParameterValue ("release");
    pKnee  = apvts.getRawParameterValue ("knee");
    pDepth = apvts.getRawParameterValue ("depth");
    pMix   = apvts.getRawParameterValue ("mix");
    pHpf   = apvts.getRawParameterValue ("hpf");
    pOut   = apvts.getRawParameterValue ("output");
    pMode  = apvts.getRawParameterValue ("mode");
    pLen   = apvts.getRawParameterValue ("kicklen");
    pShape = apvts.getRawParameterValue ("shape");
}

juce::AudioProcessorValueTreeState::ParameterLayout KickDuckAudioProcessor::createLayout()
{
    using P = juce::AudioParameterFloat;
    juce::AudioProcessorValueTreeState::ParameterLayout l;

    auto add = [&] (juce::String id, juce::String name, float lo, float hi,
                    float def, float step, juce::String suffix, float skew = 1.0f)
    {
        juce::NormalisableRange<float> r (lo, hi, step);
        if (skew != 1.0f) r.setSkewForCentre (skew);
        l.add (std::make_unique<P> (juce::ParameterID { id, 1 }, name, r, def,
                                    juce::AudioParameterFloatAttributes().withLabel (suffix)));
    };

    add ("input", "Input", -12.0f, 12.0f, 0.0f, 0.1f, "dB");

    // COMP-режим
    add ("threshold", "Threshold", -60.0f, 0.0f,  -24.0f, 0.1f,  "dB");
    add ("ratio",     "Ratio",       1.0f, 20.0f,  6.0f,  0.1f,  ":1");
    add ("attack",    "Attack",      0.1f, 100.0f, 5.0f,  0.1f,  "ms",  10.0f);
    add ("release",   "Release",     5.0f, 1000.0f, 120.0f, 1.0f, "ms",  100.0f);
    add ("knee",      "Knee",        0.0f, 24.0f,  6.0f,  0.1f,  "dB");

    // общие
    add ("depth",  "Max duck",  0.0f, 24.0f,   9.0f, 0.1f, "dB");
    add ("mix",    "Mix",       0.0f, 100.0f, 100.0f, 1.0f, "%");
    add ("hpf",    "SC HPF",   20.0f, 2000.0f, 60.0f, 1.0f, "Hz", 200.0f);
    add ("output", "Output", -12.0f, 12.0f,   0.0f, 0.1f,  "dB");

    // KICK-режим
    add ("shape",   "Shape",  0.5f, 8.0f,     3.0f, 0.01f,  "");
    add ("kicklen", "Length", 0.03125f, 8.0f, 0.5f, 0.001f, "bt", 0.5f);

    l.add (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "mode", 1 },
                                                       "Kick mode", false));

    return l;
}

bool KickDuckAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    if (layouts.getMainInputChannelSet() != layouts.getMainOutputChannelSet())
        return false;

    const auto sc = layouts.getChannelSet (true, 1);
    return sc.isDisabled() || sc == juce::AudioChannelSet::stereo();
}

void KickDuckAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sr = sampleRate;
    env = 0.0f;
    grSmoothed = 0.0f;
    lastHpf = -1.0f;
    freeBeatPos = 0.0;
    lastBeatFloor = -1.0;
    framePos = 0;
    frameWrite = 0;
    frameFree = 1;

    sampleRateAtomic.store ((float) sampleRate, std::memory_order_relaxed);
    framePublished.store (-1, std::memory_order_relaxed);
    frameVersion.store (0, std::memory_order_relaxed);
    grLevel.store (0.0f, std::memory_order_relaxed);

    juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32) samplesPerBlock, 1 };
    for (auto& f : scHP)
        f.prepare (spec);
}

void KickDuckAudioProcessor::publishFrame()
{
    if (framePos <= 0)
        return;

    {
        juce::SpinLock::ScopedLockType sl (frameLock);
        frameLens[frameWrite] = framePos;
        framePublished.store (frameWrite, std::memory_order_release);
        frameVersion.fetch_add (1, std::memory_order_relaxed);

        const int prev = frameWrite;
        frameWrite = frameFree;
        frameFree  = prev;
        framePos = 0;
    }
}

void KickDuckAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const float thr     = pThr->load();
    const float ratio   = pRatio->load();
    const float attack  = pAtk->load();
    const float release = pRel->load();
    const float knee    = pKnee->load();
    const float depth   = pDepth->load();
    const float mix     = pMix->load() * 0.01f;
    const float outGain = pOut->load();
    const float shape   = pShape->load();
    const bool  kickMode = pMode->load() > 0.5f;

    juce::Optional<juce::AudioPlayHead::PositionInfo> posInfo;
    double bpm = 120.0;
    bool havePlay = false;

    if (auto* ph = getPlayHead())
        if ((posInfo = ph->getPosition()).hasValue())
        {
            if (auto bpmOpt = posInfo->getBpm())
                bpm = *bpmOpt;
            havePlay = posInfo->getIsPlaying();
        }

    bpmAtomic.store ((float) bpm, std::memory_order_relaxed);

    const bool usePpq = havePlay && posInfo->getPpqPosition().hasValue();
    const double beatDur = 60.0 / bpm;
    const float lenBeats = pLen->load();
    const float lenSec = (float) (beatDur * (double) lenBeats);

    const float kickCoef = std::exp (-1.0f / (float) (sr * 0.0015));
    const float kickDispCoef = std::exp (-1.0f / (float) (sr * 0.04));
    float kickDisp = 0.0f;

    duckLenSamples.store (lenSec * (float) sr, std::memory_order_relaxed);

    auto mainBuf = getBusBuffer (buffer, true, 0);
    auto scBuf   = getBusBuffer (buffer, true, 1);
    const int numMain = getChannelCountOfBus (true, 0);
    const int numSc   = getChannelCountOfBus (true, 1);
    const int n = buffer.getNumSamples();

    // входной уровень применяется до всей обработки (метры и триггеры это учитывают)
    const float inGain = juce::Decibels::decibelsToGain (pIn->load());
    for (int ch = 0; ch < mainBuf.getNumChannels(); ++ch)
    {
        auto* d = mainBuf.getWritePointer (ch);
        for (int i = 0; i < n; ++i)
            d[i] *= inGain;
    }

    if (numSc > 0 && ! kickMode)
    {
        const float hpfHz = pHpf->load();
        if (hpfHz != lastHpf)
        {
            auto coeffs = juce::dsp::IIR::Coefficients<float>::makeHighPass (sr, hpfHz);
            for (auto& f : scHP)
                f.coefficients = coeffs;
            lastHpf = hpfHz;
        }

        for (int ch = 0; ch < numSc; ++ch)
        {
            auto* d = scBuf.getWritePointer (ch);
            for (int i = 0; i < n; ++i)
                d[i] = scHP[ch].processSample (d[i]);
        }
    }

    const float atkCoef = std::exp (-1.0f / (float) (sr * attack  * 0.001));
    const float relCoef = std::exp (-1.0f / (float) (sr * release * 0.001));
    const float grCoef  = std::exp (-1.0f / (float) (sr * 0.005));

    float peakIn = 0.0f, peakOut = 0.0f, peakSc = 0.0f, peakDuck = 0.0f;
    float scDisp = 0.0f;
    float prevGr = grSmoothed;

    for (int i = 0; i < n; ++i)
    {
        float gr = 0.0f;

        if (! kickMode)
        {
            float sc = 0.0f;
            if (numSc > 0)
                for (int ch = 0; ch < numSc; ++ch)
                    sc = juce::jmax (sc, std::abs (scBuf.getSample (ch, i)));
            else
                for (int ch = 0; ch < numMain; ++ch)
                    sc = juce::jmax (sc, std::abs (mainBuf.getSample (ch, i)));

            env = (sc > env) ? env + (sc - env) * atkCoef
                             : env + (sc - env) * relCoef;

            const float envDb = juce::Decibels::gainToDecibels (juce::jmax (env, 1.0e-6f));
            const float over  = envDb - thr;

            if (over <= -0.5f * knee)
                gr = 0.0f;
            else if (over < 0.5f * knee)
            {
                const float t = over + 0.5f * knee;
                gr = (1.0f / ratio - 1.0f) * t * t / (2.0f * knee);
            }
            else
                gr = over * (1.0f / ratio - 1.0f);

            gr = juce::jmax (gr, -depth);
            grSmoothed += (gr - grSmoothed) * grCoef;

            if (prevGr > -0.1f && grSmoothed <= -0.1f)
                publishFrame();

            peakSc = juce::jmax (peakSc, sc);
            scDisp = sc;
        }
        else
        {
            double beatPos;
            if (usePpq)
                beatPos = *posInfo->getPpqPosition() + (double) i * bpm / (60.0 * (double) sr);
            else
                beatPos = 0.0;

            float tn = 0.0f;
            const double frac = beatPos - std::floor (beatPos);
            if (lenSec > 0.0f)
                tn = juce::jlimit (0.0f, 1.0f, (float) (frac * beatDur / (double) lenSec));

            const float target = -depth * std::pow (1.0f - tn, shape);
            grSmoothed += (target - grSmoothed) * kickCoef;

            const double bf = std::floor (beatPos);
            if (usePpq && bf != lastBeatFloor)
            {
                lastBeatFloor = bf;
                kickDisp = 1.0f;
                publishFrame();
            }
            kickDisp *= kickDispCoef;

            peakSc = juce::jmax (peakSc, kickDisp);
            scDisp = kickDisp;
        }

        grSmoothed = juce::jlimit (-depth, 0.0f, grSmoothed);
        prevGr = grSmoothed;
        peakDuck = juce::jmax (peakDuck, -grSmoothed);

        const float g   = juce::Decibels::decibelsToGain (grSmoothed + outGain);
        const float wet = g * mix + (1.0f - mix);

        float pre = 0.0f;
        for (int ch = 0; ch < numMain; ++ch)
            pre += mainBuf.getSample (ch, i);
        if (numMain > 0) pre /= (float) numMain;

        peakIn  = juce::jmax (peakIn,  std::abs (pre));
        peakOut = juce::jmax (peakOut, std::abs (pre) * wet);

        for (int ch = 0; ch < numMain; ++ch)
            mainBuf.setSample (ch, i, mainBuf.getSample (ch, i) * wet);

        if (framePos < frameSize)
        {
            frameMain[frameWrite][framePos] = pre;
            frameOut [frameWrite][framePos] = pre * wet;
            frameSc  [frameWrite][framePos] = scDisp;
            frameGr  [frameWrite][framePos] = grSmoothed;
            ++framePos;
        }
    }

    const float decay = std::exp (-(float) n / (float) (sr * 0.4f));
    auto updateLevel = [&] (std::atomic<float>& lvl, float peak)
    {
        lvl.store (juce::jmax (peak, lvl.load (std::memory_order_relaxed) * decay),
                   std::memory_order_relaxed);
    };
    updateLevel (inLevel,  peakIn);
    updateLevel (outLevel, peakOut);
    updateLevel (scLevel,  peakSc);

    const float grDecay = std::exp (-(float) n / (float) (sr * 0.3f));
    grLevel.store (juce::jmax (peakDuck,
                   grLevel.load (std::memory_order_relaxed) * grDecay),
                   std::memory_order_relaxed);
}

void KickDuckAudioProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    juce::MemoryOutputStream mos (dest, false);
    apvts.state.writeToStream (mos);
}

void KickDuckAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    auto tree = juce::ValueTree::fromXml (juce::String::createStringFromData (data, sizeInBytes));
    if (tree.isValid())
        apvts.replaceState (tree);
}

juce::AudioProcessorEditor* KickDuckAudioProcessor::createEditor()
{
    return new KickDuckAudioProcessorEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new KickDuckAudioProcessor();
}

