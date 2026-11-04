#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include <atomic>
#include <vector>

//==============================================================================
// Tipos de sala: escala del tamaño, rango de realimentación y brillo
struct RoomSpec
{
    const char* name;
    float scale;      // escala de los tiempos de retardo
    float fbLo;       // realimentación con Size = 0
    float fbHi;       // realimentación con Size = 1
    float dampBias;   // ajuste del amortiguamiento (brillo)
};

inline constexpr int numRooms = 6;

inline constexpr RoomSpec roomSpecs[numRooms] =
{
    { "ROOM",      0.45f, 0.55f, 0.84f,  0.00f },
    { "PLATE",     0.70f, 0.65f, 0.90f, -0.05f },
    { "CHAMBER",   0.85f, 0.68f, 0.92f,  0.00f },
    { "HALL",      1.20f, 0.74f, 0.95f,  0.00f },
    { "CATHEDRAL", 1.70f, 0.80f, 0.965f, 0.04f },
    { "SPACE",     2.20f, 0.88f, 0.965f, 0.06f }
};

//==============================================================================
// Presets de fábrica
struct ReverbPreset
{
    const char* name;
    int room;
    float size, damping, width, preDelay, lowCut, mix;
};

inline constexpr ReverbPreset factoryPresets[] =
{
    { "Small Room",      0, 0.35f, 0.50f, 0.80f,  5.0f,  80.0f, 0.22f },
    { "Tight Ambience",  0, 0.20f, 0.70f, 0.60f,  0.0f, 120.0f, 0.18f },
    { "Drum Room",       0, 0.50f, 0.45f, 0.90f,  0.0f, 100.0f, 0.30f },
    { "Vocal Plate",     1, 0.50f, 0.35f, 1.00f, 15.0f, 150.0f, 0.25f },
    { "Bright Plate",    1, 0.65f, 0.20f, 1.00f, 20.0f, 200.0f, 0.30f },
    { "Warm Chamber",    2, 0.55f, 0.60f, 0.90f, 12.0f,  90.0f, 0.28f },
    { "Concert Hall",    3, 0.60f, 0.50f, 1.00f, 25.0f, 100.0f, 0.30f },
    { "Big Hall",        3, 0.80f, 0.45f, 1.00f, 35.0f, 120.0f, 0.35f },
    { "Cathedral",       4, 0.70f, 0.55f, 1.00f, 40.0f, 100.0f, 0.35f },
    { "Huge Cathedral",  4, 0.90f, 0.50f, 1.00f, 60.0f, 120.0f, 0.40f },
    { "Ambient Space",   5, 0.70f, 0.40f, 1.00f, 50.0f, 150.0f, 0.45f },
    { "Endless Dark",    5, 1.00f, 0.75f, 1.00f, 80.0f, 200.0f, 0.50f }
};

inline constexpr int numPresets = (int) (sizeof (factoryPresets) / sizeof (factoryPresets[0]));

//==============================================================================
// Filtro de peine con amortiguación y longitud variable
struct CombFilter
{
    std::vector<float> buffer;
    int length = 1;
    int index = 0;
    float store = 0.0f;

    void init (int maxSize)
    {
        buffer.assign ((size_t) juce::jmax (1, maxSize), 0.0f);
        length = (int) buffer.size();
        index = 0;
        store = 0.0f;
    }

    void setLength (int newLength)
    {
        length = juce::jlimit (1, (int) buffer.size(), newLength);
        if (index >= length)
            index = 0;
    }

    void clear()
    {
        std::fill (buffer.begin(), buffer.end(), 0.0f);
        index = 0;
        store = 0.0f;
    }

    inline float process (float input, float feedback, float damp)
    {
        const float out = buffer[(size_t) index];
        store = out * (1.0f - damp) + store * damp;
        buffer[(size_t) index] = input + store * feedback;
        if (++index >= length)
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

//==============================================================================
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

    const juce::String getName() const override { return "Musikverb"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 20.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // Carga un preset de fábrica (lo usa la interfaz)
    void loadPreset (int index);

    // Duración aproximada del reverb en segundos (para el color y el indicador)
    static float estimateDecaySeconds (int room, float size);

    juce::AudioProcessorValueTreeState apvts;

    std::atomic<float> outputLevel { 0.0f };
    int currentPresetIndex = 6;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    void applyRoom (int room);
    float computeFeedback (int room) const;
    float computeDamp (int room) const;

    static constexpr int numCombs = 8;
    static constexpr int numAllpasses = 4;

    std::array<CombFilter, numCombs> combL, combR;
    std::array<AllpassFilter, numAllpasses> allpassL, allpassR;

    std::vector<float> preDelayBuffer;
    int preDelayWrite = 0;
    int currentRoom = -1;

    float hpStateL = 0.0f, hpStateR = 0.0f;
    double currentSampleRate = 44100.0;

    juce::SmoothedValue<float> feedbackSmooth, dampSmooth, mixSmooth, widthSmooth, lowCutSmooth;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ReverbProcessor)
};
