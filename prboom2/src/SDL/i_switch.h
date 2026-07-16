/* Emacs style mode select   -*- C -*-
 *-----------------------------------------------------------------------------
 *
 *  Nyan Doom - Nintendo Switch (libnx) platform glue.
 *
 *  This file is only compiled when targeting the Switch (__SWITCH__). All
 *  Switch-specific initialisation lives here so the rest of the engine stays
 *  free of console ifdefs and remains easy to merge with upstream nyan-doom.
 *
 *-----------------------------------------------------------------------------
 */

#ifndef I_SWITCH_H
#define I_SWITCH_H

// Base writable directory on the SD card for config, saves and WADs.
#define SWITCH_DATA_DIR "sdmc:/switch/nyan-doom"

// Initialise libnx subsystems (sockets, working dir). Call early in main.
void I_SwitchInit(void);

// Tear down libnx subsystems. Registered as an exit handler.
void I_SwitchShutdown(void);

// Pop up the system software keyboard and return the result in buf (size buf_size).
// For numeric settings: shows a numpad prefilled with current_value; returns 1 on confirm.
// For string settings: shows the full keyboard prefilled with current; returns 1 on confirm.
// Both return 0 on cancel or failure.
int I_SwitchGetNumericInput(int current_value, char *buf, size_t buf_size);
int I_SwitchGetStringInput(const char *current, char *buf, size_t buf_size);

#endif // I_SWITCH_H
