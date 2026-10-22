// Espazo.h — A reverb de Barullo Verb, sen JUCE.
//
//   entrada ─ predelay ─┬─ primeiras reflexións (doce tomas por lado) ────────────────┐
//                       └─ catro paso todo (difusión) ─┐                              │
//                         ┌────────────────────────────┴──────────────┐               │
//                         │  16 liñas de atraso, cada unha co seu      │               ├─ corte de
//                         │  filtro de tres bandas (graves, medios,    │── cola ───────┤  graves e
//                         │  agudos: cada banda cae no seu tempo) e    │               │  agudos,
//                         │  movéndose un chisco; mestúranse cunha     │               │  anchura
//                         │  matriz de Hadamard e volven entrar        │               │  e atenuación
//                         └─ (shimmer: a cola unha oitava por riba) ───┘               ┘
//
// Os tipos (cuarto, sala, placa, igrexa, shimmer) cambian a forma: o tamaño das liñas, as
// reflexións, a difusión e o movemento. O tempo de caída é o mesmo para todas as liñas
// (cada unha calcula a súa ganancia polo seu largo), así que a cola cae limpa, sen ecos soltos.
// Sen latencia e sen reservar memoria fóra de preparar().
#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <vector>

namespace verb {

constexpr double PI = 3.14159265358979323846;

enum Tipo { CUARTO = 0, SALA, PLACA, IGREXA, SHIMMER, NUM_TIPOS };

struct Axustes
{
    int tipo = SALA;
    double caida = 2.5;          // s (T60 dos medios)
    double predelay = 20;        // ms
    double brillo = 0.55;        // 0–1: canto aguantan os agudos
    double difusion = 0.7;       // 0–1
    double modulacion = 0.35;    // 0–1
    double graves = 1.0;         // 0,5–2: o tempo dos graves fronte aos medios
    double reflexions = 0.5;     // 0–1
    double anchura = 1.0;        // 0–1,5
    double corteGraves = 80;     // Hz (na cola)
    double corteAgudos = 16000;  // Hz (na cola)
    double shimmer = 0;          // 0–1
    double atenuacion = 0;       // 0–1: a cola baixa mentres soa a entrada (ata −24 dB)
    bool conxelar = false;       // a cola queda soando sen fin e non entra nada novo
};

// unha liña de atraso con lectura fraccionaria (a mostra máis nova é a 0)
struct Linea
{
    std::vector<float> b;
    int w = 0;
    void preparar (int max) { b.assign ((size_t) std::max (8, max + 4), 0.0f); w = 0; }
    void limpar() { std::fill (b.begin(), b.end(), 0.0f); }
    void escribir (float x) { if (--w < 0) w = (int) b.size() - 1; b[(size_t) w] = x; }
    float ler (double atras) const
    {
        const int n = (int) b.size();
        atras = std::clamp (atras, 0.0, (double) (n - 3));
        const int i = (int) atras;
        const float f = (float) (atras - i);
        int a = w + i; if (a >= n) a -= n;
        int c = a + 1; if (c >= n) c -= n;
        return b[(size_t) a] + (b[(size_t) c] - b[(size_t) a]) * f;
    }
    float lerEnteiro (int atras) const { int a = w + atras; if (a >= (int) b.size()) a -= (int) b.size(); return b[(size_t) a]; }
};

// biquad (fórmulas do RBJ), para os cortes da saída
struct Biquad
{
    double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;
    void reset() { z1 = z2 = 0; }
    double run (double x) { const double y = b0 * x + z1; z1 = b1 * x - a1 * y + z2; z2 = b2 * x - a2 * y; return y; }
    void paso (double fs, double f, bool alto)
    {
        const double w = 2 * PI * std::min (f, 0.45 * fs) / fs, c = std::cos (w), al = std::sin (w) / (2 * 0.7071);
        const double a0 = 1 + al;
        if (alto) { b0 = (1 + c) / 2 / a0; b1 = -(1 + c) / a0; b2 = b0; }
        else { b0 = (1 - c) / 2 / a0; b1 = (1 - c) / a0; b2 = b0; }
        a1 = -2 * c / a0; a2 = (1 - al) / a0;
    }
};

class Espazo
{
public:
    static constexpr int N = 16;

    void preparar (double frecuencia)
    {
        fs = frecuencia;
        predelay[0].preparar ((int) (0.5 * fs)); predelay[1].preparar ((int) (0.5 * fs));   // o predelay e as reflexións máis afastadas
        for (auto& d : difusor) for (auto& c : d) c.preparar ((int) (0.05 * fs));
        for (auto& l : linas) l.preparar ((int) (0.32 * fs));
        oitava.preparar ((int) (0.2 * fs));
        limpar();
        primeira = true;
        axustar (a);
    }

