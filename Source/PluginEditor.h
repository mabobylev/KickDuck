#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

class KickDuckLookAndFeel : public juce::LookAndFeel_V4
{
public:
    void drawRotarySlider (juce::Graphics&, int, int, int, int, float,
                           float, float, juce::Slider&) override;
};

class KickDuckAudioProcessorEditor : public juce::AudioProcessorEditor,
                                     private juce::Timer
{
public:
    explicit KickDuckAudioProcessorEditor (KickDuckAudioProcessor& p);
    ~KickDuckAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;

private:
    void timerCallback() override;
    void drawMeter (juce::Graphics& g, juce::Rectangle<float> area,
                    float db, const juce::String& label);
    void drawGrMeter (juce::Graphics& g, juce::Rectangle<float> area,
                      float db, const juce::String& label);
    void drawWaveforms (juce::Graphics& g, juce::Rectangle<float> area);

    juce::Rectangle<float> getWaveArea() const;
    bool isKickMode() const;
    bool getShapeHandlePos (juce::Rectangle<float> area, float& hx, float& hy) const;
    void setParamsFromMouse (const juce::MouseEvent& e);
    void setShapeFromMouse (const juce::MouseEvent& e);
    void updateModeUI();
    void applyPreset (int index);

    KickDuckAudioProcessor& proc;
    KickDuckLookAndFeel lnf;

    static constexpr int numKnobs = 11;
    juce::OwnedArray<juce::Slider> sliders;
    juce::OwnedArray<juce::Label> labels;
    juce::OwnedArray<juce::AudioProcessorValueTreeState::SliderAttachment> attachments;

    juce::ComboBox presetCombo;
    juce::TextButton dspModeButton { "COMP" };
    juce::TextButton displayButton { "IN" };
    bool showOutput = false;

    bool dragging = false;
    bool draggingShape = false;
    bool overWaveArea = false;
    bool overShapeHandle = false;

    static constexpr int frameSize = KickDuckAudioProcessor::frameSize;

    // локальная копия последнего опубликованного кадра
    float frameMain[frameSize] = {};
    float frameOut [frameSize] = {};
    float frameSc  [frameSize] = {};
    float frameGr  [frameSize] = {};
    int   frameLen = 0;
    int   lastFrameVersion = -1;

    float inDb = -100.0f, outDb = -100.0f, grDb = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (KickDuckAudioProcessorEditor)
};

