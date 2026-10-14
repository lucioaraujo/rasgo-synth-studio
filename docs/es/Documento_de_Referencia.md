# Rasgo Synth — Documento de Referencia del Proyecto

Instrumento generativo — concepto, historia, arquitectura, funcionalidades, flujo de uso, procedimientos realizados, validaciones, seguridad, layout y pendientes.

> **Sobre este documento**: es la referencia técnica estable del proyecto, actualizada conforme ocurren mejoras reales (no en cada sesión). Para el diario de a bordo fase a fase, con el historial completo de decisiones y el motivo de cada una, la fuente de verdad sigue siendo [`Plano_de_Upgrade_Instrumento_Generativo.md`](../../Plano_de_Upgrade_Instrumento_Generativo.md) (solo en portugués) en la raíz del proyecto — en caso de divergencia entre ambos, ese documento es autoritativo.

Última actualización: 22 de agosto de 2026 (referencia sincronizada con el trabajo operativo de las fases 118–123).

---

## 1. Concepto

Rasgo Synth es un **instrumento generativo**: usted define una configuración inicial (una semilla numérica, un tempo) y pulsa componer, el software escribe solo una pieza entera, con desarrollo y variación continua a lo largo del tiempo, no un bucle que se repite. Una consola de performance en vivo sigue siendo posible como modo secundario (la app ya reproduce audio en tiempo real vía `juce_audio_devices`), pero el objetivo principal siempre fue otro: servir de base para pistas de álbum.

La frase que resume el proyecto, y que hoy aparece en la propia interfaz de la app, es: **"se arriesga, y suena"** ("se arrisca, e soa"). Cada ejecución es sonoramente única por naturaleza, el motor está construido sobre dinámica caótica (nueve mapas — Lorenz, Hénon, Latoocarfian y otros seis del "Chaos Cookbook") y osciladores inestables que se cruzan y se realimentan, la misma cualidad sonora encontrada experimentando con los circuitos CMOS del proyecto de hardware Antitotem que precede a este software.

Un principio de diseño que atraviesa todo el proyecto: **portar la idea, no copiar el circuito**. Cada vez que un esquema real (CBS, Buchla, TR-808, MFOS/Ray Wilson, Instruō, Qu-Bit Electronix y varios otros) fue estudiado, el objetivo fue entender el comportamiento sonoro y reimplementarlo en C++ puro, nunca simular transistor por transistor.

El porqué filosófico detrás de esta técnica está registrado en [`Manifesto.md`](Manifesto.md).

Otro principio, reafirmado varias veces a lo largo del desarrollo: cuando un parámetro puede ser decidido por la semilla o por control manual del usuario, el proyecto tiende a elegir uno de los dos, no una mezcla arbitraria. La escala misma dejó de ser un control manual (fase 48) — hoy es siempre una elección de la propia semilla, dentro de un repertorio de 39 escalas investigadas más una porción de escalas generadas algorítmicamente (crivos de Xenakis, fase 61). Los controles manuales sobrevivieron solo donde tienen sentido como "afinación del instrumento" (el tempo, el volumen de salida), no como superposición de decisiones estructurales que la semilla ya toma.

Un giro de fase explícito ocurrió en la fase 80: de "agregar módulos aislados" a "multiplicar encuentros entre módulos ya existentes". Los **pesos de juego de Caillois** (Alea/Agon/Ilinx/Mimicry, ver sección 4.9) formalizan esto en una verdadera máquina de reglas, y la fase 84 (Curto-Circuito, sección 4.10) lleva esa lógica a un extremo: un mismo evento puede romper el propio protocolo que la semilla había establecido.

---

## 2. Historia

### 2.1 Kosmische (2013)

Una de las referencias técnicas iniciales fue una pericia sobre Kosmische, proyecto externo de Nicolas Steven Miller: una app Android que embebía un servidor SuperCollider completo vía el NDK. El binario nativo de Kosmische traía tres familias de UGens relevantes: `ChaosUGens` (los nueve mapas caóticos), `GendynUGens` (síntesis estocástica dinámica de Xenakis) y `AY_UGen` (emulación del chip AY-3-8910). Esa investigación informó conceptos de Rasgo Synth, pero Kosmische no integra Rasgo ni constituye su origen de autoría.

### 2.2 Antitotem y los módulos físicos de Rasgo

Antitotem es un instrumento de hardware construido a mano, de naturaleza modular y CMOS/discreta, que antecede a Rasgo y fue posteriormente incorporado al proyecto como su precursor y patrono. La expresión "Rasgo en hardware" se refiere al conjunto de módulos físicos de autoría de esa fase (desde 2013), ya concebidos con vistas al estándar Eurorack.