    void limpar()
    {
        for (auto& p : predelay) p.limpar();
        for (auto& d : difusor) for (auto& c : d) c.limpar();
        for (auto& l : linas) l.limpar();
        oitava.limpar();
        lpG.fill (0); lpA.fill (0); saida.fill (0);
        for (auto& b : cortes) b.reset();
        env = 0; faseShimmer = 0; shimmerLp = 0;
        fases.fill (0);
    }

    // os axustes (unha vez por bloque): calcula as ganancias e os obxectivos; o que move
    // tempos (predelay, tamaño das liñas) esvara mostra a mostra, sen saltos
    void axustar (const Axustes& x)
    {
        a = x;
        const auto& f = FORMA[std::clamp (a.tipo, 0, NUM_TIPOS - 1)];
        const double t60 = std::clamp (a.caida, 0.1, 30.0);
        escala = f.linas * std::clamp (0.8 + 0.2 * std::log2 (t60), 0.6, 1.5);
        for (int i = 0; i < N; ++i)
        {
            obxLonx[(size_t) i] = LONX_MS[i] * escala * fs / 1000.0;
            // cada banda cae no seu tempo: os graves co multiplicador, os agudos segundo o brillo
            auto g = [&] (double t) { return std::pow (10.0, -3.0 * obxLonx[(size_t) i] / (std::max (0.05, t) * fs)); };
            const double brillo = std::clamp (a.brillo, 0.0, 1.0);
            gM[(size_t) i] = a.conxelar ? 1.0 : g (t60);
            gG[(size_t) i] = a.conxelar ? 1.0 : g (t60 * std::clamp (a.graves, 0.3, 2.5));
            gA[(size_t) i] = a.conxelar ? 1.0 : g (t60 * (0.08 + 0.92 * brillo * brillo) * f.agudos);
        }
        // os dous filtros de cada liña: graves por debaixo de 250 Hz, agudos por riba de 2,5–6 kHz
        cG = 1.0 - std::exp (-2 * PI * 250.0 / fs);
        cA = 1.0 - std::exp (-2 * PI * (2500.0 + 3500.0 * a.brillo) / fs);
        obxPre = std::clamp (a.predelay, 0.0, 250.0) * fs / 1000.0;
        difK = (0.35 + 0.4 * std::clamp (a.difusion, 0.0, 1.0)) * f.difusion;
        prof = std::clamp (a.modulacion, 0.0, 1.0) * f.movemento * 0.0016 * fs;   // ata 1,6 ms
        erNivel = std::clamp (a.reflexions, 0.0, 1.0) * f.reflexions;
        erEscala = f.reflEscala * std::clamp (0.7 + 0.15 * std::log2 (t60), 0.5, 1.4);
        cortes[0].paso (fs, std::clamp (a.corteGraves, 20.0, 1000.0), true); cortes[1].paso (fs, std::clamp (a.corteGraves, 20.0, 1000.0), true);
        cortes[2].paso (fs, std::clamp (a.corteAgudos, 1000.0, 20000.0), false); cortes[3].paso (fs, std::clamp (a.corteAgudos, 1000.0, 20000.0), false);
        envAtaque = 1.0 - std::exp (-1.0 / (0.005 * fs));
        envSolta = std::exp (-1.0 / (0.25 * fs));
    }

