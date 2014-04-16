// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYSTEM_H_
#define SYSTEM_H_

#include <bx/bx.h>
#include <bx/spscqueue.h>

// The main message pump is on the initial thread.
// It kicks off the "real" main thread.
// After kicking off the main thread, it blocks in a WaitMessage/Dispatch loop.
// The WndProc on the initial thread puts Events onto a SpScQueue.
// The main thread initializes the graphics stack, popping events off the
// queue, and kicking draws.

struct MouseButton {
  enum Enum {
    None,
    Left,
    Middle,
    Right,
    Count
  };
};

struct Modifier {
  enum Enum {
    None = 0,
    LeftAlt = 0x01,
    RightAlt = 0x02,
    LeftCtrl = 0x04,
    RightCtrl = 0x08,
    LeftShift = 0x10,
    RightShift = 0x20,
    LeftMeta = 0x40,
    RightMeta = 0x80,
  };
};

struct Key {
  enum Enum {
    None = 0,
    Esc,
    Return,
    Tab,
    Space,
    Backspace,
    Up,
    Down,
    Left,
    Right,
    PageUp,
    PageDown,
    Home,
    End,
    Print,
    Plus,
    Minus,
    F1,
    F2,
    F3,
    F4,
    F5,
    F6,
    F7,
    F8,
    F9,
    F10,
    F11,
    F12,
    NumPad0,
    NumPad1,
    NumPad2,
    NumPad3,
    NumPad4,
    NumPad5,
    NumPad6,
    NumPad7,
    NumPad8,
    NumPad9,
    Key0,
    Key1,
    Key2,
    Key3,
    Key4,
    Key5,
    Key6,
    Key7,
    Key8,
    Key9,
    KeyA,
    KeyB,
    KeyC,
    KeyD,
    KeyE,
    KeyF,
    KeyG,
    KeyH,
    KeyI,
    KeyJ,
    KeyK,
    KeyL,
    KeyM,
    KeyN,
    KeyO,
    KeyP,
    KeyQ,
    KeyR,
    KeyS,
    KeyT,
    KeyU,
    KeyV,
    KeyW,
    KeyX,
    KeyY,
    KeyZ,
  };
};

struct Event {
  enum Enum {
    Exit,
    Key,
    Mouse,
    Size,
  };

  Event::Enum type;
};

struct KeyEvent : public Event {
  Key::Enum key;
  int modifiers;
  bool down;
};

struct MouseEvent : public Event {
  int mx;
  int my;
  int wheel;
  MouseButton::Enum button;
  bool down;
  bool move;
};

struct SizeEvent : public Event {
  uint32_t width;
  uint32_t height;
};

class EventQueue {
 public:
  // Post* are only to be called from the initial thread that's pulling off the
  // system message loop. The consumer is the main thread, via Poll.

  void PostExitEvent() {
    Event* e = new Event;
    e->type = Event::Exit;
    queue_.push(e);
  }

  void PostKeyEvent(Key::Enum key, int modifiers, bool down) {
    KeyEvent* e = new KeyEvent;
    e->type = Event::Key;
    e->key = key;
    e->modifiers = modifiers;
    e->down = down;
    queue_.push(e);
  }

  void PostMouseEventMove(int mx, int my) {
    MouseEvent* e = new MouseEvent;
    e->type = Event::Mouse;
    e->mx = mx;
    e->my = my;
    e->wheel = 0;
    e->button = MouseButton::None;
    e->down = false;
    queue_.push(e);
  }

  void PostMouseEventWheel(int mx, int my, int wheel) {
    MouseEvent* e = new MouseEvent;
    e->type = Event::Mouse;
    e->mx = mx;
    e->my = my;
    e->wheel = wheel;
    e->button = MouseButton::None;
    e->down = false;
    queue_.push(e);
  }

  void PostMouseEventButton(int mx, int my, MouseButton::Enum button, bool down) {
    MouseEvent* e = new MouseEvent;
    e->type = Event::Mouse;
    e->mx = mx;
    e->my = my;
    e->wheel = 0;
    e->button = button;
    e->down = down;
    queue_.push(e);
  }

  void PostSizeEvent(uint32_t width, uint32_t height) {
    SizeEvent* e = new SizeEvent;
    e->type = Event::Size;
    e->width = width;
    e->height = height;
    queue_.push(e);
  }
  
  const Event* Poll() {
    return queue_.pop();
  }

  void Release(const Event* event) const {
    delete event;
  }

 private:
  bx::SpScUnboundedQueue<Event> queue_;
};

const Event* Poll();
void Release(const Event* event);

#endif  // SYSTEM_H_
