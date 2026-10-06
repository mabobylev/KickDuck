#include "PluginEditor.h"
#include <vector>
#include <cmath>

namespace
{
juce::String noteName (float beats)
{
    if (beats >= 3.999f) return "4/4";
    if (beats >= 1.999f) return "2/4";
    if (beats >= 0.999f) return "1/4";
    if (beats >= 0.499f) return "1/8";
    if (beats >= 0.249f) return "1/16";
    if (beats >= 0.124f) return "1/32";
    return "1/64";
}

struct PresetDef
{
    const char* name;
    std::vector<std::pair<const char*, float>> params;
};

const PresetDef presets[] =
{
    { "Kickstart Pump", { { "mode", 1.0f }, { "depth", 9.0f },  { "shape", 3.0f },
                          { "kicklen", 0.5f }, { "mix", 100.0f }, { "output", 0.0f } } },
    { "Gentle Sway",    { { "mode", 1.0f }, { "depth", 4.0f },  { "shape", 2.0f },
                          { "kicklen", 1.0f } } },
    { "Trance Gate",    { { "mode", 1.0f }, { "depth", 20.0f }, { "shape", 5.0f },
                          { "kicklen", 0.25f } } },
    { "Comp Glue",      { { "mode", 0.0f }, { "threshold", -30.0f }, { "ratio", 4.0f },
                          { "attack", 10.0f }, { "release", 150.0f }, { "knee", 6.0f },
                          { "depth", 6.0f } } },
    { "Bass Killer",    { { "mode", 0.0f }, { "threshold", -38.0f }, { "ratio", 12.0f },
                          { "attack", 0.5f }, { "release", 90.0f }, { "knee", 2.0f },
                          { "depth", 15.0f } } },
};
}

void KickDuckLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h,
                                            float sliderPos, float rotaryStartAngle,
                                            float rotaryEndAngle, juce::Slider& slider)
{
    const float angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
    const auto area = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h);
    const auto centre = area.getCentre();
    const float radius = juce::jmin (area.getWidth(), area.getHeight()) * 0.5f - 2.0f;
    const bool enabled = slider.isEnabled();

    juce::Path bg;
    bg.addCentredArc (centre.x, centre.y, radius, radius, 0.0f,
                      rotaryStartAngle, rotaryEndAngle, true);
    g.setColour (juce::Colour (0xff2e323a));
    g.strokePath (bg, juce::PathStrokeType (4.0f,
                   juce::PathStrokeType::curved, juce::PathStrokeType::butt));

    if (sliderPos > 0.0f)
    {
        juce::Path val;
        val.addCentredArc (centre.x, centre.y, radius, radius, 0.0f,
                           rotaryStartAngle, angle, true);
        g.setColour (enabled ? juce::Colour (0xff4fc3f7) : juce::Colour (0xff3a3e46));
        g.strokePath (val, juce::PathStrokeType (4.0f,
                       juce::PathStrokeType::curved, juce::PathStrokeType::butt));
    }

    const float body = radius * 0.72f;
    auto bodyRect = juce::Rectangle<float> (centre.x - body, centre.y - body,
                                            body * 2.0f, body * 2.0f);
    g.setColour (juce::Colour (0xff1f2228));
    g.fillEllipse (bodyRect);
    g.setColour (juce::Colour (0xff3a3e46));
    g.drawEllipse (bodyRect, 1.0f);

    juce::Path pointer;
    pointer.startNewSubPath (centre.x + std::sin (angle) * body * 0.35f,
                             centre.y - std::cos (angle) * body * 0.35f);
    pointer.lineTo (centre.x + std::sin (angle) * (body - 4.0f),
                    centre.y - std::cos (angle) * (body - 4.0f));
    g.setColour (enabled ? juce::Colours::white : juce::Colour (0xff555a63));
    g.strokePath (pointer, juce::PathStrokeType (2.0f,
                  juce::PathStrokeType::curved, juce::PathStrokeType::butt));
}