    // unha mostra: entran l e r (a entrada) e saen a cola e as reflexións (só o molleiro)
    void procesar (float& l, float& r)
    {
        const double inL = l, inR = r;
        if (primeira) { lonx = obxLonx; pre = obxPre; primeira = false; }   // a primeira mostra, xa nos tempos (sen esvarar)
        // atenuación: a cola baixa mentres a entrada soa forte
        const double pk = std::max (std::abs (inL), std::abs (inR));
        env = pk > env ? env + (pk - env) * envAtaque : env * envSolta;
        const double envDb = 20.0 * std::log10 (env + 1e-9);
        const double duck = a.atenuacion > 0 ? std::pow (10.0, -a.atenuacion * 24.0 * std::clamp ((envDb + 40.0) / 40.0, 0.0, 1.0) / 20.0) : 1.0;

        pre += (obxPre - pre) * 0.0008;
        predelay[0].escribir ((float) inL); predelay[1].escribir ((float) inR);
        const double pl = a.conxelar ? 0.0 : predelay[0].ler (pre), pr = a.conxelar ? 0.0 : predelay[1].ler (pre);

        // primeiras reflexións: doce tomas por lado, alternando o signo
        double erL = 0, erR = 0;
        if (erNivel > 0)
            for (int k = 0; k < 12; ++k)
            {
                const double gk = REFL_G[k];
                erL += gk * predelay[0].ler (pre + REFL_MS[0][k] * erEscala * fs / 1000.0);
                erR += gk * predelay[1].ler (pre + REFL_MS[1][k] * erEscala * fs / 1000.0);
            }
        erL *= erNivel; erR *= erNivel;

        // difusión: catro paso todo por lado
        double x[2] = { pl + erL * 0.5, pr + erR * 0.5 };
        for (int i = 0; i < 4; ++i)
            for (int c = 0; c < 2; ++c)
            {
                auto& d = difusor[(size_t) i][(size_t) c];
                const double v = d.ler (DIF_MS[i] * (c ? 1.13 : 1.0) * fs / 1000.0), u = x[c] + difK * v;
                d.escribir ((float) u);
                x[c] = v - difK * u;
            }

        // shimmer: a cola de hai un intre unha oitava por riba (dúas lecturas que se cruzan)
        double sh = 0;
        if (a.shimmer > 0 && ! a.conxelar)   // (conxelado, o shimmer non: sumaría sen fin)
        {
            const double xin = 0.5 * (saida[0] + saida[1]);
            oitava.escribir ((float) xin);
            const double ventana = 0.06 * fs;
            faseShimmer += 1.0 / ventana;   // a lectura vai ao dobre de velocidade: unha oitava
            if (faseShimmer >= 1.0) faseShimmer -= 1.0;
            const double f2 = faseShimmer + 0.5 >= 1.0 ? faseShimmer - 0.5 : faseShimmer + 0.5;
            const double v1 = oitava.ler ((1.0 - faseShimmer) * ventana + 2), v2 = oitava.ler ((1.0 - f2) * ventana + 2);
            const double w1 = std::sin (PI * faseShimmer), w2 = std::sin (PI * f2);
            shimmerLp += ((v1 * w1 * w1 + v2 * w2 * w2) - shimmerLp) * 0.35;
            sh = std::tanh (shimmerLp * 1.5) / 1.5 * std::clamp (a.shimmer, 0.0, 1.0) * 0.3;
        }

        // as 16 liñas: ler (movéndose), filtrar e mesturar
        std::array<double, N> s {};
        for (int i = 0; i < N; ++i)
        {
            lonx[(size_t) i] += (obxLonx[(size_t) i] - lonx[(size_t) i]) * 0.0004;
            auto& fa = fases[(size_t) i];
            if (! a.conxelar) { fa += 2 * PI * MOV_HZ[i] / fs; if (fa > 2 * PI) fa -= 2 * PI; }
            const double m = prof > 0 ? prof * (0.5 + 0.5 * std::sin (fa + i * 1.7)) : 0.0;
            // conxelada, as liñas quedan quedas e lense sen interpolar: así a cola non perde nada
            double v = a.conxelar ? linas[(size_t) i].lerEnteiro ((int) std::lround (lonx[(size_t) i] + m)) : linas[(size_t) i].ler (lonx[(size_t) i] + m);
            lpG[(size_t) i] += (v - lpG[(size_t) i]) * cG;
            lpA[(size_t) i] += (v - lpA[(size_t) i]) * cA;
            const double graves = lpG[(size_t) i], agudos = v - lpA[(size_t) i], medios = v - graves - agudos;
            s[(size_t) i] = graves * gG[(size_t) i] + medios * gM[(size_t) i] + agudos * gA[(size_t) i];
        }
        // as saídas, antes de mesturar
        double oL = 0, oR = 0;
        for (int i = 0; i < N; ++i) { oL += s[(size_t) i] * SAIDA_L[i]; oR += s[(size_t) i] * SAIDA_R[i]; }
        oL *= 0.25; oR *= 0.25;
        // Hadamard 16 (rápido) e normalizado: a enerxía non cambia ao mesturar
        for (int len = 1; len < N; len <<= 1)
            for (int i = 0; i < N; i += len << 1)
                for (int j = i; j < i + len; ++j) { const double u = s[(size_t) j], v = s[(size_t) (j + len)]; s[(size_t) j] = u + v; s[(size_t) (j + len)] = u - v; }
        const double entradaL = x[0] * 0.5 + sh, entradaR = x[1] * 0.5 + sh;
        for (int i = 0; i < N; ++i) linas[(size_t) i].escribir ((float) (s[(size_t) i] * 0.25 + ((i & 1) ? entradaR : entradaL)));
        saida[0] = oL; saida[1] = oR;

        // a saída: cola e reflexións, cortes, anchura e atenuación
        double wl = oL * NIVEL + erL, wr = oR * NIVEL + erR;
        wl = cortes[2].run (cortes[0].run (wl)); wr = cortes[3].run (cortes[1].run (wr));
        const double mid = 0.5 * (wl + wr), side = 0.5 * (wl - wr) * std::clamp (a.anchura, 0.0, 1.5);
        l = (float) ((mid + side) * duck); r = (float) ((mid - side) * duck);
    }

