# Rasgo Synth app assets

Runtime packages must keep this `assets/` directory beside the executable, or
set `RASGO_SYNTH_ASSETS_DIR` to its installed location. A conventional Unix
package may also install it as `share/rasgo-synth/assets` relative to the
application prefix.

## Asset inventory

| Asset | Origin and status | License or notice |
|---|---|---|
| `rasgo_logo_2026.svg` | Rasgo artwork vectorised by Lucio de Araujo | © 2026 Lucio de Araujo; project artwork, not covered by the font licenses |
| `Screenshot from 2026-08-01 03-29-34_b.png` | Rasgo Synth development screenshot | © 2026 Lucio de Araujo; documentation image |
| `fonts/BigShouldersDisplay.ttf` | Big Shoulders family, The Big Shoulders Project Authors | SIL Open Font License 1.1; see `fonts/LICENSE-BigShoulders.txt` |
| `fonts/IBMPlexMono-Regular.ttf` | IBM Plex Mono, IBM Corp. | SIL Open Font License 1.1; see `fonts/LICENSE-IBMPlexMono.txt` |
| `fonts/IBMPlexMono-Bold.ttf` | IBM Plex Mono, IBM Corp. | SIL Open Font License 1.1; see `fonts/LICENSE-IBMPlexMono.txt` |

The authoritative family sources consulted for the notices are the official
Google Fonts directories for
[Big Shoulders](https://github.com/google/fonts/tree/main/ofl/bigshoulders) and
[IBM Plex Mono](https://github.com/google/fonts/tree/main/ofl/ibmplexmono).
Current asset hashes:

```text
60e208dc276a1c35fc5b62e94f9fb959c40c11783a9eb7548175c14b1fbeb720  fonts/BigShouldersDisplay.ttf
ac27abd6450a64dd94467580a02fe6235156d5b92f2926ebbc8e7489df64e0be  fonts/IBMPlexMono-Bold.ttf
6a3412f058c7d8dfd9170c41e85ade48e5156ecb89356110ca57a0a27734af46  fonts/IBMPlexMono-Regular.ttf
b600ed690d6475cd202984669c55c7451bfac7aa83b056da2a0391d3e0ab58a6  rasgo_logo_2026.svg
```

Upstream identity verified on2026-10-14 by downloading the three official
binaries and comparing SHA256 with these local files (all three identical):

- BigShouldersDisplay.ttf is the unmodified variable
  BigShouldersDisplay[wght].ttf from google/fonts commit
  `7d86539c3e24d8ea2cae7b3ff04c64250dca99c5`, path
  `ofl/bigshouldersdisplay/BigShouldersDisplay[wght].ttf`.
- Both IBM Plex Mono files match google/fonts commit
  `633f3200539c52ee0aba2dfd7f46921417a81877`, under `ofl/ibmplexmono/`.

These identify byte-equal published upstream copies, not the unknown original
download date of the historical local files. Font binaries were not modified
or replaced. Official immutable references:
[Big Shoulders](https://github.com/google/fonts/tree/7d86539c3e24d8ea2cae7b3ff04c64250dca99c5/ofl/bigshouldersdisplay),
[IBM Plex Mono](https://github.com/google/fonts/tree/633f3200539c52ee0aba2dfd7f46921417a81877/ofl/ibmplexmono).
