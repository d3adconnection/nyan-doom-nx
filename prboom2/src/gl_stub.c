/* Nyan Doom - Nintendo Switch: stubs for the OpenGL renderer.
 *
 * The Switch (libnx) only exposes OpenGL ES, so the desktop GL renderer is
 * not compiled. The engine is forced to software mode (V_IsOpenGLMode() is
 * always false), but many translation units still reference gld_* symbols at
 * link time inside dead GL branches. This file provides empty stubs so the
 * binary links. None of these are ever called.
 */

#ifdef __SWITCH__

#include <stdarg.h>

typedef unsigned int GLenum;
typedef unsigned int GLuint;

/* --- GL state globals expected by the engine --- */
int gl_colorbuffer_bits = 0;
int gl_depthbuffer_bits = 0;
int gl_drawskys = 0;
int gl_use_stencil = 0;
int SceneInTexture = 0;
GLuint glSceneImageTextureFBOTexID = 0;
void *GLEXT_glBindFramebufferEXT = 0;
void *anim_flats = 0;
void *anim_textures = 0;

#define GL_STUB_V(name)        void name() {}
#define GL_STUB_I(name)        int  name() { return 0; }

/* --- fixed-function GL entry points (not in GLES) --- */
GL_STUB_V(glBegin)        GL_STUB_V(glEnd)          GL_STUB_V(glVertex2f)
GL_STUB_V(glTexCoord2f)   GL_STUB_V(glColor4f)      GL_STUB_V(glMatrixMode)
GL_STUB_V(glLoadIdentity) GL_STUB_V(glOrtho)        GL_STUB_V(glScalef)
GL_STUB_V(glTranslatef)   GL_STUB_V(glClear)        GL_STUB_V(glClearColor)
GL_STUB_V(glEnable)       GL_STUB_V(glDisable)      GL_STUB_V(glViewport)
GL_STUB_V(glScissor)      GL_STUB_V(glBindTexture)  GL_STUB_V(glGenTextures)
GL_STUB_V(glDeleteTextures) GL_STUB_V(glTexImage2D) GL_STUB_V(glTexSubImage2D)
GL_STUB_V(glTexParameteri) GL_STUB_V(glTexEnvi)

/* --- engine GL renderer (gl_*.c excluded on Switch) --- */
GL_STUB_V(gld_Init)            GL_STUB_V(gld_InitCommandLine)
GL_STUB_V(gld_MultisamplingInit) GL_STUB_V(gld_MultisamplingSet)
GL_STUB_V(gld_PreprocessLevel) GL_STUB_V(gld_ProcessTexturedMap)
GL_STUB_V(gld_CleanMemory)     GL_STUB_V(gld_CleanStaticMemory)
GL_STUB_V(gld_Finish)          GL_STUB_V(gld_FlushTextures)
GL_STUB_V(gld_SetPalette)      GL_STUB_V(gld_Set2DMode)
GL_STUB_V(gld_StartDrawScene)  GL_STUB_V(gld_InitDrawScene)
GL_STUB_V(gld_DrawScene)       GL_STUB_V(gld_EndDrawScene)
GL_STUB_V(gld_DrawWeapon)      GL_STUB_V(gld_ProjectSprite)
GL_STUB_V(gld_AddWall)         GL_STUB_V(gld_AddPlane)
GL_STUB_V(gld_FrustumSetup)    GL_STUB_V(gld_ReadScreen)
GL_STUB_V(gld_ResetAutomapTransparency) GL_STUB_V(gld_ResetTexturedAutomap)
GL_STUB_V(gld_UpdateSplitData) GL_STUB_V(gld_DrawNiceThings)
GL_STUB_V(gld_AddNiceThing)    GL_STUB_V(gld_ClearNiceThings)
GL_STUB_V(gld_DrawMapLines)    GL_STUB_V(gld_MapDrawSubsectors)
GL_STUB_V(gld_BeginUIDraw)     GL_STUB_V(gld_EndUIDraw)
GL_STUB_V(gld_BeginMenuDraw)   GL_STUB_V(gld_EndMenuDraw)
GL_STUB_V(gld_BeginAutomapDraw) GL_STUB_V(gld_EndAutomapDraw)
GL_STUB_V(gld_FillBlock)       GL_STUB_V(gld_FillBlockShaded)
GL_STUB_V(gld_FillPatch)       GL_STUB_V(gld_FillRaw)
GL_STUB_V(gld_FillRaw_f)       GL_STUB_V(gld_DrawNumPatch)
GL_STUB_V(gld_DrawNumPatch_f)  GL_STUB_V(gld_DrawPoint)
GL_STUB_V(gld_DrawLine_f)      GL_STUB_V(gld_DrawShaded)
GL_STUB_V(gld_clipper_SafeAddClipRange)
GL_STUB_V(gld_wipe_StartScreen) GL_STUB_V(gld_wipe_EndScreen)
GL_STUB_V(gld_wipe_exitMelt)   GL_STUB_V(gld_wipe_renderMelt)
GL_STUB_I(gld_clipper_SafeCheckRange)
GL_STUB_V(GetBestBleedSector)  GL_STUB_V(GetBestFake)

/* --- POSIX process calls missing from newlib --- */
int execl(const char *path, const char *arg, ...) { (void)path; (void)arg; return -1; }
int pipe(int fds[2]) { (void)fds; return -1; }
int waitpid(int pid, int *status, int options) { (void)pid; (void)status; (void)options; return -1; }

#endif