    const Axustes& axustes() const { return a; }

    // para a pantalla: o tempo de caída de cada banda (0 graves, 1 medios, 2 agudos) e onde caen
    // as reflexións (ms despois do predelay) e canto
    static double tempoBanda (const Axustes& x, int banda)
    {
        const auto& f = FORMA[std::clamp (x.tipo, 0, NUM_TIPOS - 1)];
        const double t60 = std::clamp (x.caida, 0.1, 30.0), b = std::clamp (x.brillo, 0.0, 1.0);
        return banda == 0 ? t60 * std::clamp (x.graves, 0.3, 2.5) : banda == 1 ? t60 : t60 * (0.08 + 0.92 * b * b) * f.agudos;
    }
    static int numReflexions() { return 12; }
    static double reflexionMs (const Axustes& x, int k)
    {
        const auto& f = FORMA[std::clamp (x.tipo, 0, NUM_TIPOS - 1)];
        return REFL_MS[0][k] * f.reflEscala * std::clamp (0.7 + 0.15 * std::log2 (std::clamp (x.caida, 0.1, 30.0)), 0.5, 1.4);
    }
    static double reflexionNivel (const Axustes& x, int k)
    {
        return std::abs (REFL_G[k]) * std::clamp (x.reflexions, 0.0, 1.0) * FORMA[std::clamp (x.tipo, 0, NUM_TIPOS - 1)].reflexions;
    }

private:
    struct Forma { double linas, reflexions, reflEscala, difusion, movemento, agudos; };
    // cuarto: liñas curtas e reflexións fortes; sala: o equilibrio; placa: sen reflexións, moi
    // difusa e brillante; igrexa: liñas longas, reflexións afastadas e agudos que caen antes;
    // shimmer: sala con moito movemento
    static constexpr Forma FORMA[NUM_TIPOS] = {
        { 0.55, 1.00, 0.55, 0.95, 0.6, 0.85 },
        { 1.00, 0.70, 1.00, 1.00, 1.0, 0.80 },
        { 0.80, 0.00, 0.40, 1.15, 0.5, 1.00 },
        { 1.55, 0.55, 1.90, 1.00, 0.8, 0.60 },
        { 1.15, 0.40, 1.20, 1.05, 1.6, 0.90 },
    };
    static constexpr double LONX_MS[N] = { 23.3, 27.7, 31.9, 36.1, 40.3, 44.9, 49.1, 53.9, 58.3, 63.1, 67.9, 72.7, 77.9, 83.3, 89.1, 95.3 };
    static constexpr double MOV_HZ[N] = { 0.31, 0.47, 0.53, 0.67, 0.71, 0.83, 0.89, 0.97, 1.03, 1.13, 0.37, 0.59, 0.79, 0.43, 0.61, 1.07 };
    static constexpr double SAIDA_L[N] = { 1, -1, 1, 1, -1, 1, -1, -1, 1, 1, -1, 1, -1, 1, 1, -1 };
    static constexpr double SAIDA_R[N] = { -1, 1, 1, -1, 1, 1, -1, 1, -1, 1, 1, -1, -1, -1, 1, 1 };
    static constexpr double DIF_MS[4] = { 4.77, 3.59, 12.73, 9.31 };
    static constexpr double REFL_MS[2][12] = { { 4.3, 7.1, 11.9, 15.7, 19.3, 23.9, 29.1, 33.7, 41.3, 47.9, 55.1, 63.7 },
                                               { 5.3, 8.9, 10.7, 17.3, 21.1, 26.3, 30.7, 37.9, 43.1, 51.7, 58.9, 67.3 } };
    static constexpr double REFL_G[12] = { 0.62, -0.52, 0.46, -0.41, 0.36, 0.33, -0.29, 0.26, -0.23, 0.2, -0.18, 0.16 };
    static constexpr double NIVEL = 1.7;   // co ruído continuo e a caída de partida, a cola sae uns 3 dB por debaixo da entrada

    double fs = 48000;
    Axustes a;
    bool primeira = true;
    double escala = 1, obxPre = 0, pre = 0, difK = 0.6, prof = 0, erNivel = 0, erEscala = 1, cG = 0.03, cA = 0.3;
    double env = 0, envAtaque = 0.004, envSolta = 0.9999, faseShimmer = 0, shimmerLp = 0;
    std::array<Linea, 2> predelay;
    std::array<std::array<Linea, 2>, 4> difusor;
    std::array<Linea, N> linas;
    Linea oitava;
    std::array<double, N> lonx {}, obxLonx {}, gG {}, gM {}, gA {}, lpG {}, lpA {}, fases {};
    std::array<double, 2> saida {};
    std::array<Biquad, 4> cortes;
};

} // namespace verb
