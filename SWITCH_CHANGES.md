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
- Guards `fl_add_sfloader()` with `#ifndef __SWITCH__` at its definition and all call sites — FluidLite does not implement `fluid_sfloader_set_callbacks`. Three guard sites: the function definition itself, the call before the soundfont block in `fl_init`, and the SNDFONT lump fallback in `fl_init` (replaced with a warning directing users to place an `.sf2` in `sdmc:/switch/nyan-doom/soundfonts/`).
- Guards the SNDFONT fallback branch in `fl_reload_soundfont` — returns `0` on Switch since WAD-embedded soundfont loading is not supported.
- Guards `fluid_synth_all_notes_off`/`fluid_synth_all_sounds_off` loop in `fl_reload_soundfont` — not present in FluidLite's API.
- Casts `fl_null_logger` to `fluid_log_function_t` to suppress a function-pointer type mismatch warning introduced by FluidLite's differing log callback signature. Applied unconditionally (not Switch-only) since the cast is valid for both libraries.
- Defines `FLUID_OK`/`FLUID_FAILED` and maps `FLUIDLITE_VERSION_*` → `FLUIDSYNTH_VERSION_*` as compat shims.
- Nulls `f_syn`/`f_set` in two `fl_init` failure paths where upstream omits these assignments, preventing a potential double-free on re-init.

### `prboom2/src/dsda/configuration.c`
- Switch-specific compile-time defaults via `#ifdef __SWITCH__` in the `dsda_config[]` array:
  - `screen_resolution`: `"1280x720"` (upstream: `"640x480"`)
  - `use_fullscreen`: `1` (upstream: `0`)
  - `render_vsync`: `1` (upstream: `0`)
  - `use_game_controller`: `1` (upstream: `0`)

### `prboom2/src/SDL/i_main.c`
- Calls `I_SwitchInit()` at the very start of `main()`.

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
- `I_ReloadSoundfont()`: adds an `#ifdef __SWITCH__` else-branch that re-initialises `fl_player` when it previously failed to start. On other platforms `fl_player` always initialises via the SNDFONT lump in `nyan-doom.wad`; on Switch that lump path is blocked (FluidLite limitation), so `fl_player` fails at startup when no file soundfont is configured. Without this branch the soundfont selection in the Options menu silently reverts to "Internal".

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
