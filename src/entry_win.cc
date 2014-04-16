/*
 * Copyright 2011-2014 Branimir Karadzic. All rights reserved.
 * License: http://www.opensource.org/licenses/BSD-2-Clause
 */

#include "system.h"

#if BX_PLATFORM_WINDOWS

#include <bgfxplatform.h>

#include <bx/uint32_t.h>
#include <bx/thread.h>

#include <windowsx.h>

#define ENTRY_DEFAULT_WIDTH 1280
#define ENTRY_DEFAULT_HEIGHT 720

extern int _main_(int _argc, char** _argv);

struct KeyModifiersMapping {
  int vk;
  Modifier::Enum modifier;
};

static const KeyModifiersMapping s_translate_key_modifiers[8] = {
    {VK_LMENU, Modifier::LeftAlt},
    {VK_RMENU, Modifier::RightAlt},
    {VK_LCONTROL, Modifier::LeftCtrl},
    {VK_RCONTROL, Modifier::RightCtrl},
    {VK_LSHIFT, Modifier::LeftShift},
    {VK_RSHIFT, Modifier::RightShift},
    {VK_LWIN, Modifier::LeftMeta},
    {VK_RWIN, Modifier::RightMeta},
};

static uint8_t TranslateKeyModifiers() {
  uint8_t modifiers = 0;
  for (uint32_t i = 0; i < BX_COUNTOF(s_translate_key_modifiers); ++i) {
    const KeyModifiersMapping& kmm = s_translate_key_modifiers[i];
    modifiers |= ::GetKeyState(kmm.vk) < 0 ? kmm.modifier : Modifier::None;
  }
  return modifiers;
}

static uint8_t s_translate_key[256];

static Key::Enum TranslateKey(WPARAM wparam) {
  return (Key::Enum)s_translate_key[wparam & 0xff];
}

struct MainThreadEntry {
  int argc;
  char** argv;

  static int32_t ThreadFunc(void* user_data);
};

struct Context {
  Context() : m_frame(true), m_init(false), m_exit(false) {
    memset(s_translate_key, 0, sizeof(s_translate_key));
    s_translate_key[VK_ESCAPE] = Key::Esc;
    s_translate_key[VK_RETURN] = Key::Return;
    s_translate_key[VK_TAB] = Key::Tab;
    s_translate_key[VK_BACK] = Key::Backspace;
    s_translate_key[VK_SPACE] = Key::Space;
    s_translate_key[VK_UP] = Key::Up;
    s_translate_key[VK_DOWN] = Key::Down;
    s_translate_key[VK_LEFT] = Key::Left;
    s_translate_key[VK_RIGHT] = Key::Right;
    s_translate_key[VK_PRIOR] = Key::PageUp;
    s_translate_key[VK_NEXT] = Key::PageUp;
    s_translate_key[VK_HOME] = Key::Home;
    s_translate_key[VK_END] = Key::End;
    s_translate_key[VK_SNAPSHOT] = Key::Print;
    s_translate_key[VK_OEM_PLUS] = Key::Plus;
    s_translate_key[VK_OEM_MINUS] = Key::Minus;
    s_translate_key[VK_F1] = Key::F1;
    s_translate_key[VK_F2] = Key::F2;
    s_translate_key[VK_F3] = Key::F3;
    s_translate_key[VK_F4] = Key::F4;
    s_translate_key[VK_F5] = Key::F5;
    s_translate_key[VK_F6] = Key::F6;
    s_translate_key[VK_F7] = Key::F7;
    s_translate_key[VK_F8] = Key::F8;
    s_translate_key[VK_F9] = Key::F9;
    s_translate_key[VK_F10] = Key::F10;
    s_translate_key[VK_F11] = Key::F11;
    s_translate_key[VK_F12] = Key::F12;
    s_translate_key[VK_NUMPAD0] = Key::NumPad0;
    s_translate_key[VK_NUMPAD1] = Key::NumPad1;
    s_translate_key[VK_NUMPAD2] = Key::NumPad2;
    s_translate_key[VK_NUMPAD3] = Key::NumPad3;
    s_translate_key[VK_NUMPAD4] = Key::NumPad4;
    s_translate_key[VK_NUMPAD5] = Key::NumPad5;
    s_translate_key[VK_NUMPAD6] = Key::NumPad6;
    s_translate_key[VK_NUMPAD7] = Key::NumPad7;
    s_translate_key[VK_NUMPAD8] = Key::NumPad8;
    s_translate_key[VK_NUMPAD9] = Key::NumPad9;
    s_translate_key['0'] = Key::Key0;
    s_translate_key['1'] = Key::Key1;
    s_translate_key['2'] = Key::Key2;
    s_translate_key['3'] = Key::Key3;
    s_translate_key['4'] = Key::Key4;
    s_translate_key['5'] = Key::Key5;
    s_translate_key['6'] = Key::Key6;
    s_translate_key['7'] = Key::Key7;
    s_translate_key['8'] = Key::Key8;
    s_translate_key['9'] = Key::Key9;
    s_translate_key['A'] = Key::KeyA;
    s_translate_key['B'] = Key::KeyB;
    s_translate_key['C'] = Key::KeyC;
    s_translate_key['D'] = Key::KeyD;
    s_translate_key['E'] = Key::KeyE;
    s_translate_key['F'] = Key::KeyF;
    s_translate_key['G'] = Key::KeyG;
    s_translate_key['H'] = Key::KeyH;
    s_translate_key['I'] = Key::KeyI;
    s_translate_key['J'] = Key::KeyJ;
    s_translate_key['K'] = Key::KeyK;
    s_translate_key['L'] = Key::KeyL;
    s_translate_key['M'] = Key::KeyM;
    s_translate_key['N'] = Key::KeyN;
    s_translate_key['O'] = Key::KeyO;
    s_translate_key['P'] = Key::KeyP;
    s_translate_key['Q'] = Key::KeyQ;
    s_translate_key['R'] = Key::KeyR;
    s_translate_key['S'] = Key::KeyS;
    s_translate_key['T'] = Key::KeyT;
    s_translate_key['U'] = Key::KeyU;
    s_translate_key['V'] = Key::KeyV;
    s_translate_key['W'] = Key::KeyW;
    s_translate_key['X'] = Key::KeyX;
    s_translate_key['Y'] = Key::KeyY;
    s_translate_key['Z'] = Key::KeyZ;
  }

