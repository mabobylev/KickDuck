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

    std::unique_ptr<juce::Slider> mixSlider;
    std::unique_ptr<juce::Slider> outGainSlider;
    std::unique_ptr<juce::ToggleButton> bypassButton;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> mixAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> outGainAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttachment;

    juce::OpenGLContext openGLContext;

    void timerCallback() override;
    void updateScopeData();
    juce::Path scopePath;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (KickDuckAudioProcessorEditor)
};
