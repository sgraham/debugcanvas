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

float Slide(float to, float current, float rate = 0.08) {
  float remaining = (to - current);
  current = current + remaining * rate;
  return current;
}

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

static uint32_t s_debug = BGFX_DEBUG_NONE;
static uint32_t s_reset = BGFX_RESET_NONE;
static float s_scale_target = 1.f;
static float s_scale = 1.f;
static float s_text_scroll = 0;
static bool s_exit = false;

bool ProcessEvents(uint32_t& _width,
                   uint32_t& _height,
                   uint32_t& _debug,
                   uint32_t& _reset) {
  s_debug = _debug;
  s_reset = _reset;

  const Event* ev;
  do {
    struct SE {
      const Event* m_ev;
      SE() : m_ev(Poll()) {}
      ~SE() {
        if (NULL != m_ev) {
          Release(m_ev);
        }
      }
    } scopeEvent;
    ev = scopeEvent.m_ev;

    if (NULL != ev) {
      switch (ev->type) {
        case Event::Exit:
          return true;

        case Event::Key: {
          const KeyEvent* key = static_cast<const KeyEvent*>(ev);
          if (key->key == Key::Key0 &&
                     (key->modifiers &
                      (Modifier::LeftCtrl | Modifier::RightCtrl))) {
            s_scale_target = 1.f;
          }
        } break;

        case Event::Mouse: {
          const MouseEvent* mouse = static_cast<const MouseEvent*>(ev);
          if (mouse->modifiers & (Modifier::LeftCtrl | Modifier::RightCtrl)) {
            const float kWheelScaleFactor = 1.08f;
            if (mouse->wheel > 0.f)
              s_scale_target *= kWheelScaleFactor;
            else if (mouse->wheel < 0.f)
              s_scale_target /= kWheelScaleFactor;
          } else {
            s_text_scroll -= mouse->wheel * 3.f;
            if (s_text_scroll < 0.f)
              s_text_scroll = 0;
          }
        } break;

        case Event::Size: {
          const SizeEvent* size = static_cast<const SizeEvent*>(ev);
          _width = size->width;
          _height = size->height;
          _reset = !s_reset;  // force reset
        } break;

        default:
          break;
      }
    }
  } while (NULL != ev);

  if (_reset != s_reset) {
    _reset = s_reset;
    bgfx::reset(_width, _height, _reset);
  }

  _debug = s_debug;

  return s_exit;
}

int RealMain(int /*_argc*/, char** /*_argv*/) {
  uint32_t width = 1280;
  uint32_t height = 720;
  uint32_t debug = BGFX_DEBUG_TEXT;
  uint32_t reset = BGFX_RESET_NONE;

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
  FontHandle fontScaled = fontManager->createScaledFontToPixelSize(fontSdf, 12);

  TextLineMetrics metrics(fontManager->getFontInfo(fontScaled));
  // uint32_t lineCount = metrics.getLineCount(bigText);

  int visibleLineCount = 50;

  const char* textBegin = 0;
  const char* textEnd = 0;
  metrics.getSubText(bigText, 0, visibleLineCount, textBegin, textEnd);

  TextBufferHandle scrollableBuffer = textBufferManager->createTextBuffer(
      FONT_TYPE_DISTANCE_SUBPIXEL, BufferType::Transient);
  textBufferManager->setTextColor(scrollableBuffer, 0x839496ff);
  //textBufferManager->setTextColor(scrollableBuffer, 0xffffffff);

  textBufferManager->appendText(
      scrollableBuffer, fontScaled, textBegin, textEnd);

  float textScroll = 0;

  bgfx::setDebug(BGFX_DEBUG_STATS | BGFX_DEBUG_TEXT);

  while (!ProcessEvents(width, height, debug, reset)) {
    bool recomputeVisibleText = textScroll != s_text_scroll;

    if (recomputeVisibleText) {
      textScroll = s_text_scroll;
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

    s_scale = Slide(s_scale_target, s_scale);
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

    float textCenterMat[16];
    float textScaleMat[16];
    float screenCenterMat[16];

    mtxTranslate(textCenterMat,
                 -(textAreaWidth * 0.5f),
                 (-visibleLineCount) * metrics.getLineHeight() * 0.5f,
                 0);
    mtxScale(textScaleMat, s_scale, s_scale, 1.0f);
    mtxTranslate(screenCenterMat, ((width) * 0.5f), ((height) * 0.5f), 0);

    // first translate to text center, then scale
    float tmpMat2[16];
    mtxMul(tmpMat2, textCenterMat, textScaleMat);

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
