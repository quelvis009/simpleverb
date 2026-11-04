#include "PluginEditor.h"

ReverbEditor::ReverbEditor (ReverbProcessor& p)
    : AudioProcessorEditor (&p), verbProcessor (p)
{
    setLookAndFeel (&laf);

    setupKnob (knobSize,     "size",     "SIZE");
    setupKnob (knobDamping,  "damping",  "DAMPING");
    setupKnob (knobWidth,    "width",    "WIDTH");
    setupKnob (knobPreDelay, "predelay", "PRE-DELAY");
    setupKnob (knobLowCut,   "lowcut",   "LOW CUT");
    setupKnob (knobMix,      "mix",      "MIX");

    // Presets
    for (int i = 0; i < numPresets; ++i)
        presetBox.addItem (factoryPresets[i].name, i + 1);

    presetBox.setSelectedItemIndex (verbProcessor.currentPresetIndex, juce::dontSendNotification);
    presetBox.onChange = [this] { verbProcessor.loadPreset (presetBox.getSelectedItemIndex()); };
    addAndMakeVisible (presetBox);

    prevButton.onClick = [this] { stepPreset (-1); };
    nextButton.onClick = [this] { stepPreset (1); };
    addAndMakeVisible (prevButton);
    addAndMakeVisible (nextButton);

    // Botones de tipo de sala
    for (int i = 0; i < numRooms; ++i)
    {
        auto& b = roomButtons[(size_t) i];
        b.setButtonText (roomSpecs[i].name);
        b.setClickingTogglesState (false);
        b.onClick = [this, i] { setRoom (i); };
        addAndMakeVisible (b);
    }

    // Tema inicial según el estado actual
    int room = 3;
    float hueTarget = 0.6f, darkTarget = 0.3f, size = 0.6f;
    readTargets (room, hueTarget, darkTarget, size);
    hue = hueTarget;
    darkness = darkTarget;
    decaySeconds = ReverbProcessor::estimateDecaySeconds (room, size);
    updateTheme();

    setSize (760, 470);
    startTimerHz (30);
}

ReverbEditor::~ReverbEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void ReverbEditor::setupKnob (Knob& knob, const juce::String& paramId, const juce::String& name)
{
    knob.slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    knob.slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 96, 24);
    knob.slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.2f,
                                     juce::MathConstants<float>::pi * 2.8f, true);
    knob.slider.setMouseDragSensitivity (200);
    addAndMakeVisible (knob.slider);

    knob.label.setText (name, juce::dontSendNotification);
    knob.label.setJustificationType (juce::Justification::centred);
    knob.label.setFont (juce::Font (juce::FontOptions (13.0f, juce::Font::bold)));
    knob.label.setColour (juce::Label::textColourId, MusikColours::text);
    addAndMakeVisible (knob.label);

    knob.attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        verbProcessor.apvts, paramId, knob.slider);
}

void ReverbEditor::setRoom (int room)
{
    if (auto* p = verbProcessor.apvts.getParameter ("room"))
        p->setValueNotifyingHost (p->convertTo0to1 ((float) room));
}

void ReverbEditor::stepPreset (int delta)
{
    const int current = juce::jmax (0, presetBox.getSelectedItemIndex());
    const int next = (current + delta + numPresets) % numPresets;
    presetBox.setSelectedItemIndex (next, juce::sendNotificationSync);
}

void ReverbEditor::readTargets (int& room, float& hueTarget, float& darkTarget, float& sizeOut) const
{
    // Un color base por tipo de sala
    static const float roomHues[numRooms] = { 0.08f, 0.50f, 0.38f, 0.60f, 0.74f, 0.88f };

    room = juce::jlimit (0, numRooms - 1, (int) verbProcessor.apvts.getRawParameterValue ("room")->load());
    sizeOut = verbProcessor.apvts.getRawParameterValue ("size")->load();
    hueTarget = roomHues[room];

    // Cuanto más largo el reverb, más oscuro el plugin
    const float rt = ReverbProcessor::estimateDecaySeconds (room, sizeOut);
    darkTarget = juce::jlimit (0.0f, 1.0f, std::log (1.0f + rt) / std::log (17.0f));
}