KickDuckAudioProcessorEditor::KickDuckAudioProcessorEditor (KickDuckAudioProcessor& p)
    : AudioProcessorEditor (&p), proc (p)
{
    static constexpr const char* ids[] =
        { "threshold", "ratio", "attack", "release", "knee",
          "depth", "shape", "kicklen", "mix", "hpf", "output" };
    static constexpr const char* names[] =
        { "Thr", "Ratio", "Atk", "Rel", "Knee",
          "Depth", "Shape", "Len", "Mix", "HPF", "Out" };

    setLookAndFeel (&lnf);

    for (int i = 0; i < numKnobs; ++i)
    {
        auto* s = new juce::Slider();
        s->setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        s->setTextBoxStyle (juce::Slider::TextBoxBelow, false, 70, 16);
        s->setColour (juce::Slider::textBoxTextColourId, juce::Colour (0xffc8ccd4));
        addAndMakeVisible (s);
        sliders.add (s);

        auto* l = new juce::Label();
        l->setText (names[i], juce::dontSendNotification);
        l->setJustificationType (juce::Justification::centred);
        l->setFont (juce::Font (juce::FontOptions (12.0f, juce::Font::bold)));
        l->setColour (juce::Label::textColourId, juce::Colour (0xff8a909b));
        addAndMakeVisible (l);
        labels.add (l);

        attachments.add (new juce::AudioProcessorValueTreeState::SliderAttachment (proc.apvts, ids[i], *s));
    }

    addAndMakeVisible (presetCombo);
    presetCombo.setTextWhenNothingSelected ("Preset");
    for (int i = 0; i < (int) std::size (presets); ++i)
        presetCombo.addItem (presets[i].name, i + 1);
    presetCombo.setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff26292f));
    presetCombo.setColour (juce::ComboBox::textColourId, juce::Colour (0xffc8ccd4));
    presetCombo.setColour (juce::ComboBox::arrowColourId, juce::Colour (0xff4fc3f7));
    presetCombo.onChange = [this]
    {
        if (presetCombo.getSelectedItemIndex() >= 0)
            applyPreset (presetCombo.getSelectedItemIndex());
    };

    addAndMakeVisible (dspModeButton);
    dspModeButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xff2b2f36));
    dspModeButton.setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xff4fc3f7));
    dspModeButton.onClick = [this]
    {
        const bool kickNow = isKickMode();
        if (auto* prm = proc.apvts.getParameter ("mode"))
        {
            prm->beginChangeGesture();
            prm->setValueNotifyingHost (kickNow ? 0.0f : 1.0f);
            prm->endChangeGesture();
        }
        updateModeUI();
    };

    addAndMakeVisible (displayButton);
    displayButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xff2b2f36));
    displayButton.onClick = [this]
    {
        showOutput = ! showOutput;
        displayButton.setButtonText (showOutput ? "OUT" : "IN");
    };

    setSize (1000, 330);
    startTimerHz (30);
    updateModeUI();
}

KickDuckAudioProcessorEditor::~KickDuckAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

bool KickDuckAudioProcessorEditor::isKickMode() const
{
    return proc.apvts.getRawParameterValue ("mode")->load() > 0.5f;
}

void KickDuckAudioProcessorEditor::updateModeUI()
{
    const bool kick = isKickMode();
    dspModeButton.setButtonText (kick ? "KICK" : "COMP");

    for (int i : { 0, 1, 2, 3, 4, 9 })
        sliders[i]->setEnabled (! kick);
    for (int i : { 6, 7 })
        sliders[i]->setEnabled (kick);
}

void KickDuckAudioProcessorEditor::applyPreset (int index)
{
    for (auto& kv : presets[index].params)
        if (auto* prm = proc.apvts.getParameter (kv.first))
        {
            prm->beginChangeGesture();
            prm->setValueNotifyingHost (prm->convertTo0to1 (kv.second));
            prm->endChangeGesture();
        }
    updateModeUI();
    repaint();
}

