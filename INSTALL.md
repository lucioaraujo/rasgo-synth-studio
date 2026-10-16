# Rasgo Synth Studio — instalação / installation

Revisão: 2026-10-16 · versão 0.1.1 experimental.

PT · [Português](#portugues) / EN · [English](#english) /
FR · [Français](#francais) / ES · [Español](#espanol)

## Português

### Escolher o arquivo

| Seu sistema | Arquivo |
| --- | --- |
| Ubuntu, Mint, Debian x86_64 | `.deb` ou `.tar.gz` |
| Fedora, Arch e outros Linux glibc x86_64 | `.tar.gz` ou AppImage, respeitando as bibliotecas mínimas abaixo |
| Windows x64 | Instalador `.exe` ou pasta portátil `.zip` |
| Mac Intel ou Apple Silicon (M1 ou posterior) | Um único `.dmg` com `macos-universal` no nome |

Os pacotes estão na [publicação experimental 0.1.1](https://github.com/lucioaraujo/rasgo-synth-studio/releases/tag/v0.1.1).
O [código-fonte é público](https://github.com/lucioaraujo/rasgo-synth-studio).
Use somente os arquivos dessa publicação oficial do Studio.
Este guia explica os formatos; não certifica todos os sistemas.

### Windows x64

1. Baixe o `.exe` e abra o instalador; escolha o destino e siga os passos.
2. Abra Rasgo Synth Studio pelo atalho criado. Para a opção portátil, extraia
   o ZIP inteiro e abra `Rasgo Synth Studio.exe` dentro da pasta extraída.
3. Preserve `assets`, fontes e licenças junto do executável. Não execute de
   dentro do ZIP nem mova apenas o `.exe` da versão portátil.
4. Se o SmartScreen mostrar um aviso, confira origem, nome e SHA256 do arquivo
   antes de decidir abrir. Não desative a proteção do Windows globalmente.

Alvo: Windows 10/11 de 64 bits. Render, caminhos acentuados e fechamento
passaram em teste automatizado da árvore instalada; a instalação manual
e a escuta física ainda precisam de testes. Não existe pacote Windows ARM ou32 bits. Não é preciso instalar
Python, JUCE ou compilador para usar o pacote pronto. Para remover a versão
instalada, use Aplicativos do Windows; preserve suas gravações e preferências.

### macOS — Intel ou Apple Silicon

1. Use o DMG Universal 2, compatível com Intel e Apple Silicon, sem Rosetta.
2. Abra o DMG e copie `Rasgo Synth Studio.app` para Aplicativos.
3. Ejete a imagem e abra a cópia em Aplicativos, não a aplicação dentro do DMG.
4. Se o macOS bloquear a abertura, confira a origem e siga a opção de abertura
   específica em Ajustes do Sistema → Privacidade e Segurança. Não desative o
   Gatekeeper nem remova proteções globalmente.

Alvo de compilação: macOS12 ou posterior, não um mínimo comprovado por testes.
O mesmo DMG Universal 2 passou nos testes automatizados de abertura, render
e fechamento em Intel e Apple Silicon. O aplicativo inteiro tem assinatura
ad-hoc de integridade; não alegamos Developer ID nem notarização.
Isso não substitui instalação manual ou escuta em hardware real.

### Se não instalar ou não abrir

| Sintoma | O que verificar |
| --- | --- |
| Windows: “O Windows protegeu o computador” | Confira a publicação oficial e o SHA256. Se confiar no arquivo e a opção estiver disponível, use Mais informações → Executar assim mesmo somente para ele. Não crie uma exclusão geral no antivírus. Um aviso explícito de malware exige interromper a instalação. |
| Windows: `VCRUNTIME140.dll` ou `MSVCP140.dll` ausente | O pacote Studio é preparado com runtime C++ estático; essa DLL não deve ser necessária. Baixe novamente o pacote completo e informe versão/erro se persistir. Não baixe DLLs avulsas nem aplique automaticamente a solução de versões antigas de outro instrumento. |
| Windows: o atalho não abre | Tente o executável na pasta escolhida durante a instalação. Se ele abrir, o problema é o atalho: reinstale o pacote oficial no mesmo destino e envie o caminho do atalho antigo. Não apague gravações ou preferências. |
| Imagens/fontes ausentes, ou ZIP que não abre | Extraia tudo antes de executar. Mantenha `assets` e `licenses` com o programa; não copie só o `.exe`. Use a pasta completa da versão nova, sem misturar arquivos de versões diferentes. |
| Mac: “a Apple não pôde verificar” ou desenvolvedor não identificado | Depois de tentar abrir a cópia em Aplicativos, e somente se confiar na origem e integridade, vá a Ajustes do Sistema → Privacidade e Segurança → Abrir Mesmo Assim; confirme Abrir. Isso é uma exceção para aquele aplicativo, não uma aprovação de segurança pela Apple. |
| Mac: “danificado”, alerta de malware, ou falta de permissão para liberar | Não trate todos os avisos como falsos positivos. Pare, baixe novamente da fonte oficial e compare o hash; se continuar, envie a mensagem. Num Mac administrado, consulte o responsável. Não desative o Gatekeeper globalmente. |
| Atualização, versão errada ou programa já aberto | Feche o Studio antes de atualizar. No Mac, ejete o DMG e use a cópia em Aplicativos. No Windows, mantenha o mesmo destino; remova duplicatas por Configurações → Aplicativos, sem excluir as pastas de dados. |

Para conferir um arquivo no Windows, abra PowerShell na pasta do download e
use `Get-FileHash ".\NOME-EXATO-DO-ARQUIVO.exe" -Algorithm SHA256`;
compare o resultado com a linha correspondente em `SHA256SUMS`. No Mac,
use `shasum -a 256 "/caminho/NOME-EXATO-DO-ARQUIVO.dmg"`. Substitua o nome
pelo arquivo baixado. Hash igual confirma os bytes, não a confiabilidade de
uma fonte desconhecida. Uma assinatura ad-hoc preserva a integridade local,
mas não equivale a Developer ID ou notarização.

Se a janela abrir, mas não houver som, a instalação já não é a primeira
hipótese: confira o mute/volume e a saída de áudio do sistema, a indicação
de áudio no rodapé do Studio, gere uma peça curta e então use Play. Nenhum
microfone ou controlador MIDI é necessário. Se fechar durante o render,
isso é uma falha de execução a investigar, não motivo para reinstalar DLLs.

Para pedir ajuda, informe sistema/versão, processador, versão do Studio,
nome exato do pacote e mensagem/captura. Antes de reabrir o programa, copie
`rasgo_synth.log` e `rasgo_synth_console.log`: eles são reiniciados no arranque.
Sem overrides, ficam em:

- Windows: `%APPDATA%\Rasgo Synth\Logs` (cole na barra do Explorador).
- Mac: `~/Library/Rasgo Synth/Logs` (Finder → Ir → Ir para a pasta).
- Linux: `~/.config/Rasgo Synth/Logs`, ou a pasta definida por `XDG_CONFIG_HOME`.

Se usar `RASGO_SYNTH_DATA_DIR`, procure `Logs` nesse diretório. Remova dados
pessoais dos logs antes de enviar. Use [Issues do Studio](https://github.com/lucioaraujo/rasgo-synth-studio/issues)
ou [email](mailto:rasgo.instruments@gmail.com). Não envie áudios sem desejar
compartilhá-los. Orientações oficiais: [Apple](https://support.apple.com/en-us/102445)
e [Microsoft SmartScreen](https://learn.microsoft.com/en-us/windows/security/operating-system-security/virus-and-threat-protection/microsoft-defender-smartscreen/).

### Configuração e compatibilidade

O pacote Linux x86_64 foi compilado em Ubuntu 22.04 e testado nessa base isolada
e no Linux Mint 22. Os formatos de distribuição são `.deb`, AppImage e
`.tar.gz`; produzir o arquivo não equivale a testar todas as distribuições.

| Item | Requisito / referência |
| --- | --- |
| Processador | x86_64, com SSE2; este pacote não atende ARM ou sistemas de 32 bits |
| Sistema | Linux com glibc; o candidato atual exige glibc 2.35+, GLIBCXX_3.4.30 e libasound.so.2 (símbolo ALSA_0.9.0rc4) |
| Interface gráfica | Sessão X11 ou XWayland com acesso ao display; Wayland nativo não foi validado |
| Tela | 1366 × 768 é a referência de uso; 900 × 600 permite acesso por rolagem, já verificado localmente |
| Áudio | Saída estéreo via ALSA; PipeWire/PulseAudio podem oferecer essa saída; render interno em 48 kHz |
| Memória | Referência provisória: 8 GiB de RAM, com pelo menos 4 GiB disponíveis; não é um mínimo certificado |
| Armazenamento | Reserve 500 MiB para aplicação e margem de trabalho, além do espaço dos áudios |

Recomendamos 16 GiB de RAM para trabalhar com álbuns e outros programas
abertos. Não há benchmark suficiente para prometer uma duração de render nem
um modelo mínimo de processador. O teste local utilizou um Intel i5-6500 e
24 GiB de RAM; não prova que 24 GiB sejam necessários. Compilar requer mais
recursos que executar o aplicativo pronto.

Um WAV estéreo PCM24 em 48 kHz ocupa aproximadamente 17,3 MB por minuto.
Reserve cerca de 1,1 GB por hora de áudio, mais os arquivos acompanhantes.
O render também mantém áudio na RAM; duração e opções podem aumentar o uso.

O piso das bibliotecas é medido no executável, não deduzido do nome do Linux.
O candidato foi testado localmente em Ubuntu22.04 isolado e Mint22.
Os pacotes da CI também passaram abertura, layout e fechamento em Debian12,
Ubuntu24.04, Fedora e Arch com tela virtual; isso não é escuta em hardware.
Mint21 e todas as versões dessas distribuições não foram certificados. AppImage
não resolve por si só uma glibc mais antiga. Linux musl/Alpine e Linux ARM
não são atendidos por este pacote. Windows e macOS usam os pacotes próprios
descritos acima. O AppImage foi testado extraído, não montado por FUSE.

### Abrir o arquivo .tar.gz

Extraia a pasta inteira, não somente o executável. Abra um terminal nela:

```bash
chmod +x run
./run
```

O launcher verifica as bibliotecas antes de abrir. Se aparecer `not found`,
`GLIBC_... not found` ou `GLIBCXX_... not found`, o pacote não está atendido
pelo sistema. Não copie bibliotecas aleatórias para `/usr/lib` e não tente
substituir a glibc. Informe a distribuição, versão e mensagem completa.

Nos novos pacotes que incluem `install.sh`, você também pode instalar no
menu de aplicativos sem root:

```bash
./install.sh
```

O instalador opcional precisa de Python3.8+, mas o instrumento não precisa
de Python para abrir por `./run`. Não use sudo nesse script. Ele instala em
`~/.local`, incluindo fontes, imagens e licenças, e cria um atalho com caminho
absoluto. Para atualizar, feche o Studio e execute o script do pacote novo;
o destino permanece o mesmo. Não instale simultaneamente as variantes local
e `.deb` sem necessidade, para evitar entradas de menu que confundam versões.

Para remover os arquivos registrados: `./install.sh --remove`. Composições,
preferências, arquivos desconhecidos e arquivos instalados que você modificou
são preservados. O script pode deixar uma pasta que contenha arquivos seus.
Uma atualização recusa substituir arquivos desconhecidos/modificados; informe
a mensagem em vez de apagar diretórios inteiros. Mudanças são atômicas por
arquivo, não uma transação completa de todos os arquivos.

Destino alternativo: `PREFIX="/caminho/privado" ./install.sh`; use o mesmo
PREFIX na remoção. Destinos com symlinks ou caracteres especiais não suportados
são recusados. Os pacotes anteriores que não contêm `install.sh` continuam
portáteis: não recebem essa função retroativamente.

### Pacote .deb

Para um candidato compatível com seu sistema, abra o terminal na pasta do
arquivo e use o nome exato que baixou:

```bash
sudo apt install ./rasgo-synth-studio-0.1.1-linux-x86_64.deb
```

Depois abra Rasgo Synth Studio no menu de aplicativos ou execute
`rasgo-synth-studio`. A instalação usa o gerenciador do sistema; para remover:
`sudo apt remove rasgo-synth-studio`. Não remove suas composições.

### AppImage

Marque o arquivo como executável nas propriedades ou pelo terminal:

```bash
chmod +x rasgo-synth-studio-0.1.1-linux-x86_64.AppImage
./rasgo-synth-studio-0.1.1-linux-x86_64.AppImage
```

Se o sistema não oferece FUSE, tente a extração temporária:

```bash
APPIMAGE_EXTRACT_AND_RUN=1 ./rasgo-synth-studio-0.1.1-linux-x86_64.AppImage
```

Não requer instalação permanente. Confira o hash com `sha256sum -c SHA256SUMS`
na pasta que contém os pacotes. Só execute arquivos da publicação do autor.

### Primeira escuta e seus arquivos

Escolha uma pasta gravável para as composições. Comece com volume baixo,
uma peça curta e o modo álbum desligado. O tutorial explica a composição e a
escuta passo a passo; não há necessidade de MIDI, GPU dedicada ou internet
para compor. A interface USB AudioBox não é obrigatória.

Sem override, gravações vão para a pasta Music/Rasgo Synth do usuário e as
preferências para o diretório de dados indicado pelo JUCE. Esses dados não
ficam dentro do pacote. Para testes isolados, use `RASGO_SYNTH_DATA_DIR` e
`RASGO_SYNTH_RECORDINGS_DIR`; não sobrescreva sua configuração habitual.

## English

### Choose your package

| Your system | Package |
| --- | --- |
| Ubuntu, Mint, Debian x86_64 | `.deb` or `.tar.gz` |
| Fedora, Arch and other glibc-based Linux x86_64 | `.tar.gz` or AppImage, subject to the library floor below |
| Windows x64 | `.exe` installer or portable `.zip` |
| Intel or Apple Silicon Mac (M1 or later) | One DMG named `macos-universal` |

Packages are available in the [experimental 0.1.1 release](https://github.com/lucioaraujo/rasgo-synth-studio/releases/tag/v0.1.1).
[Source code is public](https://github.com/lucioaraujo/rasgo-synth-studio).
Only use files from this official Studio release. These
instructions explain the formats; they do not certify every operating system.

### Windows x64

1. Download and open the `.exe` installer, select its destination and follow
   the prompts. Start Rasgo Synth Studio using the installed shortcut.
2. For the portable option, extract the entire ZIP, then open
   `Rasgo Synth Studio.exe` from the extracted folder.
3. Keep `assets`, fonts and licenses beside the executable. Do not run inside
   the ZIP or move just the portable executable.
4. If SmartScreen warns, verify the source, filename and SHA256 before
   deciding to open. Do not disable Windows protection globally.

Target: 64-bit Windows10/11. Automated installed-tree render, Unicode paths
and close passed; manual installation and physical listening remain untested.
No Windows ARM/32-bit package. Python, JUCE and a compiler are not required
to use the finished package. Remove the installed version through Windows
Apps; keep recordings and preferences separate.

### macOS — Intel or Apple Silicon

1. Use the Universal 2 DMG for Intel and Apple Silicon; Rosetta is not needed.
2. Open it and copy `Rasgo Synth Studio.app` into Applications.
3. Eject the image and open the copy in Applications, not the mounted DMG.
4. If macOS blocks it, verify its origin and use the application-specific
   opening option in System Settings → Privacy & Security. Never disable
   Gatekeeper or remove security protections globally.

Build target: macOS12 or later, not a tested minimum. The same Universal 2
DMG passed automated startup/render/close tests on Intel and Apple Silicon.
The whole app has an ad-hoc integrity seal, not Developer ID or notarization.
This does not replace manual installation
or listening on real hardware.

### If installation or startup fails

| Symptom | What to check |
| --- | --- |
| Windows: “Windows protected your PC” | Verify the official release and SHA256. If you trust the file and the option exists, use More info → Run anyway for this file only. Do not add a general antivirus exclusion. Stop if malware is explicitly detected. |
| Windows: missing `VCRUNTIME140.dll` or `MSVCP140.dll` | Studio is packaged with the static C++ runtime; these DLLs should not be required. Download the complete package again and report the version/error if it persists. Do not download loose DLLs or automatically apply a workaround for an old version of another instrument. |
| Windows: shortcut does nothing | Try the executable in the installation folder you selected. If it opens, reinstall the official package in the same destination and report the old shortcut target. Do not delete recordings or preferences. |
| Missing images/fonts, or failure inside ZIP | Extract everything first. Keep `assets` and `licenses` with the program; do not copy only the executable. Use the complete new-version folder without mixing files from different versions. |
| Mac: Apple cannot verify the app or developer | First try the copy in Applications. Only if you trust its source and integrity, use System Settings → Privacy & Security → Open Anyway, then confirm Open. This is an exception for that app, not Apple security approval. |
| Mac: “damaged”, malware warning, or no permission to allow it | Do not assume every warning is a false positive. Stop, download again from the official source and compare the hash; report the message if it persists. On a managed Mac, ask its administrator. Do not disable Gatekeeper globally. |
| Updating, wrong version or an already-open app | Close Studio first. On Mac, eject the DMG and use the Applications copy. On Windows, keep the same installation destination; remove duplicates through Settings → Apps without deleting user-data folders. |

In PowerShell in the download folder, run
`Get-FileHash ".\EXACT-DOWNLOAD-NAME.exe" -Algorithm SHA256` and compare
the result with that file's `SHA256SUMS` entry. On Mac use
`shasum -a 256 "/path/EXACT-DOWNLOAD-NAME.dmg"`. Substitute the actual
download name. A matching hash verifies bytes, not an unknown source's
trustworthiness. An ad-hoc signature seals local integrity; it is neither
a Developer ID signature nor notarization.

If the window opens without sound, first check system mute/volume and output,
the Studio audio status in the footer, generate a short piece, then press
Play. No microphone or MIDI controller is needed. An exit during rendering
is a runtime failure to investigate, not a reason to install random DLLs.

For help, include OS/version, processor, Studio version, exact package name
and the message/screenshot. Before reopening Studio, copy `rasgo_synth.log`
and `rasgo_synth_console.log`: startup resets them. Without overrides:

- Windows: `%APPDATA%\Rasgo Synth\Logs` in the Explorer address bar.
- Mac: `~/Library/Rasgo Synth/Logs` via Finder → Go → Go to Folder.
- Linux: `~/.config/Rasgo Synth/Logs`, or the `XDG_CONFIG_HOME` location.

With `RASGO_SYNTH_DATA_DIR`, use its `Logs` directory. Remove personal
information before sharing logs. Use [Studio Issues](https://github.com/lucioaraujo/rasgo-synth-studio/issues)
or [email](mailto:rasgo.instruments@gmail.com); audio sharing is optional.
Official guidance: [Apple](https://support.apple.com/en-us/102445) and
[Microsoft SmartScreen](https://learn.microsoft.com/en-us/windows/security/operating-system-security/virus-and-threat-protection/microsoft-defender-smartscreen/).

### Requirements and compatibility

The Linux x86_64 package was built on Ubuntu 22.04 and tested in that isolated
environment and on Linux Mint 22. Distribution formats are `.deb`, AppImage and `.tar.gz`;
building a package does not establish support for every distribution.

| Item | Requirement / reference |
| --- | --- |
| CPU | x86_64 with SSE2; not ARM or 32-bit |
| OS | glibc-based Linux; current candidate requires glibc 2.35+, GLIBCXX_3.4.30 and libasound.so.2 (ALSA_0.9.0rc4 symbol) |
| Display server | X11 or XWayland with display access; native Wayland not validated |
| Screen | 1366 × 768 reference; 900 × 600 with scrolling verified locally |
| Audio | Stereo ALSA output, including an ALSA endpoint provided by PipeWire/PulseAudio; internal render at 48 kHz |
| RAM | Provisional starting reference: 8 GiB total, at least 4 GiB available; not a certified minimum |
| Storage | Allow 500 MiB for application/workspace plus generated audio |

16 GiB RAM is recommended for albums alongside other applications, not a
measured minimum. Local testing used an i5-6500 with 24 GiB RAM. Render time
depends on the piece and settings; no minimum CPU model is certified.
PCM24 stereo at 48 kHz uses about 17.3 MB/minute (1.1 GB/hour). Rendered audio
also occupies RAM; longer pieces/settings may need more. Compilation needs
more resources than running the finished application.

Local checks covered isolated Ubuntu22.04 and Mint22. CI packages also passed
startup, layout and cooperative close on Debian12, Ubuntu24.04, Fedora and
Arch using a virtual display, not hardware listening. Mint21 and every
version of these distributions have not been certified.
AppImage does not remove the glibc floor. Linux musl/Alpine and Linux ARM
are not supported by this package. Windows/macOS use their own packages above.
AppImage was tested extracted, not mounted through FUSE.
MIDI, a dedicated GPU and internet are not required to
compose; a USB AudioBox is optional.

### Install and start

`.tar.gz`: extract the whole folder, open a terminal there, run `chmod +x run`
then `./run`. The launcher checks libraries. Do not replace glibc or copy
random libraries when a `not found` message occurs; report your OS/version
and the complete error.

New packages containing `install.sh` also support `./install.sh` to add the
application to the menu under `~/.local`, without sudo. Python3.8+ is needed
only for this optional installer, not to run the instrument. Fonts, images
and licenses are included. Close Studio before updating with the new package's
script; the destination stays the same. Remove registered files with
`./install.sh --remove`. Compositions, preferences, unknown files and edited
installed files are preserved; updates refuse unknown/modified targets.
Operations are atomic per file, not a complete transaction. For a custom
destination use `PREFIX="/private/path" ./install.sh` and the same PREFIX
for removal. Symlinks and unsupported special path characters are refused.
Prefer one installation method rather than duplicate local and `.deb`
installations. Older archives without the script remain portable only.

`.deb`: from the downloaded file's folder run
`sudo apt install ./rasgo-synth-studio-0.1.1-linux-x86_64.deb`, then open
Rasgo Synth Studio from the application menu or run `rasgo-synth-studio`.
Removal: `sudo apt remove rasgo-synth-studio`; compositions are preserved.

AppImage: `chmod +x rasgo-synth-studio-0.1.1-linux-x86_64.AppImage`, then
`./rasgo-synth-studio-0.1.1-linux-x86_64.AppImage`. Without FUSE, try
`APPIMAGE_EXTRACT_AND_RUN=1 ./rasgo-synth-studio-0.1.1-linux-x86_64.AppImage`.
Check downloads with `sha256sum -c SHA256SUMS`. Only run the author's release
files. No permanent installation is required for AppImage or `.tar.gz`.

Choose a writable output directory, start with low volume and a short piece,
and follow the tutorial. By default, compositions use the user's Music/Rasgo
Synth folder; preferences use JUCE's user data directory. Both stay outside
the package. Isolated tests may override `RASGO_SYNTH_DATA_DIR` and
`RASGO_SYNTH_RECORDINGS_DIR` without replacing existing settings.

## Français

### Choisir le fichier

| Votre système | Fichier |
| --- | --- |
| Ubuntu, Mint, Debian x86_64 | `.deb` ou `.tar.gz` |
| Fedora, Arch et autres Linux glibc x86_64 | `.tar.gz` ou AppImage, selon les bibliothèques minimales ci-dessous |
| Windows x64 | Installateur `.exe` ou archive portable `.zip` |
| Mac Intel ou Apple Silicon (M1 ou suivant) | Un seul DMG portant `macos-universal` |

Les paquets sont disponibles dans la [publication expérimentale 0.1.1](https://github.com/lucioaraujo/rasgo-synth-studio/releases/tag/v0.1.1).
Le [code source est public](https://github.com/lucioaraujo/rasgo-synth-studio).
Utiliser uniquement ces fichiers officiels. Ces instructions ne certifient pas tous les systèmes.

### Windows x64

1. Ouvrir l'installateur `.exe`, choisir la destination et suivre les étapes.
   Lancer Rasgo Synth Studio par le raccourci créé.
2. Pour la version portable, extraire tout le ZIP, puis ouvrir
   `Rasgo Synth Studio.exe` dans le dossier extrait.
3. Conserver `assets`, polices et licences avec l'exécutable. Ne pas lancer
   depuis le ZIP ni déplacer uniquement l'exécutable portable.
4. En cas d'avertissement SmartScreen, vérifier provenance, nom et SHA256 avant
   de décider d'ouvrir. Ne pas désactiver globalement la protection Windows.

Cible : Windows10/11 en64 bits. Rendu, chemins accentués et fermeture passent
en test automatisé de l'arbre installé ; installation manuelle et écoute
restent à tester. Pas de paquet Windows ARM/32 bits. Python, JUCE et compilateur
ne sont pas nécessaires. Désinstaller par les Applications Windows, sans
effacer ses enregistrements et préférences.

### macOS — Intel ou Apple Silicon

1. Utiliser le DMG Universal 2 pour Intel et Apple Silicon, sans Rosetta.
2. L'ouvrir et copier `Rasgo Synth Studio.app` dans Applications.
3. Éjecter l'image puis ouvrir la copie, pas l'application dans le DMG.
4. Si macOS bloque l'ouverture, vérifier la provenance et utiliser l'option
   spécifique à l'application dans Réglages Système → Confidentialité et
   sécurité. Ne pas désactiver globalement Gatekeeper ou les protections.

Cible de compilation : macOS12 ou suivant, pas un minimum vérifié.
Le même DMG Universal 2 a passé les tests automatisés de démarrage/rendu/
fermeture sur Intel et Apple Silicon. L'application entière porte une
signature ad-hoc d'intégrité, sans Developer ID ni notarisation.
Cela ne remplace ni installation manuelle ni écoute sur matériel réel.

### En cas de problème d'installation ou de démarrage

| Symptôme | Vérification |
| --- | --- |
| Windows : « Windows a protégé votre ordinateur » | Vérifier la publication officielle et le SHA256. Si la source est fiable et l'option disponible, choisir Informations complémentaires → Exécuter quand même uniquement pour ce fichier. Pas d'exclusion générale de l'antivirus. Arrêter si un logiciel malveillant est explicitement détecté. |
| Windows : `VCRUNTIME140.dll` ou `MSVCP140.dll` absent | Le Studio utilise le runtime C++ statique ; ces DLL ne devraient pas être nécessaires. Télécharger à nouveau le paquet complet et signaler version/erreur si cela persiste. Ne pas télécharger de DLL isolée ni appliquer automatiquement une solution propre à une ancienne version d'un autre instrument. |
| Windows : raccourci inopérant | Essayer l'exécutable dans le dossier choisi à l'installation. S'il fonctionne, réinstaller le paquet officiel au même endroit et signaler la cible de l'ancien raccourci. Conserver enregistrements et préférences. |
| Images/polices absentes, lancement depuis le ZIP | Extraire tout le dossier, conserver `assets` et `licenses` avec le programme. Ne pas déplacer seulement l'exécutable ni mélanger les fichiers de différentes versions. |
| Mac : Apple ne peut pas vérifier l'app ou le développeur | Essayer d'abord la copie dans Applications. Uniquement si provenance et intégrité sont fiables : Réglages Système → Confidentialité et sécurité → Ouvrir quand même, puis Ouvrir. Cette exception concerne l'app ; ce n'est pas une approbation de sécurité par Apple. |
| Mac : app « endommagée », alerte de malware ou autorisation impossible | Ne pas supposer que toute alerte est fausse. Arrêter, retélécharger depuis la source officielle et comparer le hash ; signaler le message s'il persiste. Sur un Mac administré, consulter le responsable. Ne pas désactiver Gatekeeper globalement. |
| Mise à jour, mauvaise version ou app déjà ouverte | Fermer le Studio. Sur Mac, éjecter le DMG et utiliser la copie dans Applications. Sous Windows, garder la même destination ; retirer les doublons par Paramètres → Applications, sans supprimer les données personnelles. |

Dans PowerShell, depuis le dossier téléchargé :
`Get-FileHash ".\NOM-EXACT-DU-FICHIER.exe" -Algorithm SHA256` ; comparer
à l'entrée correspondante de `SHA256SUMS`. Sur Mac :
`shasum -a 256 "/chemin/NOM-EXACT-DU-FICHIER.dmg"`. Remplacer le nom par
celui du téléchargement. Un hash identique vérifie les octets, pas la
fiabilité d'une source inconnue. Une signature ad-hoc scelle l'intégrité
locale ; ce n'est ni une signature Developer ID ni une notarisation.

Fenêtre ouverte sans son : vérifier mute/volume, sortie du système et état
audio au pied du Studio ; générer une pièce courte puis lancer Play. Ni
microphone ni contrôleur MIDI nécessaire. Une fermeture pendant le rendu
est un incident d'exécution, pas une raison d'installer des DLL au hasard.

Pour demander de l'aide : système/version, processeur, version du Studio,
nom exact du paquet, message/capture. Avant de rouvrir, copier
`rasgo_synth.log` et `rasgo_synth_console.log` : ils sont réinitialisés au
démarrage. Sans overrides :

- Windows : `%APPDATA%\Rasgo Synth\Logs` dans l'Explorateur.
- Mac : `~/Library/Rasgo Synth/Logs` par Finder → Aller → Aller au dossier.
- Linux : `~/.config/Rasgo Synth/Logs`, ou l'emplacement `XDG_CONFIG_HOME`.

Avec `RASGO_SYNTH_DATA_DIR`, chercher son dossier `Logs`. Retirer les données
personnelles avant de partager. Utiliser [Issues du Studio](https://github.com/lucioaraujo/rasgo-synth-studio/issues)
ou [email](mailto:rasgo.instruments@gmail.com) ; partager l'audio est facultatif.
Références officielles : [Apple](https://support.apple.com/en-us/102445) et
[Microsoft SmartScreen](https://learn.microsoft.com/en-us/windows/security/operating-system-security/virus-and-threat-protection/microsoft-defender-smartscreen/).

### Configuration et compatibilité

Le paquet Linux x86_64 a été compilé sous Ubuntu 22.04 et testé dans cet
environnement isolé et sous Linux Mint 22. Formats : `.deb`, AppImage et `.tar.gz`.
Produire un paquet ne valide pas toutes les distributions.

| Élément | Exigence / référence |
| --- | --- |
| Processeur | x86_64 avec SSE2 ; ni ARM ni système 32 bits |
| Système | Linux avec glibc ; candidat actuel : glibc 2.35+, GLIBCXX_3.4.30 et libasound.so.2 (symbole ALSA_0.9.0rc4) |
| Affichage | X11 ou XWayland accessible ; Wayland natif non validé |
| Écran | Référence 1366 × 768 ; accès par défilement vérifié à 900 × 600 |
| Audio | Sortie stéréo ALSA, éventuellement fournie par PipeWire/PulseAudio ; rendu interne à 48 kHz |
| Mémoire | Repère provisoire : 8 Gio au total, au moins 4 Gio disponibles ; pas un minimum certifié |
| Stockage | Prévoir 500 Mio pour application/marge de travail, plus les fichiers audio |

16 Gio sont conseillés pour les albums avec d'autres applications ouvertes.
Test local : i5-6500 et 24 Gio ; cela ne prouve pas que 24 Gio soient
nécessaires. Aucun processeur minimal ni temps de rendu garanti. Le PCM24
stéréo à 48 kHz occupe environ 17,3 Mo/minute, soit 1,1 Go/heure. Le rendu
utilise aussi la RAM ; durée et réglages peuvent augmenter la consommation.
Compiler demande plus de ressources que lancer l'application.

Tests locaux : Ubuntu22.04 isolé et Mint22. Les paquets CI passent aussi
démarrage, disposition et fermeture sous Debian12, Ubuntu24.04, Fedora et
Arch avec écran virtuel ; ce n'est pas une écoute sur matériel réel.
Mint21 et toutes les versions de ces distributions ne sont pas certifiés.
AppImage ne supprime pas l'exigence glibc. Ce paquet ne couvre pas Linux
musl/Alpine ou Linux ARM. Windows/macOS ont leurs propres paquets ci-dessus.
AppImage a été testé extrait, pas monté via FUSE. MIDI, carte graphique dédiée et connexion
internet ne sont pas nécessaires pour composer ; l'AudioBox USB est facultative.

### Installer et lancer

`.tar.gz` : extraire le dossier entier. Dans son terminal : `chmod +x run`,
puis `./run`. Le lanceur vérifie les bibliothèques. En cas de `not found`,
ne remplacez pas glibc et ne copiez pas de bibliothèques au hasard ; indiquez
votre système, sa version et le message complet.

Les nouveaux paquets contenant `install.sh` permettent aussi `./install.sh`
pour ajouter le programme au menu dans `~/.local`, sans sudo. Python3.8+ est
nécessaire uniquement pour cet installateur facultatif, pas pour lancer
l'instrument. Polices, images et licences sont incluses. Fermer le Studio
avant de mettre à jour avec le script du nouveau paquet ; le chemin reste
identique. Retirer les fichiers enregistrés : `./install.sh --remove`.
Compositions, préférences, fichiers inconnus ou modifiés sont conservés ;
la mise à jour refuse d'écraser des fichiers inconnus/modifiés. Opérations
atomiques par fichier, pas une transaction globale. Autre destination :
`PREFIX="/chemin/privé" ./install.sh`, avec le même PREFIX pour retirer.
Liens symboliques et caractères spéciaux non pris en charge sont refusés.
Éviter de cumuler installation locale et `.deb`. Les anciens paquets sans
ce script restent utilisables en mode portable.

`.deb` : `sudo apt install ./rasgo-synth-studio-0.1.1-linux-x86_64.deb` dans
le dossier du fichier ; lancer Rasgo Synth Studio depuis le menu ou avec
`rasgo-synth-studio`. Désinstaller : `sudo apt remove rasgo-synth-studio`.
Vos compositions sont conservées.

AppImage : `chmod +x rasgo-synth-studio-0.1.1-linux-x86_64.AppImage`, puis
`./rasgo-synth-studio-0.1.1-linux-x86_64.AppImage`. Sans FUSE, essayer
`APPIMAGE_EXTRACT_AND_RUN=1 ./rasgo-synth-studio-0.1.1-linux-x86_64.AppImage`.
Vérifier les fichiers par `sha256sum -c SHA256SUMS`. N'exécuter que les
fichiers publiés par l'auteur. AppImage et `.tar.gz` n'exigent pas
d'installation permanente.

Choisir un dossier accessible en écriture, commencer à faible volume avec
une courte pièce et suivre le tutoriel. Par défaut : Music/Rasgo Synth pour
les compositions et le répertoire utilisateur JUCE pour les préférences,
hors du paquet. Tests isolés : `RASGO_SYNTH_DATA_DIR` et
`RASGO_SYNTH_RECORDINGS_DIR`, sans remplacer les réglages habituels.

## Español

### Elegir el archivo

| Su sistema | Archivo |
| --- | --- |
| Ubuntu, Mint, Debian x86_64 | `.deb` o `.tar.gz` |
| Fedora, Arch y otros Linux glibc x86_64 | `.tar.gz` o AppImage, según las bibliotecas mínimas indicadas abajo |
| Windows x64 | Instalador `.exe` o archivo portátil `.zip` |
| Mac Intel o Apple Silicon (M1 o posterior) | Un único DMG con `macos-universal` en el nombre |

Los paquetes están en la [publicación experimental 0.1.1](https://github.com/lucioaraujo/rasgo-synth-studio/releases/tag/v0.1.1).
El [código fuente es público](https://github.com/lucioaraujo/rasgo-synth-studio).
Utilice solo archivos de esa publicación oficial.
Estas instrucciones no certifican todos los sistemas.

### Windows x64

1. Abra el instalador `.exe`, elija el destino y siga los pasos. Abra
   Rasgo Synth Studio mediante el acceso directo creado.
2. Para la versión portátil, extraiga todo el ZIP y abra
   `Rasgo Synth Studio.exe` desde la carpeta extraída.
3. Mantenga `assets`, fuentes y licencias junto al ejecutable. No lo abra
   dentro del ZIP ni mueva solo el ejecutable portátil.
4. Si SmartScreen advierte, compruebe origen, nombre y SHA256 antes de decidir
   abrir. No desactive globalmente la protección de Windows.

Objetivo: Windows10/11 de64 bits. Render, rutas acentuadas y cierre pasaron
la prueba automatizada del árbol instalado; faltan instalación manual y
escucha física. Sin paquete Windows ARM/32 bits. No requiere Python, JUCE o
compilador para usarlo. Desinstale mediante Aplicaciones de Windows,
conservando grabaciones y preferencias.

### macOS — Intel o Apple Silicon

1. Use el DMG Universal 2 para Intel y Apple Silicon; no requiere Rosetta.
2. Ábralo y copie `Rasgo Synth Studio.app` a Aplicaciones.
3. Expulse la imagen y abra la copia, no la aplicación dentro del DMG.
4. Si macOS bloquea la apertura, compruebe el origen y use la opción
   específica en Ajustes del Sistema → Privacidad y seguridad. No desactive
   Gatekeeper ni las protecciones globalmente.

Objetivo de compilación: macOS12 o posterior, no un mínimo comprobado.
El mismo DMG Universal 2 pasó las pruebas automatizadas de inicio/render/
cierre en Intel y Apple Silicon. La app completa tiene firma ad-hoc de
integridad, sin Developer ID ni notarización.
Esto no sustituye la instalación manual ni la escucha en hardware real.

### Si falla la instalación o el inicio

| Síntoma | Qué comprobar |
| --- | --- |
| Windows: «Windows protegió su PC» | Compruebe publicación oficial y SHA256. Si confía en el archivo y existe la opción, use Más información → Ejecutar de todas formas solo para ese archivo. No añada una exclusión general al antivirus. Deténgase si se detecta malware explícitamente. |
| Windows: falta `VCRUNTIME140.dll` o `MSVCP140.dll` | Studio incluye el runtime C++ estático; no deberían necesitarse esas DLL. Descargue de nuevo el paquete completo e informe de versión/error si persiste. No descargue DLL sueltas ni aplique automáticamente soluciones de versiones antiguas de otro instrumento. |
| Windows: acceso directo inoperante | Pruebe el ejecutable en la carpeta elegida durante la instalación. Si abre, reinstale el paquete oficial en el mismo destino e indique la ruta del acceso anterior. Conserve grabaciones y preferencias. |
| Faltan imágenes/fuentes, o intenta abrir desde ZIP | Extraiga todo; mantenga `assets` y `licenses` con el programa. No copie solo el ejecutable ni mezcle archivos de versiones distintas. |
| Mac: Apple no puede verificar la app o el desarrollador | Primero intente abrir la copia de Aplicaciones. Solo si confía en origen e integridad: Ajustes del Sistema → Privacidad y seguridad → Abrir igualmente; confirme Abrir. Es una excepción para esa app, no una aprobación de seguridad de Apple. |
| Mac: app «dañada», aviso de malware o sin permiso para autorizar | No suponga que todo aviso es un falso positivo. Deténgase, descargue de nuevo de la fuente oficial y compare el hash; informe si persiste. En un Mac administrado, consulte al responsable. No desactive Gatekeeper globalmente. |
| Actualización, versión incorrecta o app ya abierta | Cierre Studio. En Mac, expulse el DMG y utilice la copia de Aplicaciones. En Windows, conserve el mismo destino; quite duplicados mediante Configuración → Aplicaciones sin borrar carpetas de datos. |

En PowerShell, desde la carpeta descargada:
`Get-FileHash ".\NOMBRE-EXACTO-DEL-ARCHIVO.exe" -Algorithm SHA256`; compare
con la entrada correspondiente de `SHA256SUMS`. En Mac:
`shasum -a 256 "/ruta/NOMBRE-EXACTO-DEL-ARCHIVO.dmg"`. Sustituya el nombre
por el archivo descargado. El hash comprueba los bytes, no la fiabilidad de
una fuente desconocida. Una firma ad-hoc sella la integridad local; no es
una firma Developer ID ni una notarización.

Si la ventana abre sin sonido, compruebe mute/volumen, salida del sistema y
estado de audio al pie de Studio; genere una pieza corta y pulse Play.
No requiere micrófono ni controlador MIDI. Un cierre durante el render es
un fallo de ejecución a investigar, no motivo para instalar DLL al azar.

Para pedir ayuda: sistema/versión, procesador, versión de Studio, nombre
exacto del paquete y mensaje/captura. Antes de volver a abrir, copie
`rasgo_synth.log` y `rasgo_synth_console.log`: se reinician al arrancar.
Sin overrides:

- Windows: `%APPDATA%\Rasgo Synth\Logs` en el Explorador.
- Mac: `~/Library/Rasgo Synth/Logs` mediante Finder → Ir → Ir a la carpeta.
- Linux: `~/.config/Rasgo Synth/Logs`, o la ubicación `XDG_CONFIG_HOME`.

Con `RASGO_SYNTH_DATA_DIR`, busque su carpeta `Logs`. Quite datos personales
antes de compartir. Use [Issues de Studio](https://github.com/lucioaraujo/rasgo-synth-studio/issues)
o [email](mailto:rasgo.instruments@gmail.com); enviar audio es opcional.
Orientación oficial: [Apple](https://support.apple.com/en-us/102445) y
[Microsoft SmartScreen](https://learn.microsoft.com/en-us/windows/security/operating-system-security/virus-and-threat-protection/microsoft-defender-smartscreen/).

### Configuración y compatibilidad

El paquete Linux x86_64 se compiló en Ubuntu 22.04 y se probó en ese entorno
aislado y en Linux Mint 22. Formatos: `.deb`, AppImage y `.tar.gz`. Generar un paquete
no demuestra compatibilidad con todas las distribuciones.

| Elemento | Requisito / referencia |
| --- | --- |
| Procesador | x86_64 con SSE2; no ARM ni sistemas de 32 bits |
| Sistema | Linux con glibc; candidato actual: glibc 2.35+, GLIBCXX_3.4.30 y libasound.so.2 (símbolo ALSA_0.9.0rc4) |
| Entorno gráfico | X11 o XWayland accesible; Wayland nativo no validado |
| Pantalla | Referencia 1366 × 768; acceso con desplazamiento verificado a 900 × 600 |
| Audio | Salida estéreo ALSA, también mediante PipeWire/PulseAudio; render interno a 48 kHz |
| Memoria | Referencia provisional: 8 GiB totales, al menos 4 GiB disponibles; no es un mínimo certificado |
| Disco | Reservar 500 MiB para aplicación/margen de trabajo, más el audio generado |

Se recomiendan 16 GiB para álbumes con otras aplicaciones abiertas. Prueba
local: i5-6500 con 24 GiB; no demuestra que 24 GiB sean necesarios. No se
certifica un procesador mínimo ni un tiempo de render. PCM24 estéreo a
48 kHz: unos 17,3 MB/minuto, 1,1 GB/hora. El render también ocupa RAM y
puede requerir más según duración y opciones. Compilar necesita más recursos
que ejecutar la aplicación.

Pruebas locales: Ubuntu22.04 aislado y Mint22. Los paquetes CI también pasaron
inicio, disposición y cierre en Debian12, Ubuntu24.04, Fedora y Arch con
pantalla virtual, no escucha en hardware. Mint21 y todas las versiones de
esas distribuciones no están certificadas.
AppImage no elimina el requisito de glibc. Este paquete no cubre Linux
musl/Alpine ni Linux ARM. Windows/macOS tienen sus propios paquetes arriba.
AppImage se probó extraído, no montado mediante FUSE.
No se requiere MIDI, GPU dedicada ni internet para
componer; la AudioBox USB es opcional.

### Instalar y abrir

`.tar.gz`: extraer la carpeta completa; desde su terminal ejecutar
`chmod +x run`, después `./run`. El lanzador comprueba las bibliotecas. Si
aparece `not found`, no sustituya glibc ni copie bibliotecas al azar; informe
del sistema, versión y error completo.

Los paquetes nuevos que incluyen `install.sh` permiten también `./install.sh`
para añadir el programa al menú en `~/.local`, sin sudo. Python3.8+ solo es
necesario para este instalador opcional, no para ejecutar el instrumento.
Incluye fuentes, imágenes y licencias. Cierre Studio antes de actualizar con
el script del paquete nuevo; se conserva el mismo destino. Para retirar los
archivos registrados: `./install.sh --remove`. Se conservan composiciones,
preferencias, archivos desconocidos y archivos instalados modificados.
La actualización rechaza sobrescribir archivos desconocidos/modificados.
Cambios atómicos por archivo, no una transacción completa. Otro destino:
`PREFIX="/ruta/privada" ./install.sh`; use el mismo PREFIX para retirar.
Se rechazan enlaces simbólicos y caracteres especiales no admitidos.
Evite duplicar instalaciones locales y `.deb`. Los paquetes anteriores sin
el script siguen siendo portátiles.

`.deb`: `sudo apt install ./rasgo-synth-studio-0.1.1-linux-x86_64.deb` desde
la carpeta del archivo. Abrir Rasgo Synth Studio en el menú o ejecutar
`rasgo-synth-studio`. Desinstalar: `sudo apt remove rasgo-synth-studio`;
se conservan las composiciones.

AppImage: `chmod +x rasgo-synth-studio-0.1.1-linux-x86_64.AppImage`, después
`./rasgo-synth-studio-0.1.1-linux-x86_64.AppImage`. Sin FUSE, intentar
`APPIMAGE_EXTRACT_AND_RUN=1 ./rasgo-synth-studio-0.1.1-linux-x86_64.AppImage`.
Verificar con `sha256sum -c SHA256SUMS`. Ejecute solo archivos de la
publicación del autor. AppImage y `.tar.gz` no requieren instalación permanente.

Elegir una carpeta con permiso de escritura, empezar a volumen bajo con una
pieza corta y seguir el tutorial. Por defecto: Music/Rasgo Synth para audio
y el directorio de datos del usuario JUCE para preferencias, fuera del
paquete. Pruebas aisladas: `RASGO_SYNTH_DATA_DIR` y
`RASGO_SYNTH_RECORDINGS_DIR`, sin reemplazar la configuración habitual.
