#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    // Tiempos de los filtros a 44.1 kHz (esquema clásico tipo Freeverb)
    constexpr int combTuning[8]    = { 1116, 1188, 1277, 1356, 1422, 1491, 1557, 1617 };
    constexpr int allpassTuning[4] = { 556, 441, 341, 225 };
    constexpr int stereoSpread     = 23;
    constexpr float inputGain      = 0.015f;
    constexpr float wetScale       = 3.0f;
    constexpr float maxRoomScale   = 2.25f;

    void setParam (juce::AudioProcessorValueTreeState& state, const char* id, float value)
    {
        if (auto* p = state.getParameter (id))
            p->setValueNotifyingHost (p->convertTo0to1 (value));
    }
}

juce::AudioProcessorValueTreeState::ParameterLayout ReverbProcessor::createLayout()
{
    using P = juce::AudioParameterFloat;
    using R = juce::NormalisableRange<float>;
    using A = juce::AudioParameterFloatAttributes;

    auto percentText = [] (float v, int) { return juce::String (juce::roundToInt (v * 100.0f)) + " %"; };
    auto percentParse = [] (const juce::String& t) { return t.getFloatValue() / 100.0f; };

    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "room", 1 }, "Room",
        juce::StringArray { "Room", "Plate", "Chamber", "Hall", "Cathedral", "Space" }, 3));

    layout.add (std::make_unique<P> (juce::ParameterID { "size", 1 }, "Size",
                                     R (0.0f, 1.0f, 0.001f), 0.6f,
                                     A().withStringFromValueFunction (percentText)
                                        .withValueFromStringFunction (percentParse)));

    layout.add (std::make_unique<P> (juce::ParameterID { "damping", 1 }, "Damping",
                                     R (0.0f, 1.0f, 0.001f), 0.5f,
                                     A().withStringFromValueFunction (percentText)
                                        .withValueFromStringFunction (percentParse)));

    layout.add (std::make_unique<P> (juce::ParameterID { "width", 1 }, "Width",
                                     R (0.0f, 1.0f, 0.001f), 1.0f,
                                     A().withStringFromValueFunction (percentText)
                                        .withValueFromStringFunction (percentParse)));

    layout.add (std::make_unique<P> (juce::ParameterID { "predelay", 1 }, "Pre-Delay",
                                     R (0.0f, 200.0f, 0.1f), 20.0f,
                                     A().withStringFromValueFunction ([] (float v, int) { return juce::String (v, 1) + " ms"; })
                                        .withValueFromStringFunction ([] (const juce::String& t) { return t.getFloatValue(); })));

    layout.add (std::make_unique<P> (juce::ParameterID { "lowcut", 1 }, "Low Cut",
                                     R (20.0f, 1000.0f, 1.0f, 0.4f), 100.0f,
                                     A().withStringFromValueFunction ([] (float v, int) { return juce::String (juce::roundToInt (v)) + " Hz"; })
                                        .withValueFromStringFunction ([] (const juce::String& t) { return t.getFloatValue(); })));

    layout.add (std::make_unique<P> (juce::ParameterID { "mix", 1 }, "Mix",
                                     R (0.0f, 1.0f, 0.001f), 0.3f,
                                     A().withStringFromValueFunction (percentText)
                                        .withValueFromStringFunction (percentParse)));
    return layout;
}

ReverbProcessor::ReverbProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createLayout())
{
}

bool ReverbProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo()
        && layouts.getMainInputChannelSet() == layouts.getMainOutputChannelSet();
}

float ReverbProcessor::estimateDecaySeconds (int room, float size)
{
    room = juce::jlimit (0, numRooms - 1, room);
    const auto& r = roomSpecs[room];
    const float fb = r.fbLo + size * (r.fbHi - r.fbLo);
    const float delaySeconds = 1370.0f * r.scale / 44100.0f;
    return juce::jlimit (0.05f, 60.0f, -3.0f * delaySeconds / std::log10 (fb));
}

float ReverbProcessor::computeFeedback (int room) const
{
    const auto& r = roomSpecs[juce::jlimit (0, numRooms - 1, room)];
    const float size = apvts.getRawParameterValue ("size")->load();
    return r.fbLo + size * (r.fbHi - r.fbLo);
}

float ReverbProcessor::computeDamp (int room) const
{
    const auto& r = roomSpecs[juce::jlimit (0, numRooms - 1, room)];
    const float damping = apvts.getRawParameterValue ("damping")->load();
    return juce::jlimit (0.0f, 0.7f, damping * 0.4f + r.dampBias);
}

void ReverbProcessor::applyRoom (int room)
{
    room = juce::jlimit (0, numRooms - 1, room);
    const double srScale = currentSampleRate / 44100.0;
    const double sc = (double) roomSpecs[room].scale;

    for (int i = 0; i < numCombs; ++i)
    {
        combL[(size_t) i].setLength ((int) (combTuning[i] * srScale * sc));
        combR[(size_t) i].setLength ((int) ((combTuning[i] + stereoSpread) * srScale * sc));
        combL[(size_t) i].clear();
        combR[(size_t) i].clear();
    }

    currentRoom = room;
}