void ReverbEditor::updateTheme()
{
    laf.accentA = accentA();
    laf.accentB = accentB();
}

void ReverbEditor::timerCallback()
{
    smoothedLevel = smoothedLevel * 0.85f + verbProcessor.outputLevel.load() * 0.15f;

    int room = 3;
    float hueTarget = 0.6f, darkTarget = 0.3f, size = 0.6f;
    readTargets (room, hueTarget, darkTarget, size);

    // Transición suave del color (por el camino más corto del círculo de tonos)
    float diff = hueTarget - hue;
    if (diff > 0.5f)  diff -= 1.0f;
    if (diff < -0.5f) diff += 1.0f;
    hue += diff * 0.12f;
    if (hue < 0.0f)  hue += 1.0f;
    if (hue >= 1.0f) hue -= 1.0f;

    darkness += (darkTarget - darkness) * 0.12f;
    decaySeconds = ReverbProcessor::estimateDecaySeconds (room, size);

    for (int i = 0; i < numRooms; ++i)
        roomButtons[(size_t) i].setToggleState (i == room, juce::dontSendNotification);

    phase += 0.003f + size * 0.006f + smoothedLevel * 0.01f;
    if (phase >= 1.0f)
        phase -= 1.0f;

    updateTheme();
    repaint();
}