void KickDuckAudioProcessorEditor::timerCallback()
{
    const int ver = proc.frameVersion.load (std::memory_order_relaxed);
    if (ver != lastFrameVersion)
    {
        lastFrameVersion = ver;

        juce::SpinLock::ScopedLockType sl (proc.frameLock);
        const int pub = proc.framePublished.load (std::memory_order_relaxed);
        if (pub >= 0)
        {
            const int len = juce::jmin (proc.frameLens[pub], frameSize);
            if (len > 0)
            {
                for (int k = 0; k < len; ++k)
                {
                    frameMain[k] = proc.frameMain[pub][k];
                    frameOut[k]  = proc.frameOut[pub][k];
                    frameSc[k]   = proc.frameSc[pub][k];
                    frameGr[k]   = proc.frameGr[pub][k];
                }
                frameLen = len;
            }
        }
    }

    inDb  = juce::Decibels::gainToDecibels (proc.inLevel.load(), -100.0f);
    outDb = juce::Decibels::gainToDecibels (proc.outLevel.load(), -100.0f);
    grDb  = proc.grLevel.load (std::memory_order_relaxed);
    repaint();
}

juce::Rectangle<float> KickDuckAudioProcessorEditor::getWaveArea() const
{
    auto mid = getLocalBounds().reduced (10).toFloat();
    mid.removeFromTop (28.0f);
    mid.removeFromBottom (118.0f);
    mid.removeFromLeft (56.0f);
    mid.removeFromRight (112.0f);   // справа два индикатора: GR и OUT
    return mid.reduced (6.0f);
}

bool KickDuckAudioProcessorEditor::getShapeHandlePos (juce::Rectangle<float> area,
                                                      float& hx, float& hy) const
{
    if (! isKickMode() || frameLen <= 0)
        return false;

    const float depthDb = proc.apvts.getRawParameterValue ("depth")->load();
    const float shape   = proc.apvts.getRawParameterValue ("shape")->load();
    const float duckFrac = juce::jlimit (0.02f, 1.0f,
            proc.duckLenSamples.load() / (float) frameLen);
    const float endX = area.getX() + duckFrac * area.getWidth();
    const float tx = area.getX() + 0.5f * (endX - area.getX());

    const float tn = 0.5f * duckFrac;
    const float duckDb = depthDb * std::pow (1.0f - tn, shape);
    const float frac = juce::jlimit (0.0f, 1.0f, 1.0f - duckDb / 24.0f);

    hx = tx;
    hy = area.getY() + (1.0f - 0.9f * frac) * area.getHeight();
    return true;
}

void KickDuckAudioProcessorEditor::setParamsFromMouse (const juce::MouseEvent& e)
{
    const auto area = getWaveArea();
    const bool kick = isKickMode();
    const int len = juce::jmax (1, frameLen);
    const float srHz = juce::jmax (1.0f, proc.sampleRateAtomic.load());

    const float fracY = juce::jlimit (0.0f, 1.0f,
            (area.getBottom() - e.position.y) / (area.getHeight() * 0.9f));
    if (auto* dp = proc.apvts.getParameter ("depth"))
        dp->setValueNotifyingHost (dp->convertTo0to1 (fracY * 24.0f));

    const float fracX = juce::jlimit (0.0f, 1.0f,
            (e.position.x - area.getX()) / area.getWidth());

    if (kick)
    {
        const float frameSec = (float) len / srHz;
        const float bpm = juce::jmax (20.0f, proc.bpmAtomic.load());
        const float beats = juce::jlimit (0.03125f, 8.0f, fracX * frameSec * bpm / 60.0f);
        if (auto* hp = proc.apvts.getParameter ("kicklen"))
            hp->setValueNotifyingHost (hp->convertTo0to1 (beats));
    }
    else
    {
        const float relMs = juce::jlimit (5.0f, 1000.0f, fracX * (float) len / srHz * 1000.0f);
        if (auto* hp = proc.apvts.getParameter ("release"))
            hp->setValueNotifyingHost (hp->convertTo0to1 (relMs));
    }
}

void KickDuckAudioProcessorEditor::setShapeFromMouse (const juce::MouseEvent& e)
{
    const auto area = getWaveArea();

    const float depthDb = proc.apvts.getRawParameterValue ("depth")->load();
    if (depthDb <= 0.01f || frameLen <= 0)
        return;

    const float duckFrac = juce::jlimit (0.02f, 1.0f,
            proc.duckLenSamples.load() / (float) frameLen);
    const float tn = 0.5f * duckFrac;

    const float yFrac = juce::jlimit (0.0f, 1.0f,
            (area.getBottom() - e.position.y) / (area.getHeight() * 0.9f));
    const float duckDb = 24.0f * (1.0f - yFrac);
    const float r = juce::jlimit (0.02f, 1.0f, duckDb / depthDb);

    float s = std::log (r) / std::log (1.0f - tn);
    s = juce::jlimit (0.5f, 8.0f, s);

    if (auto* prm = proc.apvts.getParameter ("shape"))
        prm->setValueNotifyingHost (prm->convertTo0to1 (s));
}