Desde entonces, Rasgo evolucionó hacia algo mayor: hoy es el proyecto paraguas que reúne todos los instrumentos y programas musicales de autoría de Lucio de Araujo. Rasgo Synth es una obra dentro de ese conjunto, mientras Antitotem permanece simultáneamente como obra incorporada y origen simbólico del linaje. Rasgo Synth Performance es ahora un prototipo standalone local e independiente para DSP en vivo; una consola embebida o hardware siguen siendo posibilidades posteriores, no la definición de este instrumento de software.

### 2.3 Decisión de arquitectura

La decisión técnica fue no revivir el servidor SuperCollider como dependencia de runtime. Los conceptos investigados fueron reimplementados en C++ como código propio de Rasgo Synth, y luego ampliamente expandidos — hoy, decenas de mecanismos más allá de lo que existía en 2013.

### 2.4 Licenciamiento

El código propio actual de Rasgo Synth Studio usa **AGPL-3.0-or-later**. Kosmische también es GPLv3, de forma independiente. Las condiciones específicas de la app construida con JUCE están registradas en `THIRD_PARTY_NOTICES.md`; toda la atribución por mecanismo está en `CREDITS.md`.

---

## 3. Arquitectura JUCE / C++

El proyecto se divide en dos subproyectos independientes.

### 3.1 `rasgo-synth-core` — el motor DSP

C++17 puro, sin ninguna dependencia de plataforma. Compilado directamente con g++ (sin CMake en esta máquina).

- **`src/dsp/`** — decenas de clases DSP: osciladores clásicos con PolyBLEP; nueve mapas caóticos; un oscilador Walsh (base ortogonal ±1, fase 59); VOSIM (pulsos sin² decayendo, fase 72); filtro ladder Moog, filtro state-variable Chamberlin y filtro Polivoks auto-oscilante (fase 75); waveshaper de diodo; resonador modal; nube granular; divisor de reloj XOR polirrítmico; ocho voces de percusión digitales; `RhythmSieve.hpp` (crivo rítmico de Xenakis, fase 82) y `DrumArchetypes.hpp` (7 arquetipos nombrados + un slot de crivo); Rungler, Fracture Folder, Leibniz Rectifier; `ParenthesisGlitch.hpp` (fase 70); `SieveScale.hpp` (escala generada por crivo de Xenakis, fase 61); Gendyn, AY-3-8910; una fuente de voltaje aleatorio tipo Buchla.
- **`src/sequencer/`** — 39 escalas investigadas (`Scales.hpp`, fase 48) más crivos generados; un secuenciador de pasos (`StepSequencer.hpp`) con conducción de voz real por semitonos, hocketing/cascada polifónica, progresiones de acordes y transformaciones Neo-Riemannianas P/L/R (mutuamente exclusivas); un arco narrativo por cadena de Markov con gramáticas conectables (`GenerativeArc.hpp` + `ArcTemplates.hpp`).
- **`src/engine/`** — `PieceRenderer.hpp` (la función de render principal, ~50 mecanismos controlados por la semilla) y `AlbumRenderer.hpp` (fase 83: cada pista de álbum ocupa un punto interpolado en una trayectoria de pesos de juego de Caillois, eligiendo libremente su propia escala — ya no un centro tonal forzado).
- **`src/io/`** — `WavWriter.hpp` (metadatos vía chunk RIFF LIST/INFO) y `Recorder.hpp`.
- **Demos CLI** — `rasgo_integrated_demo` (pieza aislada) y `rasgo_album_demo` (conjunto de pistas).
- **`regression_test.py`** — script de prueba automatizado (sección 7).

### 3.2 `rasgo-synth-app` — la aplicación de escritorio JUCE

- **`src/MainComponent.h`** — la pantalla principal (sección 5).
- **`src/RasgoLookAndFeel.h`** — la identidad visual "Cobre" (sección 9), incluyendo el `LogoMark` (el logo real vectorizado por el propio autor, cargado en tiempo de ejecución desde un archivo SVG vía `juce::Drawable`).
- **`src/ArcTimelineView.h`, `WaveformView.h`, `OscilloscopeView.h`** (ahora estéreo), **`SpectrumView.h`** (banco de filtros Goertzel), **`LevelMetersView.h`** (RMS/pico horizontal), **`GameWeightsView.h`**, **`GranulationView.h`** (partículas) — las visualizaciones de la pantalla principal.
- **`src/BufferPlaybackSource.h`** — la fuente de reproducción JUCE construida a partir del buffer ya renderizado, con un filtro en vivo y una ganancia master.
- **`src/Main.cpp`** — la ventana se abre maximizada por defecto (`setFullScreen`); un `juce::Viewport` (`RasgoViewport`) garantiza un desplazamiento real en monitores más pequeños de lo que el layout cómodo requiere, en vez de recortar o superponer controles.
- **`build_juce_minimal.sh` / `build_app.sh`** — la receta de compilación sin CMake/Projucer.
- **`autotest.sh`** — la prueba de humo automatizada de la app (sección 7).

