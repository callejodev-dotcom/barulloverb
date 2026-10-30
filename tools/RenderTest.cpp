// Proba sen interface: os tempos de caída de cada tipo, o predelay, conxelar, o shimmer, todos os
// presets, a mestura e o bypass, o estado, o A/B, os presets propios e o desfacer, o limitador e a
// independencia do tamaño de bloque; tamén capturas da interface.
//   BVRenderTest [saida.wav]                         proba enteira (devolve o número de fallos)
//   BVRenderTest --snapshot saida.png [mais]         a interface (con «mais», o panel Máis aberto)
//   BVRenderTest --render saida.wav [preset]         un preset cun son de proba (golpes e un acorde)
//   BVRenderTest --demo preset entrada.wav saida.wav un preset sobre un son (coa cola enteira)
#include <juce_audio_formats/juce_audio_formats.h>
#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Presets.h"
#include <cstdio>
#include <cstring>

using escoita::U;

static void writeWav (const juce::File& f, const juce::AudioBuffer<float>& b, double sr)
{
    f.deleteFile();
    juce::WavAudioFormat wav;
    if (auto os = std::unique_ptr<juce::OutputStream> (f.createOutputStream()))
        if (auto w = std::unique_ptr<juce::AudioFormatWriter> (wav.createWriterFor (os.get(), sr, (unsigned) b.getNumChannels(), 24, {}, 0)))
        {
            os.release();
            w->writeFromAudioSampleBuffer (b, 0, b.getNumSamples());
            std::printf ("WAV: %s\n", f.getFullPathName().toRawUTF8());
        }
}

// pasa un son polo plugin en bloques (o son pode ser máis curto: despois, silencio). A saída vén á par
// da entrada, coma no DAW: sen as primeiras mostras da latencia (a anticipación do teito)
static juce::AudioBuffer<float> procesar (BVProcessor& p, const juce::AudioBuffer<float>& in, double len, int blockSize, double sr, bool preparar = true)
{
    if (preparar) { p.setPlayConfigDetails (2, 2, sr, blockSize); p.prepareToPlay (sr, blockSize); }
    const int total = (int) (len * sr), lat = p.getLatencySamples();
    juce::AudioBuffer<float> out (2, total), buf (2, blockSize);
    out.clear();
    juce::MidiBuffer midi;
    for (int pos = 0; pos < total + lat; pos += blockSize)
    {
        const int n = juce::jmin (blockSize, total + lat - pos);
        buf.setSize (2, n, false, false, true);
        buf.clear();
        for (int c = 0; c < 2; ++c)
            for (int i = 0; i < n; ++i)
                if (pos + i < in.getNumSamples()) buf.setSample (c, i, in.getSample (c % in.getNumChannels(), pos + i));
        p.processBlock (buf, midi);
        for (int c = 0; c < 2; ++c)
            for (int i = 0; i < n; ++i)
                if (pos + i >= lat && pos + i - lat < total) out.setSample (c, pos + i - lat, buf.getSample (c, i));
    }
    return out;
}

static void setParam (BVProcessor& p, const juce::String& id, float v)
{
    if (auto* prm = p.apvts.getParameter (id)) prm->setValueNotifyingHost (prm->convertTo0to1 (v));
}

// o son de proba: catro golpes (ruído que cae axiña) e un acorde de sinusoides que se apaga
static juce::AudioBuffer<float> sonDeProba (double sr, double len = 2.0)
{
    juce::AudioBuffer<float> b (2, (int) (len * sr));
    b.clear();
    juce::Random rnd (1984);
    for (double t0 : { 0.0, 0.25, 0.5, 0.875 })
        for (int i = 0; i < (int) (0.12 * sr); ++i)
        {
            const float e = 0.5f * std::exp (-(float) i / (float) (0.025 * sr));
            const int k = (int) (t0 * sr) + i;
            b.setSample (0, k, b.getSample (0, k) + e * (rnd.nextFloat() * 2 - 1));
            b.setSample (1, k, b.getSample (1, k) + e * (rnd.nextFloat() * 2 - 1));
        }
    for (double f : { 261.63, 329.63, 392.0, 523.25 })
        for (int i = 0; i < (int) (0.8 * sr); ++i)
        {
            const int k = (int) (1.2 * sr) + i;
            if (k >= b.getNumSamples()) break;
            const float x = 0.09f * std::sin (2.0f * juce::MathConstants<float>::pi * (float) (f * i / sr)) * std::exp (-(float) i / (float) (0.25 * sr))
                            * juce::jmin (1.0f, (float) i / 100.0f);
            b.setSample (0, k, b.getSample (0, k) + x);
            b.setSample (1, k, b.getSample (1, k) + x);
        }
    return b;
}

