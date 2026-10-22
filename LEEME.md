**Barullo Verb** é unha reverb: cuartos, salas, placas, igrexas e shimmer. Para o día a día abondan un preset, o tipo de espazo e catro mandos (tamaño, predelay, brillo e mestura). Se queres ir a máis, detrás de **MÁIS** están a difusión, o movemento, os graves, as reflexións, a anchura, os cortes, o shimmer e a atenuación.

Unha ferramenta de [Escoita](https://www.escoita.link).

## Descargas

| Sistema | Instalador | Á man |
|---|---|---|
| **Mac** (Apple Silicon e Intel, macOS 11+): AU, VST3, CLAP e app | `BarulloVerb-…-mac.pkg` | `BarulloVerb-…-mac.zip` |
| **Windows** (64 bits): VST3, CLAP e app | `BarulloVerb-…-windows-setup.exe` | `BarulloVerb-…-windows-x64.zip` |
| **Linux** (x86_64): VST3, LV2, CLAP e app | — | `BarulloVerb-…-linux-x86_64.tar.gz` → `./instalar.sh` |

Se o sistema avisa ao abrilo:
- **Mac:** clic dereito sobre o instalador → *Abrir* → *Abrir*.
- **Windows:** «Windows protexeu o teu PC» → *Máis información* → *Executar de todos os xeitos*.

Avisan porque os instaladores non levan sinatura de pago, non porque haxa nada raro.

## Que ten

- **20 presets de fábrica**, en grupos: *Salas* (Sala de concertos, Sala grande, Auditorio escuro, Cordas longas, Ambiente), *Cuartos* (Cuarto pequeno, Batería no cuarto, Estudio de madeira, Piano íntimo, Garaxe), *Placas* (Placa dourada, Voz pop, Placa escura), *Igrexas* (Igrexa de pedra, Catedral, Cova, Túnel) e *Shimmer* (Néboa, Ceo, Infinito). Son tamén os programas do DAW.
- **O tipo de espazo**: CUARTO, SALA, PLACA, IGREXA ou SHIMMER. Cada un ten o seu tamaño de liñas, as súas reflexións, a súa difusión e o seu brillo.
- **Os catro mandos**: *Tamaño* (canto tarda en apagarse, de 0,1 a 20 s), *Predelay* (ata 250 ms), *Brillo* (canto aguantan os agudos) e *Mestura* (con potencia constante: ao 50 %, o seco e a reverb a −3 dB).
- **CONXELAR**: a cola queda soando sen fin (sen perder nada) e xa non entra son novo. Ao soltalo, apágase no seu tempo.
- **MÁIS**:
  - *O espazo*: difusión, movemento (a cola móvese un chisco, coma un chorus, para que non soe metálica), graves (o tempo dos graves fronte aos medios, de ×0,5 a ×2), reflexións e anchura (de mono a 150 %);
  - *O son da cola*: corte de graves e de agudos (só na reverb, non no seco), shimmer (a cola sobe unha oitava e volve entrar), atenuación (a reverb baixa mentres soa o seco) e volume.
- **A pantalla**: como cae a reverb en cada banda (graves, medios e agudos, cada unha no seu tempo), as primeiras reflexións, o predelay e os niveis de entrada, cola e saída.
- **Presets propios, A/B e desfacer** (do núcleo común): os presets van en `.bvpreset`, en Documentos/Barullo Verb/Presets. Cada clic é un paso de desfacer.
- **Bypass** de verdade: o son pasa tal cal e a reverb segue a soar por dentro (ao quitalo, a cola está aí).
- Un **limitador de seguridade** á saída: nunca pasa de −1 dBFS.
- Entrada mono ou estéreo; sen latencia.

## Como se fixo

`Source/Espazo.h`, sen JUCE:

```
entrada ─ predelay ─┬─ primeiras reflexións (doce tomas por lado) ────────────────┐
                    └─ difusión (catro paso todo) ─ 16 liñas realimentadas ─────┴─ cortes ─ anchura ─ atenuación
                                                    (Hadamard, tres bandas, movemento, shimmer)
```

- **A cola** son 16 liñas de atraso realimentadas e mesturadas cunha matriz de Hadamard normalizada (a enerxía non cambia ao mesturar). O tamaño das liñas vai co tipo e co tempo de caída.
- **Tres bandas**: en cada liña, os graves (por debaixo de 250 Hz), os medios e os agudos (por riba de 2,5–6 kHz, segundo o brillo) teñen cada un a súa ganancia, calculada para que cada banda caia 60 dB no seu tempo.
- **O movemento**: cada liña lese nun punto que se move moi amodo (a súa propia velocidade e fase). **Conxelada**, as liñas quedan quedas e lense sen interpolar: así a cola non perde nada.
- **O shimmer**: un cambio de tono dunha oitava (dúas lecturas que se cruzan) dentro da realimentación.
- **A atenuación** segue a envolvente da entrada e baixa a reverb ata 24 dB.

## Estrutura

| Ficheiro | Que é |
|---|---|
| `Source/Espazo.h` | A reverb |
| `Source/Presets.h` | Os presets de fábrica |
| `Source/PluginProcessor.*` | O plugin: parámetros, presets, estado e a mestura |
| `Source/PluginEditor.*` | A interface |
| `tools/RenderTest.cpp` | A proba |

Usa o núcleo común de Escoita ([escoita-nucleo](https://github.com/callejodev-dotcom/escoita-nucleo), v0.3.0): o texto en galego, o aviso das versións novas, os presets propios, o A/B e o desfacer (`escoita::Estado`), o limitador de seguridade (`escoita::Teito`), a CI e os instaladores.

## Probas

- **Ao abrir**: o primeiro preset, cos parámetros nos seus valores de partida.
- **Caída**: cada tipo, a 2 s e a 5 s, apágase no seu tempo (medido coa curva de Schroeder na resposta ao impulso).
- **Graves e brillo**: máis graves, cola máis longa nos graves; máis brillo, máis agudos ao final.
- **Predelay**: a reverb chega no seu tempo (0, 50 e 200 ms).
- **Mestura a 0 e bypass**: o son sae tal cal, bit a bit.
- **Bloques**: con bloques de 512 e de 37 mostras sae o mesmo, mostra a mostra (e a súa pegada).
- **Conxelar**: a cola non perde nada en 4 s e despois apágase.
- **Shimmer ao máximo e 20 s**: estable.
- **Os 20 presets** soan, sen valores non numéricos e sen tocar o teito.
- **Sonoridade**: ao 100 %, a reverb soa coma o seco (±4 dB) en todos os tipos.
- **Atenuación**, **mono a estéreo**, **estado**, **A/B**, **presets propios**, **desfacer**, **programas do DAW** e **limitador**.
- **Validación**: pluginval ao nivel máximo (10), auval no Mac e clap-validator.

Na CI van en Mac, Windows e Linux, e tamén con AddressSanitizer e UBSan (`tools/probas-ci.sh`).

## Compilar

Nun Mac: `./build_mac.sh` (compila e instala no teu usuario). Á man:

```
cmake -B build -DCMAKE_BUILD_TYPE=Release -DBARULLO_TESTS=ON
cmake --build build --config Release
build/BVRenderTest_artefacts/Release/BVRenderTest
```

Outras ordes da proba: `--snapshot saida.png [mais]` (a interface), `--render saida.wav [preset]` (un preset cun son de proba) e `--demo preset entrada.wav saida.wav` (un preset sobre un son teu).

## Publicar unha versión

```
git tag v0.1.0 && git push origin v0.1.0
```

A versión sae da liña `project(BarulloVerb VERSION …)` de `CMakeLists.txt`; o texto da versión, de `packaging/NOTAS-VERSION.md`.

## Créditos e licenzas

- **Barullo Verb é software libre**, baixo a licenza **GNU Affero General Public License v3.0** (ficheiro `LICENSE`): podes usalo, estudalo, modificalo e compartilo; se distribúes unha versión modificada, ten que ir co seu código e coa mesma licenza. Usa JUCE 8 coa súa licenza AGPLv3.
- Tipografías Montserrat e Inter (licenza OFL, `Resources/Montserrat-OFL.txt` e `Resources/Inter-OFL.txt`).
