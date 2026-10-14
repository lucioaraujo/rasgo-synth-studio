# Studio — build nativa e distribuição multiplataforma

Atualizado em 2026-10-14. Código próprio sob AGPL-3.0-or-later; a revisão
JUCE e seus avisos estão em `DEPENDENCIES.md` e `THIRD_PARTY_NOTICES.md`.
Este documento não declara versões Windows/macOS disponíveis.

## Receita CMake

O aplicativo tem agora um alvo JUCE em `rasgo-synth-app/CMakeLists.txt`,
com C++17, módulos GUI/áudio, assets explícitos e tratamento de compilador
MSVC/GCC/Clang. A dependência precisa ser um checkout limpo no commit
`f8f8864172464b9adf9eba6101e1f784838d1597`. A configuração recusa outra
revisão e não baixa código. Não usa nenhum instrumento irmão no build.

Da raiz do Studio, com compiladores, CMake3.22+ e dependências locais:

```bash
cmake -S rasgo-synth-app -B .local-build/studio-cmake \
  -DRASGO_JUCE_ROOT=/caminho/ao/JUCE-fixado -DCMAKE_BUILD_TYPE=Release
cmake --build .local-build/studio-cmake --config Release --parallel 1
```

O primeiro comando pode compilar o auxiliar juceaide; aplicar o procedimento
de recursos também na configuração. Os comandos acima são a receita geral,
não os limites de tempo/memória de uma validação automatizada. Usar uma pasta
de saída nova e orçamento proporcional; não instalar dependências no anfitrião
sem autorização. Em geradores multiconfig, `--config Release` seleciona o perfil.

No macOS, planejar build Intel/Apple Silicon, SDK e deployment target
explícitos e ensaio do bundle; não pressupor que uma compilação numa arquitetura
produza as duas. No Windows, usar toolchain MSVC e confirmar arquitetura,
runtime, caminhos Unicode e backend de áudio. A receita ainda não foi
executada nesses sistemas.

Assets ficam ao lado do executável ou em `Contents/Resources/assets` no
bundle macOS. O CMake agora também reúne avisos explícitos e prepara a árvore
de instalação. Continua sendo candidato de desenvolvimento: fontes
correspondentes, dependências, assinaturas e pacotes nativos ainda exigem
validação antes do envio.

## Instalação e CPack — preparação de 14/10

`InstallStudio.cmake` seleciona executável, assets, fontes, desktop/ícone Linux,
avisos autorais e oito avisos da revisão JUCE, incluindo zlib quando incorporada.
`PackagePolicy.cmake` prepara .deb Linux, NSIS/.zip Windows e .dmg macOS.
CPack só roda por comando explícito; não há publicação ou instalação automática.

Depois de uma build nativa validada, para testar num destino privado novo:

```bash
cmake --install .local-build/studio-cmake --config Release \
  --prefix /caminho/privado/para/teste
cpack --config .local-build/studio-cmake/CPackConfig.cmake \
  -C Release -B .local-build/pacotes-nativos
```

Para NSIS é necessária a ferramenta nativa correspondente. Os atalhos apontam
à raiz onde o exe é instalado; pasta e chave de registro são estáveis, sem
versão, e a política prevê desinstalação anterior sem alterar os dados externos
do usuário. Esses campos foram testados como metadados, não num Windows real.
macOS exige seleção explícita de arquitetura; o nome universal só é preparado
para arm64+x86_64, mas verificar o binário com lipo continua obrigatório.
Não alegar assinatura/notarização por gerar um DMG.

Referência consultada sem alteração: Rasgo Modular, commit
`92d36face6d55c0d2305ccd6d47b2117c6cc26a9`, CMakeLists.txt SHA256
`162d9fbbcaec49e72c0c02491a92d53afa4a45078a2a8acee6813afcb303b719` e
install.sh SHA256 `73a9bbc6b731d74b98837dc567221e06bf8da93272bbbe2449290a96d3eab345`.
O Studio mantém implementação própria e não depende desse projeto.

O tar Linux continua gerado pelo empacotador específico: traz `./run` e o
instalador opcional `./install.sh`, sem sudo, com atualização no mesmo destino
e remoção dos arquivos registrados. Python3.8+ só para essa instalação local.
Não confundir o tar com a árvore .deb sob /usr.

