// Presets.h — Os presets de fábrica de Barullo Verb, por grupos. O primeiro de «Salas» é o de
// partida (os parámetros saen del).
#pragma once
#include "Espazo.h"

namespace verb {

struct Preset
{
    const char* nome;
    const char* grupo;
    const char* descricion;
    int tipo;
    double caida, predelay, brillo, mestura, difusion, modulacion, graves, reflexions, anchura, corteGraves, corteAgudos, shimmer, atenuacion;
};

inline const Preset PRESETS[] = {
    // grupo           nome                 tipo     caída  pre  brillo mest  dif  mov  graves refl anch corteG corteA  shim aten
    { "Sala de concertos", "Salas", "Unha sala de concertos boa: cola longa e redonda, as primeiras reflexións ao seu tempo.",
      SALA,    2.4,  25, 0.50, 0.30, 0.80, 0.35, 1.10, 0.50, 1.00,   70, 12000, 0.0, 0.0 },
    { "Sala grande", "Salas", "Máis grande e máis aberta: para cordas, coros e fondos.",
      SALA,    3.8,  40, 0.45, 0.35, 0.85, 0.40, 1.20, 0.45, 1.10,   60, 10000, 0.0, 0.0 },
    { "Auditorio escuro", "Salas", "Unha sala con cortinas: os agudos apáganse axiña e quedan os graves.",
      SALA,    3.0,  30, 0.20, 0.32, 0.80, 0.35, 1.40, 0.40, 1.00,   50,  6000, 0.0, 0.0 },
    { "Cordas longas", "Salas", "Cinco segundos de sala que se move un pouco: as cordas e os fondos flotan.",
      SALA,    5.0,  30, 0.50, 0.35, 0.90, 0.45, 1.20, 0.30, 1.20,   60, 10000, 0.0, 0.0 },
    { "Ambiente", "Salas", "Case non se oe: só pon a pista nun sitio. Para pegar unha mestura.",
      SALA,    0.9,  10, 0.55, 0.15, 0.70, 0.20, 1.00, 0.50, 1.00,  120, 12000, 0.0, 0.0 },

    { "Cuarto pequeno", "Cuartos", "Un cuarto pequeno e vivo: dá corpo sen que se note a reverb.",
      CUARTO,  0.5,   5, 0.60, 0.22, 0.70, 0.20, 1.00, 0.70, 0.90,  100, 12000, 0.0, 0.0 },
    { "Batería no cuarto", "Cuartos", "Curto e con pegada, e baixa mentres soa a batería para que non embarre.",
      CUARTO,  0.8,   0, 0.70, 0.20, 0.80, 0.15, 0.90, 0.90, 1.00,  120, 11000, 0.0, 0.2 },
    { "Estudio de madeira", "Cuartos", "Un estudio cálido, de madeira: os agudos máis suaves e os graves un pouco máis longos.",
      CUARTO,  1.1,  12, 0.50, 0.25, 0.75, 0.25, 1.20, 0.60, 1.00,   80,  9000, 0.0, 0.0 },
    { "Piano íntimo", "Cuartos", "Un cuarto bonito para un piano ou unha voz soa, preto.",
      CUARTO,  1.3,  15, 0.60, 0.25, 0.80, 0.25, 1.10, 0.50, 1.10,   60, 12000, 0.0, 0.0 },
    { "Garaxe", "Cuartos", "Paredes duras e reflexións que se oen: un local de ensaio.",
      CUARTO,  1.6,   8, 0.75, 0.30, 0.55, 0.20, 0.90, 0.90, 1.10,   90, 14000, 0.0, 0.0 },

    { "Placa dourada", "Placas", "A placa clásica: densa, brillante e sen reflexións. Para voces e caixas.",
      PLACA,   1.8,  15, 0.65, 0.28, 0.85, 0.25, 0.90, 0.00, 1.10,  120, 14000, 0.0, 0.0 },
    { "Voz pop", "Placas", "Placa curta cun predelay longo: a voz vai diante e a reverb detrás, e baixa mentres cantas.",
      PLACA,   1.4,  60, 0.70, 0.22, 0.80, 0.20, 0.80, 0.00, 1.00,  180, 12000, 0.0, 0.35 },
    { "Placa escura", "Placas", "Unha placa longa e morna, para que a reverb non tape nada por riba.",
      PLACA,   2.6,  10, 0.25, 0.30, 0.90, 0.30, 1.10, 0.00, 1.00,   90,  7000, 0.0, 0.0 },

    { "Igrexa de pedra", "Igrexas", "Pedra por todas partes: unha cola longa e as reflexións lonxe.",
      IGREXA,  4.5,  35, 0.45, 0.35, 0.85, 0.30, 1.20, 0.60, 1.10,   50,  9000, 0.0, 0.0 },
    { "Catedral", "Igrexas", "Oito segundos de catedral: para coros, órganos e notas soltas.",
      IGREXA,  8.0,  60, 0.35, 0.40, 0.90, 0.35, 1.30, 0.50, 1.20,   40,  8000, 0.0, 0.0 },
    { "Cova", "Igrexas", "Unha cova: escura, cos graves que non acaban e un eco que volve.",
      IGREXA,  6.0,  80, 0.20, 0.40, 0.60, 0.20, 1.50, 0.80, 1.00,   40,  5000, 0.0, 0.0 },
    { "Túnel", "Igrexas", "Un túnel de formigón: reflexións marcadas e moito predelay.",
      IGREXA,  3.5, 120, 0.40, 0.35, 0.40, 0.10, 1.00, 1.00, 0.80,   80,  8000, 0.0, 0.0 },

    { "Néboa", "Shimmer", "A cola sobe unha oitava e brilla por riba: para fondos e guitarras.",
      SHIMMER, 6.0,  40, 0.60, 0.40, 0.90, 0.60, 1.00, 0.30, 1.30,  150, 12000, 0.45, 0.0 },
    { "Ceo", "Shimmer", "Moito shimmer e doce segundos: as notas quedan no aire, unha oitava por riba.",
      SHIMMER, 12.0, 80, 0.70, 0.45, 0.95, 0.70, 0.90, 0.20, 1.40,  200, 15000, 0.70, 0.0 },
    { "Infinito", "Shimmer", "Vinte segundos que case non acaban (con CONXELAR, xa non acaban).",
      SALA,   20.0,  30, 0.45, 0.45, 0.90, 0.50, 1.00, 0.20, 1.30,   80, 10000, 0.0, 0.0 },
};

constexpr int NUM_PRESETS = (int) (sizeof (PRESETS) / sizeof (PRESETS[0]));

} // namespace verb