static juce::AudioBuffer<float> impulso (double sr)
{
    juce::AudioBuffer<float> b (2, (int) (0.01 * sr));
    b.clear();
    b.setSample (0, 0, 0.5f); b.setSample (1, 0, 0.5f);
    return b;
}

static float pico (const juce::AudioBuffer<float>& b, int desde = 0, int ata = -1)
{
    if (ata < 0) ata = b.getNumSamples();
    return juce::jmax (b.getMagnitude (0, desde, ata - desde), b.getMagnitude (1, desde, ata - desde));
}
static bool finito (const juce::AudioBuffer<float>& b)
{
    for (int c = 0; c < b.getNumChannels(); ++c)
        for (int i = 0; i < b.getNumSamples(); ++i) if (! std::isfinite (b.getSample (c, i))) return false;
    return true;
}
static float diferenza (const juce::AudioBuffer<float>& a, const juce::AudioBuffer<float>& b)
{
    float d = 0;
    for (int c = 0; c < 2; ++c)
        for (int i = 0; i < juce::jmin (a.getNumSamples(), b.getNumSamples()); ++i) d = juce::jmax (d, std::abs (a.getSample (c, i) - b.getSample (c, i)));
    return d;
}
static double enerxia (const juce::AudioBuffer<float>& b, double sr, double t0, double t1)
{
    double e = 0;
    for (int c = 0; c < 2; ++c)
        for (int i = (int) (t0 * sr); i < juce::jmin (b.getNumSamples(), (int) (t1 * sr)); ++i) e += (double) b.getSample (c, i) * b.getSample (c, i);
    return e;
}

// o tempo de caída medido: a curva de Schroeder (a enerxía que queda) de −5 a −25 dB, estendida a 60
static double t60 (const juce::AudioBuffer<float>& b, double sr)
{
    const int n = b.getNumSamples();
    std::vector<double> edc ((size_t) n + 1, 0.0);
    for (int i = n - 1; i >= 0; --i)
        edc[(size_t) i] = edc[(size_t) i + 1] + (double) b.getSample (0, i) * b.getSample (0, i) + (double) b.getSample (1, i) * b.getSample (1, i);
    const double total = edc[0] + 1e-30;
    int a = -1, z = -1;
    for (int i = 0; i < n; ++i)
    {
        const double db = 10.0 * std::log10 (edc[(size_t) i] / total + 1e-30);
        if (a < 0 && db <= -5.0) a = i;
        if (z < 0 && db <= -25.0) { z = i; break; }
    }
    return (a < 0 || z < 0) ? -1.0 : 3.0 * (z - a) / sr;
}

// a pegada dun son: FNV-1a de 64 bits das mostras (un cambio de organización non pode movela nin un bit)
static uint64_t pegada (const juce::AudioBuffer<float>& b)
{
    uint64_t h = 1469598103934665603ull;
    for (int c = 0; c < b.getNumChannels(); ++c)
        for (int i = 0; i < b.getNumSamples(); ++i)
        {
            const float x = b.getSample (c, i);
            uint32_t u; std::memcpy (&u, &x, 4);
            for (int k = 0; k < 4; ++k) { h ^= (u >> (8 * k)) & 0xFF; h *= 1099511628211ull; }
        }
    return h;
}