El audio en vivo es servido por `juce_audio_devices`/`juce_audio_basics`, con ALSA como único camino en Linux, y el pseudo-dispositivo "pulse" preferido sobre el "default" bruto.

---

## 4. Funcionalidades

### 4.1 Núcleo sonoro

Seis principios de síntesis distintos coexisten: sustractiva (osciladores + ladder digital, Chamberlin o Polivoks), aditiva (Walsh, VOSIM), resonante (resonador modal, incluyendo la voz de tuba/soplo continuo), granular (`GranularCloud`), estocástica (`GendynOscillator`) y vectorial (Vector Synthesis, fase 76 — un crossfade 2D real entre un banco dedicado de Gendyn/Walsh/Atari Punk/ruido coloreado, posición dirigida por dos mapas caóticos). Los nueve mapas caóticos y los cuatro LFOs lentos siguen todos activos.

Los dos osciladores principales (`osc1`/`osc2`) pueden modularse mutuamente en **cross-FM bidireccional**. Un seguidor de envolvente lento puede realimentar la propia salida de audio de la pieza en la tasa del mapa de Lorenz (**self-feedback**). Una capa de **ruido coloreado** se suma a la textura granular/Gendyn.

Capas aditivas de acompañamiento: **Harmonàig** (fase 77, un pad de acorde real de 4 voces — raíz/3ra/5ta/7ma, inspirado en el módulo real de Instruō de Jason Lim), **polifonía de crivos de Xenakis** (fase 78, 4 canales independientes — madera/soplo, metal/vidrio, agua, plasma reactivo) y **Bloom** (fase 79, una melodía fractal Trunk/Branch/Path, inspirada en el módulo real de Qu-Bit Electronix de Andrew Ikenberry).

### 4.2 La capa narrativa — el arco generativo

La duración y la forma de cada pieza son una **caminata de Markov** sobre cinco roles estructurales, terminando solo en la coda. Dos gramáticas conectables y mezclables: **Dramatic** (pirámide de Freytag) y **Kishōtenketsu**. Un accelerando hacia el clímax (fase 68) refleja el ritardando de coda ya existente.

### 4.3 Secuenciación y escalas

39 escalas investigadas (fase 48) más escalas generadas algorítmicamente por teoría de crivos de Xenakis (fase 61, rechazadas por muestreo contra el catálogo real). Conducción de voz real por distancia en semitonos (fase 62), progresiones de acordes reales (fase 34), y, como alternativa mutuamente exclusiva, transformaciones triádicas Neo-Riemannianas P/L/R (fase 65, conducción de voz parsimoniosa verificada). Hocketing con una cascada polifónica tipo registro de desplazamiento (fase 63). Un divisor de reloj XOR genera polirritmia independiente de la grilla euclidiana principal; el puntero de lectura de la melodía puede ser direccionado por una fuente caótica dedicada en vez del reloj lineal (fase 64).

### 4.4 Percusión

Ocho voces digitales independientes (bombo, caja, hi-hat, cowbell, clap, tom/conga, platillo, maracas), inspiradas en parte en la TR-808. La presencia ahora está ligada a la densidad del arco, no una constante fija (fase 66) — el 12% de las piezas no tienen batería en absoluto, y una fracción tiene fallas reales por golpe. Ocho opciones de arquetipo rítmico por sección del arco (fases 67 + 82): Marcha, Soul-backbeat, Metal-blast-beat, Jazz-swing, Samba-inspirado, dos arquetipos aksak balcánicos reales (Constantin Brăiloiu/Stoyan Dzhudzhev, 9+7 y 7+7+2), y un crivo rítmico de Xenakis generado (misma matemática que la sección 4.3, aplicada a puntos en el tiempo). Ritmo euclidiano real (Bjorklund/Toussaint) como alternativa para el cowbell (fase 57). Dinámica expandida: hi-hat en la contagión del caos, ratchets en los toms, sustain de platillo extendido (fase 56).

### 4.5 Salida estéreo

Los canales divergen realmente a partir de la etapa de delay/reverb (líneas independientes, levemente desincronizadas). Un latido binaural caótico (fase 73) suma una segunda fuente real y verificable de divergencia L/R, independiente del pan/delay.

### 4.6 Modo álbum

