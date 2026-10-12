# Porting notes

## What changed relative to upstream 0.8.3
1. Debian/Ubuntu patch series applied (libplist 2.x, GLib DateTime ambiguity, itdb_resolve_path speed-up, kFreeBSD).
2. Autotools (configure.ac, intltool, gtk-doc, gnome-common) replaced by a library-only CMake build.
3. `config.h` was never defining `HAVE_UNISTD_H` in the new build; that exposed missing `read/write/close/unlink/sync`
   declarations, which are hard errors on current Apple clang. Fixed by detecting `unistd.h`.
4. `ithumb-writer.c`: `g_object_ref()` results were assigned to `GdkPixbuf *` without a cast (incompatible pointer
   type: a hard error on newer compilers). Cast added.
5. Relocatable `libgpod-1.0.pc`; installed header layout is unchanged (`<gpod/itdb.h>`).
6. New hardware-free test (`cmake_tests/roundtrip.c`): fake Classic tree, write signed DB, re-parse, remove, re-write.

## Verified so far
- Linux x86-64, GCC 13, glib 2.80: builds, `roundtrip` passes, installed package usable via pkg-config.
- Everything else: **not yet verified** - that is what the CI matrix (.github/workflows/ci.yml) is for.

## Not built yet
- `tools/` (udev/HAL/SCSI/USB helpers such as `ipod-read-sysinfo-extended`) and the python/mono `bindings/` - Linux-specific.
  Consequence: on macOS/Windows the library cannot fetch the FireWire GUID from the device; callers must supply it
  (`itdb_device_set_sysinfo(dev, "FirewireGuid", ...)`) or read it from the OS (`system_profiler SPUSBDataType` on macOS).
- Old gtk-doc documentation and translations (`po/`).
- MSVC: only MinGW-style toolchains are targeted (the sources use POSIX calls and GNU C).

## Known issues / limitations inherited from 0.8.3
- GCC reports a few likely-benign warnings: `db-artwork-parser.c` stores the address of a local in a context
  (`dangling-pointer`), `rijndael.c` (`stringop-overflow` in the AES helpers), `strncpy` bound in the artwork writer.
  These deserve an audit rather than a blind fix because they sit in crypto/DB parsing code.
- Sqlite and libplist are hard dependencies even though a Classic needs neither (they serve newer iPods).
- Unaligned pointer reads when parsing binary DBs should be audited for 32-bit ARM.
- Whole-database rewrite on every `itdb_write`, linear list lookups: slow beyond ~10k tracks.
- `itdb_track_remove()` does not remove a track from playlists in this version; use
  `itdb_playlist_remove_track(NULL, t); itdb_track_unlink(t); itdb_track_free(t);` instead.
- Model detection is by serial-number suffix; if the serial was edited, set `ModelNumStr` manually.
- Smart-playlist rules with unknown action types are preserved but not evaluated (log noise).
- Nano 5G+/hashAB needs a proprietary blob that is not available.

## Roadmap
1. Get the CI matrix green on macOS (arm64+x86-64), Linux arm64, Windows (MSYS2).
2. Make sqlite / libplist optional via `#ifdef`, so the Classic build needs only glib (+ pixbuf).
3. Port the GUID-reading helper for macOS (IOKit) and Windows (SetupAPI) as an optional `libgpod-device` module.
4. Replace deprecated GLib calls (`g_memdup`, `GTimeVal`, `g_module_build_path`), audit the warnings above.
5. Faster incremental writes / hash tables instead of lists.