  int32_t Run(int argc, char** argv) {
    HINSTANCE instance = GetModuleHandle(NULL);

    WNDCLASSEX wnd;
    // XXX?
    memset(&wnd, 0, sizeof(wnd));
    wnd.cbSize = sizeof(wnd);
    wnd.lpfnWndProc = ::DefWindowProc;
    wnd.hInstance = instance;
    wnd.hIcon = ::LoadIcon(instance, IDI_APPLICATION);
    wnd.hCursor = ::LoadCursor(instance, IDC_ARROW);
    wnd.hbrBackground = (HBRUSH)::GetStockObject(BLACK_BRUSH);
    wnd.lpszClassName = "seaborgium_letterbox";
    wnd.hIconSm = ::LoadIcon(instance, IDI_APPLICATION);
    ::RegisterClassExA(&wnd);

    memset(&wnd, 0, sizeof(wnd));
    wnd.cbSize = sizeof(wnd);
    wnd.style = CS_HREDRAW | CS_VREDRAW;
    wnd.lpfnWndProc = WndProc;
    wnd.hInstance = instance;
    wnd.hIcon = ::LoadIcon(instance, IDI_APPLICATION);
    wnd.hCursor = ::LoadCursor(instance, IDC_ARROW);
    wnd.lpszClassName = "seaborgium";
    wnd.hIconSm = ::LoadIcon(instance, IDI_APPLICATION);
    ::RegisterClassExA(&wnd);

    HWND hwnd = ::CreateWindowA("seaborgium_letterbox",
                                "Seaborgium",
                                WS_POPUP | WS_SYSMENU,
                                -32000,
                                -32000,
                                0,
                                0,
                                NULL,
                                NULL,
                                instance,
                                0);

    m_hwnd = ::CreateWindowA("seaborgium",
                             "Seaborgium",
                             WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                             0,
                             0,
                             ENTRY_DEFAULT_WIDTH,
                             ENTRY_DEFAULT_HEIGHT,
                             hwnd,
                             NULL,
                             instance,
                             0);

    bgfx::winSetHwnd(m_hwnd);

    Adjust(ENTRY_DEFAULT_WIDTH, ENTRY_DEFAULT_HEIGHT, true);
    m_width = ENTRY_DEFAULT_WIDTH;
    m_height = ENTRY_DEFAULT_HEIGHT;
    m_oldWidth = ENTRY_DEFAULT_WIDTH;
    m_oldHeight = ENTRY_DEFAULT_HEIGHT;

    MainThreadEntry mte;
    mte.argc = argc;
    mte.argv = argv;

    bx::Thread thread;
    thread.init(mte.ThreadFunc, &mte);
    m_init = true;

    event_queue_.PostSizeEvent(m_width, m_height);

    MSG msg;
    msg.message = WM_NULL;

    while (!m_exit) {
      WaitMessage();

      while (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE) != 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
      }
    }

