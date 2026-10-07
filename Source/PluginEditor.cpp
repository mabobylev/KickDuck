#include "PluginEditor.h"

KickDuckAudioProcessorEditor::KickDuckAudioProcessorEditor (KickDuckAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    mixSlider = std::make_unique<juce::Slider>();
    mixSlider->setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    mixSlider->setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 20);

    outGainSlider = std::make_unique<juce::Slider>();
    outGainSlider->setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    outGainSlider->setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 20);

    bypassButton = std::make_unique<juce::ToggleButton>();
    bypassButton->setButtonText ("IN/OUT");

    addAndMakeVisible (mixSlider.get());
    addAndMakeVisible (outGainSlider.get());
    addAndMakeVisible (bypassButton.get());

    mixAttachment.reset (new juce::AudioProcessorValueTreeState::SliderAttachment (p.apvts, "mix", *mixSlider));
    outGainAttachment.reset (new juce::AudioProcessorValueTreeState::SliderAttachment (p.apvts, "outGain", *outGainSlider));
    bypassAttachment.reset (new juce::AudioProcessorValueTreeState::ButtonAttachment (p.apvts, "bypass", *bypassButton));

    openGLContext.setComponentPaintingEnabled (true);
    openGLContext.attachTo (*this);

    startTimerHz (60);

    setSize (600, 400);
}

KickDuckAudioProcessorEditor::~KickDuckAudioProcessorEditor()
{
    openGLContext.detach();
    stopTimer();
}

void KickDuckAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::darkslategrey);
    g.setFont (juce::Font ("Inter", "Bold", 16.0f));
    g.setColour (juce::Colours::white);
    g.drawFittedText ("KickDuck", getLocalBounds().removeFromTop (30), juce::Justification::centred, 1);

    g.setColour (juce::Colours::cyan.withAlpha(0.8f));
    g.strokePath (scopePath, juce::PathStrokeType (1.5f));
}

void KickDuckAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds().reduced (10);
    auto slidersArea = bounds.removeFromTop (100);
    mixSlider->setBounds (slidersArea.removeFromLeft (getWidth() / 2 - 20));
    outGainSlider->setBounds (slidersArea.removeFromLeft (getWidth() / 2 - 20));
    bypassButton->setBounds (bounds.removeFromTop (30).withSizeKeepingCentre (80, 25));
}

void KickDuckAudioProcessorEditor::timerCallback()
{
    updateScopeData();
    repaint();
}

void KickDuckAudioProcessorEditor::updateScopeData()
{
    juce::AbstractFifo::ScopedRead read (audioProcessor.scopeFifo, 1);
    if (read.blockSize1 > 0)
    {
        scopePath.clear();
        auto bounds = getLocalBounds().toFloat().reduced(10);
        float w = bounds.getWidth();
        float h = bounds.getHeight();

        scopePath.startNewSubPath(0, h * 0.5f);
        float val = audioProcessor.scopeData[read.startIndex1];
        scopePath.lineTo(w, (1.0f - val) * h);
    }
}
