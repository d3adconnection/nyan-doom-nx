# Nintendo Switch Port — Changes from Nyan Doom

All Switch-specific code is isolated behind `#ifdef __SWITCH__` or `if(NINTENDO_SWITCH)` CMake guards.

---

## New files

### `prboom2/cmake/SwitchToolchain.cmake`
CMake toolchain file for devkitA64 cross-compilation. Includes `$DEVKITPRO/cmake/Switch.cmake` and defines `NINTENDO_SWITCH`.

### `prboom2/src/SDL/i_switch.c` / `i_switch.h`
All Switch platform glue, called from `i_main.c`:
- `I_SwitchInit()` — initialises BSD sockets (nxlink debug), creates `sdmc:/switch/nyan-doom/` and `chdir`s into it so all relative paths (config, saves, WADs) land on the SD card.
- `I_SwitchShutdown()` — tears down sockets.
- `I_SwitchApplyAudioDefaults()` — called after `M_LoadDefaults()`; boosts the FluidSynth gain default from 50 → 100 (synth.gain 0.5 → 1.0) on first run; scans the data directory for the first `.sf2` soundfont and writes its path into `snd_soundfont` if not already configured; if a soundfont was auto-detected and `snd_midiplayer` is not already configured, also sets it to `"fluidsynth"`.

### `prboom2/src/gl_stub.c`
Stub implementations of every OpenGL fixed-function and `gld_*` call. The Switch only has GLES/EGL — no desktop GL — so the GL renderer is compiled out entirely and these stubs satisfy the linker.

### `prboom2/ICONS/nyan-doom-256.jpg`
256×256 JPEG icon embedded in the `.nro` by `elf2nro`, displayed by Switch homebrew launchers (e.g. Sphaira).

### `build-switch.ps1`
PowerShell 5.1 build script at the repo root. Supports incremental builds by default (`-Clean` flag required for a full wipe). Runs CMake configuration with the Switch toolchain and invokes Ninja to produce `build-switch/src/nyan-doom.nro`. Hardcodes devkitPro and repository paths for Windows.

---

## Modified files

### `prboom2/cmake/NyanDependencies.cmake`
- OpenGL and PortMidi dependency blocks gated behind `NOT NINTENDO_SWITCH`.
- FluidSynth detection block: on Switch, locates `libfluidlite.a` (FluidLite — a glib-free, API-compatible SF2 renderer) and creates a `FluidSynth::libfluidsynth` STATIC IMPORTED target pointing to it, so the rest of the build sees a uniform target name.

### `prboom2/src/CMakeLists.txt`
- `gl_*.c` source files wrapped in `if(NOT NINTENDO_SWITCH)`.
- `gl_stub.c` and `SDL/i_switch.c` added for Switch builds.
- Links `libnx` on Switch.
- Sets binary suffix to `.elf` on Switch.
- Post-build commands: `nacptool --create` generates a `.nacp` metadata file, then `elf2nro` packages `nyan-doom.nro` with the `.nacp` and the 256×256 icon.

### `prboom2/src/MUSIC/flplayer.c`
FluidSynth depends on GLib (a large GNOME utility library) which is not available for Switch homebrew and would be impractical to port. [FluidLite](https://github.com/divideconcept/FluidLite) is a stripped-down fork that removes the GLib dependency and several other desktop-only features, making it straightforward to cross-compile for devkitA64.

- Includes `<fluidlite.h>` instead of `<fluidsynth.h>`.
- Skips the `fluid_version()` runtime call (not exported by FluidLite); assumes `sratemin = 8000`.
- Guards off the SNDFONT lump loader — FluidLite removed `fluid_sfloader_set_callbacks`, so WAD-embedded soundfonts cannot be loaded from memory. Replaced with a warning directing users to place an `.sf2` file in `sdmc:/switch/nyan-doom/`.
- Casts `fl_null_logger` to match FluidLite's `char *` log-function signature (vs FluidSynth's `const char *`).
- Defines `FLUID_OK`/`FLUID_FAILED` and maps `FLUIDLITE_VERSION_*` → `FLUIDSYNTH_VERSION_*` as compat shims.
- Bypasses `I_FindFile2()` (desktop file search) when loading a soundfont — uses `fopen(snd_soundfont, "rb")` directly since `snd_soundfont` is always an absolute `sdmc:/` path on Switch.
- Nulls `f_syn`/`f_set` in all failure paths to prevent double-free.

