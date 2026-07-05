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
- `I_SwitchDetectSoundfont()` — called after `M_LoadDefaults()`; manages soundfont auto-configuration on every launch:
  - If `snd_soundfont` is set but the file no longer exists, clears it (allows a replacement to be picked up).
  - If `snd_soundfont` is blank and a `.sf2` file is present in the data directory, sets `snd_soundfont` to its path and, if `snd_midiplayer` is blank, sets it to `"fluidsynth"`.
  - If `snd_midiplayer` is blank and no soundfont found, sets it to `"opl"`.
  - Returns `1` if the config was changed (caller saves); `0` if nothing was done.
- `I_SwitchGetNumericInput(current_value, buf, buf_size)` — opens the system numpad (`SwkbdType_NumPad`) prefilled with the current integer value. Returns 1 on confirm (result in `buf`), 0 on cancel or failure. Wraps the call in `appletLockExit`/`appletUnlockExit` and flushes SDL events on return.
- `I_SwitchGetStringInput(current, buf, buf_size)` — same but opens the full QWERTY keyboard (`SwkbdConfigMakePresetDefault`) prefilled with the current string.

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
- Guards off `fl_add_sfloader()` — FluidLite removed `fluid_sfloader_set_callbacks`, so the custom WAD sfloader cannot be registered. Two guard sites: before the main soundfont block (`#ifndef __SWITCH__`) and inside the SNDFONT lump fallback (`#ifdef __SWITCH__` warning directing users to place an `.sf2` in `sdmc:/switch/nyan-doom/`).
- Soundfont file loading uses `I_GetSoundfontFile()` identically to the desktop path — `snd_soundfont` is always an absolute `sdmc:/` path on Switch, which `M_FileExists` (plain `fopen`) resolves directly without needing special handling.
- Casts `fl_null_logger` to match FluidLite's `char *` log-function signature (vs FluidSynth's `const char *`).
- Defines `FLUID_OK`/`FLUID_FAILED` and maps `FLUIDLITE_VERSION_*` → `FLUIDSYNTH_VERSION_*` as compat shims.
- Nulls `f_syn`/`f_set` in all failure paths to prevent double-free.

### `prboom2/src/dsda/configuration.c`
- Switch-specific compile-time defaults via `#ifdef __SWITCH__` in the `dsda_config[]` array:
  - `screen_resolution`: `"1280x720"` (upstream: `"640x480"`)
  - `use_fullscreen`: `1` (upstream: `0`)
  - `render_vsync`: `1` (upstream: `0`)
  - `use_game_controller`: `1` (upstream: `0`)
  - `mus_fluidsynth_gain`: `100` (upstream: `50`)
  - `mus_opl_gain`: `100` (upstream: `50`)

### `prboom2/src/dsda/configuration.h`
- Exports `dsda_HackStringConfig()` — sets and persists a string config value without firing its `onUpdate` callback. Used by `I_SwitchDetectSoundfont()` to default `snd_midiplayer` before the WAD system is ready; a full `dsda_UpdateStringConfig` call would fire `M_ChangeMIDIPlayer`, which calls `S_RestartMusic` → `W_LumpByNum` before WADs are loaded, causing a crash.

### `prboom2/src/SDL/i_main.c`
- Calls `I_SwitchInit()` at the very start of `main()`.
- Calls `I_SwitchDetectSoundfont()` immediately after `M_LoadDefaults()`; calls `M_SaveDefaults()` only if it returns `1` (config was changed)

### `prboom2/src/SDL/i_system.c`
- `I_GetXDGDataHome()`, `I_GetXDGDataDirs()`, and `I_ConfigDir()` all return `SWITCH_DATA_DIR` (`"sdmc:/switch/nyan-doom"`) on Switch so configs, saves, and asset lookups are rooted on the SD card.

### `prboom2/src/SDL/i_video.c`
- Forces `desired_screenwidth = 1280` and `desired_screenheight = 720` on Switch after parsing the config, so the render resolution is always the native panel size regardless of the saved `screen_resolution` value.
- The software renderer is enforced at build time (GL sources compiled out, `gl_stub.c` linked in) rather than at runtime.

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
- `M_ItemDisabled()`: returns `true` on Switch for settings:
  - **Video mode**, **Screen Resolution**, **Fullscreen Video mode**, **Exclusive Fullscreen** — forced to fixed values at startup (1280×720, software renderer, fullscreen).
  - **Mute When Out of Focus** — Switch apps are suspended when not in focus
  - **Enable Gamepad** — the Joy-Con is always active on Switch (forced in `game_controller.c`)
- `M_ClearMenus()`: on Switch, calls `M_SaveDefaults()` when the menu being closed is within the Options (Settings) hierarchy — detected by walking the `prevMenu` chain to find `&OptionsDef`.
- **Software keyboard (numeric fields):** On Switch, pressing confirm on an `S_NUM`/`S_PERC` menu item (e.g. deadzone values) calls `I_SwitchGetNumericInput()` instead of entering the keyboard-gather loop. The system numpad overlay appears, and the confirmed value is committed directly.
- **Software keyboard (string fields):** Same for `S_STRING` items (e.g. player name) — calls `I_SwitchGetStringInput()` and commits the result directly, bypassing the character-by-character editing buffer.

### `prboom2/src/SDL/i_sound.c`
- `midiplayers[]`: null-terminated after `"opl"` on Switch so PortMIDI does not appear in the Preferred MIDI Player menu (PortMIDI is not available on Switch).
- `M_ChangeMIDIPlayer()`: portmidi selection branch guarded with `#ifndef __SWITCH__` to prevent a null-pointer dereference if an old config file has `snd_midiplayer portmidi`.

### `prboom2/src/s_sound.c`
- `S_ChangeMusInfoMusic()`: early-return guard tightened from `if (music->lumpnum == lumpnum)` to `if (music->lumpnum == lumpnum && mus_playing)`. After `S_StopMusic()`, `mus_playing` is `NULL` but the lumpnum field retains its old value, so the original guard fired and silently skipped the restart. This caused silence after switching MIDI players on any map using MUSINFO/MAPINFO music.

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
- In `TXT_GetChar()`, upstream's `SDL_CONTROLLERBUTTONDOWN` handler (A=confirm, B=cancel, DPad=arrows) covers Switch correctly since SDL_CONTROLLER_BUTTON_A maps to the physical south/bottom button — the Nintendo confirm button (B). No Switch-specific changes needed.
