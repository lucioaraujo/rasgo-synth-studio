# Rasgo Synth — visión integral del instrumento

Este documento explica el instrumento completo: qué recibe, cómo construye una
pieza, cómo leer el Studio y cómo se relaciona el catálogo de módulos con la
música.

## Studio y Performance

Rasgo Synth Studio es el instrumento de escritorio existente para componer,
renderizar, escuchar e investigar. Rasgo Synth Performance es un instrumento
hermano de DSP en vivo, con contratos, motor, host standalone y una primera
superficie local de cuatro escenas y cinco gestos. Aún no está publicado: faltan
las validaciones gráfica, de dispositivo, musical y de build portátil. Los
controles en vivo del Studio no constituyen una release de Performance.

## De la seed a la pieza

Rasgo Synth es un instrumento de software generativo, no un loop fijo, un
reproductor de samples ni un teclado nota por nota. Una seed elige una
trayectoria dentro de un espacio autoral de identidad, escalas, ritmos, voces,
probabilidades, forma narrativa y materia sonora. Después el motor renderiza
una pieza completa.

El recorrido tiene nueve capas: identidad; juego (Alea, Agon, Ilinx, Mimicry);
tiempo; materia; forma; relaciones; expresión; atención y mezcla; y registro.
El tiempo incluye rejilla, pulso, subdivisión, swing, microtiming,
humanización, gate, probabilidad, fase, polimetría, deriva y memoria. La forma
sigue una caminata de Markov por apertura, desarrollo, clímax, ruptura,
asentamiento y cierre.

No todos los módulos están activos en cada seed. `Disponible`, `manifestado`,
`audible` e `influyente` son estados distintos. La presencia de flauta, tuba o
un cuerpo de bajo nunca resume el instrumento entero.

## Bajo, melodía y percusión

El bajo se separa en función, frase y cuerpo. La función puede ser fundamental,
pedal, camino, tensión, contrapunto o espacio. La frase controla registro,
dirección, repetición, relación con el kick y densidad. El cuerpo puede ser una
tuba sintética, una voz acid o un `core-sub` de respaldo. El cuerpo no decide
por sí solo la línea.

La melodía utiliza escalas, conducción de voces, contorno, voicing, tratamiento
de disonancia, afinación expresiva y memoria. La percusión tiene voces
independientes, arquetipos rítmicos, estructuras euclidianas y aksak,
articulación, variación tímbrica e interlocking. `TemporalGrooveImprint`
alimenta el bajo expresivo y la percusión de arquetipo con influencia limitada.

## Leer el Studio

- **SEED / ALEA**: elegir o derivar el punto de partida.
- **TEMPO**: fijar el pulso inicial; el arco puede derivar después.
- **MASTER**: fijar el nivel final, no la energía compositiva.
- **MODO SALVAJE**: permitir trayectorias entrópicas adicionales.
- **AUDITORÍA DE STEMS**: medir supervivencia posespacio y atención narrativa.
- **A/B EXPRESIVO**: activar comportamiento temporal y gestual experimental.
- **COMPONER**: renderizar la pieza en segundo plano.
- **LANG**: cambiar entre inglés, portugués, francés y español.

Los paneles leen un mismo resultado: forma de onda, osciloscopio, espectro,
espectrograma, goniómetro, retrato de fase, granulación, genealogía del arco,
pesos del juego, cuadrícula de módulos y log. El archivo `.txt` acompañante es
un relato legible de secciones, decisiones, voces y mecanismos; no es una
partitura tradicional.

La auditoría de stems es una lente de medición, no un requisito para escuchar.
`COPIAR` copia el log y `LIMPIAR` limpia su presentación sin borrar audio. Una
región de la forma de onda puede reproducirse o utilizarse para derivar una
nueva seed. `VARIAR SEED` parte de un instante de reproducción para componer
otra pieza.

## Modo Álbum

El Modo Álbum crea varias pistas relacionadas desde una seed de álbum.
`AlbumRenderer` deriva seeds de pista, conserva continuidad tonal amplia y deja
que cada pista tenga su propia narrativa y selección de módulos. No es un
archivo largo cortado en partes: es una familia de piezas.

## Flujo y catálogo

El flujo arquitectónico muestra causalidad: la seed alimenta identidad,
secuenciación, arco y DSP; el arco alimenta densidad, registro y energía; juego,
relaciones y mezcla observan o influyen dentro de límites. Las flechas sólidas
son flujos principales; las punteadas son observación o influencia limitada.

El catálogo web contiene 142 mecanismos con categoría, descripción y referencia.
Describe posibilidades, no promete que todos se escuchen en cada pieza. Consulta
la [guía integral portuguesa](../pt/Instrumento_Integral.md) y el
[catálogo](../../website/assets/mechanisms.pt.json).

## Límites y continuación

Algunos contratos de performance, memoria, atención y mezcla todavía tienen
consumidores locales u observacionales. La aceptación musical requiere tanto
telemetría como escucha humana. El siguiente trabajo busca un bajo más legible
sin imponer un cuerpo, más contrapunto cuando la forma lo permita y morfología
de nota en más familias de voces.