### `prboom2/src/SDL/i_main.c`
- Calls `I_SwitchInit()` at the very start of `main()`.
- Calls `I_SwitchApplyAudioDefaults()` immediately after `M_LoadDefaults()`.

### `prboom2/src/SDL/i_system.c`
- `I_GetXDGDataHome()`, `I_GetXDGDataDirs()`, and `I_ConfigDir()` all return `SWITCH_DATA_DIR` (`"sdmc:/switch/nyan-doom"`) on Switch so configs, saves, and asset lookups are rooted on the SD card.

### `prboom2/src/SDL/i_video.c`
- Forces `desired_screenwidth = 1280`, `desired_screenheight = 720`, fullscreen, and the software renderer on Switch (no desktop GL available). These are applied after the config is read so they override any saved values.

### `prboom2/src/m_misc.c`
- **Config parser:** `sscanf` format in `M_LoadDefaults()` changed from `%[^\n]` to `%[^\r\n]`. On Switch and other platforms where `fopen` does not strip `\r`, a CRLF config file causes `strparm` to capture a trailing `\r`. The existing `strparm[len-1] = 0` then removes `\r` instead of the closing `"`, so the stored string is `fluidsynth"` instead of `fluidsynth`. Stopping at `\r` or `\n` fixes this.
- **Input defaults (`#ifdef __SWITCH__` guarded):** Default controller button bindings adjusted for Switch conventions so PC/Xbox gamepad defaults are unchanged:

  | Action | Old default | New default | Physical button |
  |---|---|---|---|
  | `input_menu_enter` | `BUTTON_A` | `BUTTON_B` | A (right) — Nintendo confirm |
  | `input_menu_backspace` | `BUTTON_B` | `BUTTON_A` | B (bottom) — Nintendo cancel |
  | `input_use` | `BUTTON_A` | `BUTTON_B` | A (right) |
  | `input_jump` | `BUTTON_B` | `BUTTON_A` | B (bottom) |

  `input_nextweapon` (`BUTTON_Y` → physical X) and `input_prevweapon` (`BUTTON_X` → physical Y) were already routed to the correct physical buttons; only their display labels required the fix in `game_controller.c`.

### `prboom2/src/m_menu.c`
- `M_ItemDisabled()`: returns `true` on Switch for the four video settings that are forced at runtime — Video mode, Screen Resolution, Fullscreen Video mode, and Exclusive Fullscreen — so they are greyed out and non-interactive in the settings menu.

### `prboom2/src/SDL/i_sound.c`
- `midiplayers[]`: null-terminated after `"opl"` on Switch so PortMIDI does not appear in the Preferred MIDI Player menu (PortMIDI is not available on Switch).
- `M_ChangeMIDIPlayer()`: portmidi selection branch guarded with `#ifndef __SWITCH__` to prevent a null-pointer dereference if an old config file has `snd_midiplayer portmidi`.

### `prboom2/src/dsda/endoom.c`
- Calls `SDL_FlushEvents(SDL_FIRSTEVENT, SDL_LASTEVENT)` before the ENDOOM wait loop to drain any button-press events that were queued during the quit dialog, preventing an instant-exit.

### `prboom2/src/dsda/game_controller.c`
- `dsda_InitGameController()`: forces `use_game_controller = 1` on Switch unless `-nojoy` is passed, since the Joy-Con is always present.
- `button_names[]`: Switch builds swap the display labels for the four face buttons to match physical Joy-Con markings. SDL uses the Xbox positional layout, so the indices are:

  | `DSDA_CONTROLLER_BUTTON_*` | SDL position | Physical Switch button | Display label |
  |---|---|---|---|
  | `A` | south | B (bottom) | "pad b" |
  | `B` | east | A (right) | "pad a" |
  | `X` | west | Y (left) | "pad y" |
  | `Y` | north | X (top) | "pad x" |

### `prboom2/src/gl_opengl.h`
- Guards the `#include <GL/glu.h>` line behind `!defined(__SWITCH__)`.

### `prboom2/src/textscreen/txt_sdl.c`
- In `TXT_GetChar()`, `SDL_CONTROLLERBUTTONDOWN` and `SDL_JOYBUTTONDOWN` events return `1` on Switch. This covers all textscreen UI — the ENDOOM screen, setup/configuration menus, and confirmation dialogs — none of which can receive keyboard input on Switch (no physical keyboard).