Una semilla de álbum deriva determinísticamente N semillas de pista. Hasta la fase 82, todas las pistas compartían un centro tonal forzado (misma escala/raíz). **A partir de la fase 83**, eso cambió: el álbum ahora recorre una **trayectoria en línea recta a través de los cuatro pesos de juego de Caillois** (sección 4.9) — la primera pista nace en un punto sorteado, la última termina en otro punto también sorteado, cada pista intermedia ocupa su propio punto interpolado, y cada pista elige libremente su propia escala, como lo haría una pieza aislada. Un **tema** nombrado aún define bpm/nota raíz/sesgo de energía (mood), pero ya no la escala.

### 4.7 Calidad sonora — higiene técnica y masterización

Dither TPDF, `DcBlocker`, `LookaheadLimiter` + saturador de seguridad, PolyBLEP (~46dB de reducción de aliasing medida), sobremuestreo en la mezcla de diodo, masterización de álbum en dos pasadas. Un bug real de inestabilidad continua en el `ModalResonator` del canal tuba/madera (excitación continua empujando el resonador cerca de su propio techo de seguridad, ~25x más fuerte que la pieza) fue corregido en la fase 81 con un limitador de salida suave `tanh`, el mismo patrón ya usado para Polivoks.

### 4.8 Captura, metadatos e índice

Cada WAV graba metadatos incrustados (chunk RIFF LIST/INFO) y un archivo `.txt` compañero legible como una partitura (fase 54) con la trayectoria completa de la pieza. Un relato en texto plano de la pieza misma (semilla, secciones) vive en un chunk RIFF final legible en un editor de texto/hex, invisible para cualquier reproductor (self-epitaph, fase 46). Cada grabación exitosa agrega una línea a `rasgo_index.jsonl`.

### 4.9 Pesos de juego de Caillois (fase 80)

Cuatro pesos continuos — **Alea, Agon, Ilinx, Mimicry** — muestreados uniformemente del 4-símplex vía "stick-breaking" (tres puntos de corte aleatorios ordenados, los cuatro huecos son los pesos), de modo que cada pieza obtiene su propia mezcla de autoría en vez de elegir de una lista fija de "juegos". Alea no tiene grupo de mecanismos propio — ES la ausencia de sesgo temático, el comportamiento base del motor. Los otros tres influyen (nunca fuerzan) once mecanismos ya existentes: **Agon** (voces compitiendo/coordinando) → Agon mismo, Harmonàig, polifonía de crivos; **Ilinx** (vértigo/pérdida de control) → Rungler, colapso de Rössler, thermal fade, self-feedback, stochastic mute, Leibniz rectifier — y, desde la fase 84, también Curto-Circuito (sección 4.10); **Mimicry** (simulacro) → desgarro apócrifo, self-epitaph, señal extranjera, paréntesis, elemento extraño. Honestamente atribuido a Roger Caillois (*Les Jeux et les Hommes*, 1958).

### 4.10 Curto-Circuito / "Cortocircuito" (fase 84)

Un evento único por pieza, sesgado por Ilinx, con dos funciones en la misma instancia. **Un corte seco real**: una compuerta (~3ms de rampa lineal, nunca una curva de fade) se cierra a silencio verdadero durante 100-400ms y se reabre de la misma manera — exactamente la firma identificada en un análisis real de forma de onda/espectrograma de una de las propias pistas de referencia del autor. Una "chispa" de ruido cabalga solo los bordes de transición, literalizando "cortocircuito" como un artefacto eléctrico, no un simple silencio. **Mecanismos colados**: en el mismo instante, seis mecanismos que la semilla ya había sorteado como DESACTIVADOS para esta pieza (desgarro apócrifo, entonación libre, modulación tonal, self-feedback, rungler, elemento extraño) obtienen cada uno una oportunidad real y elevada del 40% de activarse igualmente — reutilizando exactamente el mismo idioma de "activación forzada" que la garantía de intensidad de la fase 40 ya había establecido, solo que disparado por una ruptura a mitad de pieza en vez de una red de seguridad al inicio.

### 4.11 Otras rupturas y glitches

Fracture Folder + Rungler (fase 41), Leibniz Rectifier (fase 42), contagio del caos (fase 44), thermal fade (fase 45), colapso de Rössler + self-epitaph (fase 46), stochastic mute (fase 47), elemento extraño (fase 50), una señal extranjera real vía temperatura/carga del host (fase 51, la única excepción honesta a la reproducibilidad pura), desbordamiento de punto fijo Q1.15 (fase 52), spectral ghost vía FFT/IFFT (fase 53), sample-hold glitch (fase 55), contagio cruzado de dominio (un pico de audio empujando el reloj, fase 60), Paréntesis (redirección a un bucle de tartamudeo, fase 70), lectura de paso direccionada por el caos (fase 64), y Curto-Circuito (sección 4.10, fase 84) coronando la familia.

---

### Estado operativo actual (22 de agosto de 2026)

