/*
  ==============================================================================

    PluginProcessor.h
    Created: 7 Oct 2026
    KickDuck Audio Processor

  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include <juce_dsp/juce_dsp.h>
#include <juce_opengl/juce_opengl.h>

class KickDuckAudioProcessor  : public juce::AudioProcessor
{
public:
    KickDuckAudioProcessor();
    ~KickDuckAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

    friend class KickDuckAudioProcessorEditor;

private:
    std::atomic<float>* mixParam = nullptr;
    std::atomic<float>* maxDuckParam = nullptr;
    std::atomic<float>* outGainParam = nullptr;
    std::atomic<float>* scHpfParam = nullptr;
    std::atomic<float>* bypassParam = nullptr;

    enum class DuckMode { COMP, KICK };
    std::atomic<DuckMode> currentMode { DuckMode::COMP };
    std::atomic<DuckMode> targetMode { DuckMode::COMP };

    bool isFading = false;
    int fadeCounter = 0;
    const int fadeLengthSamples = 480;

    float currentMix = 1.0f, targetMix = 1.0f;
    float currentGain = 1.0f, targetGain = 1.0f;
    float currentHpfFreq = 20.0f, targetHpfFreq = 20.0f;

    juce::dsp::ProcessorChain<juce::dsp::IIR::Filter<float>> scHpfChain;
    juce::dsp::Gain<float> bypassGain;
    juce::dsp::Gain<float> outputGain;

    double lastPlayheadSample = 0.0;
    double phaseAccumulator = 0.0;
    bool wasPlaying = false;

    juce::dsp::AudioBlock<float> mainBlock;
    juce::dsp::AudioBlock<float> sidechainBlock;

    juce::AbstractFifo scopeFifo { 1024 };
    std::vector<float> scopeData;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (KickDuckAudioProcessor)
};
