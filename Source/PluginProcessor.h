#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <escoita_nucleo/escoita_nucleo.h>
#include "Espazo.h"

// Barullo Verb: unha reverb. Catro mandos (tamaño, predelay, brillo, mestura) e o tipo de espazo
// para o día a día; detrás de «Máis», a difusión, o movemento, os graves, as reflexións, a anchura,
// os cortes, o shimmer e a atenuación. Os presets de fábrica son os programas.
class BVProcessor : public juce::AudioProcessor
{
public:
    BVProcessor();
    ~BVProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;
    juce::AudioProcessorParameter* getBypassParameter() const override { return apvts.getParameter ("bypass"); }

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Barullo Verb"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override { return preset; }
    void setCurrentProgram (int index) override { cargarPreset (index); }
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    juce::UndoManager undo;   // desfacer e refacer (declárase antes ca o apvts)
    juce::AudioProcessorValueTreeState apvts;
    // presets propios (.bvpreset en Documentos/Barullo Verb/Presets), A/B e desfacer, do núcleo
    escoita::Estado estado { apvts, &undo, opcionsEstado() };

    // ---- presets de fábrica (desde o fío da interface)
    void cargarPreset (int i);
    int getPreset() const { return preset; }
    juce::String nomePreset() const { return nome; }
    bool presetEditado() const;   // os mandos xa non son os do preset

    verb::Axustes axustes() const;   // os parámetros de agora (para a pantalla)

    // para a pantalla (escríbese desde o fío de audio)
    std::atomic<float> nivelEntrada { 0 }, nivelCola { 0 }, nivelSaida { 0 };
    float uiScale = 1.0f;

private:
    escoita::Estado::Opcions opcionsEstado();
    int preset = 0;
    juce::String nome;

    verb::Espazo espazo;
    escoita::Teito teito;
    double sr = 48000, mestS = 0, volS = 1, bypassS = 0;
    std::atomic<float> *tipo = nullptr, *caida = nullptr, *predelay = nullptr, *brillo = nullptr, *mestura = nullptr, *difusion = nullptr,
                       *modulacion = nullptr, *graves = nullptr, *reflexions = nullptr, *anchura = nullptr, *corteGraves = nullptr,
                       *corteAgudos = nullptr, *shimmer = nullptr, *atenuacion = nullptr, *conxelar = nullptr, *volume = nullptr, *bypass = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BVProcessor)
};