Studio ahora incluye agencia material, observación de audibilidad después del
espacio, memoria de orquestación relacional, contratos melódicos y de escalas
ampliados, articulación variada de percusión y un Modo Salvaje auditable. El
Modo Salvaje es el predeterminado de Studio: la entropía real vuelve a sembrar
el generador compartido en las fronteras de compás. Desactivarlo restaura la
reproducibilidad por semilla cuando coinciden tempo, configuración, versión del
motor y estado externo pertinente. Dos tomas recientes con la misma semilla,
una normal y otra salvaje, tuvieron hashes SHA-256 distintos con el mismo
material macro. La base de verificación actual es CTest 19/19, regresión normal
6/6 y regresión salvaje 3/3, sin clipping.

## 5. Flujo de usuario

### 5.1 En la aplicación de escritorio

1. Abrir la app: `cd rasgo-synth-app && ./rasgo_synth_app` — se abre maximizada.
2. Definir la semilla en el campo SEED, o hacer clic en ALEA para sortear una nueva.
3. Ajustar el tempo en el deslizador TEMPO (20–220 bpm). La escala ya no es un control manual — la semilla la elige.
4. Ajustar el volumen de salida en el deslizador MASTER (junto a NÍVEL, un medidor RMS/pico horizontal).
5. Verificar/ajustar el DIRECTORIO DE SALIDA, TÍTULO y ARTISTA (más estrechos ahora, compartiendo espacio con PESOS DE JOGO).
6. Hacer clic en el botón redondo COMPOR (ícono de corchea ♪, rojo) — el renderizado corre en un hilo separado; la línea de tiempo del arco enciende la sección activa, la forma de onda se dibuja.
7. Hacer clic en PLAY (ícono de play/pausa, dorado) para escuchar el resultado, o hacer clic directamente en la forma de onda (titulada WAVEFORM) para reproducir desde ese punto.
8. Arrastrar sobre la forma de onda selecciona un tramo — habilita TOCAR TRECHO y SEED DO TRECHO (deriva una nueva semilla vía un hash FNV-1a y dispara inmediatamente una nueva composición).
9. Durante la reproducción: el deslizador BRILHO AO VIVO aplica un filtro pasa-bajos real ajustable en vivo; VARIAR SEED captura el instante exacto y deriva una nueva semilla.
10. Debajo de la forma de onda: ESPECTRO (banco Goertzel) y GRANULAÇÃO (partículas reactivas a la amplitud). PESOS DE JOGO muestra las cuatro barras coloreadas de Alea/Agon/Ilinx/Mimicry de la pieza actual. El panel de mecanismos muestra, en cuadrícula, cuáles de los ~50 mecanismos del motor se manifestaron en esta pieza específica.
11. OSCILOSCÓPIO (ESTÉREO) (dorado) y LOG (con COPIAR/LIMPAR debajo de la caja) se dividen la banda inferior, 60%/40% de ancho.
12. TUTORIAL y ABOUT (esquina superior derecha) abren diálogos temáticos con instrucciones de uso y la descripción real del proyecto.
13. MODO ALBUM (botón rectangular, se rellena de cobre al activarse) cambia el panel a TEMA/FAIXAS/OUVIR.

### 5.2 Vía línea de comandos

```bash
# Pieza aislada
./rasgo_integrated_demo [seed] [outDir] [bpm]

# Conjunto de pistas (álbum) -- trayectoria de pesos de juego, ya no una escala fija
./rasgo_album_demo [albumSeed] [trackCount] [outDir] [tema]

# Batería de regresión
python3 regression_test.py --seeds 20

# Prueba de humo de la app
./autotest.sh --skip-build --timeout=30
```

---

## 6. Trabajo realizado (línea de tiempo por fases)

El documento vivo del proyecto ([`Plano_de_Upgrade_Instrumento_Generativo.md`](../../Plano_de_Upgrade_Instrumento_Generativo.md), solo en portugués) es la fuente de verdad completa, fase por fase. Resumen cronológico:

