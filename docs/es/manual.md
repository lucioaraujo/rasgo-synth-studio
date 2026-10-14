# Rasgo Synth Studio — Manual

Versión experimental 0.1.0 — 2026

Revisado el 2026-10-12 según los controles y contratos actuales. [Tutorial paso a paso](../../website/quickstart.html#es).

## 1. Concepto

Rasgo Synth Studio es un instrumento musical generativo: eliges condiciones de composición y él sintetiza una pieza completa. La seed es un punto de partida; tempo, selección de módulos, expresión, dirección musical y estado pertinente también importan. No es un teclado tocado nota a nota ni un rack con cables manuales.

Los sonidos se sintetizan, no se montan desde una biblioteca externa de muestras grabadas. Escalas, ritmos, voces, texturas y forma narrativa comparten un espacio de posibilidades de autor. Una pieza puede repetir, transformar, interrumpir o dejar material en silencio; variar no garantiza un sonido nuevo en cada evento.

MODO SALVAJE está activado por defecto y añade entropía durante el render. Para comparar de forma controlada, desactívalo y conserva seed, tempo, elecciones, versión y estado externo pertinente. La seed sola no garantiza audio idéntico.

La duración de una pieza individual emerge de la narrativa. En el álbum puedes conservarla o activar planificación de duración. Studio compone/renderiza primero y reproduce después; los controles de escucha no reescriben el WAV guardado en tiempo real. Studio y Performance son instrumentos separados, con publicaciones independientes.

## 2. Historia

Rasgo Synth es una obra dentro de Rasgo, el proyecto paraguas de Lucio de Araujo para sus instrumentos y programas musicales de autoría. Antitotem precede a Rasgo y luego fue incorporado como su precursor y patrono. Los módulos físicos de Rasgo ya estaban concebidos con vistas al estándar Eurorack, desde 2013; hoy el nombre abarca un proyecto de instrumentos musicales más amplio.

Kosmische es un instrumento Android GPLv3 independiente, de Nicolas Steven Miller. Fue una de las referencias conceptuales estudiadas al inicio de Rasgo Synth. No forma parte de Rasgo, y Rasgo Synth no es ni un fork ni su continuación de autoría.

La implementación específica del proyecto fue reescrita desde cero en C++17 y JUCE, bajo la dirección de Lucio de Araujo, con Anthropic Claude usado como herramienta de desarrollo. No se copió ningún código fuente de Kosmische ni de Antitotem. El instrumento recibió posteriormente muchos sistemas, voces y mecanismos composicionales más allá de esas referencias iniciales -- según el último recuento, más de ochenta fases de desarrollo documentadas.

## 3. Investigación y procedencia

Los nueve mapas caóticos son implementaciones independientes de recurrencias matemáticas publicadas. Kosmische y los Chaos UGens de SuperCollider, creados por Lance Putnam, fueron examinados como contexto técnico; *The Chaos Cookbook* es el libro de Joe Pritchard publicado en 1992. Estas referencias no convierten las implementaciones de Rasgo en portes de código fuente.

Los instrumentos de hardware y software citados por Rasgo son referencias de diseño, no reivindicaciones de autoría o emulación exacta. El resonador modal, la textura granular, las fuentes aleatorias, los filtros y los módulos inspirados en circuitos son interpretaciones comportamentales originales basadas en conceptos públicos, literatura, documentación y esquemas. Los creadores y colaboradores históricos están nombrados en [CREDITS.md](../../CREDITS.md).

Las ocho voces de percusión son estilizaciones digitales originales, inspiradas en parte en el comportamiento documentado de la Roland TR-808. No usan muestras, no copian firmware y no son réplicas de circuitos a nivel de transistor. Cada pieza elige entre ocho arquetipos rítmicos posibles: Soul, Jazz, Metal, Marcha, Samba, dos arquetipos aksak balcánicos reales (documentados por Constantin Brăiloiu y Stoyan Dzhudzhev), y un crivo rítmico generado algorítmicamente mediante la teoría de crivos de Xenakis.

Para la lista autoritativa de procedencia, véase [CREDITS.md](../../CREDITS.md). Las bibliotecas incorporadas y sus términos de licencia se listan por separado en [THIRD_PARTY_NOTICES.md](../../THIRD_PARTY_NOTICES.md).

## 4. Referencias seleccionadas

- Lucio de Araujo: Rasgo y Antitotem.
- Nicolas Steven Miller: Kosmische (GPLv3), referencia conceptual externa.
- Joe Pritchard: *The Chaos Cookbook* (1992).
- Lance Putnam: Chaos UGens de SuperCollider (2004).
- Forrest M. Mims III: el Stepped-Tone Generator, popularizado como Atari Punk Console.
- Iannis Xenakis: síntesis estocástica dinámica y teoría de crivos (*Formalized Music*, cap. XI).
- Instruō, Ben "DivKid" Wilson (ochd) y Jason Lim (Harmonàig).
- Qu-Bit Electronix, Andrew Ikenberry: Bloom.
- Émilie Gillet / Mutable Instruments: conceptos de DSP Eurorack publicados.
- Roland Corporation: documentación de la TR-808 y referencia de diseño sonoro.
- Roger Caillois: *Los Juegos y los Hombres* (1958), las cuatro categorías de juego (Alea/Agon/Ilinx/Mimicry).
- Constantin Brăiloiu, Stoyan Dzhudzhev: teoría del ritmo aditivo/aksak balcánico.
- Los demás creadores listados en [CREDITS.md](../../CREDITS.md).

## 5. Licencia

El código propio de Rasgo Synth Studio se distribuye bajo la GNU Affero General Public License versión 3 o posterior (AGPL-3.0-or-later); véase `LICENSE`. La licencia GPLv3 de Kosmische se aplica a Kosmische y no establece autoría compartida ni código copiado.

La aplicación de escritorio usa JUCE. El código local de JUCE 9 ofrece AGPLv3 o una licencia comercial de JUCE; por lo tanto, quien distribuya una compilación combinada debe cumplir los términos de JUCE aplicables además de la propia licencia de Rasgo Synth. Otros componentes incorporados o enlazados mantienen sus respectivas licencias; véase [THIRD_PARTY_NOTICES.md](../../THIRD_PARTY_NOTICES.md).

## 6. Tutorial

### Antes de empezar

Esta guía describe Studio 0.1.0 experimental, no Rasgo Modular ni Performance. Mantén bajo el volumen, elige una carpeta con permiso de escritura y trabaja con una pieza a la vez. El icono de corchea ♪ inicia la composición; PLAY reproduce audio ya generado. Los nombres siguientes corresponden a la interfaz española.

### Tu primera pieza — seis pasos

1. Comprueba la salida de audio, baja MASTER y elige la carpeta de salida. Puedes introducir título y artista; los archivos se guardarán allí.
2. Define SEED y TEMPO. Desactiva MODO SALVAJE para este ejercicio. Deja DIRECCIÓN MUSICAL y EXPRESIVO desactivados y módulos en Automático como referencia inicial.
3. Pulsa COMPONER (♪). Espera el render y la confirmación de guardado; PLAY no inicia la composición.
4. Pulsa PLAY. Escucha primero sin cambiar nada: identifica entradas, salidas, pausas, registro grave, percusión y texturas.
5. Arrastra en la forma de onda para seleccionar una región y utiliza REPRODUCIR REGIÓN. SEED DE LA REGIÓN deriva otro origen, sin exportar una muestra. Compón de nuevo para escucharlo.
6. Conserva el WAV y los archivos acompañantes. Usa ALEA para otra seed o sigue los ejercicios cambiando una elección cada vez.

### Seed y tempo

Qué es: SEED elige un punto de partida; TEMPO define el tempo inicial de la pieza.

Cómo entenderlo: la seed es una condición de composición, no un nombre de preset que explica todo. El motor puede transformar el tempo durante la pieza. El motor elige la escala; este panel no tiene selector manual de escala.

Controles: SEED, ALEA, TEMPO y MODO SALVAJE. En modo álbum, el tema proporciona el BPM-base; el slider de la pieza está desactivado.

Prueba: anota seed y tempo, genera con MODO SALVAJE desactivado y escucha. Conserva seed, cambia solamente TEMPO y genera otra vez. Compara duración de gestos, espaciado y energía percibida; no es solo cambiar la velocidad del mismo WAV.

### Elegir quién puede tocar

Qué es: ELEGIR MÓDULOS abre 34 participantes entre fuentes, texturas y batería.

Cómo entenderlo: eliges elegibilidad, no un solo o mute del audio actual. Incluir permite participación; notas, entradas, pausas y dependencias siguen determinando la ejecución. Otros mecanismos y efectos continúan automáticos. La cuadrícula de actividad no es esta lista de controles.

Controles: Automático conserva la elección generativa; Incluir permite la fuente designada; Excluir la bloquea. Confirma con APLICAR AL PRÓXIMO RENDER. Cerrar sin aplicar descarta ediciones; TODOS AUTOMÁTICOS reinicia el borrador antes de confirmar. Reiniciar la aplicación restablece las elecciones.

Prueba: conserva seed, tempo y modos. Incluye Capa de ruido coloreado, aplica y genera. Después excluye esa misma capa y genera otra vez. Compara textura y espacio ocupado por ruido. Otros generadores aún pueden producir ruido: Excluir no silencia toda una familia sonora.

### Batería y groove

Qué es: bombo, caja, charles, cencerro, palmas, toms, plato y maracas están sintetizados, no muestreados.

Cómo entenderlo: los participantes tienen funciones rítmicas, pero Studio no ofrece knobs individuales de afinación o volumen. Excluir Batería bloquea sus elementos aunque Bombo esté en Incluir. Incluir no fuerza golpes constantes.

Controles: ELEGIR MÓDULOS; activa DIRECCIÓN MUSICAL y EXPRESIVO antes de componer para explorar los estudios adicionales de groove e interpretación. Seed y secciones eligen familias y respuestas; no hay selector manual de género o polirritmia 3:2.

Prueba: incluye Batería, Bombo, Caja y Charles y genera. Sin cambiar lo demás, excluye Cencerro y Maracas y genera otra vez. Escucha si el pulso se vuelve más legible. No subas MASTER para compensar un elemento: el desequilibrio de mezcla es una limitación conocida, no un error tuyo.

### Registro grave y línea de bajo

Qué es: Bajo acid y Tuba son fuentes seleccionables; el bajo interno core-sub es una función distinta de Voz principal.

Cómo entenderlo: frecuencia grave, línea de bajo musical y volumen son distintos. Una línea requiere eventos, registro y articulación; el nombre del módulo no garantiza presencia clara en la mezcla. El fallback core-sub no tiene una entrada propia en el selector.

Controles: selección de Bajo acid/Tuba y DIRECCIÓN MUSICAL + EXPRESIVO. No hay fader individual de bajo ni botón solo.

Prueba: con ambos modos activos, incluye Bajo acid y excluye Tuba; aplica y genera. Luego invierte esa elección. Escucha ataque, sostenimiento, pausas y relación con el bombo. Excluir ambos puede dejar disponible el fallback según el plan musical; no garantiza un bajo reconocible. Informa seed y elecciones si el grave queda enmascarado.

### Melodía, timbre y relaciones

Qué es: voces como Flauta FM, Theremin y Coro, y texturas como Granular, ofrecen diferentes materiales sonoros.

Cómo entenderlo: seleccionar no es girar knobs de un rack. Las trayectorias se calculan durante el render. Coro, Scat y Harmonaig dependen de la progresión de acordes generada; una dependencia ausente puede bloquear su participación, registrada como `blockedBy` en los metadatos.

Controles: ELEGIR MÓDULOS, DIRECCIÓN MUSICAL y EXPRESIVO. Dirección organiza entradas, retiradas y relaciones; Expresivo añade interpretación experimental. Son opciones distintas, desactivadas por defecto, y necesitan nuevo render.

Prueba: genera una referencia con ambos modos desactivados. Activa solamente Dirección y genera; después activa también Expresivo y genera otra vez. Mantén seed, tempo, módulos y Auditoría de stems. Compara pausas, respuestas, ataques y timbre, no solo volumen. El contraste puede ser discreto; no se garantiza mayor emoción ni diálogo audible en cada pieza.

### Escucha y visualizaciones

Qué es: forma de onda, espectro, espectrograma y otras vistas ayudan a observar audio.

Cómo entenderlo: un indicador muestra actividad, no prueba de audibilidad o calidad musical. Las animaciones del website son ilustraciones, no el audio de tu aplicación.

Controles: PLAY, posición en la forma de onda, REPRODUCIR REGIÓN, MASTER y brillo de escucha. No reescriben el WAV guardado. En ventanas pequeñas, desplázate verticalmente para llegar al registro y las vistas.

Prueba: escucha una región y relaciona un ataque con la forma de onda; después busca una textura sostenida en el espectro. Una amplitud pequeña dibujada no demuestra por sí sola un mal mastering. Si no identificas una voz, informa del caso en vez de aumentar volumen sin límite.

### Tu primer álbum

Qué es: MODO ÁLBUM crea una colección desde seed, tema y planificación.

Cómo entenderlo: el tema proporciona BPM-base; las elecciones afectan los próximos renders, no las pistas ya guardadas. Aún no hay memoria composicional que reaprenda temas entre pistas.

Controles: tema, PISTAS (1–40; defecto 8), VARIAR BPM (desactivado; amplitud 0–35%, defecto 15%), trayectoria equilibrada/contrastante/creciente/arco. PLANIFICAR DURACIÓN también comienza desactivado. Al activarlo, permite un objetivo total de 1–45 minutos y distribución equilibrada/contrastante/libre. El plan limita pistas a 15 segundos–8 minutos y total a 45 minutos; la vista previa distingue total solicitado y previsto si una petición no cabe.

Prueba: elige 2 pistas, activa PLANIFICAR DURACIÓN y solicita 2 minutos totales. Lee la vista previa; activa VARIAR BPM para comparar tempos iniciales. Compón y escucha con el selector ESCUCHAR. PLAY restaura las vistas; CONFIGURAR ÁLBUM vuelve a las opciones. La duración musical prevista no es tiempo de procesamiento ni un corte posterior del WAV.

Las calibraciones preceden los renders finales. `album_plan.json` registra el plan y archivos efectivos; los álbumes interrumpidos no están terminados. Selección y modos expresivos se aplican a las pistas siguientes. Auditoría de stems hereda la opción definida en modo pieza aunque su control esté desactivado en modo álbum: configúrala antes de entrar. Su antigua descripción emergente no explica correctamente esa herencia.

### Progreso, archivos y problemas

Qué es: el porcentaje sigue frames musicales de la pasada actual, no tiempo real transcurrido.

Cómo entenderlo: 100% de composición puede ir seguido de procesamiento final y guardado. En el álbum, porcentaje y calibración no constituyen una estimación única del tiempo restante. Auditoría de stems añade análisis y procesamiento y puede cambiar el resultado, no solamente el tiempo de render.

Controles: registro, COPIAR, carpeta de salida, ACERCA DE y TUTORIAL. ACERCA DE muestra versión y fecha/hora de compilación. Cerrar durante un render solicita cancelación y espera un checkpoint; una escritura activa puede necesitar terminar. No garantiza recuperación después de crash o corte eléctrico.

Prueba: tras la primera pieza, encuentra el WAV y sus acompañantes. Si algo falla, copia el registro e indica versión, sistema, dispositivo, seed, modos, selección y acción anterior al fallo. No publiques grabaciones personales automáticamente. Evita renders/builds simultáneos con presión de memoria.

### Limitaciones de esta versión

Una publicación experimental no es una mezcla aprobada: bajo discreto, percusión dominante y timbres repetidos siguen siendo posibles. Mezcla generativa y diversidad interpretativa continuarán después de publicar. Ruido, silencio y ruptura son posibilidades, no estética obligatoria. Revisar instrucciones no sustituye validación de escucha, accesibilidad o de todos los teclados/idiomas.

[Guía integral](Instrumento_Integral.md) · [Créditos](../../CREDITS.md) · [Método rítmico y referencias](../GROOVE_RHYTHM.md)
