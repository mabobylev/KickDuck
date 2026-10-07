#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class KickDuckAudioProcessorEditor  : public juce::AudioProcessorEditor,
                                      private juce::Timer
{
public:
    KickDuckAudioProcessorEditor (KickDuckAudioProcessor&);
    ~KickDuckAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    KickDuckAudioProcessor& audioProcessor;

    // GUI
    std::unique_ptr<juce::Slider> mixSlider;
    std::unique_ptr<juce::Slider> outGainSlider;
    std::unique_ptr<juce::ToggleButton> bypassButton;

    // Attachments (теперь обычные переменные, а не unique_ptr в списке инициализации)
    juce::AudioProcessorValueTreeState::SliderAttachment mixAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment outGainAttachment;
    juce::AudioProcessorValueTreeState::ButtonAttachment bypassAttachment;

    // OpenGL
    juce::OpenGLContext openGLContext;

    // Осциллограф
    void timerCallback() override;
    void updateScopeData();
    juce::Path scopePath;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (KickDuckAudioProcessorEditor)
};
