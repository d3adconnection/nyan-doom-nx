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

void I_SwitchApplyAudioDefaults(void)
{
  const char *sf;
  const char *player;
  DIR *d;
  struct dirent *ent;
  char found_path[512];
  int found = 0;

  // Boost FluidSynth gain on Switch if still at the upstream default (50 → gain=0.5).
  // The Switch audio pipeline is quieter; 100 → gain=1.0 gives better volume at max.
  // Only applies on first run; user changes in the menu are saved and take precedence.
  if (dsda_IntConfig(dsda_config_mus_fluidsynth_gain) == 50)
    dsda_UpdateIntConfig(dsda_config_mus_fluidsynth_gain, 100, true);

  // Only auto-set snd_soundfont when the user hasn't configured it.
  sf = dsda_StringConfig(dsda_config_snd_soundfont);
  if (sf && sf[0])
    return;

  // Scan SWITCH_DATA_DIR for the first .sf2 soundfont file (any filename).
  // Use "." since I_SwitchInit already chdired to SWITCH_DATA_DIR.
  d = opendir(".");
  if (!d)
    return;

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
    return;

  // Persist so subsequent launches skip auto-detection (user can override in cfg).
  dsda_UpdateStringConfig(dsda_config_snd_soundfont, found_path, true);
  lprintf(LO_INFO, "I_SwitchApplyAudioDefaults: auto-detected soundfont %s\n", found_path);

  // Switch to FluidSynth player only if the user hasn't explicitly chosen one.
  player = dsda_StringConfig(dsda_config_snd_midiplayer);
  if (!player || !player[0])
    dsda_UpdateStringConfig(dsda_config_snd_midiplayer, "fluidsynth", true);
}

#endif // __SWITCH__