static void snapshot (const juce::File& png, bool mais)
{
    BVProcessor p;
    const double sr = 48000;
    p.setPlayConfigDetails (2, 2, sr, 512);
    p.prepareToPlay (sr, 512);
    p.cargarPreset (mais ? 17 : 0);
    std::unique_ptr<juce::AudioProcessorEditor> ed (p.createEditor());
    auto* ce = dynamic_cast<BVEditor*> (ed.get());
    ed->setSize (ui::MainView::W, ui::MainView::H);
    if (mais) ce->view().mostrarMais (true);
    const auto son = sonDeProba (sr);
    juce::AudioBuffer<float> buf (2, 512);
    for (int blk = 0; blk < 60; ++blk)
    {
        buf.clear();
        for (int c = 0; c < 2; ++c) buf.copyFrom (c, 0, son, c, blk * 512, 512);
        juce::MidiBuffer m; p.processBlock (buf, m);
        ce->view().timerCallback();
    }
    auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, 1.5f);
    png.deleteFile();
    juce::FileOutputStream os (png);
    juce::PNGImageFormat().writeImageToStream (img, os);
    std::printf ("PNG: %s\n", png.getFullPathName().toRawUTF8());
}

int main (int argc, char** argv)
{
    std::setvbuf (stdout, nullptr, _IONBF, 0);
    juce::ScopedJuceInitialiser_GUI init;
    const double sr = 48000;
    const juce::String mode = argc > 1 ? juce::String (argv[1]) : juce::String();

    if (mode == "--snapshot" && argc > 2) { snapshot (juce::File (juce::String (argv[2])), argc > 3 && juce::String (argv[3]) == "mais"); return 0; }

    if (mode == "--render" && argc > 2)
    {
        BVProcessor p;
        p.cargarPreset (argc > 3 ? atoi (argv[3]) : 0);
        writeWav (juce::File (juce::String (argv[2])), procesar (p, sonDeProba (sr), 2.0 + juce::jmin (12.0, p.getTailLengthSeconds()), 512, sr), sr);
        return 0;
    }

    if (mode == "--demo" && argc > 4)
    {
        juce::AudioFormatManager fm; fm.registerBasicFormats();
        std::unique_ptr<juce::AudioFormatReader> r (fm.createReaderFor (juce::File (juce::String (argv[3]))));
        if (r == nullptr) { std::printf ("non se pode ler %s\n", argv[3]); return 1; }
        juce::AudioBuffer<float> in ((int) r->numChannels, (int) r->lengthInSamples);
        r->read (&in, 0, (int) r->lengthInSamples, 0, true, true);
        BVProcessor p;
        p.cargarPreset (atoi (argv[2]));
        const double len = (double) in.getNumSamples() / r->sampleRate + juce::jmin (12.0, p.getTailLengthSeconds());
        writeWav (juce::File (juce::String (argv[4])), procesar (p, in, len, 512, r->sampleRate), r->sampleRate);
        return 0;
    }

    int fails = 0;
    auto check = [&fails] (bool ok, const char* what, const juce::String& extra = {})
    {
        std::printf ("%-52s %s %s\n", what, ok ? "OK" : "FALLO", extra.toRawUTF8());
        fails += ok ? 0 : 1;
    };

    {   // ao abrir: o primeiro preset, cos parámetros nos seus valores de partida (sen editar)
        BVProcessor p;
        juce::String mal;
        for (auto* prm : p.getParameters())
            if (auto* r = dynamic_cast<juce::RangedAudioParameter*> (prm))
                if (std::abs (r->getValue() - r->getDefaultValue()) > 1e-6f) mal << r->getParameterID() << " ";
        check (mal.isEmpty() && p.getPreset() == 0 && ! p.presetEditado() && p.nomePreset() == U (verb::PRESETS[0].nome),
               "ao abrir: o primeiro preset, nos valores de partida", mal);
    }
    {   // a caída: cada tipo (a 2 s e a 5 s) apágase no seu tempo, medido na resposta ao impulso
        juce::String que; bool ok = true;
        for (int t = 0; t < verb::NUM_TIPOS; ++t)
            for (double c : { 2.0, 5.0 })
            {
                BVProcessor p;
                setParam (p, "tipo", (float) t); setParam (p, "caida", (float) c); setParam (p, "mestura", 100.0f);
                setParam (p, "predelay", 0.0f); setParam (p, "brillo", 100.0f); setParam (p, "shimmer", 0.0f);
                setParam (p, "corteGraves", 20.0f); setParam (p, "corteAgudos", 20000.0f);
                const auto o = procesar (p, impulso (sr), c * 1.6 + 0.5, 512, sr);
                const double m = t60 (o, sr);
                ok = ok && finito (o) && m > c * 0.7 && m < c * 1.25;
                que << juce::String (m, 2) << " ";
            }
        check (ok, "caída: cada tipo no seu tempo (2 s e 5 s)", "(" + que.trim() + ")");
    }
    {   // os graves e o brillo: máis graves, máis caída nos graves; menos brillo, menos agudos ao final
        auto banda = [&] (float graves, float brillo, bool agudos)
        {
            BVProcessor p;
            setParam (p, "mestura", 100.0f); setParam (p, "caida", 3.0f); setParam (p, "graves", graves); setParam (p, "brillo", brillo);
            setParam (p, "predelay", 0.0f); setParam (p, "corteGraves", 20.0f); setParam (p, "corteAgudos", 20000.0f);
            auto o = procesar (p, impulso (sr), 2.5, 512, sr);
            // filtro dun polo: baixos (por debaixo de 200 Hz) ou altos (por riba de 4 kHz)
            const double fc = agudos ? 4000.0 : 200.0, k = 1.0 - std::exp (-2 * verb::PI * fc / sr);
            for (int c = 0; c < 2; ++c) { double z = 0; for (int i = 0; i < o.getNumSamples(); ++i) { z += (o.getSample (c, i) - z) * k; o.setSample (c, i, (float) (agudos ? o.getSample (c, i) - z : z)); } }
            return 10.0 * std::log10 ((enerxia (o, sr, 1.5, 2.5) + 1e-30) / (enerxia (o, sr, 0.0, 0.2) + 1e-30));
        };
        const double gPoucos = banda (0.6f, 50, false), gMoitos = banda (1.8f, 50, false);
        const double aEscuro = banda (1.0f, 10, true), aBrillante = banda (1.0f, 90, true);
        check (gMoitos > gPoucos + 6.0 && aBrillante > aEscuro + 6.0, "graves e brillo: cada banda no seu tempo",
               juce::String::formatted ("(graves %.0f/%.0f dB; agudos %.0f/%.0f dB)", gPoucos, gMoitos, aEscuro, aBrillante));
    }
    {   // o predelay: a reverb chega despois do predelay (e non moito despois)
        bool ok = true; juce::String que;
        for (float pd : { 0.0f, 50.0f, 200.0f })
        {
            BVProcessor p;
            setParam (p, "mestura", 100.0f); setParam (p, "predelay", pd);
            const auto o = procesar (p, impulso (sr), 0.6, 512, sr);
            int onde = -1;
            for (int i = 0; i < o.getNumSamples() && onde < 0; ++i) if (std::abs (o.getSample (0, i)) > 1e-3f || std::abs (o.getSample (1, i)) > 1e-3f) onde = i;
            const double ms = onde * 1000.0 / sr;
            ok = ok && ms >= pd - 0.5 && ms < pd + 40.0;
            que << juce::String (ms, 1) << " ";
        }
        check (ok, "predelay: a reverb chega no seu tempo", "(" + que.trim() + " ms)");
    }
    {   // a mestura a 0 é o son tal cal; o bypass, tamén (coa reverb soando)
        const auto son = sonDeProba (sr, 1.0);
        BVProcessor a; setParam (a, "mestura", 0.0f);
        const auto seco = procesar (a, son, 1.0, 512, sr);
        BVProcessor b; setParam (b, "bypass", 1.0f); setParam (b, "volume", 6.0f);
        const auto byp = procesar (b, son, 1.0, 512, sr);
        check (diferenza (seco, son) == 0.0f && diferenza (byp, son) == 0.0f, "mestura 0 e bypass: o son tal cal",
               juce::String::formatted ("(%g, %g)", diferenza (seco, son), diferenza (byp, son)));
    }
    {   // bloques: con bloques de 512 e de 37 mostras sae o mesmo, mostra a mostra (shimmer, atenuación e movemento)
        auto render = [&] (int bloque)
        {
            BVProcessor p;
            p.cargarPreset (17);   // Néboa: shimmer e movemento
            setParam (p, "atenuacion", 40.0f); setParam (p, "reflexions", 60.0f);
            return procesar (p, sonDeProba (sr), 4.0, bloque, sr);
        };
        const auto x = render (512), y = render (37);
        check (diferenza (x, y) == 0.0f && finito (x) && pico (x) > 0.05f, "mesmo son con bloques de 512 e de 37");
        std::printf ("pegada da saída: %016llx\n", (unsigned long long) pegada (x));
        if (argc > 1 && ! mode.startsWith ("--")) writeWav (juce::File (mode), x, sr);
    }
    {   // conxelar: a cola queda (sen apagarse) e o que entra despois xa non entra
        BVProcessor p;
        setParam (p, "mestura", 100.0f); setParam (p, "caida", 1.0f); setParam (p, "predelay", 0.0f);
        p.setPlayConfigDetails (2, 2, sr, 512); p.prepareToPlay (sr, 512);
        auto ruido = sonDeProba (sr, 1.0);
        const auto antes = procesar (p, ruido, 1.0, 512, sr, false);
        setParam (p, "conxelar", 1.0f);
        const auto durante = procesar (p, ruido, 6.0, 512, sr, false);   // outra vez o son: non debe entrar
        const double e0 = enerxia (durante, sr, 0.5, 1.5), e1 = enerxia (durante, sr, 4.5, 5.5);
        const double perda = 10.0 * std::log10 ((e1 + 1e-30) / (e0 + 1e-30));
        setParam (p, "conxelar", 0.0f);
        juce::AudioBuffer<float> silencio (2, 1);
        silencio.clear();   // (un AudioBuffer novo non vén baleiro: en Windows trae lixo)
        const auto despois = procesar (p, silencio, 4.0, 512, sr, false);
        const double fin = 10.0 * std::log10 ((enerxia (despois, sr, 3.0, 4.0) + 1e-30) / (e0 + 1e-30));
        check (perda > -1.0 && perda < 1.0 && fin < -40.0 && finito (durante) && pico (antes) > 0.01f, "conxelar: a cola queda e despois apágase",
               juce::String::formatted ("(%.1f dB en 4 s; ao soltar, %.0f dB)", perda, fin));
    }
    {   // o shimmer ao máximo e a caída máis longa: estable, sen valores non numéricos e acotado
        BVProcessor p;
        setParam (p, "tipo", (float) verb::SHIMMER); setParam (p, "shimmer", 100.0f); setParam (p, "caida", 20.0f);
        setParam (p, "mestura", 100.0f); setParam (p, "brillo", 100.0f); setParam (p, "graves", 2.0f);
        const auto o = procesar (p, sonDeProba (sr), 20.0, 512, sr);
        const float pk = pico (o), fin = pico (o, (int) (18 * sr));
        check (finito (o) && pk <= 0.8913f && fin < pk, "shimmer ao máximo e 20 s: estable", juce::String::formatted ("(pico %.3f; ao final %.3f)", pk, fin));
    }
    {   // os presets: todos soan, sen valores non numéricos, sen tocar o teito co son de proba
        int mal = 0; float peor = 0; juce::String cal;
        for (int i = 0; i < verb::NUM_PRESETS; ++i)
        {
            BVProcessor p;
            p.cargarPreset (i);
            const auto o = procesar (p, sonDeProba (sr), 4.0, 512, sr);
            const float pk = pico (o), cola = pico (o, (int) (1.0 * sr), (int) (1.15 * sr));   // despois do último golpe
            if (! finito (o) || cola < 0.001f || p.presetEditado()) { ++mal; std::printf ("   mal: %s\n", verb::PRESETS[i].nome); }
            if (pk > peor) { peor = pk; cal = U (verb::PRESETS[i].nome); }
        }
        check (mal == 0 && peor < 0.8913f, "os presets soan", "(" + juce::String (verb::NUM_PRESETS) + U ("; o máis alto, ") + cal
               + juce::String::formatted (": %.1f dBFS)", juce::Decibels::gainToDecibels (peor)));
    }
    {   // a sonoridade: a reverb ao 100 % soa coma o son seco (ruído rosa, ±4 dB) en todos os tipos
        juce::AudioBuffer<float> ruido (2, (int) (3 * sr));
        juce::Random rnd (7);
        for (int c = 0; c < 2; ++c)
        {
            double b0 = 0, b1 = 0, b2 = 0;
            for (int i = 0; i < ruido.getNumSamples(); ++i)
            {
                const double w = rnd.nextFloat() * 2 - 1;
                b0 = 0.99765 * b0 + w * 0.0990460; b1 = 0.96300 * b1 + w * 0.2965164; b2 = 0.57000 * b2 + w * 1.0526913;
                ruido.setSample (c, i, (float) (0.05 * (b0 + b1 + b2 + w * 0.1848)));
            }
        }
        const double seco = enerxia (ruido, sr, 1.5, 3.0);
        juce::String que; bool ok = true;
        for (int t = 0; t < verb::NUM_TIPOS; ++t)
        {
            BVProcessor p;
            setParam (p, "tipo", (float) t); setParam (p, "mestura", 100.0f);
            const double db = 10.0 * std::log10 (enerxia (procesar (p, ruido, 3.0, 512, sr), sr, 1.5, 3.0) / seco);
            ok = ok && std::abs (db) < 4.0;
            que << juce::String (db, 1) << " ";
        }
        check (ok, "sonoridade: a reverb soa coma o seco", "(" + que.trim() + " dB)");
    }
    {   // a atenuación: mentres soa o seco, a reverb baixa
        auto cola = [&] (float at)
        {
            BVProcessor p;
            setParam (p, "mestura", 100.0f); setParam (p, "atenuacion", at);
            juce::AudioBuffer<float> tono (2, (int) (2 * sr));
            for (int i = 0; i < tono.getNumSamples(); ++i) { const float x = 0.3f * std::sin (2.0f * juce::MathConstants<float>::pi * 220.0f * (float) i / (float) sr); tono.setSample (0, i, x); tono.setSample (1, i, x); }
            return enerxia (procesar (p, tono, 2.0, 512, sr), sr, 1.0, 2.0);
        };
        const double db = 10.0 * std::log10 (cola (100.0f) / cola (0.0f));
        check (db < -6.0, "atenuación: a reverb baixa mentres soa o seco", juce::String::formatted ("(%.1f dB)", db));
    }
    {   // mono → estéreo: unha entrada mono sae en estéreo (a reverb abre o son)
        BVProcessor p;
        juce::AudioProcessor::BusesLayout l;
        l.inputBuses.add (juce::AudioChannelSet::mono()); l.outputBuses.add (juce::AudioChannelSet::stereo());
        const bool soporta = p.setBusesLayout (l);
        setParam (p, "mestura", 100.0f);
        p.prepareToPlay (sr, 512);
        auto son = sonDeProba (sr, 1.0);
        juce::AudioBuffer<float> buf (2, 512);
        double dif = 0;
        for (int pos = 0; pos + 512 <= son.getNumSamples(); pos += 512)
        {
            buf.clear(); buf.copyFrom (0, 0, son, 0, pos, 512);
            juce::MidiBuffer m; p.processBlock (buf, m);
            for (int i = 0; i < 512; ++i) dif += std::abs (buf.getSample (0, i) - buf.getSample (1, i));
        }
        check (soporta && dif > 1.0, "mono a estéreo");
    }
    {   // estado: os parámetros, o preset e a escala gárdanse e recupéranse; soa igual
        BVProcessor a;
        a.cargarPreset (14);
        setParam (a, "brillo", 33.0f); setParam (a, "volume", -3.0f);
        a.uiScale = 0.8f;
        juce::MemoryBlock mb; a.getStateInformation (mb);
        BVProcessor b; b.setStateInformation (mb.getData(), (int) mb.getSize());
        bool ok = b.getPreset() == 14 && b.nomePreset() == a.nomePreset() && b.presetEditado() && std::abs (b.uiScale - 0.8f) < 1e-6f;
        const auto x = procesar (a, sonDeProba (sr), 2.5, 512, sr), y = procesar (b, sonDeProba (sr), 2.5, 512, sr);
        ok = ok && diferenza (x, y) == 0.0f;
        check (ok, "estado gardado/recuperado (soa igual)", "(" + juce::String ((int) mb.getSize()) + " bytes)");
    }
    {   // A/B e presets propios (do núcleo), co nome do preset
        BVProcessor p;
        p.cargarPreset (0);
        p.estado.cambiarLado();
        p.cargarPreset (10);
        p.estado.cambiarLado();
        bool ok = p.estado.lado() == 0 && p.getPreset() == 0 && ! p.presetEditado();
        p.estado.cambiarLado();
        ok = ok && p.getPreset() == 10 && p.nomePreset() == U (verb::PRESETS[10].nome);
        check (ok, "A/B (os mandos e o preset)");

        const auto dir = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("barulloverb_presets_proba");
        dir.deleteRecursively();
        p.estado.setCarpeta (dir);
        setParam (p, "caida", 7.0f);
        const bool saved = p.estado.gardar ("Proba: placa longa");
        p.cargarPreset (3);
        const auto list = p.estado.presets();
        const bool loaded = list.size() == 1 && list[0].hasFileExtension ("bvpreset") && p.estado.cargar (list[0]);
        const float caida = p.apvts.getRawParameterValue ("caida")->load();
        check (saved && loaded && std::abs (caida - 7.0f) < 0.01f && p.getPreset() == 10, "presets propios: gardar e cargar");
        dir.deleteRecursively();
    }
    {   // desfacer e refacer: cargar un preset e mover un mando, cada un no seu paso
        BVProcessor p;
        p.undo.clearUndoHistory();
        p.estado.novoPaso(); p.cargarPreset (5); p.apvts.copyState(); p.estado.novoPaso();
        setParam (p, "predelay", 100.0f); p.apvts.copyState(); p.estado.novoPaso();
        p.undo.undo();
        const bool u1 = p.getPreset() == 5 && ! p.presetEditado();
        p.undo.undo();
        const bool u2 = p.getPreset() == 0 && ! p.presetEditado();
        p.undo.redo(); p.undo.redo();
        check (u1 && u2 && p.getPreset() == 5 && p.presetEditado(), "desfacer e refacer (preset e mandos)");
    }
    {   // os programas do DAW son os presets
        BVProcessor p;
        p.setCurrentProgram (12);
        check (p.getNumPrograms() == verb::NUM_PRESETS && p.getCurrentProgram() == 12 && p.getProgramName (12) == U (verb::PRESETS[12].nome),
               "programas do DAW = presets");
    }
    {   // o limitador do núcleo: un son forte, ao 100 % e a +6 dB, non pasa de −1 dBFS; a latencia é a anticipación (1,5 ms)
        BVProcessor p;
        p.cargarPreset (16);
        setParam (p, "mestura", 60.0f); setParam (p, "volume", 6.0f);
        auto son = sonDeProba (sr);
        son.applyGain (3.0f);
        const float pk = pico (procesar (p, son, 3.0, 512, sr));
        check (pk <= 0.8913f && p.getLatencySamples() == 72, "teito a -1 dBFS, con 1,5 ms de anticipación", juce::String::formatted ("(pico %.4f; latencia %d)", pk, p.getLatencySamples()));
    }
    return fails;
}
