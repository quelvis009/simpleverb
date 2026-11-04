#include "PluginEditor.h"

ReverbEditor::ReverbEditor (ReverbProcessor& p)
    : AudioProcessorEditor (&p), verbProcessor (p)
{
    setupKnob (knobSize,     "size",     "SIZE");
    setupKnob (knobDamping,  "damping",  "DAMPING");
    setupKnob (knobWidth,    "width",    "WIDTH");
    setupKnob (knobPreDelay, "predelay", "PRE-DELAY");
    setupKnob (knobLowCut,   "lowcut",   "LOW CUT");
    setupKnob (knobMix,      "mix",      "MIX");

    setSize (720, 380);
    startTimerHz (30);
}

ReverbEditor::~ReverbEditor()
{
    stopTimer();

    for (auto* k : { &knobSize, &knobDamping, &knobWidth, &knobPreDelay, &knobLowCut, &knobMix })
        k->slider.setLookAndFeel (nullptr);
}

void ReverbEditor::setupKnob (Knob& knob, const juce::String& paramId, const juce::String& name)
{
    knob.slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    knob.slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 90, 22);
    knob.slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.2f,
                                     juce::MathConstants<float>::pi * 2.8f, true);
    knob.slider.setMouseDragSensitivity (200);
    knob.slider.setLookAndFeel (&laf);
    addAndMakeVisible (knob.slider);

    knob.label.setText (name, juce::dontSendNotification);
    knob.label.setJustificationType (juce::Justification::centred);
    knob.label.setFont (juce::Font (juce::FontOptions (13.0f, juce::Font::bold)));
    knob.label.setColour (juce::Label::textColourId, MusikColours::text);
    addAndMakeVisible (knob.label);

    knob.attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        verbProcessor.apvts, paramId, knob.slider);
}

void ReverbEditor::timerCallback()
{
    const float level = verbProcessor.outputLevel.load();
    smoothedLevel = smoothedLevel * 0.85f + level * 0.15f;

    const float size = verbProcessor.apvts.getRawParameterValue ("size")->load();
    phase += 0.003f + size * 0.006f + smoothedLevel * 0.01f;
    if (phase >= 1.0f)
        phase -= 1.0f;

    repaint();
}

void ReverbEditor::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    const float w = bounds.getWidth();

    // Fondo con degradado
    juce::ColourGradient bg (MusikColours::bgTop, 0.0f, 0.0f,
                             MusikColours::bgBot, w, bounds.getHeight(), false);
    g.setGradientFill (bg);
    g.fillRect (bounds);

    // Ondas animadas que reaccionan al audio
    const auto centre = bounds.getCentre().translated (0.0f, 30.0f);
    const float maxR = w * 0.5f;
    for (int i = 0; i < 6; ++i)
    {
        const float t = std::fmod (phase + (float) i / 6.0f, 1.0f);
        const float r = t * maxR;
        const float alpha = juce::jlimit (0.0f, 1.0f, (1.0f - t) * (0.10f + smoothedLevel * 0.5f));
        g.setColour ((i % 2 == 0 ? MusikColours::cyan : MusikColours::magenta).withAlpha (alpha));
        g.drawEllipse (centre.x - r, centre.y - r, r * 2.0f, r * 2.0f, 2.0f);
    }

    // Título con degradado
    g.setFont (juce::Font (juce::FontOptions (40.0f, juce::Font::bold)));
    g.setGradientFill (juce::ColourGradient (MusikColours::cyan, w * 0.3f, 0.0f,
                                             MusikColours::magenta, w * 0.7f, 0.0f, false));
    g.drawText ("MUSIKVERB", 0, 14, getWidth(), 48, juce::Justification::centred);

    g.setFont (juce::Font (juce::FontOptions (13.0f, juce::Font::plain)));
    g.setColour (juce::Colours::white.withAlpha (0.45f));
    g.drawText ("R  E  V  E  R  B", 0, 62, getWidth(), 18, juce::Justification::centred);

    // Línea decorativa
    g.setColour (juce::Colours::white.withAlpha (0.08f));
    g.drawLine (40.0f, 92.0f, w - 40.0f, 92.0f, 1.0f);
}

void ReverbEditor::resized()
{
    auto area = getLocalBounds().reduced (24);
    area.removeFromTop (84);
    area.removeFromBottom (10);

    constexpr int numKnobs = 6;
    const int cellW = area.getWidth() / numKnobs;

    Knob* knobs[numKnobs] = { &knobSize, &knobDamping, &knobWidth,
                              &knobPreDelay, &knobLowCut, &knobMix };

    for (auto* k : knobs)
    {
        auto cell = area.removeFromLeft (cellW);
        k->label.setBounds (cell.removeFromTop (24));
        k->slider.setBounds (cell);
    }
}
