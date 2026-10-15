# Empacotamento Linux do Studio

Atualização: 2026-10-15.

Estado atual15/10: o candidato público33fbe3c foi compilado em Ubuntu22.04
na rodada38654762892 e passou pelos ensaios Debian12/Ubuntu24.04/Fedora/Arch.
Foi preparado um [kit de fontes/avisos do runtime](../appimage/RUNTIME_SOURCES.md)
e um suplementador offline de AppImage. Este usa SOMENTE o plugin de saída
do linuxdeploy fixado: invocar linuxdeploy completo novamente recopia
dependências do host e não é aceitável para suplementar um candidato já
testado. A conferência exige que executável, bibliotecas, assets e atalhos
anteriores permaneçam byte a byte; apenas avisos identificados são novos.
O AppImage suplementado abriu/fechou no Mint, layout sem violações em1366×768;
não houve render/escuta nesse ensaio nem montagem FUSE (`/dev/fuse` ausente).
Release/site ainda aguardam o fechamento macOS. Os parágrafos históricos
abaixo não substituem esse estado nem autorizam repetição de build.

Padrão RASGO: `.deb`, AppImage e `.tar.gz`, assets, menu desktop, licenças,
instruções e checksums. Não requer outro instrumento no build.

Referência consultada somente em leitura: Rasgo Modular, commit
`92d36face6d55c0d2305ccd6d47b2117c6cc26a9`, `packaging/linux/empacotar.sh`
SHA256 `83464127d1afa6f200faa21d6be76e85f197ab13bf8cf9781a560700e338e7a7`.
Implementação do Studio escrita para sua receita direta de build, assets e
dados próprios. Não se copiou o instalador destrutivo nem se alterou `$HOME`.
Código próprio sob AGPL-3.0-or-later; referências preservam sua autoria.

```bash
bash packaging/linux/empacotar.sh --binary /caminho/rasgo_synth_app \
  --output .local-build/pacote-novo --deb
```

A saída deve ser nova, dentro de `.local-build/`. Só o executável indicado e
uma lista explícita de assets, avisos e instruções entram no pacote, nunca
áudios, preferências, diários ou aliases para outros instrumentos. O script
gera inventário ELF, manifesto de seleção, checksums e arquivos `.tar.gz` e,
com `--deb`, `.deb` usando dependências calculadas por `dpkg-shlibdeps`.
Não instala nada no sistema. Os formatos partem da mesma árvore `usr/`.

Novo tar inclui `install.sh` e `install_studio.py`: `./install.sh` instala
em `~/.local` sem root; `./install.sh --remove` retira somente arquivos
registrados e não modificados. Python3.8+ é requisito apenas desse instalador
opcional; `./run` continua abrindo o instrumento sem Python. O payload,
incluindo assets e avisos, fica em `PREFIX/lib/rasgo-synth-studio`, com launcher,
menu e ícone nos destinos pessoais usuais. PREFIX pode selecionar um destino
privado alternativo para testes ou uso. O atalho usa caminho absoluto.

Implementação própria, tomando o procedimento do Modular como referência,
sem dependência ou mudança daquele instrumento. Recibo com hashes, lock de
instalação, validação dos checksums do pacote, limites64 MiB/256 entradas e
recusa de symlinks, travessia de diretórios e colisões desconhecidas.
Atualizações usam o mesmo destino e não criam pasta por versão; fechar o
aplicativo antes de atualizar. Arquivos modificados são preservados na remoção
e impedem sobrescrita na atualização. Remoção nunca é recursiva; pastas vazias
próprias podem ser retiradas. Não é uma transação global nem garante durabilidade
contra queda de energia. Usar somente pacotes obtidos do autor: checksums
detectam alteração, mas não autenticam a origem de um pacote malicioso.

Os scripts ficam na raiz do tar; não entram no `usr/` do `.deb` ou AppImage.
Arquivos antigos não foram sobrescritos nem relabelados como novos.

AppImage exige `--linuxdeploy /caminho/linuxdeploy-x86_64.AppImage`.
A ferramenta é verificada ANTES de executar: versão `1-alpha-20251107-1`,
SHA256 `c20cd71e3a4e3b80c3483cef793cda3f4e990aca14014d23c544ca3ce1270b4d`,
o mesmo pin consultado no Modular. Não há download implícito. Fonte primária:
[release oficial](https://github.com/linuxdeploy/linuxdeploy/releases/tag/1-alpha-20251107-1).
Também exige `--runtime /caminho/runtime-x86_64` e
`--runtime-license /caminho/AppImage-runtime-LICENSE`. Runtime fixado pela
release oficial (metadados de28/09/2026): commit
`8f39b89e2ac31e1640b3d3f7e9a5108e6ce805fa`, asset596078161,
SHA256 `156f4bdbde9c52d01814600013e0a273f0118dc2de98975f3c8c63427ec79074`;
aviso SHA256 `aa154fc9070614bbe7921f89db11efd1dba7a1f3a41685958110e2230f9c0ca1`.
A URL `continuous` não é tratada como pin: mudança de bytes faz o script
recusar a execução. Fonte: [runtime oficial](https://github.com/AppImage/type2-runtime)
e [aviso da revisão](https://github.com/AppImage/type2-runtime/blob/8f39b89e2ac31e1640b3d3f7e9a5108e6ce805fa/LICENSE).
Bibliotecas copiadas pelo linuxdeploy precisam de revisão dos seus avisos e
fontes correspondentes antes da publicação; AppImage não elimina exigências
de glibc/CPU nem comprova compatibilidade entre distribuições.

Base para o candidato amplo: Ubuntu22.04.5. Build completa e três formatos
gerados13/10 em raiz isolada; piso GLIBC2.35/GLIBCXX3.4.30. Instalação .deb,
arranque/fechamento dos três formatos e render passaram na base Ubuntu.
AppImage ensaiado pela árvore extraída, não montagem FUSE. Candidatos anteriores
do Mint22 têm outro piso ABI e permanecem preservados. Ensaios do candidato
novo no Mint/Debian/Fedora/Arch e aprovação de release ainda pendentes.
Não ativar Actions nem publicar só por concluir o empacotamento.

Receita serial `preparar-build.sh` e workflow `package-linux.yml` preparados.
Workflow manual, sem push/tag/release automático, com limite de duração e
matriz de arranque. Ainda não executado; na posição atual do monorepo é
inerte. Orçamento estimado30–60 minutos Linux por acionamento, a revisar antes
de executar. Não repetir os builds a cada edição.

O candidato Ubuntu compila quatro unidades C JPEG/PNG do JUCE fixado, com
avisos IJG/libpng e sem link direto jpeg8. Bibliotecas transitivas de FreeType
podem incluir libpng. AppImage exigiu instalar `file` apenas no ambiente
isolado; o workflow preparado inclui agora essa dependência. Os pacotes
validados precedem a adaptação posterior de logs/CMake: não contêm essa nova
mudança de fonte e não são substituídos automaticamente por ela.

Receita CMake e pendências Windows/macOS: [`PORTABILIDADE.md`](../PORTABILIDADE.md).

Próximos portões: seleção limpa de fontes correspondentes Studio/JUCE,
base Linux antiga, arranque isolado de cada formato/distribuição,
revisão dos avisos de bibliotecas embarcadas e aprovação do autor.
