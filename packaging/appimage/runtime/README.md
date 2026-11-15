# AppImage type2-runtime guardado no repositório

O runtime do AppImage que o Studio usa desde a 0.1.1 fica aqui, e não é mais
baixado na CI. Decisão do autor (16/11/2026), por recomendação técnica.

**Por quê:** a CI baixava o runtime de
`AppImage/type2-runtime/releases/download/continuous/`, uma release que o
projeto AppImage troca de conteúdo. Em 19/10/2026 o arquivo foi trocado; o hash
travado pelo Studio deixou de existir em qualquer release e o empacotamento do
Linux passou a recusar a execução (corretamente). Guardar a cópia exata mantém
o build reproduzível e o kit de fontes correspondente
([`../RUNTIME_SOURCES.md`](../RUNTIME_SOURCES.md)) continua válido sem mudança.

**Procedência:**

- projeto: [AppImage/type2-runtime](https://github.com/AppImage/type2-runtime),
  commit `8f39b89e2ac31e1640b3d3f7e9a5108e6ce805fa`;
- origem dos arquivos: artefato `artifacts x86_64` (id 10989035536) da build
  oficial [run 36463736478](https://github.com/AppImage/type2-runtime/actions/runs/36463736478),
  de 28/09/2026, baixado em 16/11/2026 (expira no GitHub em 27/12/2026);
- `runtime-x86_64`: SHA-256
  `156f4bdbde9c52d01814600013e0a273f0118dc2de98975f3c8c63427ec79074`, idêntico
  ao registrado em `tools/package_studio_linux.py` (`RUNTIME_SHA`) e usado na
  0.1.1;
- `runtime-x86_64.sig` e `signing-pubkey.asc`: a assinatura e a chave pública
  que vieram no mesmo artefato;
- `LICENSE`: o aviso MIT do mesmo commit
  ([raw](https://raw.githubusercontent.com/AppImage/type2-runtime/8f39b89e2ac31e1640b3d3f7e9a5108e6ce805fa/LICENSE)),
  SHA-256 `aa154fc9070614bbe7921f89db11efd1dba7a1f3a41685958110e2230f9c0ca1`
  (`RUNTIME_LICENSE_SHA`).

Hashes em [`SHA256SUMS`](SHA256SUMS). O empacotador continua conferindo o
hash antes de executar qualquer ferramenta. Licença do runtime: MIT; as
bibliotecas que ele embute e suas fontes estão em `../RUNTIME_SOURCES.md`.
