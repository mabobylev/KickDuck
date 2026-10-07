/*
  ==============================================================================

    PluginEditor.cpp
    Created: 7 Oct 2026
    KickDuck Audio Processor Editor

  ==============================================================================
*/

#include "PluginEditor.h"

//==============================================================================
KickDuckAudioProcessorEditor::KickDuckAudioProcessorEditor (KickDuckAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p),
      mixAttachment (p.apvts, "mix", *mixSlider),
      outGainAttachment (p.apvts, "outGain", *outGainSlider),
      bypassAttachment (p.apvts, "bypass", *bypassButton)
{
    addAndMakeVisible (mixSlider = std::make_unique<juce::Slider>());
    mixSlider->setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    mixSlider->setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 20);

    addAndMakeVisible (outGainSlider = std::make_unique<juce::Slider>());
    outGainSlider->setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    outGainSlider->setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 20);

    addAndMakeVisible (bypassButton = std::make_unique<juce::ToggleButton>());
    bypassButton->setButtonText ("IN/OUT");

    // OpenGL
    openGLContext.setComponentPaintingEnabled (true);
    openGLContext.attachTo (*this);

    // Timer
    startTimerHz (60);

    setSize (600, 400);
}

KickDuckAudioProcessorEditor::~KickDuckAudioProcessorEditor()
{
    openGLContext.detach();
    stopTimer();
}

//==============================================================================
void KickDuckAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::darkslategrey);

    // Современный шрифт (исправление API)
    g.setFont (juce::Font ("Inter", "Bold", 16.0f));
    g.setColour (juce::Colours::white);
    g.drawFittedText ("KickDuck", getLocalBounds().removeFromTop (30), juce::Justification::centred, 1);

    // Осциллограф
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
    float fifoBuffer[64];
    int numReady = scopeFifo.getNumReady();
    
    if (numReady > 0)
    {
        int numRead = scopeFifo.read(fifoBuffer, juce::jmin(numReady, 64));
        
        scopePath.clear();
        if (numRead > 0)
        {
            auto bounds = getLocalBounds().toFloat().reduced(10);
            float w = bounds.getWidth();
            float h = bounds.getHeight();

            scopePath.startNewSubPath(0, h * 0.5f);
            for (int i = 0; i < numRead; ++i)
            {
                float x = (float)i / (float)numRead * w;
                float y = (1.0f - fifoBuffer[i]) * h;
                scopePath.lineTo(x, y);
            }
            scopePath.startNewSubPath(w, h * 0.5f);
            scopePath.scaleToFit(bounds.getX(), bounds.getY(), bounds.getWidth(), bounds.getHeight(), false);
        }
    }
}