O log de arranque agora usa descritores POSIX em Linux/macOS e `_wfreopen_s`/
`_dup2` no Windows, com caminho nativo Unicode e inicialização dos streams
CRT antes de duplicar descritores (aplicativos GUI podem iniciar sem console).
Referências técnicas: Microsoft,
[`_fileno`](https://learn.microsoft.com/en-us/cpp/c-runtime-library/reference/fileno?view=msvc-170),
[`_dup2`](https://learn.microsoft.com/en-us/cpp/c-runtime-library/reference/dup-dup2?view=msvc-170) e
[`_wfreopen_s`](https://learn.microsoft.com/en-us/cpp/c-runtime-library/reference/freopen-s-wfreopen-s?view=msvc-170).
Sem código Microsoft copiado. Testes executados apenas
no Linux; não apresentar a adaptação como validação Windows.

## Portões ainda necessários

### Combo nativo preparado em 14/10

`.github/workflows/package-native.yml` prepara Windows2022/x64 (NSIS + ZIP),
macOS15/Intel e macOS15/Apple Silicon (DMG separados, alvo12.0 explícito).
JUCE continua fixado; builds e configuração são seriais. O CRT Windows foi
selecionado estático antes dos alvos JUCE, para não depender de uma instalação
separada do Visual C++ Runtime no ZIP. Falta comprovar isso no executável nativo.
Nenhum pacote Windows/macOS foi ainda produzido. Não há assinatura/notarização,
nem teste de áudio/GUI nativo implícito na inspeção lipo/otool preparada.

O combo é manual, com confirmação de orçamento inicialmente desmarcada.
Linux é opcional e reutiliza o workflow existente, agora também workflow_call;
não é refeito por cada push. Ambas as receitas continuam inativas enquanto
aninhadas no monorepositório. Nenhum workflow de outro instrumento foi alterado.
Cinco testes de contrato/sintaxe passaram localmente; PowerShell e execução
Actions não foram validados por esses testes.

Orçamento antes da primeira execução: teto nativo25 min Windows +35 min
por arquitetura macOS =95 runner-minutos. Linux completo opcional acrescenta
25 min build +4×15 min de testes =85 min. Em cobrança sem franquia, usando
as tarifas padrão consultadas em14/10 (Linux USD0,006/min, Windows0,010/min,
macOS0,062/min), referência máxima USD4,59 nativo ou USD5,10 com Linux,
sem armazenamento/franquia/arredondamentos adicionais. É uma estimativa,
não preço garantido nem aprovação de despesa. Os runners padrão em repositório
público têm tratamento distinto; confirmar visibilidade, saldo e política
antes de acionar. Fontes oficiais:
[tarifas](https://docs.github.com/en/billing/reference/actions-runner-pricing) e
[runners](https://docs.github.com/en/actions/reference/runners/github-hosted-runners).

Destino local verificado: origin ainda é rasgo-instruments, não repositório
independente do Studio. Não promover a raiz desse monorepositório nem enviar
projetos privados como efeito colateral. A seleção de fontes e o destino são
a próxima decisão antes da execução; publicação de release/site permanece
separada e sem gatilho automático.

- Linux: consolidar o candidato Ubuntu22.04, ensaio no Mint e demais
  distribuições declaradas, avisos/fontes, requisitos e seleção de release.
- Windows: build nativa, pacote portátil e instalador no padrão RASGO,
  abertura por atalho, áudio, render/export/álbum, nomes acentuados, cancelamento
  e fechamento. Testar instalação/atualização sem apagar dados do usuário.
- macOS: build e bundle Intel/Apple Silicon, assets/fontes, áudio, render,
  acentuação, cancelamento e fechamento; preparar `.dmg` e documentar assinatura
  e Gatekeeper. Não alegar Developer ID ou notarização inexistentes.
- Todos: versão consistente, seleção explícita de fontes e artefatos, notices,
  checksums, limitações conhecidas, instruções e links nos quatro idiomas.

CI multiplataforma só após seleção de fontes/repositório e decisão de executar.
Manter acionamento manual e sem publicação automática. Uma rodada de três
sistemas pode ter custo muito superior ao tempo de relógio, especialmente
macOS; rever o orçamento real antes de acionar. Nesta etapa nenhum serviço
externo foi acionado e nenhum release foi publicado.

Configuração e build CMake completas passaram no Mint22 em13/10, assim como
janela/rolagem/redimensionamento/fechamento e um render limitado. Os testes
Linux não comprovam os ramos MSVC/macOS. O binário CMake Mint possui piso
GLIBC2.38/GLIBCXX3.4.32; não substitui o candidato Ubuntu2.35 por renomeação.

Evidências Linux e estado de cada validação: `CONTINUIDADE.md` e
`TAREFAS_STUDIO.md` (S28/S29). O novo CMake não altera retroativamente a
receita, o código ou o piso ABI dos pacotes já gerados.