- **Fases 1–23** — núcleo DSP validado, secuenciador, capa narrativa por cadena de Markov, rediseño visual "Sinal" (reemplazado después por "Cobre", ver sección 9), ocho voces de percusión, salida estéreo real, modo álbum inicial, calidad sonora audiófila (dither/DC-block/limiter/PolyBLEP), los nueve mapas caóticos activos, módulos de circuitos reales (Atari Punk, envelope-oscillator, Noise Cornucopia), carácter Turing Machine en el secuenciador, self-feedback de audio real, cross-FM, ruido coloreado.
- **Fases 24–48** — batería real audible con jitter de mezcla por semilla, desgarro apócrifo, interacción real con la forma de onda, modo álbum en la UI, variación de tempo, primer incremento de armonía funcional, performance en vivo (filtro + VARIAR AGORA), modulación de centro tonal, modulación modal, progresiones de acordes reales, humanización rítmica, efectos por voz de percusión, hocketing, red de influencia cruzada, entonación libre, garantía de intensidad (cada pieza se prueba contra al menos un mecanismo de riesgo), Fracture Folder + Rungler, Leibniz Rectifier, voz de tuba, contagio del caos, thermal fade, colapso de Rössler + self-epitaph, stochastic mute, y la expansión de escalas de 9 a 39 con el control ESCALA removido de la UI.
- **Fases 49–68** — Agon (cuatro voces compitiendo por poder, con una "rusga"), elemento extraño (interno y literal vía una lectura real del host), desbordamiento de punto fijo, spectral ghost, partitura .txt compañera, sample-hold glitch, dinámica de batería expandida, ritmo euclidiano real, oscilador Walsh, contagio cruzado de dominio (audio→reloj), escala generada por crivo de Xenakis, conducción de voz real, cascada polifónica, lectura de paso direccionada por el caos, transformaciones Neo-Riemannianas, batería como un instrumento real (a partir de una escucha real de un álbum completo), arquetipos rítmicos con identidad real, accelerando hacia el clímax.
- **Fases 69–76** — mercado de probabilidades (precios persistidos entre renders), Paréntesis (redirección a un bucle de tartamudeo), Heisenberg (precisión rítmica/de afinación acopladas inversamente), VOSIM, latido binaural caótico, envolvente de asfixia por sidechain, filtro Polivoks auto-oscilante, Vector Synthesis (crossfade 2D real).
- **Fases 77–82** — Harmonàig (pad de 4 voces, atribuido a Jason Lim/Instruō), polifonía de crivos de Xenakis (4 canales), Bloom (melodía fractal, atribuida a Andrew Ikenberry/Qu-Bit Electronix), pesos de juego de Caillois (fase 80, un giro explícito), corrección de un bug real de inestabilidad continua en el `ModalResonator` de tuba/madera (fase 81), investigación rítmica profunda — dos arquetipos aksak reales + un crivo rítmico de Xenakis generado (fase 82).
- **Fase 83** — fin de la afinidad tonal forzada del álbum, reemplazada por una trayectoria en línea recta a través de los pesos de juego de Caillois; cada pista elige libremente su propia escala.
- **Fase 84** — "Curto-Circuito" ("Cortocircuito"): un corte seco real (compuerta + chispa de ruido) que, en el mismo instante, da a los mecanismos que la semilla había dejado desactivados una segunda oportunidad de colarse en la pieza.
- **Rediseño de layout de la app** (entre las fases 82 y 83) — identidad visual cambiada de verde fosforescente ("Sinal") a cobre; el logo real del autor vectorizado y aplicado como emblema; el panel de módulos expandido en una cuadrícula cubriendo ~50 mecanismos; osciloscopio estéreo; nuevos paneles ESPECTRO/NÍVEL/GRANULAÇÃO/PESOS DE JOGO (bancos Goertzel, medidor RMS/pico, partículas, barras coloreadas); BPM y MASTER como deslizadores horizontales; íconos reales para COMPOR/PLAY; ABOUT/TUTORIAL temáticos; la ventana se abre maximizada con un respaldo de desplazamiento.

---

## 7. Validaciones

### 7.1 Lo que existe

- **`regression_test.py`** — compila el motor, renderiza un lote de semillas, mide duración/pico/fracción de recorte/historial de secciones. Criterios: duración 15–300s, recorte < 0,05%, historial no vacío.
- **`autotest.sh`** — compila la app, la ejecuta con `RASGO_AUTOTEST=1`, confirma la línea "gravado:" en el log y una consola nativa limpia.

### 7.2 Lo que realmente se ha verificado

Cada fase desde la 13 ha pasado por `regression_test.py` (lotes de 15–20 semillas) con cero muestras saturadas, y la app fue recompilada y pasó `autotest.sh` después de cada una. Más allá de eso, cada nuevo mecanismo controlado por la semilla desde la fase 49 recibió una **auditoría empírica de tasa de disparo** (renderizar un lote de semillas y confirmar en los metadatos que el mecanismo realmente se dispara a la tasa esperada, no solo que compila) — una disciplina que ya atrapó bugs reales (una tasa de colisión del 11,6% en el crivo de altura antes de corregirse, un contador de brecha en la unidad equivocada en Agon, la inestabilidad del `ModalResonator` de la tuba). Varias fases nacieron directamente de la **escucha real** del usuario, no solo de la medición: la batería (fase 66), la audibilidad de la tuba (fase 81), y el propio análisis de forma de onda/espectrograma que originó Curto-Circuito (fase 84).

### 7.3 Lo que aún no se ha verificado

