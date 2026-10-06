#pragma once

#include <juce_dsp/juce_dsp.h>
#include <juce_audio_processors/juce_audio_processors.h>

class KickDuckAudioProcessor : public juce::AudioProcessor
{
public:
    KickDuckAudioProcessor();

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override             { return true; }
    const juce::String getName() const override { return "KickDuck"; }
    bool acceptsMidi() const override           { return false; }
    bool producesMidi() const override          { return false; }
    bool isMidiEffect() const override          { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override                        { return 1; }
    int getCurrentProgram() override                     { return 0; }
    void setCurrentProgram (int) override                {}
    const juce::String getProgramName (int) override     { return "Default"; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& dest) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

    static constexpr int frameSize = 131072;   // кадр: до ~3 с при 44.1 кГц

    std::atomic<float> inLevel { 1.0e-5f };
    std::atomic<float> outLevel { 1.0e-5f };
    std::atomic<float> scLevel  { 1.0e-5f };

    std::atomic<float> duckLenSamples  { 0.0f };
    std::atomic<float> bpmAtomic       { 120.0f };
    std::atomic<float> sampleRateAtomic { 44100.0f };

    // тройная буферизация кадров: аудио пишет в один, редактор читает опубликованный
    std::atomic<int> framePublished { -1 };
    std::atomic<int> frameVersion   { 0 };
    juce::SpinLock frameLock;
    float frameMain[3][frameSize] = {};
    float frameOut [3][frameSize] = {};
    float frameSc  [3][frameSize] = {};
    float frameGr  [3][frameSize] = {};
    int   frameLens[3] = { 0, 0, 0 };

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void publishFrame();

    double sr = 44100.0;
    float env = 0.0f;
    float grSmoothed = 0.0f;
    float lastHpf = -1.0f;
    double freeBeatPos = 0.0;
    double lastBeatFloor = -1.0;

    int frameWrite = 0;
    int frameFree  = 1;
    int framePos   = 0;

    std::atomic<float>* pThr = nullptr;
    std::atomic<float>* pRatio = nullptr;
    std::atomic<float>* pAtk = nullptr;
    std::atomic<float>* pRel = nullptr;
    std::atomic<float>* pKnee = nullptr;
    std::atomic<float>* pDepth = nullptr;
    std::atomic<float>* pMix = nullptr;
    std::atomic<float>* pHpf = nullptr;
    std::atomic<float>* pOut = nullptr;
    std::atomic<float>* pMode = nullptr;
    std::atomic<float>* pLen = nullptr;
    std::atomic<float>* pShape = nullptr;

    juce::dsp::IIR::Filter<float> scHP[2];

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (KickDuckAudioProcessor)
};

