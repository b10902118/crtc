#include "egl.hpp"
#include "log.hpp"
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <vector>

#ifndef EGL_PLATFORM_SURFACELESS_MESA
#define EGL_PLATFORM_SURFACELESS_MESA 0x31DD
#endif

typedef EGLDisplay(EGLAPIENTRYP PFNEGLGETPLATFORMDISPLAYEXTPROC)(
    EGLenum platform, void *native_display, const EGLint *attrib_list);

static EGLDisplay egl_display = EGL_NO_DISPLAY;
static EGLContext egl_context = EGL_NO_CONTEXT;
static EGLSurface egl_surface = EGL_NO_SURFACE;

bool init_headless_egl(int width, int height) {
  LOGD("Initializing headless EGL display...");
  PFNEGLGETPLATFORMDISPLAYEXTPROC eglGetPlatformDisplayEXT =
      (PFNEGLGETPLATFORMDISPLAYEXTPROC)eglGetProcAddress(
          "eglGetPlatformDisplayEXT");
  if (eglGetPlatformDisplayEXT) {
    egl_display = eglGetPlatformDisplayEXT(EGL_PLATFORM_SURFACELESS_MESA,
                                           EGL_DEFAULT_DISPLAY, NULL);
  } else {
    egl_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
  }
  if (egl_display == EGL_NO_DISPLAY) {
    LOGE("Failed to get EGL display. Error: ", eglGetError());
    return false;
  }

  EGLint major, minor;
  if (!eglInitialize(egl_display, &major, &minor)) {
    LOGE("Failed to initialize EGL. Error: ", eglGetError());
    return false;
  }
  LOGD("EGL initialized successfully. Version: ", major, ".", minor);

  EGLint config_attribs[] = {EGL_SURFACE_TYPE,
                             EGL_PBUFFER_BIT,
                             EGL_BLUE_SIZE,
                             8,
                             EGL_GREEN_SIZE,
                             8,
                             EGL_RED_SIZE,
                             8,
                             EGL_DEPTH_SIZE,
                             16,
                             EGL_RENDERABLE_TYPE,
                             EGL_OPENGL_ES2_BIT,
                             EGL_NONE};

  EGLConfig config;
  EGLint num_configs;
  if (!eglChooseConfig(egl_display, config_attribs, &config, 1, &num_configs) ||
      num_configs == 0) {
    LOGE("Failed to choose EGL config. Error: ", eglGetError());
    return false;
  }

  EGLint pbuffer_attribs[] = {EGL_WIDTH, width, EGL_HEIGHT, height, EGL_NONE};
  egl_surface = eglCreatePbufferSurface(egl_display, config, pbuffer_attribs);
  if (egl_surface == EGL_NO_SURFACE) {
    LOGE("Failed to create EGL Pbuffer surface. Error: ", eglGetError());
    return false;
  }

  EGLint context_attribs[] = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE};
  egl_context =
      eglCreateContext(egl_display, config, EGL_NO_CONTEXT, context_attribs);
  if (egl_context == EGL_NO_CONTEXT) {
    LOGE("Failed to create EGL context. Error: ", eglGetError());
    return false;
  }

  if (!eglMakeCurrent(egl_display, egl_surface, egl_surface, egl_context)) {
    LOGE("Failed to make EGL context current. Error: ", eglGetError());
    return false;
  }

  LOGD("Headless EGL context successfully created and bound to thread.");
  return true;
}

void cleanup_egl() {
  LOGD("Cleaning up EGL context...");
  if (egl_display != EGL_NO_DISPLAY) {
    eglMakeCurrent(egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    if (egl_context != EGL_NO_CONTEXT) {
      eglDestroyContext(egl_display, egl_context);
    }
    if (egl_surface != EGL_NO_SURFACE) {
      eglDestroySurface(egl_display, egl_surface);
    }
    eglTerminate(egl_display);
  }
  egl_display = EGL_NO_DISPLAY;
  egl_context = EGL_NO_CONTEXT;
  egl_surface = EGL_NO_SURFACE;
  LOGD("EGL cleanup done.");
}

std::vector<unsigned char> get_screenshot_pixels_rgb(int width, int height) {
  LOGD("Capturing framebuffer pixels (", width, "x", height, ")...");
  std::vector<unsigned char> pixels(width * height * 4);
  glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());

  std::vector<unsigned char> rgb_pixels;
  rgb_pixels.reserve(width * height * 3);
  // Convert RGBA to RGB and flip vertically (OpenGL bottom-left to standard
  // top-left)
  for (int y = height - 1; y >= 0; --y) {
    for (int x = 0; x < width; ++x) {
      int idx = (y * width + x) * 4;
      rgb_pixels.push_back(pixels[idx]);     // R
      rgb_pixels.push_back(pixels[idx + 1]); // G
      rgb_pixels.push_back(pixels[idx + 2]); // B
    }
  }
  return rgb_pixels;
}