#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Presets.h"

using escoita::U;

BVProcessor::BVProcessor()
    : AudioProcessor (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true)
                                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, &undo, "BarulloVerb", createLayout())
{
    auto p = [this] (const char* id) { return apvts.getRawParameterValue (id); };
    tipo = p ("tipo"); caida = p ("caida"); predelay = p ("predelay"); brillo = p ("brillo"); mestura = p ("mestura");
    difusion = p ("difusion"); modulacion = p ("modulacion"); graves = p ("graves"); reflexions = p ("reflexions");
    anchura = p ("anchura"); corteGraves = p ("corteGraves"); corteAgudos = p ("corteAgudos"); shimmer = p ("shimmer");
    atenuacion = p ("atenuacion"); conxelar = p ("conxelar"); volume = p ("volume"); bypass = p ("bypass");
    nome = U (verb::PRESETS[0].nome);
}

juce::AudioProcessorValueTreeState::ParameterLayout BVProcessor::createLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout l;
    const auto& p0 = verb::PRESETS[0];   // os valores de partida son os do primeiro preset
    const StringArray onOff { "Off", "On" };
    // o texto cun número fixo de decimais (texto → valor → texto dá o mesmo texto)
    auto real = [] (const char* id, const String& nomeP, NormalisableRange<float> r, float def, const char* unidade, int decimais = 1)
    {
        return std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, nomeP, r, def, AudioParameterFloatAttributes().withLabel (unidade)
                   .withStringFromValueFunction ([decimais] (float v, int) { return String (v, decimais); }));
    };
    auto centro = [] (float mn, float mx, float c) { NormalisableRange<float> r (mn, mx); r.setSkewForCentre (c); return r; };
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "tipo", 1 }, "Tipo", StringArray { "Cuarto", "Sala", "Placa", "Igrexa", "Shimmer" }, p0.tipo));
    l.add (real ("caida", U ("Tamaño (caída)"), centro (0.1f, 20.0f, 2.0f), (float) p0.caida, "s", 2));
    l.add (real ("predelay", "Predelay", centro (0.0f, 250.0f, 40.0f), (float) p0.predelay, "ms"));
    l.add (real ("brillo", "Brillo", { 0.0f, 100.0f }, (float) p0.brillo * 100.0f, "%"));
    l.add (real ("mestura", "Mestura", { 0.0f, 100.0f }, (float) p0.mestura * 100.0f, "%"));
    l.add (real ("difusion", U ("Difusión"), { 0.0f, 100.0f }, (float) p0.difusion * 100.0f, "%"));
    l.add (real ("modulacion", "Movemento", { 0.0f, 100.0f }, (float) p0.modulacion * 100.0f, "%"));
    l.add (real ("graves", "Graves", centro (0.5f, 2.0f, 1.0f), (float) p0.graves, "x", 2));
    l.add (real ("reflexions", U ("Reflexións"), { 0.0f, 100.0f }, (float) p0.reflexions * 100.0f, "%"));
    l.add (real ("anchura", "Anchura", { 0.0f, 150.0f }, (float) p0.anchura * 100.0f, "%"));
    l.add (real ("corteGraves", "Corte de graves", centro (20.0f, 1000.0f, 120.0f), (float) p0.corteGraves, "Hz", 0));
    l.add (real ("corteAgudos", "Corte de agudos", centro (1000.0f, 20000.0f, 6000.0f), (float) p0.corteAgudos, "Hz", 0));
    l.add (real ("shimmer", "Shimmer", { 0.0f, 100.0f }, (float) p0.shimmer * 100.0f, "%"));
    l.add (real ("atenuacion", U ("Atenuación"), { 0.0f, 100.0f }, (float) p0.atenuacion * 100.0f, "%"));
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "conxelar", 1 }, "Conxelar", onOff, 0));
    l.add (real ("volume", "Volume", { -24.0f, 6.0f, 0.1f }, 0.0f, "dB"));
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "bypass", 1 }, "Bypass", onOff, 0));
    return l;
}

// ================================================================ presets
int BVProcessor::getNumPrograms() { return verb::NUM_PRESETS; }
const juce::String BVProcessor::getProgramName (int i) { return U (verb::PRESETS[juce::jlimit (0, verb::NUM_PRESETS - 1, i)].nome); }