void KickDuckAudioProcessorEditor::mouseDown (const juce::MouseEvent& e)
{
    const auto area = getWaveArea();
    if (! area.contains (e.position))
        return;

    dragging = true;

    float hx, hy;
    if (getShapeHandlePos (area, hx, hy)
        && juce::Point<float> (hx, hy).getDistanceFrom (e.position) < 14.0f)
    {
        draggingShape = true;
        if (auto* prm = proc.apvts.getParameter ("shape"))
            prm->beginChangeGesture();
        setShapeFromMouse (e);
        return;
    }

    const char* ids[] = { "depth", isKickMode() ? "kicklen" : "release" };
    for (auto* id : ids)
        if (auto* prm = proc.apvts.getParameter (id))
            prm->beginChangeGesture();
    setParamsFromMouse (e);
}

void KickDuckAudioProcessorEditor::mouseDrag (const juce::MouseEvent& e)
{
    if (! dragging)
        return;

    if (draggingShape)
        setShapeFromMouse (e);
    else
        setParamsFromMouse (e);
}

void KickDuckAudioProcessorEditor::mouseUp (const juce::MouseEvent&)
{
    if (dragging)
    {
        if (draggingShape)
        {
            if (auto* prm = proc.apvts.getParameter ("shape"))
                prm->endChangeGesture();
        }
        else
        {
            const char* ids[] = { "depth", isKickMode() ? "kicklen" : "release" };
            for (auto* id : ids)
                if (auto* prm = proc.apvts.getParameter (id))
                    prm->endChangeGesture();
        }
    }
    dragging = false;
    draggingShape = false;
}

void KickDuckAudioProcessorEditor::mouseMove (const juce::MouseEvent& e)
{
    const auto area = getWaveArea();
    const bool over = area.contains (e.position);

    float hx, hy;
    const bool onHandle = over && getShapeHandlePos (area, hx, hy)
        && juce::Point<float> (hx, hy).getDistanceFrom (e.position) < 14.0f;

    if (onHandle != overShapeHandle || over != overWaveArea)
    {
        overWaveArea = over;
        overShapeHandle = onHandle;
        setMouseCursor (onHandle ? juce::MouseCursor::PointingHandCursor
                        : over   ? juce::MouseCursor::UpDownResizeCursor
                                 : juce::MouseCursor::NormalCursor);
        repaint();
    }
}

void KickDuckAudioProcessorEditor::paint (juce::Graphics& g)
{
    juce::ColourGradient bg (juce::Colour (0xff23262c), 0.0f, 0.0f,
                             juce::Colour (0xff14161a), 0.0f, (float) getHeight(), false);
    g.setGradientFill (bg);
    g.fillAll();

    g.setColour (juce::Colour (0xffc8ccd4));
    g.setFont (juce::Font (juce::FontOptions (18.0f, juce::Font::bold)));
    g.drawText ("KickDuck", getLocalBounds().removeFromTop (30).removeFromLeft (200),
                juce::Justification::centred);
    g.setFont (13.0f);
    g.setColour (juce::Colour (0xff6b7280));
    g.drawText ("sidechain ducker", getLocalBounds().removeFromTop (30).removeFromLeft (340),
                juce::Justification::centredLeft);

    auto mid = getLocalBounds().reduced (10);
    mid.removeFromTop (28);
    mid.removeFromBottom (118);

    drawMeter (g, mid.removeFromLeft (56).toFloat(), inDb, "IN");

    auto rightBlock = mid.removeFromRight (108);
    auto grRect  = rightBlock.removeFromLeft (52).toFloat();
    drawGrMeter (g, grRect, grDb, "GR");
    drawMeter (g, rightBlock.toFloat(), outDb, "OUT");

    drawWaveforms (g, mid.toFloat().reduced (6.0f));
}

