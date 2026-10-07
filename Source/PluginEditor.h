/*
  ==============================================================================

    PluginEditor.h
    Created: 7 Oct 2026
    KickDuck Audio Processor Editor

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
class KickDuckAudioProcessorEditor  : public juce::AudioProcessorEditor,
                                      private juce::Timer
{
public:
    KickDuckAudioProcessorEditor (KickDuckAudioProcessor&);
    ~KickDuckAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    KickDuckAudioProcessor& audioProcessor;

    // GUI
    std::unique_ptr<juce::Slider> mixSlider;
    std::unique_ptr<juce::Slider> outGainSlider;
    std::unique_ptr<juce::ToggleButton> bypassButton;

    // Attachments
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> mixAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> outGainAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttachment;

    // OpenGL
    juce::OpenGLContext openGLContext;

    // --- ОСЦИЛЛОГРАФ ---
    void timerCallback() override;
    void updateScopeData();
    juce::Path scopePath;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (KickDuckAudioProcessorEditor)
};