static std::vector<std::pair<const char*, float>> valoresDe (const verb::Preset& p)
{
    return { { "tipo", (float) p.tipo }, { "caida", (float) p.caida }, { "predelay", (float) p.predelay }, { "brillo", (float) p.brillo * 100.0f },
             { "mestura", (float) p.mestura * 100.0f }, { "difusion", (float) p.difusion * 100.0f }, { "modulacion", (float) p.modulacion * 100.0f },
             { "graves", (float) p.graves }, { "reflexions", (float) p.reflexions * 100.0f }, { "anchura", (float) p.anchura * 100.0f },
             { "corteGraves", (float) p.corteGraves }, { "corteAgudos", (float) p.corteAgudos }, { "shimmer", (float) p.shimmer * 100.0f },
             { "atenuacion", (float) p.atenuacion * 100.0f }, { "conxelar", 0.0f } };
}

void BVProcessor::cargarPreset (int i)
{
    i = juce::jlimit (0, verb::NUM_PRESETS - 1, i);
    const auto antes = std::make_pair (preset, nome);
    for (auto [id, v] : valoresDe (verb::PRESETS[i]))
        if (auto* prm = apvts.getParameter (id))
        {
            const float n = prm->convertTo0to1 (v);
            if (std::abs (prm->getValue() - n) < 1.0e-6f) continue;
            prm->beginChangeGesture(); prm->setValueNotifyingHost (n); prm->endChangeGesture();
        }
    const auto despois = std::make_pair (i, U (verb::PRESETS[i].nome));
    // o número e o nome do preset tamén se desfán (os parámetros xa os leva o desfacer do APVTS)
    estado.cambio (U ("preset"), [this, despois] { preset = despois.first; nome = despois.second; },
                                 [this, antes] { preset = antes.first; nome = antes.second; });
    updateHostDisplay (ChangeDetails().withProgramChanged (true));
}

bool BVProcessor::presetEditado() const
{
    const int i = juce::jlimit (0, verb::NUM_PRESETS - 1, preset);
    if (nome != U (verb::PRESETS[i].nome)) return false;   // un preset propio
    for (auto [id, v] : valoresDe (verb::PRESETS[i]))
        if (auto* prm = apvts.getParameter (id))
            if (std::abs (prm->getValue() - prm->convertTo0to1 (v)) > 0.002f) return true;
    return false;
}

escoita::Estado::Opcions BVProcessor::opcionsEstado()
{
    escoita::Estado::Opcions o;
    o.extension = "bvpreset";
    o.excluir = { "bypass" };
    // o nome do que se está a escoitar vai co estado (un preset propio leva o seu)
    o.extra = [this] { juce::ValueTree t ("Preset"); t.setProperty ("numero", preset, nullptr); t.setProperty ("nome", nome, nullptr); return t; };
    o.aplicarExtra = [this] (const juce::ValueTree& t) { preset = juce::jlimit (0, verb::NUM_PRESETS - 1, (int) t.getProperty ("numero", 0)); nome = t.getProperty ("nome", nome).toString(); };
    return o;
}

verb::Axustes BVProcessor::axustes() const
{
    verb::Axustes a;
    a.tipo = (int) std::lround (tipo->load());
    a.caida = caida->load(); a.predelay = predelay->load(); a.brillo = brillo->load() / 100.0;
    a.difusion = difusion->load() / 100.0; a.modulacion = modulacion->load() / 100.0; a.graves = graves->load();
    a.reflexions = reflexions->load() / 100.0; a.anchura = anchura->load() / 100.0; a.corteGraves = corteGraves->load();
    a.corteAgudos = corteAgudos->load(); a.shimmer = shimmer->load() / 100.0; a.atenuacion = atenuacion->load() / 100.0;
    a.conxelar = conxelar->load() > 0.5f;
    return a;
}

double BVProcessor::getTailLengthSeconds() const { return juce::jmin (30.0, (double) caida->load() * 1.5 + 0.3); }

// ================================================================ audio
// o volume en pasos de 0,1 dB: a 0 dB, a ganancia é 1 xusto (coa mestura a 0, o son sae tal cal)
static double ganancia (float db) { return juce::Decibels::decibelsToGain (std::round ((double) db * 10.0) / 10.0); }