void KickDuckAudioProcessorEditor::drawMeter (juce::Graphics& g, juce::Rectangle<float> area,
                                              float db, const juce::String& label)
{
    constexpr float minDb = -60.0f, maxDb = 6.0f;

    g.setColour (juce::Colour (0xff1f2228));
    g.fillRoundedRectangle (area, 5.0f);
    g.setColour (juce::Colours::black.withAlpha (0.35f));
    g.drawRoundedRectangle (area, 5.0f, 1.0f);

    auto barArea = area.reduced (7.0f);
    barArea.removeFromTop (18.0f);
    barArea.removeFromBottom (22.0f);

    g.setColour (juce::Colours::grey.withAlpha (0.5f));
    for (int t = -60; t <= 0; t += 12)
    {
        const float frac = (float) (t - minDb) / (maxDb - minDb);
        const float y = barArea.getBottom() - barArea.getHeight() * frac;
        g.drawLine (barArea.getX(), y, barArea.getX() + 4.0f, y);
    }

    const float norm = juce::jlimit (0.0f, 1.0f, (db - minDb) / (maxDb - minDb));
    const float h = barArea.getHeight() * norm;

    juce::ColourGradient grad (juce::Colours::limegreen, barArea.getX(), barArea.getBottom(),
                               juce::Colours::orangered, barArea.getX(), barArea.getY(), false);
    grad.addColour (0.7, juce::Colours::yellow);
    g.setGradientFill (grad);
    if (h > 0.0f)
        g.fillRoundedRectangle (barArea.getX(), barArea.getBottom() - h,
                                barArea.getWidth(), h, 2.0f);

    g.setColour (juce::Colours::white);
    g.setFont (13.0f);
    const juce::String dbText = (db <= minDb) ? "-oo" : juce::String (db, 1);
    g.drawText (dbText + " dB", area.removeFromBottom (22.0f), juce::Justification::centred);
    g.drawText (label, area.removeFromTop (18.0f), juce::Justification::centred);
}

void KickDuckAudioProcessorEditor::drawGrMeter (juce::Graphics& g, juce::Rectangle<float> area,
                                                float db, const juce::String& label)
{
    constexpr float maxDuck = 24.0f;

    g.setColour (juce::Colour (0xff1f2228));
    g.fillRoundedRectangle (area, 5.0f);
    g.setColour (juce::Colours::black.withAlpha (0.35f));
    g.drawRoundedRectangle (area, 5.0f, 1.0f);

    auto barArea = area.reduced (7.0f);
    barArea.removeFromTop (18.0f);    // под лейбл
    barArea.removeFromBottom (22.0f); // под значение

    // шкала: 0 дБ сверху, -24 внизу (ниспадающий индикатор)
    g.setColour (juce::Colours::grey.withAlpha (0.5f));
    for (int t = 0; t <= 24; t += 6)
    {
        const float frac = (float) t / maxDuck;
        const float y = barArea.getY() + barArea.getHeight() * frac;
        g.drawLine (barArea.getX(), y, barArea.getX() + 4.0f, y);
    }

    const float norm = juce::jlimit (0.0f, 1.0f, db / maxDuck);
    const float h = barArea.getHeight() * norm;

    juce::ColourGradient grad (juce::Colours::orangered, barArea.getX(), barArea.getY(),
                               juce::Colours::yellow, barArea.getX(), barArea.getBottom(), false);
    g.setGradientFill (grad);
    if (h > 1.0f)
        g.fillRoundedRectangle (barArea.getX(), barArea.getY(),
                                barArea.getWidth(), h, 2.0f);

    // значение на тёмной плашке — не сливается с градиентом
    auto valueRect = area.removeFromBottom (20.0f).reduced (4.0f, 2.0f);
    g.setColour (juce::Colour (0xff14161a));
    g.fillRoundedRectangle (valueRect, 3.0f);
    g.setColour (juce::Colours::white);
    g.setFont (12.0f);
    const juce::String dbText = (db < 0.05f) ? "0.0" : "-" + juce::String (db, 1);
    g.drawText (dbText + " dB", valueRect, juce::Justification::centred);

    g.setFont (13.0f);
    g.drawText (label, area.removeFromTop (18.0f), juce::Justification::centred);
}