void ReverbEditor::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    const float w = bounds.getWidth();
    const float h = bounds.getHeight();
    const auto a = accentA();
    const auto b = accentB();

    // Fondo con degradado del color de la sala
    g.setGradientFill (juce::ColourGradient (bgTop(), 0.0f, 0.0f, bgBottom(), 0.0f, h, false));
    g.fillRect (bounds);

    // Ondas animadas que reaccionan al audio
    const juce::Point<float> centre (w * 0.5f, h * 0.55f);
    const float maxR = w * 0.55f;
    for (int i = 0; i < 6; ++i)
    {
        const float t = std::fmod (phase + (float) i / 6.0f, 1.0f);
        const float r = t * maxR;
        const float alpha = juce::jlimit (0.0f, 1.0f, (1.0f - t) * (0.12f + smoothedLevel * 0.5f));
        g.setColour ((i % 2 == 0 ? a : b).withAlpha (alpha));
        g.drawEllipse (centre.x - r, centre.y - r, r * 2.0f, r * 2.0f, 2.0f);
    }

    // Viñeta en las esquinas
    g.setGradientFill (juce::ColourGradient (juce::Colours::transparentBlack, centre.x, centre.y,
                                             juce::Colours::black.withAlpha (0.6f), 0.0f, 0.0f, true));
    g.fillRect (bounds);

    // Panel elevado de las perillas
    const auto panel = getPanelBounds().toFloat();
    juce::Path panelPath;
    panelPath.addRoundedRectangle (panel, 18.0f);
    juce::DropShadow (juce::Colours::black.withAlpha (0.7f), 26, { 0, 12 }).drawForPath (g, panelPath);

    g.setGradientFill (juce::ColourGradient (bgTop().brighter (0.10f), 0.0f, panel.getY(),
                                             bgBottom().brighter (0.04f), 0.0f, panel.getBottom(), false));
    g.fillPath (panelPath);
    g.setColour (juce::Colours::white.withAlpha (0.12f));
    g.strokePath (panelPath, juce::PathStrokeType (1.2f));
    g.setColour (juce::Colours::white.withAlpha (0.10f));
    g.drawLine (panel.getX() + 20.0f, panel.getY() + 1.5f, panel.getRight() - 20.0f, panel.getY() + 1.5f, 1.0f);

    // Título con relieve y sombra
    g.setFont (juce::Font (juce::FontOptions (44.0f, juce::Font::bold)));
    const juce::Rectangle<int> titleArea (30, 10, 400, 56);
    for (int i = 4; i >= 1; --i)
    {
        g.setColour (juce::Colours::black.withAlpha (0.16f));
        g.drawText ("MUSIKVERB", titleArea.translated (i, i + 2), juce::Justification::centredLeft, false);
    }
    g.setColour (a.darker (0.8f));
    g.drawText ("MUSIKVERB", titleArea.translated (0, 2), juce::Justification::centredLeft, false);
    g.setGradientFill (juce::ColourGradient (b.brighter (0.3f), 0.0f, 10.0f, a, 0.0f, 66.0f, false));
    g.drawText ("MUSIKVERB", titleArea, juce::Justification::centredLeft, false);

    g.setFont (juce::Font (juce::FontOptions (11.0f, juce::Font::bold)));
    g.setColour (a.withAlpha (0.8f));
    g.drawText ("R E V E R B   P R O C E S S O R", 32, 64, 400, 16, juce::Justification::centredLeft, false);

    // Etiqueta del selector de presets
    g.setFont (juce::Font (juce::FontOptions (10.0f, juce::Font::bold)));
    g.setColour (MusikColours::text.withAlpha (0.7f));
    g.drawText ("PRESET", 466, 20, 200, 14, juce::Justification::centredLeft, false);

    // Indicador de duración (pantalla LCD)
    const juce::Rectangle<float> lcd (30.0f, 424.0f, 190.0f, 28.0f);
    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.fillRoundedRectangle (lcd, 8.0f);
    g.setColour (juce::Colours::white.withAlpha (0.10f));
    g.drawRoundedRectangle (lcd, 8.0f, 1.0f);
    g.setFont (juce::Font (juce::FontOptions (13.0f, juce::Font::bold)));
    g.setColour (a.brighter (0.6f));
    g.drawText ("DECAY  " + juce::String (decaySeconds, 1) + " s", lcd.toNearestInt(),
                juce::Justification::centred, false);

    // Barra que muestra cuánto se oscurece el plugin con la duración
    g.setFont (juce::Font (juce::FontOptions (10.0f, juce::Font::bold)));
    g.setColour (MusikColours::text.withAlpha (0.7f));
    g.drawText ("TAIL", 470, 430, 40, 16, juce::Justification::centredLeft, false);

    const juce::Rectangle<float> bar (512.0f, 432.0f, 218.0f, 12.0f);
    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.fillRoundedRectangle (bar, 6.0f);
    auto fill = bar.withWidth (juce::jmax (12.0f, bar.getWidth() * darkness));
    g.setColour (a.withAlpha (0.25f));
    g.fillRoundedRectangle (fill.expanded (2.0f), 8.0f);
    g.setGradientFill (juce::ColourGradient (b, fill.getX(), 0.0f, a, fill.getRight(), 0.0f, false));
    g.fillRoundedRectangle (fill, 6.0f);
    g.setColour (juce::Colours::white.withAlpha (0.10f));
    g.drawRoundedRectangle (bar, 6.0f, 1.0f);
}

void ReverbEditor::resized()
{
    // Selector de presets
    prevButton.setBounds (430, 36, 30, 28);
    presetBox.setBounds (466, 36, 230, 28);
    nextButton.setBounds (702, 36, 28, 28);

    // Botones de tipo de sala
    const int cell = 700 / numRooms;
    for (int i = 0; i < numRooms; ++i)
        roomButtons[(size_t) i].setBounds (30 + i * cell + 4, 94, cell - 8, 34);

    // Perillas dentro del panel
    auto area = getPanelBounds().reduced (0, 10);
    const int cellW = area.getWidth() / 6;

    Knob* knobs[6] = { &knobSize, &knobDamping, &knobWidth, &knobPreDelay, &knobLowCut, &knobMix };

    for (auto* k : knobs)
    {
        auto c = area.removeFromLeft (cellW);
        k->label.setBounds (c.removeFromTop (24));
        k->slider.setBounds (c);
    }
}
