#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include <vector>

// Filtro de peine con amortiguación (lowpass en el lazo de realimentación)
struct CombFilter
{
    std::vector<float> buffer;
    int index = 0;
    float store = 0.0f;

    void init (int size)
    {
        buffer.assign ((size_t) juce::jmax (1, size), 0.0f);
        index = 0;
        store = 0.0f;
    }

    inline float process (float input, float feedback, float damp)
    {
        const float out = buffer[(size_t) index];
        store = out * (1.0f - damp) + store * damp;
        buffer[(size_t) index] = input + store * feedback;
        if (++index >= (int) buffer.size())
            index = 0;
        return out;
    }
};

// Filtro pasa-todo (difusión)
struct AllpassFilter
{
    std::vector<float> buffer;
    int index = 0;

    void init (int size)
    {
        buffer.assign ((size_t) juce::jmax (1, size), 0.0f);
        index = 0;
    }

    inline float process (float input)
    {
        const float b = buffer[(size_t) index];
        const float out = -input + b;
        buffer[(size_t) index] = input + b * 0.5f;
        if (++index >= (int) buffer.size())
            index = 0;
        return out;
    }
};

class ReverbProcessor : public juce::AudioProcessor
{
public:
    ReverbProcessor();
    ~ReverbProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "SimpleVerb"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 8.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    static constexpr int numCombs = 8;
    static constexpr int numAllpasses = 4;

    std::array<CombFilter, numCombs> combL, combR;
    std::array<AllpassFilter, numAllpasses> allpassL, allpassR;

    std::vector<float> preDelayBuffer;
    int preDelayWrite = 0;

    float hpStateL = 0.0f, hpStateR = 0.0f;
    double currentSampleRate = 44100.0;

    juce::SmoothedValue<float> feedbackSmooth, dampSmooth, mixSmooth, widthSmooth, lowCutSmooth;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ReverbProcessor)
};
