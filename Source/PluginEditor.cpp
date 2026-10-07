#include "PluginEditor.h"

//==============================================================================
KickDuckAudioProcessorEditor::KickDuckAudioProcessorEditor (KickDuckAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p),
      // Инициализируем Attachments в теле конструктора, когда виджеты уже созданы
      mixAttachment(), outGainAttachment(), bypassAttachment()
{
    // 1. Создаем виджеты
    mixSlider = std::make_unique<juce::Slider>();
    mixSlider->setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    mixSlider->setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 20);

    outGainSlider = std::make_unique<juce::Slider>();
    outGainSlider->setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    outGainSlider->setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 20);

    bypassButton = std::make_unique<juce::ToggleButton>();
    bypassButton->setButtonText ("IN/OUT");

    // 2. ПРИОРИТЕТ: Сначала добавляем в иерархию, потом привязываем APVTS
    addAndMakeVisible (mixSlider.get());
    addAndMakeVisible (outGainSlider.get());
    addAndMakeVisible (bypassButton.get());

    // 3. Теперь привязываем параметры (когда виджеты уже "живут")
    mixAttachment.reset (new juce::AudioProcessorValueTreeState::SliderAttachment (p.apvts, "mix", *mixSlider));
    outGainAttachment.reset (new juce::AudioProcessorValueTreeState::SliderAttachment (p.apvts, "outGain", *outGainSlider));
    bypassAttachment.reset (new juce::AudioProcessorValueTreeState::ButtonAttachment (p.apvts, "bypass", *bypassButton));

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

// ... paint и resized остаются как в предыдущем примере ...

void KickDuckAudioProcessorEditor::timerCallback()
{
    updateScopeData();
    repaint();
}

void KickDuckAudioProcessorEditor::updateScopeData()
{
    // ИСПРАВЛЕНО: используем ScopedRead
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
        
        // Для плавной линии лучше использовать scaleToFit для всего пути
        // Но так как у нас всего 1 семпл за кадр, рисуем просто линию
    }
}