    thread.shutdown();

    DestroyWindow(m_hwnd);
    DestroyWindow(hwnd);

    return 0;
  }

  LRESULT Process(HWND _hwnd, UINT _id, WPARAM _wparam, LPARAM _lparam) {
    if (m_init) {
      switch (_id) {
        case WM_DESTROY:
          break;

        case WM_QUIT:
        case WM_CLOSE:
          m_exit = true;
          event_queue_.PostExitEvent();
          break;

        case WM_SIZE: {
          uint32_t width = GET_X_LPARAM(_lparam);
          uint32_t height = GET_Y_LPARAM(_lparam);

          m_width = width;
          m_height = height;
          event_queue_.PostSizeEvent(m_width, m_height);
        } break;

        case WM_SYSCOMMAND:
          switch (_wparam) {
            case SC_MINIMIZE:
            case SC_RESTORE: {
              HWND parent = GetWindow(_hwnd, GW_OWNER);
              if (parent)
                PostMessage(parent, _id, _wparam, _lparam);
            }
          }
          break;

        case WM_MOUSEMOVE: {
          int32_t mx = GET_X_LPARAM(_lparam);
          int32_t my = GET_Y_LPARAM(_lparam);

          event_queue_.PostMouseEventMove(mx, my);
        } break;

        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_LBUTTONDBLCLK: {
          int32_t mx = GET_X_LPARAM(_lparam);
          int32_t my = GET_Y_LPARAM(_lparam);
          event_queue_.PostMouseEventButton(
              mx, my, MouseButton::Left, _id == WM_LBUTTONDOWN);
        } break;

        case WM_MBUTTONDOWN:
        case WM_MBUTTONUP:
        case WM_MBUTTONDBLCLK: {
          int32_t mx = GET_X_LPARAM(_lparam);
          int32_t my = GET_Y_LPARAM(_lparam);
          event_queue_.PostMouseEventButton(
              mx, my, MouseButton::Middle, _id == WM_MBUTTONDOWN);
        } break;

        case WM_RBUTTONUP:
        case WM_RBUTTONDOWN:
        case WM_RBUTTONDBLCLK: {
          int32_t mx = GET_X_LPARAM(_lparam);
          int32_t my = GET_Y_LPARAM(_lparam);
          event_queue_.PostMouseEventButton(
              mx, my, MouseButton::Right, _id == WM_RBUTTONDOWN);
        } break;

        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
        case WM_KEYUP:
        case WM_SYSKEYUP: {
          uint8_t modifiers = TranslateKeyModifiers();
          Key::Enum key = TranslateKey(_wparam);

          if (key == Key::Print && ((uint32_t)(_lparam) >> 30) == 0x3) {
            // VK_SNAPSHOT doesn't generate keydown event. Fire on down event
            // when previous
            // key state bit is set to 1 and transition state bit is set to 1.
            //
            // http://msdn.microsoft.com/en-us/library/windows/desktop/ms646280%28v=vs.85%29.aspx
            event_queue_.PostKeyEvent(key, modifiers, true);
          }

          event_queue_.PostKeyEvent(
              key, modifiers, _id == WM_KEYDOWN || _id == WM_SYSKEYDOWN);
        } break;

        default:
          break;
      }
    }

    return DefWindowProc(_hwnd, _id, _wparam, _lparam);
  }

  void Adjust(uint32_t _width, uint32_t _height, bool _windowFrame) {
    m_width = _width;
    m_height = _height;
    m_aspectRatio = float(_width) / float(_height);

    ShowWindow(m_hwnd, SW_SHOWNORMAL);
    RECT rect;
    RECT newrect = {0, 0, (LONG)_width, (LONG)_height};
    DWORD style = WS_POPUP | WS_SYSMENU;

    if (m_frame) {
      GetWindowRect(m_hwnd, &m_rect);
      m_style = GetWindowLong(m_hwnd, GWL_STYLE);
    }

    if (_windowFrame) {
      rect = m_rect;
      style = m_style;
    } else {
#if defined(__MINGW32__)
      rect = m_rect;
      style = m_style;
#else
      HMONITOR monitor = MonitorFromWindow(m_hwnd, MONITOR_DEFAULTTONEAREST);
      MONITORINFO mi;
      mi.cbSize = sizeof(mi);
      GetMonitorInfo(monitor, &mi);
      newrect = mi.rcMonitor;
      rect = mi.rcMonitor;
#endif  // !defined(__MINGW__)
    }

    SetWindowLong(m_hwnd, GWL_STYLE, style);
    uint32_t prewidth = newrect.right - newrect.left;
    uint32_t preheight = newrect.bottom - newrect.top;
    AdjustWindowRect(&newrect, style, FALSE);
    m_frameWidth = (newrect.right - newrect.left) - prewidth;
    m_frameHeight = (newrect.bottom - newrect.top) - preheight;
    UpdateWindow(m_hwnd);

    if (rect.left == -32000 || rect.top == -32000) {
      rect.left = 0;
      rect.top = 0;
    }

    int32_t left = rect.left;
    int32_t top = rect.top;
    int32_t width = (newrect.right - newrect.left);
    int32_t height = (newrect.bottom - newrect.top);

    if (!_windowFrame) {
      float aspectRatio = 1.0f / m_aspectRatio;
      width = bx::uint32_max(ENTRY_DEFAULT_WIDTH / 4, width);
      height = uint32_t(float(width) * aspectRatio);

      left = newrect.left + (newrect.right - newrect.left - width) / 2;
      top = newrect.top + (newrect.bottom - newrect.top - height) / 2;
    }

    HWND parent = GetWindow(m_hwnd, GW_OWNER);
    if (parent) {
      if (_windowFrame) {
        SetWindowPos(parent, HWND_TOP, -32000, -32000, 0, 0, SWP_SHOWWINDOW);
      } else {
        SetWindowPos(parent,
                     HWND_TOP,
                     newrect.left,
                     newrect.top,
                     newrect.right - newrect.left,
                     newrect.bottom - newrect.top,
                     SWP_SHOWWINDOW);
      }
    }

    SetWindowPos(m_hwnd, HWND_TOP, left, top, width, height, SWP_SHOWWINDOW);

    ShowWindow(m_hwnd, SW_RESTORE);

    m_frame = _windowFrame;
  }

  void setMousePos(int32_t _mx, int32_t _my) {
    POINT pt = {_mx, _my};
    ClientToScreen(m_hwnd, &pt);
    SetCursorPos(pt.x, pt.y);
  }

  static LRESULT CALLBACK
      WndProc(HWND _hwnd, UINT _id, WPARAM _wparam, LPARAM _lparam);

  EventQueue event_queue_;

  HWND m_hwnd;
  RECT m_rect;
  DWORD m_style;
  uint32_t m_width;
  uint32_t m_height;
  uint32_t m_oldWidth;
  uint32_t m_oldHeight;
  uint32_t m_frameWidth;
  uint32_t m_frameHeight;
  float m_aspectRatio;

  int32_t m_mx;
  int32_t m_my;

  bool m_frame;
  bool m_init;
  bool m_exit;
};

static Context s_ctx;

LRESULT CALLBACK
Context::WndProc(HWND _hwnd, UINT _id, WPARAM _wparam, LPARAM _lparam) {
  return s_ctx.Process(_hwnd, _id, _wparam, _lparam);
}

const Event* Poll() { return s_ctx.event_queue_.Poll(); }

void Release(const Event* event) { s_ctx.event_queue_.Release(event); }

int32_t MainThreadEntry::ThreadFunc(void* user_data) {
  MainThreadEntry* self = static_cast<MainThreadEntry*>(user_data);
  int32_t result = _main_(self->argc, self->argv);
  PostMessage(s_ctx.m_hwnd, WM_QUIT, 0, 0);
  return result;
}

int main(int argc, char** argv) {
  return s_ctx.Run(argc, argv);
}

#endif  // BX_PLATFORM_WINDOWS
