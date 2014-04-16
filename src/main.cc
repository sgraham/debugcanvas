#include "common.h"

#include <bgfx.h>
#include <bx/timer.h>
#include "fpumath.h"

#include "font/font_manager.h"
#include "font/text_metrics.h"
#include "font/text_buffer_manager.h"

#include <stdio.h>
#include <string.h>

#include "system.h"

long int fsize(FILE* _file) {
  long int pos = ftell(_file);
  fseek(_file, 0L, SEEK_END);
  long int size = ftell(_file);
  fseek(_file, pos, SEEK_SET);
  return size;
}

static char* loadText(const char* _filePath) {
  FILE* file = fopen(_filePath, "rb");
  if (NULL != file) {
    uint32_t size = (uint32_t)fsize(file);
    char* mem = (char*)malloc(size + 1);
    size_t ignore = fread(mem, 1, size, file);
    BX_UNUSED(ignore);
    fclose(file);
    mem[size - 1] = '\0';
    return mem;
  }

  return NULL;
}

TrueTypeHandle loadTtf(FontManager* _fm, const char* _filePath) {
  FILE* file = fopen(_filePath, "rb");
  if (NULL != file) {
    uint32_t size = (uint32_t)fsize(file);
    uint8_t* mem = (uint8_t*)malloc(size + 1);
    size_t ignore = fread(mem, 1, size, file);
    BX_UNUSED(ignore);
    fclose(file);
    mem[size - 1] = '\0';
    TrueTypeHandle handle = _fm->createTtf(mem, size);
    free(mem);
    return handle;
  }

  TrueTypeHandle invalid = BGFX_INVALID_HANDLE;
  return invalid;
}

int _main_(int /*_argc*/, char** /*_argv*/) {
  uint32_t width = 1280;
  uint32_t height = 720;
  uint32_t debug = BGFX_DEBUG_TEXT;
  uint32_t reset = BGFX_RESET_VSYNC;

  bgfx::init();

  bgfx::reset(width, height, reset);

  // Enable debug text.
  bgfx::setDebug(debug);

  // Set view 0 clear state.
  bgfx::setViewClear(
      0, BGFX_CLEAR_COLOR_BIT | BGFX_CLEAR_DEPTH_BIT, 0x002b36ff, 1.0f, 0);

  char* bigText = loadText("src/main.cc");

  // Init the text rendering system.
  FontManager* fontManager = new FontManager(512);
  TextBufferManager* textBufferManager = new TextBufferManager(fontManager);

  TrueTypeHandle font = loadTtf(fontManager, "art/VeraMono.ttf");
  //TrueTypeHandle font = loadTtf(fontManager, "art/consola.ttf");
  //TrueTypeHandle font = loadTtf(fontManager, "art/Inconsolata.otf");

  // Create a distance field font.
  FontHandle fontSdf = fontManager->createFontByPixelSize(
      font, 0, 48, FONT_TYPE_DISTANCE_SUBPIXEL);

  // Create a scaled down version of the same font (without adding anything to
  // the atlas).
  FontHandle fontScaled = fontManager->createScaledFontToPixelSize(fontSdf, 13);

  TextLineMetrics metrics(fontManager->getFontInfo(fontScaled));
  // uint32_t lineCount = metrics.getLineCount(bigText);

  float visibleLineCount = 160.0f;

  const char* textBegin = 0;
  const char* textEnd = 0;
  metrics.getSubText(
      bigText, 0, (uint32_t)visibleLineCount, textBegin, textEnd);

  TextBufferHandle scrollableBuffer = textBufferManager->createTextBuffer(
      FONT_TYPE_DISTANCE_SUBPIXEL, BufferType::Transient);
  textBufferManager->setTextColor(scrollableBuffer, 0x839496ff);

  textBufferManager->appendText(
      scrollableBuffer, fontScaled, textBegin, textEnd);

  entry::MouseState mouseState;
  float textScroll = 0.0f;
  float textRotation = 0.0f;
  float textScale = 1.0f;

  bgfx::setDebug(BGFX_DEBUG_STATS | BGFX_DEBUG_TEXT);

  while (!entry::processEvents(width, height, debug, reset, &mouseState)) {
    bool recomputeVisibleText = false;

    // On scroll...

    if (recomputeVisibleText) {
      textBufferManager->clearTextBuffer(scrollableBuffer);
      metrics.getSubText(bigText,
                         (uint32_t)textScroll,
                         (uint32_t)(textScroll + visibleLineCount),
                         textBegin,
                         textEnd);
      textBufferManager->appendText(
          scrollableBuffer, fontScaled, textBegin, textEnd);
    }

    // Set view 0 default viewport.
    bgfx::setViewRect(0, 0, 0, width, height);

    // This dummy draw call is here to make sure that view 0 is cleared
    // if no other draw calls are submitted to view 0.
    bgfx::submit(0);

    /*
    int64_t now = bx::getHPCounter();
    static int64_t last = now;
    const int64_t frameTime = now - last;
    last = now;
    const double freq = double(bx::getHPFrequency());
    const double toMs = 1000.0 / freq;

    bgfx::dbgTextClear();
    bgfx::dbgTextPrintf(
        0, 0, 0x0f, "Frame: % 7.3f[ms]", double(frameTime) * toMs);
        */

    float at[3] = {0, 0, 0.0f};
    float eye[3] = {0, 0, -1.0f};

    float view[16];
    float proj[16];
    mtxLookAt(view, eye, at);
    float centering = 0.5f;

    // Setup a top-left ortho matrix for screen space drawing.
    mtxOrtho(proj,
             centering,
             width + centering,
             height + centering,
             centering,
             -1.0f,
             1.0f);

    // Set view and projection matrix for view 0.
    bgfx::setViewTransform(0, view, proj);

    // very crude approximation :(
    float textAreaWidth =
        0.5f * 66.0f * fontManager->getFontInfo(fontScaled).maxAdvanceWidth;

    float textRotMat[16];
    float textCenterMat[16];
    float textScaleMat[16];
    float screenCenterMat[16];

    mtxRotateZ(textRotMat, textRotation);
    mtxTranslate(textCenterMat,
                 -(textAreaWidth * 0.5f),
                 (-visibleLineCount) * metrics.getLineHeight() * 0.5f,
                 0);
    mtxScale(textScaleMat, textScale, textScale, 1.0f);
    mtxTranslate(screenCenterMat, ((width) * 0.5f), ((height) * 0.5f), 0);

    // first translate to text center, then scale, then rotate
    float tmpMat[16];
    mtxMul(tmpMat, textCenterMat, textRotMat);

    float tmpMat2[16];
    mtxMul(tmpMat2, tmpMat, textScaleMat);

    float tmpMat3[16];
    mtxMul(tmpMat3, tmpMat2, screenCenterMat);

    // Set model matrix for rendering.
    bgfx::setTransform(tmpMat3);

    // Draw your text.
    textBufferManager->submitTextBuffer(scrollableBuffer, 0);

    // Advance to next frame. Rendering thread will be kicked to
    // process submitted rendering primitives.
    bgfx::frame();
  }

  free(bigText);

  fontManager->destroyTtf(font);
  // Destroy the fonts.
  fontManager->destroyFont(fontSdf);
  fontManager->destroyFont(fontScaled);

  textBufferManager->destroyTextBuffer(scrollableBuffer);

  delete textBufferManager;
  delete fontManager;

  // Shutdown bgfx.
  bgfx::shutdown();

  return 0;
}
