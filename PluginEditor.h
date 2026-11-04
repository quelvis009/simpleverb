#pragma once

#include "PluginProcessor.h"

// Colores del diseño
namespace MusikColours
{
    const juce::Colour cyan    { 0xff22d3ee };
    const juce::Colour magenta { 0xffe879f9 };
    const juce::Colour bgTop   { 0xff0d0a1a };
    const juce::Colour bgBot   { 0xff1e1036 };
    const juce::Colour track   { 0xff2a2540 };
    const juce::Colour text    { 0xffc9c3e6 };
}

// Perillas con brillo neón y degradado
class MusikLookAndFeel : public juce::LookAndFeel_V4
{
public:
    MusikLookAndFeel()
    {
        setColour (juce::Slider::textBoxTextColourId, juce::Colours::white);
        setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
        setColour (juce::Slider::textBoxHighlightColourId, MusikColours::magenta.withAlpha (0.4f));
    }

    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                           juce::Slider&) override
    {
        auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) width, (float) height).reduced (6.0f);
        const float radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
        const auto centre = bounds.getCentre();
        const float angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
        const float arcRadius = radius - 4.0f;

        // Pista de fondo
        juce::Path track;
        track.addCentredArc (centre.x, centre.y, arcRadius, arcRadius, 0.0f,
                             rotaryStartAngle, rotaryEndAngle, true);
        g.setColour (MusikColours::track);
        g.strokePath (track, juce::PathStrokeType (5.0f, juce::PathStrokeType::curved,
                                                   juce::PathStrokeType::rounded));

        // Arco de valor con brillo
        juce::Path value;
        value.addCentredArc (centre.x, centre.y, arcRadius, arcRadius, 0.0f,
                             rotaryStartAngle, angle, true);

        g.setColour (MusikColours::magenta.withAlpha (0.22f));
        g.strokePath (value, juce::PathStrokeType (12.0f, juce::PathStrokeType::curved,
                                                   juce::PathStrokeType::rounded));

        juce::ColourGradient grad (MusikColours::cyan, bounds.getX(), bounds.getBottom(),
                                   MusikColours::magenta, bounds.getRight(), bounds.getY(), false);
        g.setGradientFill (grad);
        g.strokePath (value, juce::PathStrokeType (5.0f, juce::PathStrokeType::curved,
                                                   juce::PathStrokeType::rounded));

        // Cuerpo de la perilla
        const float bodyR = arcRadius - 9.0f;
        juce::ColourGradient body (juce::Colour (0xff3d3660), centre.x, centre.y - bodyR,
                                   juce::Colour (0xff14111f), centre.x, centre.y + bodyR, false);
        g.setGradientFill (body);
        g.fillEllipse (centre.x - bodyR, centre.y - bodyR, bodyR * 2.0f, bodyR * 2.0f);

        g.setColour (juce::Colours::white.withAlpha (0.10f));
        g.drawEllipse (centre.x - bodyR, centre.y - bodyR, bodyR * 2.0f, bodyR * 2.0f, 1.5f);

        // Indicador
        juce::Path pointer;
        pointer.addRoundedRectangle (-2.0f, -bodyR + 4.0f, 4.0f, bodyR * 0.45f, 2.0f);
        g.setColour (juce::Colours::white);
        g.fillPath (pointer, juce::AffineTransform::rotation (angle).translated (centre.x, centre.y));
    }
};

class ReverbEditor : public juce::AudioProcessorEditor,
                     private juce::Timer
{
public:
    explicit ReverbEditor (ReverbProcessor&);
    ~ReverbEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    struct Knob
    {
        juce::Slider slider;
        juce::Label label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };

    void timerCallback() override;
    void setupKnob (Knob& knob, const juce::String& paramId, const juce::String& name);

    ReverbProcessor& verbProcessor;
    MusikLookAndFeel laf;

    Knob knobSize, knobDamping, knobWidth, knobPreDelay, knobLowCut, knobMix;

    float smoothedLevel = 0.0f;
    float phase = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ReverbEditor)
};
