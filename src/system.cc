// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <bx/bx.h>
#include <bgfx.h>
#include "system.h"

static uint32_t s_debug = BGFX_DEBUG_NONE;
static uint32_t s_reset = BGFX_RESET_NONE;
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
          if (key->key == Key::KeyV) {
            s_reset &= ~BGFX_RESET_VSYNC;
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
