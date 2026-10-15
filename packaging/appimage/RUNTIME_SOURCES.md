# AppImage runtime — sources and relinking

Prepared 2026-10-15 for Rasgo Synth Studio 0.1.0. These are the sources of
the AppImage launcher, not the musical engine. Application and JUCE sources
are separate assets in the same release. No third-party source here is
claimed as work by the author of Rasgo.

## What accompanies the binary

`manifest.json` records seven upstream archives, their immutable checksums,
the Alpine patches and their original locations. `notices/` contains the
complete selected license texts, including musl's composite COPYRIGHT,
libfuse's LGPL 2.1 and GPL 2 texts, and mimalloc's MIT notice. The runtime
source header is also preserved: its Simon Peter/Alexander Larsson and
public-domain MD5 credits are not replaced by the shorter root LICENSE.

The complete original archives are in `upstream/`. Their original copyright
headers and all other source licenses remain in place. libfuse's own LICENSE
distinguishes the LGPL library from GPL utilities. This bundle does not
relicense either. zstd offers a BSD license as well as GPLv2; its complete
original texts are retained. Alpine musl/mimalloc patches are separately
identified in `patches/`, with original authors and headers retained.

Verify the extracted kit with `sha256sum -c SHA256SUMS` before using sources.
The generator neither downloads nor runs these sources. Do not run arbitrary
downloaded build scripts on a production workstation.

## Build provenance

The official launcher is AppImage/type2-runtime commit
`8f39b89e2ac31e1640b3d3f7e9a5108e6ce805fa`, SHA-256
`156f4bdbde9c52d01814600013e0a273f0118dc2de98975f3c8c63427ec79074`.
Its upstream build is [run 36463736478](https://github.com/AppImage/type2-runtime/actions/runs/36463736478),
x86_64 job 109068477114. It used Alpine 3.21 image digest
`sha256:ce64758a109eb420d874a118f87920e625e12d3634e03b4a5573fd9f6e5d3507`,
musl 1.2.5-r11, zstd 1.5.6-r2, zlib 1.3.2-r0 and mimalloc 2.1.7-r0.
The recipe itself builds libfuse 3.15.0 and squashfuse 0.5.2.

The runtime archive includes its original Makefile, linker script, version,
Docker/chroot recipes and libfuse patch. The Alpine package recipes used to
check package revisions and patch checksums can be consulted at:

- [musl 1.2.5-r11](https://github.com/alpinelinux/aports/blob/8999cf56154b9bd09c995f293d8dfd3aa8001e3a/main/musl/APKBUILD)
- [mimalloc2 2.1.7-r0](https://github.com/alpinelinux/aports/blob/5a1a5dc3216a69e3cd66a57ee07cb5ef01b8be39/community/mimalloc2/APKBUILD)
- [zstd 1.5.6-r2](https://github.com/alpinelinux/aports/blob/5c2ddf18f193dafdd25ae7391d96c7e30766eecd/main/zstd/APKBUILD)
- [zlib 1.3.2-r0](https://github.com/alpinelinux/aports/blob/2b38f55109add14f4f99a974c2fdf421b6b9e9e9/main/zlib/APKBUILD)

Package recipe metadata is not a relicensing of the recipe itself. Recipes
are linked here, not represented as Rasgo-owned source. The musl command-line
utilities getconf/getent/iconv are not incorporated into the launcher.

## Rebuild or change libfuse

Use a disposable x86_64 Linux container/VM and inspect the original recipes
before running anything. Limit builds to one worker. The launcher can be
rebuilt entirely from source; no proprietary object or signing key is needed.

1. Extract the seven verified archives into a fresh working directory.
   The runtime source directory is `AppImage-type2-runtime-8f39b89`.
2. Supply the musl/static-library development environment from the pinned
   Alpine base. For equivalent source builds, the supplied musl patches are
   applied in the order in `manifest.json`; set its VERSION to 1.2.5. Follow
   the linked recipe for target flags. zlib and zstd have no Alpine source
   patches in these revisions. Apply the supplied mimalloc CMake patch and
   build its static secure variant (`MI_SECURE=ON`), as selected by Alpine's
   `libmimalloc.a` symlink. These instructions do not claim a bit-identical
   compiler/toolchain reconstruction.
3. In `fuse-3.15.0`, apply the runtime's `patches/libfuse/mount.c.diff` to
   `lib/mount.c`, following `scripts/common/install-dependencies.sh`. Build
   with Meson `--default-library static --prefix=/usr`, then `ninja -j1`
   and install only inside the disposable environment. To modify libfuse,
   edit this source before building. Its LGPL license permits such changes;
   preserve the original notices and mark modifications.
4. Build squashfuse 0.5.2 as in the original dependency script (`autogen.sh`,
   configure with `LDFLAGS=-static`, then `make -j1` and install). Make the
   static libraries and headers visible to clang and the linker. The source
   kit preserves the exact upstream dependency build script for details.
5. In the runtime's `src/runtime/`, preserve the supplied `version` file and
   run `make -j1`. The Makefile links `squashfuse`, `squashfuse_ll`, `zstd`,
   `z`, `fuse3` and `mimalloc`, with `-static -static-pie` and the supplied
   linker script. Re-running this step relinks against your modified libfuse.
6. Repackage the original AppDir using the generated `runtime`, e.g. set
   `LDAI_RUNTIME_FILE=/absolute/path/to/your/runtime` when using linuxdeploy's
   AppImage output plugin. The Studio packaging recipe rejects an unpinned
   runtime intentionally; a local experimental rebuild must explicitly
   change its runtime pin. Do not overwrite the original release file.
7. Test the newly named AppImage and its extracted `AppRun`. Include the
   changed source, notices and hashes when redistributing a modified build.

The runtime rebuild/relink procedure has been inspected, not executed for
this release. Runtime-source completeness checks do not validate audio,
FUSE mounting, or every distributor's legal situation. Dynamically bundled
application libraries have a separate version/copyright inventory inside
the AppImage. Installing `.deb` or the portable tar archive does not use
this AppImage launcher.