void ReverbProcessor::prepareToPlay (double sampleRate, int)
{
    currentSampleRate = sampleRate;
    const double srScale = sampleRate / 44100.0;

    for (int i = 0; i < numCombs; ++i)
    {
        combL[(size_t) i].init ((int) (combTuning[i] * srScale * maxRoomScale) + 8);
        combR[(size_t) i].init ((int) ((combTuning[i] + stereoSpread) * srScale * maxRoomScale) + 8);
    }

    for (int i = 0; i < numAllpasses; ++i)
    {
        allpassL[(size_t) i].init ((int) (allpassTuning[i] * srScale));
        allpassR[(size_t) i].init ((int) ((allpassTuning[i] + stereoSpread) * srScale));
    }

    preDelayBuffer.assign ((size_t) (sampleRate * 0.25) + 1, 0.0f);
    preDelayWrite = 0;
    hpStateL = hpStateR = 0.0f;

    const int room = juce::jlimit (0, numRooms - 1, (int) apvts.getRawParameterValue ("room")->load());
    applyRoom (room);

    for (auto* s : { &feedbackSmooth, &dampSmooth, &mixSmooth, &widthSmooth, &lowCutSmooth })
        s->reset (sampleRate, 0.05);

    feedbackSmooth.setCurrentAndTargetValue (computeFeedback (room));
    dampSmooth.setCurrentAndTargetValue (computeDamp (room));
    mixSmooth.setCurrentAndTargetValue (apvts.getRawParameterValue ("mix")->load());
    widthSmooth.setCurrentAndTargetValue (apvts.getRawParameterValue ("width")->load());
    lowCutSmooth.setCurrentAndTargetValue (apvts.getRawParameterValue ("lowcut")->load());
}

void ReverbProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numSamples = buffer.getNumSamples();
    if (buffer.getNumChannels() < 2 || numSamples == 0)
        return;

    const int room = juce::jlimit (0, numRooms - 1, (int) apvts.getRawParameterValue ("room")->load());
    if (room != currentRoom)
        applyRoom (room);

    feedbackSmooth.setTargetValue (computeFeedback (room));
    dampSmooth.setTargetValue (computeDamp (room));
    mixSmooth.setTargetValue (apvts.getRawParameterValue ("mix")->load());
    widthSmooth.setTargetValue (apvts.getRawParameterValue ("width")->load());
    lowCutSmooth.setTargetValue (apvts.getRawParameterValue ("lowcut")->load());

    const float preDelayMs = apvts.getRawParameterValue ("predelay")->load();
    const int pdSize = (int) preDelayBuffer.size();
    const int pdSamples = juce::jlimit (0, pdSize - 1, (int) (preDelayMs * 0.001 * currentSampleRate));

    auto* left  = buffer.getWritePointer (0);
    auto* right = buffer.getWritePointer (1);

    float peak = 0.0f;

    for (int n = 0; n < numSamples; ++n)
    {
        const float fb    = feedbackSmooth.getNextValue();
        const float damp  = dampSmooth.getNextValue();
        const float mix   = mixSmooth.getNextValue();
        const float width = widthSmooth.getNextValue();
        const float lcHz  = lowCutSmooth.getNextValue();

        const float hpCoef = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * lcHz
                                              / (float) currentSampleRate);

        const float dryL = left[n];
        const float dryR = right[n];

        // Pre-delay
        preDelayBuffer[(size_t) preDelayWrite] = (dryL + dryR) * inputGain;
        int readPos = preDelayWrite - pdSamples;
        if (readPos < 0)
            readPos += pdSize;
        const float input = preDelayBuffer[(size_t) readPos];
        if (++preDelayWrite >= pdSize)
            preDelayWrite = 0;

        // 8 peines en paralelo
        float outL = 0.0f, outR = 0.0f;
        for (int c = 0; c < numCombs; ++c)
        {
            outL += combL[(size_t) c].process (input, fb, damp);
            outR += combR[(size_t) c].process (input, fb, damp);
        }

        // 4 pasa-todos en serie
        for (int a = 0; a < numAllpasses; ++a)
        {
            outL = allpassL[(size_t) a].process (outL);
            outR = allpassR[(size_t) a].process (outR);
        }

        // Ancho estéreo
        const float w1 = width * 0.5f + 0.5f;
        const float w2 = (1.0f - width) * 0.5f;
        float wetL = outL * w1 + outR * w2;
        float wetR = outR * w1 + outL * w2;

        // Low cut sobre la señal procesada
        hpStateL += hpCoef * (wetL - hpStateL);
        hpStateR += hpCoef * (wetR - hpStateR);
        wetL -= hpStateL;
        wetR -= hpStateR;

        // Mezcla de potencia constante
        const float dryGain = std::cos (mix * juce::MathConstants<float>::halfPi);
        const float wetGain = std::sin (mix * juce::MathConstants<float>::halfPi) * wetScale;

        left[n]  = dryL * dryGain + wetL * wetGain;
        right[n] = dryR * dryGain + wetR * wetGain;

        peak = juce::jmax (peak, std::abs (left[n]), std::abs (right[n]));
    }

    outputLevel.store (juce::jmin (peak, 1.0f));
}

void ReverbProcessor::loadPreset (int index)
{
    index = juce::jlimit (0, numPresets - 1, index);
    currentPresetIndex = index;

    const auto& p = factoryPresets[index];
    setParam (apvts, "room",     (float) p.room);
    setParam (apvts, "size",     p.size);
    setParam (apvts, "damping",  p.damping);
    setParam (apvts, "width",    p.width);
    setParam (apvts, "predelay", p.preDelay);
    setParam (apvts, "lowcut",   p.lowCut);
    setParam (apvts, "mix",      p.mix);
}

juce::AudioProcessorEditor* ReverbProcessor::createEditor()
{
    return new ReverbEditor (*this);
}

void ReverbProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void ReverbProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ReverbProcessor();
}
