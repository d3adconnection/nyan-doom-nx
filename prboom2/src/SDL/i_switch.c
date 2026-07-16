/* Emacs style mode select   -*- C -*-
 *-----------------------------------------------------------------------------
 *
 *  Nyan Doom - Nintendo Switch (libnx) platform glue.
 *
 *  Implements Switch-specific startup/shutdown: sockets and working-directory
 *  setup so that config files, savegames and WADs land in SWITCH_DATA_DIR.
 *
 *-----------------------------------------------------------------------------
 */

#ifdef __SWITCH__

#include <switch.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "SDL.h"

#include "i_switch.h"
#include "lprintf.h"

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
