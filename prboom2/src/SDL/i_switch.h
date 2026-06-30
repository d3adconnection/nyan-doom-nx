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

// After M_LoadDefaults: auto-detect soundfont.sf2 and set audio config if not already configured.
void I_SwitchApplyAudioDefaults(void);

#endif // I_SWITCH_H
