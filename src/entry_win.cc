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

extern int RealMain(int _argc, char** _argv);

namespace {

struct KeyModifiersMapping {
  int vk;
  Modifier::Enum modifier;
};

const KeyModifiersMapping s_translate_key_modifiers[8] = {
    {VK_LMENU, Modifier::LeftAlt},
    {VK_RMENU, Modifier::RightAlt},
    {VK_LCONTROL, Modifier::LeftCtrl},
    {VK_RCONTROL, Modifier::RightCtrl},
    {VK_LSHIFT, Modifier::LeftShift},
    {VK_RSHIFT, Modifier::RightShift},
    {VK_LWIN, Modifier::LeftMeta},
    {VK_RWIN, Modifier::RightMeta},
};

uint8_t TranslateKeyModifiers() {
  uint8_t modifiers = 0;
  for (uint32_t i = 0; i < BX_COUNTOF(s_translate_key_modifiers); ++i) {
    const KeyModifiersMapping& kmm = s_translate_key_modifiers[i];
    modifiers |= ::GetKeyState(kmm.vk) < 0 ? kmm.modifier : Modifier::None;
  }
  return modifiers;
}

uint8_t s_translate_key[256];

Key::Enum TranslateKey(WPARAM wparam) {
  return (Key::Enum)s_translate_key[wparam & 0xff];
}

struct MainThreadEntry {
  int argc;
  char** argv;

  static int32_t ThreadFunc(void* user_data);
};

struct Context {
  Context() : initialized_(false), should_exit_(false) {
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

    hwnd_ = ::CreateWindowA("seaborgium",
                            "Seaborgium",
                            WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                            0,
                            0,
                            1024,
                            768,
                            NULL,
                            NULL,
                            instance,
                            0);

    bgfx::winSetHwnd(hwnd_);

    MainThreadEntry mte;
    mte.argc = argc;
    mte.argv = argv;

    bx::Thread thread;
    thread.init(mte.ThreadFunc, &mte);
    initialized_ = true;

    ShowWindow(hwnd_, SW_SHOWMAXIMIZED);

    MSG msg;
    msg.message = WM_NULL;

    while (!should_exit_) {
      WaitMessage();

      while (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE) != 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
      }
    }

    thread.shutdown();

    DestroyWindow(hwnd_);

    return 0;
  }

  LRESULT Process(HWND _hwnd, UINT _id, WPARAM _wparam, LPARAM _lparam) {
    if (initialized_) {
      switch (_id) {
        case WM_DESTROY:
          break;

        case WM_QUIT:
        case WM_CLOSE:
          should_exit_ = true;
          event_queue_.PostExitEvent();
          break;

        case WM_SIZE: {
          uint32_t width = GET_X_LPARAM(_lparam);
          uint32_t height = GET_Y_LPARAM(_lparam);
          event_queue_.PostSizeEvent(width, height);
        } break;

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
            // when previous key state bit is set to 1 and transition state
            // bit is set to 1.
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

  static LRESULT CALLBACK
      WndProc(HWND _hwnd, UINT _id, WPARAM _wparam, LPARAM _lparam);

  EventQueue event_queue_;
  HWND hwnd_;
  bool initialized_;
  bool should_exit_;
};

Context g_context;

LRESULT CALLBACK
Context::WndProc(HWND _hwnd, UINT _id, WPARAM _wparam, LPARAM _lparam) {
  return g_context.Process(_hwnd, _id, _wparam, _lparam);
}

int32_t MainThreadEntry::ThreadFunc(void* user_data) {
  MainThreadEntry* self = static_cast<MainThreadEntry*>(user_data);
  int32_t result = RealMain(self->argc, self->argv);
  PostMessage(g_context.hwnd_, WM_QUIT, 0, 0);
  return result;
}

}  // namespace

const Event* Poll() { return g_context.event_queue_.Poll(); }

void Release(const Event* event) { g_context.event_queue_.Release(event); }

int main(int argc, char** argv) {
  return g_context.Run(argc, argv);
}

#endif  // BX_PLATFORM_WINDOWS