bool BVProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    const auto in = l.getMainInputChannelSet(), out = l.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::stereo() && out != juce::AudioChannelSet::mono()) return false;
    return in == out || (in == juce::AudioChannelSet::mono() && out == juce::AudioChannelSet::stereo());
}

void BVProcessor::prepareToPlay (double sampleRate, int)
{
    sr = sampleRate;
    setLatencySamples (0);
    espazo.preparar (sampleRate);
    espazo.axustar (axustes());
    teito.preparar (sampleRate, 0.080, 0.891);
    mestS = mestura->load() / 100.0;
    volS = ganancia (volume->load());
    bypassS = bypass->load() > 0.5f ? 1.0 : 0.0;
}

void BVProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    const int ins = getTotalNumInputChannels(), outs = getTotalNumOutputChannels();
    float* L = buffer.getWritePointer (0);
    float* R = outs > 1 ? buffer.getWritePointer (1) : nullptr;
    if (R != nullptr && ins < 2) std::copy (L, L + n, R);   // mono → estéreo

    espazo.axustar (axustes());
    const double mestT = mestura->load() / 100.0, volT = ganancia (volume->load()), bypassT = bypass->load() > 0.5f ? 1.0 : 0.0;
    const double sm = 1.0 - std::exp (-1.0 / (0.02 * sr));
    float pe = 0, pc = 0, ps = 0;
    for (int i = 0; i < n; ++i)
    {
        mestS += (mestT - mestS) * sm; volS += (volT - volS) * sm; bypassS += (bypassT - bypassS) * sm;
        const double dl = L[i], dr = R != nullptr ? R[i] : L[i];
        float wl = (float) dl, wr = (float) dr;
        espazo.procesar (wl, wr);
        // mestura con potencia constante: ao 50 %, os dous a −3 dB
        const double gs = std::cos (mestS * verb::PI / 2), gm = std::sin (mestS * verb::PI / 2);
        double l = (dl * gs + wl * gm) * volS, r = (dr * gs + wr * gm) * volS;
        if (R != nullptr) teito.procesar (l, r);
        else l = r = teito.procesar (0.5 * (l + r));
        // o bypass deixa pasar o son tal cal (sen o teito); a reverb segue a soar por dentro
        l = l * (1.0 - bypassS) + dl * bypassS; r = r * (1.0 - bypassS) + dr * bypassS;
        L[i] = (float) l;
        if (R != nullptr) R[i] = (float) r;
        pe = std::max ({ pe, (float) std::abs (dl), (float) std::abs (dr) });
        pc = std::max ({ pc, std::abs (wl), std::abs (wr) });
        ps = std::max ({ ps, (float) std::abs (l), (float) std::abs (r) });
    }
    for (int ch = 2; ch < outs; ++ch) buffer.clear (ch, 0, n);
    nivelEntrada.store (pe); nivelCola.store (pc); nivelSaida.store (ps);
}

// ================================================================ estado
void BVProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto st = apvts.copyState();
    st.setProperty ("preset", preset, nullptr);
    st.setProperty ("nomePreset", nome, nullptr);
    st.setProperty ("uiScale", uiScale, nullptr);
    if (auto xml = st.createXml()) copyXmlToBinary (*xml, destData);
}

void BVProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    auto xml = getXmlFromBinary (data, sizeInBytes);
    if (! xml || ! xml->hasTagName (apvts.state.getType())) return;
    auto tree = juce::ValueTree::fromXml (*xml);
    uiScale = juce::jlimit (0.5f, 2.0f, (float) tree.getProperty ("uiScale", 1.0f));
    preset = juce::jlimit (0, verb::NUM_PRESETS - 1, (int) tree.getProperty ("preset", 0));
    nome = tree.getProperty ("nomePreset", U (verb::PRESETS[preset].nome)).toString();
    apvts.replaceState (tree);
    updateHostDisplay (ChangeDetails().withProgramChanged (true));
}

juce::AudioProcessorEditor* BVProcessor::createEditor() { return new BVEditor (*this); }

#ifndef BARULLO_NO_PLUGIN_ENTRY
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new BVProcessor(); }
#endif