void KickDuckAudioProcessorEditor::drawWaveforms (juce::Graphics& g, juce::Rectangle<float> area)
{
    const bool kick = isKickMode();

    g.setColour (juce::Colour (0xff101216));
    g.fillRoundedRectangle (area, 6.0f);
    g.setColour (juce::Colours::black.withAlpha (0.35f));
    g.drawRoundedRectangle (area, 6.0f, 1.0f);

    if (frameLen <= 0)
    {
        g.setColour (juce::Colours::grey.withAlpha (0.4f));
        g.setFont (13.0f);
        g.drawText (kick ? "waiting for a beat (start playback)..."
                         : "waiting for sidechain trigger...",
                    area, juce::Justification::centred);
        return;
    }

    const int len = frameLen;
    const int numCols = (int) area.getWidth();
    const float midY = area.getCentreY();
    const float amp  = area.getHeight() * 0.48f;

    const float depthDb = proc.apvts.getRawParameterValue ("depth")->load();
    const float shape   = proc.apvts.getRawParameterValue ("shape")->load();
    const float duckFrac = juce::jlimit (0.02f, 1.0f,
            proc.duckLenSamples.load() / (float) len);

    // маппинг уровня сжатия: 24 дБ -> низ кадра, 0 дБ -> верхняя точка
    auto yForDuck = [&] (float duckDb)
    {
        const float frac = juce::jlimit (0.0f, 1.0f, 1.0f - duckDb / 24.0f);
        return area.getY() + (1.0f - 0.9f * frac) * area.getHeight();
    };

    g.setColour (juce::Colours::grey.withAlpha (0.3f));
    g.drawHorizontalLine ((int) midY, area.getX(), area.getRight());

    // волны на одной оси
    auto drawEnvelope = [&] (const float* data, juce::Colour colour)
    {
        g.setColour (colour);
        for (int x = 0; x < numCols; ++x)
        {
            const int k0 = (int) ((juce::int64) x * len / numCols);
            const int k1 = juce::jmax (k0 + 1,
                    (int) ((juce::int64) (x + 1) * len / numCols));
            float lo = 0.0f, hi = 0.0f;
            for (int k = k0; k < k1 && k < len; ++k)
            {
                const float v = data[k];
                lo = juce::jmin (lo, v);
                hi = juce::jmax (hi, v);
            }
            const float px = area.getX() + (float) x;
            const float yTop = midY - juce::jlimit (-1.0f, 1.0f, hi) * amp;
            const float yBot = midY - juce::jlimit (-1.0f, 1.0f, lo) * amp;
            g.drawLine (px, yTop, px, yBot);
        }
    };

    drawEnvelope (showOutput ? frameOut : frameMain, juce::Colours::steelblue.withAlpha (0.9f));
    drawEnvelope (frameSc, juce::Colours::orange.withAlpha (0.8f));

    // LFO-волна сжатия.
    // KICK: идеальная кривая по Shape/Len/Depth (с ручкой формы).
    // COMP: измеренная кривая из кадра — отражает реальное сжатие от баса.
    const float endX = area.getX() + duckFrac * area.getWidth();
    // полка и спад занимают минимум 8% ширины, поэтому нарастание не правее 92%
    const float riseEndX = juce::jmin (endX, area.getX() + 0.92f * area.getWidth());
    const float plateauX = juce::jmax (riseEndX + 4.0f,
                                       area.getRight() - 0.05f * area.getWidth());

    juce::Path lfo;

    if (kick)
    {
        lfo.startNewSubPath (area.getX(), yForDuck (depthDb));
        for (float fx = area.getX() + 2.0f; fx < riseEndX; fx += 2.0f)
        {
            const float tn = juce::jlimit (0.0f, 1.0f,
                    (fx - area.getX()) / juce::jmax (1.0f, riseEndX - area.getX()));
            lfo.lineTo (fx, yForDuck (depthDb * std::pow (1.0f - tn, shape)));
        }
        lfo.lineTo (riseEndX, yForDuck (0.0f));
        lfo.lineTo (plateauX, yForDuck (0.0f));
        lfo.lineTo (area.getRight(), yForDuck (depthDb));

        g.setColour (juce::Colours::cyan.withAlpha (0.85f));
        g.strokePath (lfo, juce::PathStrokeType (2.0f));

        float hx, hy;
        if (getShapeHandlePos (area, hx, hy))
        {
            const float r = overShapeHandle || draggingShape ? 6.5f : 4.5f;
            g.setColour (draggingShape ? juce::Colours::white : juce::Colours::cyan);
            g.fillEllipse (hx - r, hy - r, r * 2.0f, r * 2.0f);
            g.setColour (juce::Colours::black.withAlpha (0.4f));
            g.drawEllipse (hx - r, hy - r, r * 2.0f, r * 2.0f, 1.0f);
        }
    }
    else
    {
        // измеренная кривая сжатия: минимум GR по каждому столбцу
        bool started = false;
        for (int x = 0; x < numCols; ++x)
        {
            const int k0 = (int) ((juce::int64) x * len / numCols);
            const int k1 = juce::jmax (k0 + 1,
                    (int) ((juce::int64) (x + 1) * len / numCols));
            float worst = 0.0f;
            for (int k = k0; k < k1 && k < len; ++k)
                worst = juce::jmin (worst, frameGr[k]);

            const float px = area.getX() + (float) x;
            const float py = yForDuck (-worst);
            if (! started) { lfo.startNewSubPath (px, py); started = true; }
            else            lfo.lineTo (px, py);
        }

        g.setColour (juce::Colours::cyan.withAlpha (0.85f));
        g.strokePath (lfo, juce::PathStrokeType (2.0f));
    }

    // подписи: Duck слева сверху, Len/Rel справа СНИЗУ (не конфликтует с легендой)
    g.setFont (11.0f);

    g.setColour (juce::Colours::cyan);
    g.drawText (juce::String ("Duck -") + juce::String (depthDb, 1) + " dB",
                (int) area.getX() + 6, (int) area.getY() + 2, 110, 14,
                juce::Justification::centredLeft);

    g.setColour (juce::Colours::white.withAlpha (0.8f));
    if (kick)
    {
        const float bpm = juce::jmax (20.0f, proc.bpmAtomic.load());
        const float lenSec = (float) len / proc.sampleRateAtomic.load();
        const float beats = juce::jlimit (0.03125f, 8.0f, duckFrac * lenSec * bpm / 60.0f);
        g.drawText ("Len " + noteName (beats),
                    (int) area.getRight() - 110, (int) area.getBottom() - 18, 104, 14,
                    juce::Justification::centredRight);
    }
    else
    {
        const float relMs = proc.apvts.getRawParameterValue ("release")->load();
        g.drawText ("Rel " + juce::String (relMs, 0) + " ms",
                    (int) area.getRight() - 110, (int) area.getBottom() - 18, 104, 14,
                    juce::Justification::centredRight);
    }

    // легенда — правый верхний угол
    auto legend = area.removeFromTop (16.0f).removeFromRight (176.0f);
    auto dot = [&] (juce::Colour c, juce::String txt, float lx, float lw)
    {
        g.setColour (c);
        g.fillRect (lx, legend.getY() + 5.0f, 8.0f, 8.0f);
        g.setColour (juce::Colours::grey);
        g.drawText (txt, (int) lx + 11, (int) legend.getY(), (int) lw, 16,
                    juce::Justification::centredLeft);
    };
    dot (juce::Colours::steelblue, showOutput ? "Ducked" : "Bass", legend.getX(), 52.0f);
    dot (juce::Colours::orange,    "Kick", legend.getX() + 60.0f, 40.0f);
    dot (juce::Colours::cyan,      "Duck", legend.getX() + 108.0f, 40.0f);
}

void KickDuckAudioProcessorEditor::resized()
{
    const int w = getWidth();
    presetCombo.setBounds   (w - 356, 4, 164, 24);
    dspModeButton.setBounds (w - 186, 4, 84, 24);
    displayButton.setBounds (w - 96, 4, 86, 24);

    auto sliderArea = getLocalBounds().reduced (10).removeFromBottom (114);
    const int cw = sliderArea.getWidth() / numKnobs;

    for (int i = 0; i < numKnobs; ++i)
    {
        auto cell = sliderArea.removeFromLeft (cw).reduced (4);
        sliders[i]->setBounds (cell.removeFromTop (cell.getHeight() - 18));
        labels[i]->setBounds (cell);
    }
}