No todas las combinaciones posibles de los ~50 mecanismos del motor han sido escuchadas a mano — la cobertura proviene de auditorías empíricas de metadatos (confirmando disparo real) más escucha puntual guiada por reportes específicos del usuario, no un barrido exhaustivo de todas las combinaciones.

---

## 8. Seguridad

- **Ejecución enteramente local** — sin red, telemetría ni recolección de datos personales en ningún componente del motor o de la app.
- **Sin credenciales ni datos sensibles** en ninguna parte del código o de los scripts de build/prueba.
- **Cuidado con el espacio en disco** y con `recordings/` — es el directorio de salida real del usuario (álbumes reales, un índice acumulado); los scripts de prueba escriben en subcarpetas aisladas o limpian exactamente el archivo que acaban de crear, nunca un barrido amplio.
- **Licencia AGPL-3.0-or-later** del código propio actual; texto completo en `LICENSE`.
- **Compilación sin privilegios elevados** — la compilación (g++ directo) no requiere sudo.

---

## 9. Layout — la identidad visual "Cobre"

La identidad visual cambió de "Sinal" (verde fosforescente sobre casi-negro frío) a **"Cobre"**: el color de acento es un naranja cobre oxidado (`#e8865f`), elegido porque referencia directamente el objeto físico real detrás del nombre "rasgo" — paneles Eurorack corroídos con ácido que el autor construye a mano desde 2013. El fondo permanece casi negro, ahora con un sesgo cálido en vez de frío.

- **Emblema** — el logo real del autor (su propia vectorización en Inkscape) cargado vía `juce::Drawable::createFromSVGFile`, teñido de cobre, aplicado junto al título.
- **Paleta ampliada** — `Palette::action` (dorado, `#f2c14e`) para los botones de acción primaria (COMPOR/PLAY); `Palette::danger` (rojo) reutilizado específicamente como color propio de COMPOR; un pequeño ciclo cobre/rojo/dorado (`vizAccent`) colorea las barras de los nuevos paneles (pesos de juego, espectro por banda, medidores L/R) en vez de todo en cobre monocromático.
- **Íconos reales** — COMPOR usa el glifo Unicode de corchea (♪, más confiable que un trazo dibujado a mano); PLAY usa un triángulo/dos barras según el estado de reproducción.
- **Osciloscopio estéreo** — dos trazas independientes (L/R), dorado, una ventana de tiempo más larga que antes.
- **Tipografía** — monoespaciada en todo el título, etiquetas y log; tamaños aumentados respecto a la versión "Sinal".
- **Textura de fondo** — una grilla de graticule de osciloscopio detrás de toda la ventana.
- **Ventana** — se abre maximizada por defecto (`setFullScreen`), con un respaldo de desplazamiento real (`juce::Viewport`) en monitores más pequeños de lo que el layout cómodo (1100×760) requiere.

**Intento revertido**: un efecto visual de "desgarro" (papel rasgado atravesando el panel de control y/o la parte superior de la ventana) fue implementado y luego completamente eliminado a pedido del usuario — no reintroducirlo sin un nuevo pedido explícito.

---

## 10. Pendientes y próximos pasos

Véase la lista viva completa en `CONTINUIDADE.md` ("Tarefas em aberto", solo en portugués) y en el Plano de Upgrade ("Em aberto"). Resumen de lo que sigue genuinamente abierto:

- **Una "nota al pie" y una voz narradora** — el mecanismo de nota al pie existe; sigue abierta una voz literaria más rica.
- **Síntesis vocal más física** — Scat y Coro existen como voces originales de formantes; un subsistema completo de tracto vocal sigue siendo una frontera a largo plazo.
- **Ampliación de los roles relacionales** — theremín, flauta FM, tuba, voces, polifonías y archivo todavía necesitan contratos causales más profundos.
- **Cambios reales de compás/métrica** — se conecta con la investigación rítmica (fase 82), deliberadamente fuera de alcance hasta ahora (cambiaría la columna vertebral rítmica fija de 16 pasos).
- **La consola embebida de la Versión Performance** — una etapa posterior a la Versión Studio, por decisión explícita del usuario.
- **Sincronización documental** — los cuatro documentos públicos principales están sincronizados en EN/FR/ES/PT; las auditorías internas de fases y el plan vivo siguen siendo documentos de trabajo en portugués.

---

## 11. Referencias e influencias

Un vacío no explica a Rasgo Synth; este es un proyecto de síntesis generativa que se declara, con honestidad, en diálogo directo con circuitos reales, instrumentos comerciales, artículos académicos y la propia trayectoria del autor. Ningún código fuente de otro instrumento ni ningún esquema eléctrico exacto fue copiado: los comportamientos fueron implementados originalmente en C++ a partir de conceptos y documentación públicos. Véase `CREDITS.md` para la tabla completa por mecanismo.

