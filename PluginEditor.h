#pragma once

#include "PluginProcessor.h"

namespace MusikColours
{
    const juce::Colour text { 0xffd2cdea };
}

//==============================================================================
// Estilo visual: perillas 3D, botones en píldora, pantallas LCD y listas
class MusikLookAndFeel : public juce::LookAndFeel_V4
{
public:
    // Colores del tema actual (los actualiza el editor)
    juce::Colour accentA { 0xff22d3ee };
    juce::Colour accentB { 0xffe879f9 };

    MusikLookAndFeel()
    {
        setColour (juce::Slider::textBoxTextColourId, juce::Colours::white);
        setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
        setColour (juce::Slider::textBoxHighlightColourId, juce::Colour (0x66ffffff));

        setColour (juce::ComboBox::textColourId, juce::Colours::white);
        setColour (juce::ComboBox::backgroundColourId, juce::Colours::transparentBlack);
        setColour (juce::ComboBox::outlineColourId, juce::Colours::transparentBlack);

        setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xff17121f));
        setColour (juce::PopupMenu::textColourId, juce::Colour (0xffd2cdea));
        setColour (juce::PopupMenu::highlightedBackgroundColourId, juce::Colour (0xff3a3355));
        setColour (juce::PopupMenu::highlightedTextColourId, juce::Colours::white);
    }

    //--------------------------------------------------------------------------
    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                           juce::Slider&) override
    {
        auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) width, (float) height).reduced (3.0f);
        const float radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
        const auto centre = bounds.getCentre();
        const float angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

        const float tickR = radius;
        const float arcR  = radius - 13.0f;
        const float bodyR = arcR - 8.0f;

        // Marcas alrededor, encendidas hasta el valor actual
        const int numTicks = 11;
        for (int i = 0; i < numTicks; ++i)
        {
            const float t = (float) i / (float) (numTicks - 1);
            const float a = rotaryStartAngle + t * (rotaryEndAngle - rotaryStartAngle);
            const auto p1 = centre.getPointOnCircumference (tickR - 4.5f, a);
            const auto p2 = centre.getPointOnCircumference (tickR, a);
            const bool lit = t <= sliderPos + 0.001f;
            g.setColour (lit ? accentA.withAlpha (0.95f) : juce::Colours::white.withAlpha (0.16f));
            g.drawLine (p1.x, p1.y, p2.x, p2.y, lit ? 2.0f : 1.4f);
        }

        // Ranura hundida
        juce::Path track;
        track.addCentredArc (centre.x, centre.y, arcR, arcR, 0.0f, rotaryStartAngle, rotaryEndAngle, true);
        g.setColour (juce::Colours::black.withAlpha (0.55f));
        g.strokePath (track, juce::PathStrokeType (8.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour (juce::Colour (0xff1a1626));
        g.strokePath (track, juce::PathStrokeType (5.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // Arco de valor con brillo
        juce::Path value;
        value.addCentredArc (centre.x, centre.y, arcR, arcR, 0.0f, rotaryStartAngle, angle, true);
        g.setColour (accentA.withAlpha (0.22f));
        g.strokePath (value, juce::PathStrokeType (12.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        juce::ColourGradient grad (accentB, centre.x - arcR, centre.y + arcR,
                                   accentA, centre.x + arcR, centre.y - arcR, false);
        g.setGradientFill (grad);
        g.strokePath (value, juce::PathStrokeType (5.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // Sombra proyectada de la perilla
        juce::Path bodyPath;
        bodyPath.addEllipse (centre.x - bodyR, centre.y - bodyR, bodyR * 2.0f, bodyR * 2.0f);
        juce::DropShadow (juce::Colours::black.withAlpha (0.8f), 14, { 0, 7 }).drawForPath (g, bodyPath);

        // Bisel exterior metálico
        juce::ColourGradient bezel (juce::Colour (0xff6a6288), centre.x, centre.y - bodyR,
                                    juce::Colour (0xff0e0b16), centre.x, centre.y + bodyR, false);
        g.setGradientFill (bezel);
        g.fillEllipse (centre.x - bodyR, centre.y - bodyR, bodyR * 2.0f, bodyR * 2.0f);

        // Cara de la perilla con luz arriba a la izquierda
        const float faceR = bodyR - 4.0f;
        juce::ColourGradient face (juce::Colour (0xff4d456e).interpolatedWith (accentA, 0.10f),
                                   centre.x - faceR * 0.35f, centre.y - faceR * 0.5f,
                                   juce::Colour (0xff15111f),
                                   centre.x + faceR * 0.15f, centre.y + faceR * 0.55f, true);
        g.setGradientFill (face);
        g.fillEllipse (centre.x - faceR, centre.y - faceR, faceR * 2.0f, faceR * 2.0f);

        // Anillos finos y reflejo
        g.setColour (juce::Colours::white.withAlpha (0.06f));
        g.drawEllipse (centre.x - faceR * 0.62f, centre.y - faceR * 0.62f, faceR * 1.24f, faceR * 1.24f, 1.0f);
        g.setColour (juce::Colours::black.withAlpha (0.35f));
        g.drawEllipse (centre.x - faceR, centre.y - faceR, faceR * 2.0f, faceR * 2.0f, 1.2f);
        g.setColour (juce::Colours::white.withAlpha (0.10f));
        g.fillEllipse (centre.x - faceR * 0.6f, centre.y - faceR * 0.92f, faceR * 1.2f, faceR * 0.55f);

        // Indicador con sombra y punta luminosa
        const auto pStart = centre.getPointOnCircumference (faceR * 0.30f, angle);
        const auto pEnd   = centre.getPointOnCircumference (faceR * 0.82f, angle);
        g.setColour (juce::Colours::black.withAlpha (0.55f));
        g.drawLine (pStart.x + 1.5f, pStart.y + 2.0f, pEnd.x + 1.5f, pEnd.y + 2.0f, 4.5f);
        g.setColour (juce::Colours::white);
        g.drawLine (pStart.x, pStart.y, pEnd.x, pEnd.y, 3.5f);
        g.setColour (accentA);
        g.fillEllipse (pEnd.x - 3.0f, pEnd.y - 3.0f, 6.0f, 6.0f);
    }

    //--------------------------------------------------------------------------
    // Textos de las perillas con aspecto de pantalla LCD hundida
    void drawLabel (juce::Graphics& g, juce::Label& label) override
    {
        if (dynamic_cast<juce::Slider*> (label.getParentComponent()) == nullptr)
        {
            LookAndFeel_V4::drawLabel (g, label);
            return;
        }

        if (label.isBeingEdited())
            return;

        auto b = label.getLocalBounds().toFloat().reduced (6.0f, 2.0f);
        g.setColour (juce::Colours::black.withAlpha (0.55f));
        g.fillRoundedRectangle (b, 6.0f);
        g.setColour (juce::Colours::black.withAlpha (0.6f));
        g.drawRoundedRectangle (b, 6.0f, 1.2f);
        g.setColour (juce::Colours::white.withAlpha (0.08f));
        g.drawLine (b.getX() + 6.0f, b.getBottom() + 0.5f, b.getRight() - 6.0f, b.getBottom() + 0.5f, 1.0f);

        g.setColour (accentA.brighter (0.6f));
        g.setFont (juce::Font (juce::FontOptions (13.0f, juce::Font::bold)));
        g.drawText (label.getText(), label.getLocalBounds(), juce::Justification::centred, false);
    }

    //--------------------------------------------------------------------------
    // Botones en forma de píldora con relieve
    void drawButtonBackground (juce::Graphics& g, juce::Button& button, const juce::Colour&,
                               bool highlighted, bool) override
    {
        auto r = button.getLocalBounds().toFloat().reduced (2.0f);
        juce::Path p;
        p.addRoundedRectangle (r, r.getHeight() * 0.5f);

        if (button.getToggleState())
        {
            juce::DropShadow (accentA.withAlpha (0.6f), 12, { 0, 0 }).drawForPath (g, p);
            juce::ColourGradient cg (accentA.brighter (0.25f), r.getX(), r.getY(),
                                     accentB.darker (0.35f), r.getX(), r.getBottom(), false);
            g.setGradientFill (cg);
            g.fillPath (p);
            g.setColour (juce::Colours::white.withAlpha (0.35f));
            g.drawLine (r.getX() + r.getHeight() * 0.5f, r.getY() + 1.5f,
                        r.getRight() - r.getHeight() * 0.5f, r.getY() + 1.5f, 1.2f);
        }
        else
        {
            juce::DropShadow (juce::Colours::black.withAlpha (0.55f), 6, { 0, 3 }).drawForPath (g, p);
            juce::ColourGradient cg (juce::Colour (0xff403858), r.getX(), r.getY(),
                                     juce::Colour (0xff16121f), r.getX(), r.getBottom(), false);
            g.setGradientFill (cg);
            g.fillPath (p);
            g.setColour (juce::Colours::white.withAlpha (highlighted ? 0.28f : 0.10f));
            g.strokePath (p, juce::PathStrokeType (1.0f));
        }
    }

    void drawButtonText (juce::Graphics& g, juce::TextButton& button, bool, bool) override
    {
        g.setFont (juce::Font (juce::FontOptions (12.0f, juce::Font::bold)));
        g.setColour (button.getToggleState() ? juce::Colour (0xff0b0713) : juce::Colour (0xffd2cdea));
        g.drawText (button.getButtonText(), button.getLocalBounds(), juce::Justification::centred, false);
    }

    //--------------------------------------------------------------------------
    // Lista desplegable de presets, hundida y con flecha de color
    void drawComboBox (juce::Graphics& g, int width, int height, bool, int, int, int, int,
                       juce::ComboBox&) override
    {
        auto r = juce::Rectangle<float> (0.0f, 0.0f, (float) width, (float) height).reduced (1.0f);
        g.setColour (juce::Colours::black.withAlpha (0.55f));
        g.fillRoundedRectangle (r, 8.0f);
        g.setColour (juce::Colours::white.withAlpha (0.12f));
        g.drawRoundedRectangle (r, 8.0f, 1.0f);

        juce::Path arrow;
        const float cx = (float) width - 18.0f;
        const float cy = (float) height * 0.5f;
        arrow.addTriangle (cx - 5.0f, cy - 2.5f, cx + 5.0f, cy - 2.5f, cx, cy + 3.5f);
        g.setColour (accentA);
        g.fillPath (arrow);
    }

    juce::Font getComboBoxFont (juce::ComboBox&) override
    {
        return juce::Font (juce::FontOptions (14.0f, juce::Font::bold));
    }
};

//==============================================================================
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
    void setRoom (int room);
    void stepPreset (int delta);
    void readTargets (int& room, float& hueTarget, float& darkTarget, float& sizeOut) const;
    void updateTheme();

    juce::Rectangle<int> getPanelBounds() const { return { 30, 140, 700, 270 }; }

    juce::Colour accentA() const { return juce::Colour::fromHSV (hue, 0.62f, 1.0f, 1.0f); }
    juce::Colour accentB() const { return juce::Colour::fromHSV (std::fmod (hue + 0.09f, 1.0f), 0.55f, 1.0f, 1.0f); }
    juce::Colour bgTop() const    { return juce::Colour::fromHSV (hue, 0.55f, juce::jmap (darkness, 0.34f, 0.07f), 1.0f); }
    juce::Colour bgBottom() const { return juce::Colour::fromHSV (hue, 0.60f, juce::jmap (darkness, 0.17f, 0.025f), 1.0f); }

    ReverbProcessor& verbProcessor;
    MusikLookAndFeel laf;

    Knob knobSize, knobDamping, knobWidth, knobPreDelay, knobLowCut, knobMix;

    juce::ComboBox presetBox;
    juce::TextButton prevButton { "<" }, nextButton { ">" };
    std::array<juce::TextButton, numRooms> roomButtons;

    float hue = 0.6f;
    float darkness = 0.3f;
    float decaySeconds = 1.0f;
    float smoothedLevel = 0.0f;
    float phase = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ReverbEditor)
};
