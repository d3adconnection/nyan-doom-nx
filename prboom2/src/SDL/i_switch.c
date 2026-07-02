/* Emacs style mode select   -*- C -*-
 *-----------------------------------------------------------------------------
 *
 *  Nyan Doom - Nintendo Switch (libnx) platform glue.
 *
 *  Implements Switch-specific startup/shutdown: sockets and working-directory
 *  setup so that config files, savegames and WADs land in SWITCH_DATA_DIR.
 *  Soundfonts are auto-detected from the SD card at first launch.
 *
 *-----------------------------------------------------------------------------
 */

#ifdef __SWITCH__

#include <switch.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <dirent.h>
#include <unistd.h>

#include "SDL.h"

#include "i_switch.h"
#include "lprintf.h"
#include "dsda/configuration.h"

static int switch_initialized = 0;

void I_SwitchShutdown(void)
{
  if (!switch_initialized)
    return;

  switch_initialized = 0;

  socketExit();
}

void I_SwitchInit(void)
{
  if (switch_initialized)
    return;

  // Enable BSD sockets so error/debug logging over nxlink works.
  socketInitializeDefault();

  // Make sure the data directory exists and use it as the base for all
  // relative paths (config, savegames, IWAD/PWAD lookup).
  mkdir("sdmc:/switch", 0777);
  mkdir(SWITCH_DATA_DIR, 0777);
  chdir(SWITCH_DATA_DIR);

  switch_initialized = 1;
}

// Scan the data directory for a .sf2 soundfont and configure snd_soundfont /
// snd_midiplayer when snd_soundfont is blank.  Returns 1 if the config was
// changed and should be saved, 0 otherwise.
int I_SwitchDetectSoundfont(void)
{
  const char *sf;
  const char *player;
  DIR *d;
  struct dirent *ent;
  char found_path[512];
  int found = 0;
  int changed = 0;

  sf = dsda_StringConfig(dsda_config_snd_soundfont);

  // If a soundfont path is configured but the file no longer exists, clear it
  // so the scan below can find a replacement (e.g. user swapped the file).
  if (sf && sf[0] && access(sf, F_OK) != 0)
  {
    lprintf(LO_INFO, "I_SwitchDetectSoundfont: %s missing, clearing\n", sf);
    dsda_UpdateStringConfig(dsda_config_snd_soundfont, "", true);
    sf = "";
    changed = 1;
  }

  // Only scan when snd_soundfont is blank.
  if (sf && sf[0])
    return 0;

  // Scan SWITCH_DATA_DIR for the first .sf2 soundfont file (any filename).
  // Use "." since I_SwitchInit already chdired to SWITCH_DATA_DIR.
  d = opendir(".");
  if (!d)
    return changed;

  while ((ent = readdir(d)) != NULL) {
    size_t len = strlen(ent->d_name);
    if (len > 4 && strcasecmp(ent->d_name + len - 4, ".sf2") == 0) {
      snprintf(found_path, sizeof(found_path), SWITCH_DATA_DIR "/%s", ent->d_name);
      found = 1;
      break;
    }
  }
  closedir(d);

  if (!found)
  {
    // No soundfont available; use OPL so MIDI works out of the box.
    // Use dsda_HackStringConfig (no callback) because this runs before WADs
    // are loaded; dsda_UpdateStringConfig would fire M_ChangeMIDIPlayer which
    // calls S_RestartMusic -> W_LumpByNum before the WAD system is ready.
    player = dsda_StringConfig(dsda_config_snd_midiplayer);
    if (!player || !player[0])
    {
      dsda_HackStringConfig(dsda_config_snd_midiplayer, "opl", true);
      changed = 1;
    }
    return changed;
  }

  dsda_UpdateStringConfig(dsda_config_snd_soundfont, found_path, true);
  lprintf(LO_INFO, "I_SwitchDetectSoundfont: auto-detected %s\n", found_path);

  // Soundfont found; enable FluidSynth if the player hasn't been chosen.
  // Same caveat: use HackStringConfig to avoid the premature callback.
  player = dsda_StringConfig(dsda_config_snd_midiplayer);
  if (!player || !player[0])
    dsda_HackStringConfig(dsda_config_snd_midiplayer, "fluidsynth", true);

  return 1;
}

// ---------------------------------------------------------------------------
// Software keyboard helpers
//
// Pop up the system swkbd overlay and return the typed string.  Both
// functions return 1 on confirm, 0 on cancel or failure.
// ---------------------------------------------------------------------------

int I_SwitchGetNumericInput(int current_value, char *buf, size_t buf_size)
{
  SwkbdConfig kbd;
  Result rc;
  char initial[24];

  snprintf(initial, sizeof(initial), "%d", current_value);

  rc = swkbdCreate(&kbd, 0);
  if (R_FAILED(rc))
  {
    lprintf(LO_WARN, "I_SwitchGetNumericInput: swkbdCreate failed (0x%x)\n", rc);
    return 0;
  }

  swkbdConfigMakePresetDefault(&kbd);
  swkbdConfigSetType(&kbd, SwkbdType_NumPad);
  swkbdConfigSetLeftOptionalSymbolKey(&kbd, "-");  // allow negative values
  swkbdConfigSetInitialText(&kbd, initial);
  swkbdConfigSetStringLenMax(&kbd, (int)(buf_size - 1));

  appletLockExit();
  rc = swkbdShow(&kbd, buf, buf_size);
  appletUnlockExit();
  swkbdClose(&kbd);

  // Drain any button events that were queued while the applet was open
  // so they don't feed back into the game's input handling.
  SDL_FlushEvents(SDL_FIRSTEVENT, SDL_LASTEVENT);

  if (R_FAILED(rc))
  {
    lprintf(LO_WARN, "I_SwitchGetNumericInput: swkbdShow failed (0x%x)\n", rc);
    return 0;
  }

  return buf[0] != '\0';
}

int I_SwitchGetStringInput(const char *current, char *buf, size_t buf_size)
{
  SwkbdConfig kbd;
  Result rc;

  rc = swkbdCreate(&kbd, 0);
  if (R_FAILED(rc))
  {
    lprintf(LO_WARN, "I_SwitchGetStringInput: swkbdCreate failed (0x%x)\n", rc);
    return 0;
  }

  swkbdConfigMakePresetDefault(&kbd);
  if (current && current[0])
    swkbdConfigSetInitialText(&kbd, current);
  swkbdConfigSetStringLenMax(&kbd, (int)(buf_size - 1));

  appletLockExit();
  rc = swkbdShow(&kbd, buf, buf_size);
  appletUnlockExit();
  swkbdClose(&kbd);

  SDL_FlushEvents(SDL_FIRSTEVENT, SDL_LASTEVENT);

  if (R_FAILED(rc))
  {
    lprintf(LO_WARN, "I_SwitchGetStringInput: swkbdShow failed (0x%x)\n", rc);
    return 0;
  }

  return 1;
}

#endif // __SWITCH__