### 11.1 Trayectoria de autoría y referencia externa inicial

- **Antitotem y módulos físicos de Rasgo (2013–)** — obras construidas por el autor. Antitotem antecede a Rasgo, fue incorporado como su precursor y patrono.
- **Kosmische (2013)** — instrumento generativo Android de Nicolas Steven Miller (github.com/zrnsm/kosmische, GPLv3), referencia técnica y conceptual inicial, no forma parte de Rasgo.

### 11.2 Hardware comercial y circuitos DIY públicamente documentados

- **Roland TR-808** — referencia sonora para parte de las ocho voces de percusión.
- **Music Thing Modular — Turing Machine** (Tom Whitwell) — el carácter de bloqueo/mutación por paso del secuenciador.
- **Mutable Instruments** (Émilie Gillet) — Rings, Clouds, Branches, reinterpretados.
- **Instruō — ochd** (Ben "DivKid" Wilson) y **Harmonàig** (Jason Lim) — el banco de LFOs y el pad de acorde de 4 voces, respectivamente.
- **Qu-Bit Electronix — Bloom** (Andrew Ikenberry) — la melodía fractal Trunk/Branch/Path.
- **Frap Tools — Sapèl** — cuatro colores de ruido.
- **Make Noise — Maths** / mezcla sin entrada — self-feedback de audio real.
- **Buchla** — procesadores leveler/lag.
- **Rob Hordijk — Benjolin** — cross-FM bidireccional.
- **Ken Stone — CGS22 "Master Divider"** — el divisor de reloj XOR polirrítmico.
- **Forrest Mims III — Stepped Tone Generator** ("Atari Punk Console") — textura stepped/glitch.
- **Ray Wilson — MFOS "Noise Cornucopia"** — ruido granulado y compuerta aleatoria.
- **General Instrument AY-3-8910** — arpegio/textura chiptune.
- **Pantala Labs** — modo delay lo-fi, saturadores de cinta/válvula.
- **Robert Moog; Tim Stilson y Julius O. Smith** — el filtro ladder.
- **Hal Chamberlin** — el filtro state-variable.
- **Polivoks (Formanta, URSS)** — el tercer color de filtro, auto-oscilación real.
- **Rob Hordijk/Benjolin, Xaoc Devices Leibniz-Rectifier** — Rungler y rectificación/corrupción de bits.
- **Sequential Circuits Prophet VS** — crossfade 2D real (Vector Synthesis).
- **Técnica de sidechain/ducking de la producción techno/EDM** — la envolvente de asfixia.

### 11.3 Matemáticas, ciencias de la computación y síntesis académica

- **Iannis Xenakis** — Síntesis Estocástica Dinámica ("Gendyn"); y, en *Formalized Music* (capítulo XI, "Sieves: A User's Guide"), la teoría de crivos por congruencia de módulo/residuo, aplicada tanto a clases de altura (fase 61) como a puntos en el tiempo (fase 82).
- **Joe Pritchard — *The Chaos Cookbook* (1992)**; **Lance Putnam — Chaos UGens de SuperCollider (2004)** — referencias de investigación para los nueve mapas caóticos.
- **Vesa Välimäki y Antti Huovilainen — PolyBLEP** — el anti-aliasing de los osciladores.
- **Dither TPDF** — una técnica estándar de procesamiento de audio digital.
- **Werner Kaegi y Stan Tempelaars, Instituto de Sonología de Utrecht — VOSIM** — síntesis por pulsos sin² decayendo.
- **Constantin Brăiloiu; Stoyan Dzhudzhev** — la teoría del ritmo aditivo/aksak balcánico, dos arquetipos reales (9+7 y 7+7+2).
- **David Lewin; teoría Neo-Riemanniana** — transformaciones triádicas P/L/R.

### 11.4 Estructuras narrativas, ludológicas y composicionales

- **Gustav Freytag — la Pirámide de Freytag** (1863) — la gramática narrativa "Dramatic".
- **Kishōtenketsu** — la estructura narrativa china/japonesa de 4 partes (qǐ-chéng-zhuǎn-hé).
- **Roger Caillois — *Los Juegos y los Hombres*** (1958) — las cuatro categorías de juego (Alea, Agon, Ilinx, Mimicry), formalizadas mecánicamente como pesos continuos que sesgan once mecanismos existentes (fase 80).

Quien quiera profundizar en cualquiera de estos puntos de partida antes de portar la próxima idea debe mantener el mismo estándar de honestidad: citar la fuente real, entender el comportamiento antes de reimplementar, y dejar claro en esta sección (o en la fase correspondiente del `Plano_de_Upgrade_Instrumento_Generativo.md`) que la idea dialoga con el trabajo de otro, no que nació aislada.
