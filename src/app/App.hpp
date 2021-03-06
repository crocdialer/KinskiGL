// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 2012-2016, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

#pragma once

#include <unordered_map>

#include <crocore/filesystem.hpp>
#include <crocore/Component.hpp>
#include <crocore/ThreadPool.hpp>

#include "gl/gl.hpp"

// explicit template instantiation for some vec types
extern template
class crocore::Property_<glm::vec2>;

extern template
class crocore::Property_<glm::vec3>;

extern template
class crocore::Property_<glm::vec4>;

namespace kinski {
DEFINE_CLASS_PTR(App);

DEFINE_CLASS_PTR(Window);

class MouseEvent;

class KeyEvent;

class JoystickState;

struct Touch;

class Window
{
public:
    typedef std::function<void()> DrawFunction;
    typedef std::function<void()> CloseFunction;

    virtual ~Window() { if(m_close_function) m_close_function(); };

    virtual void draw() const = 0;

    virtual gl::ivec2 framebuffer_size() const = 0;

    virtual gl::ivec2 size() const = 0;

    virtual void set_size(const gl::ivec2 &the_sz) = 0;

    virtual gl::ivec2 position() const = 0;

    virtual void set_position(const gl::ivec2 &the_pos) = 0;

    virtual std::string title() const = 0;

    virtual void set_title(const std::string &the_name) = 0;

    virtual uint32_t monitor_index() const = 0;

    void set_draw_function(DrawFunction the_draw_function) { m_draw_function = the_draw_function; }

    void set_close_function(CloseFunction the_close_function) { m_close_function = the_close_function; }

protected:
    DrawFunction m_draw_function;
    CloseFunction m_close_function;
};

class MouseDelegate
{
public:
    virtual void mouse_press(const MouseEvent &e) = 0;

    virtual void mouse_release(const MouseEvent &e) = 0;

    virtual void mouse_move(const MouseEvent &e) = 0;

    virtual void mouse_drag(const MouseEvent &e) = 0;

    virtual void mouse_wheel(const MouseEvent &e) = 0;

    virtual void file_drop(const MouseEvent &e, const std::vector<std::string> &files) = 0;

    virtual const MouseEvent mouse_state() const = 0;
};

class TouchDelegate
{
public:
    virtual void touch_begin(const MouseEvent &e, const std::set<const Touch *> &the_touches) = 0;

    virtual void touch_end(const MouseEvent &e, const std::set<const Touch *> &the_touches) = 0;

    virtual void touch_move(const MouseEvent &e, const std::set<const Touch *> &the_touches) = 0;
};

class KeyDelegate
{
public:
    virtual void key_press(const KeyEvent &e) = 0;

    virtual void key_release(const KeyEvent &e) = 0;
};

class App : public crocore::Component, public MouseDelegate, public KeyDelegate, public TouchDelegate
{
public:

    App(int argc = 0, char *argv[] = nullptr);

    virtual ~App();

    int run();

    // you are supposed to implement these in a subclass
    virtual void setup() = 0;

    virtual void update(float timeDelta) = 0;

    virtual void draw() = 0;

    virtual void post_draw() = 0;

    virtual void teardown() = 0;

    virtual double get_application_time() = 0;

    // these are optional overrides
    virtual void set_window_size(const glm::vec2 &size);

    virtual void set_window_title(const std::string &the_name) {};

    virtual void resize(int w, int h) {};

    void mouse_press(const MouseEvent &e) override {};

    void mouse_release(const MouseEvent &e) override {};

    void mouse_move(const MouseEvent &e) override {};

    void mouse_drag(const MouseEvent &e) override {};

    void mouse_wheel(const MouseEvent &e) override {};

    void file_drop(const MouseEvent &e, const std::vector<std::string> &files) override {};

    void touch_begin(const MouseEvent &e, const std::set<const Touch *> &the_touches) override {};

    void touch_end(const MouseEvent &e, const std::set<const Touch *> &the_touches) override {};

    void touch_move(const MouseEvent &e, const std::set<const Touch *> &the_touches) override {};

    void key_press(const KeyEvent &e) override {};

    void key_release(const KeyEvent &e) override {};

    virtual void add_window(WindowPtr the_window) {};

    virtual std::vector<JoystickState> get_joystick_states() const { return {}; };

    inline bool running() const { return m_running; };

    inline void set_running(bool b) { m_running = b; }

    virtual void set_display_gui(bool b) { m_display_gui = b; };

    inline bool display_gui() const { return m_display_gui; };

    inline float max_fps() const { return m_max_fps; };

    inline void set_max_fps(float fps) { m_max_fps = fps; };

    virtual bool fullscreen() const { return m_fullscreen; };

    virtual void set_fullscreen(bool b, int monitor_index) { m_fullscreen = b; };

    void set_fullscreen(bool b = true) { set_fullscreen(b, 0); };

    virtual bool v_sync() const { return false; };

    virtual void set_v_sync(bool b) {};

    virtual void set_cursor_position(float x, float y) = 0;

    virtual gl::vec2 cursor_position() const = 0;

    virtual bool cursor_visible() const { return m_cursorVisible; };

    virtual void set_cursor_visible(bool b = true) { m_cursorVisible = b; };

    virtual bool needs_redraw() const { return true; };

    /*!
     * return current frames per second
     */
    float fps() const { return m_framesPerSec; };

    /*!
     * the commandline arguments provided at application start
     */
    const std::vector<std::string> &args() const { return m_args; };

    /*!
     * returns true if some loading operation is in progress,
     * meaning the number of active tasks is greater than 0.
     * tasks can be announced and removed with calls to inc_task() and dec_task() respectively
     */
    bool is_loading() const;

    /*!
     * this queue is processed by the main thread
     */
    crocore::ThreadPool &main_queue() { return m_main_queue; }

    const crocore::ThreadPool &main_queue() const { return m_main_queue; }

    /*!
     * the background queue is processed by a background threadpool
     */
    crocore::ThreadPool &background_queue() { return m_background_queue; }

    const crocore::ThreadPool &background_queue() const { return m_background_queue; }

private:

    virtual void init() = 0;

    virtual void poll_events() = 0;

    virtual void swap_buffers() = 0;

    void timing(double timeStamp);

    virtual void draw_internal();

    virtual bool is_running() { return m_running; };

    uint32_t m_framesDrawn;
    double m_lastTimeStamp;
    double m_lastMeasurementTimeStamp;
    float m_framesPerSec;
    double m_timingInterval;

    bool m_running;
    bool m_fullscreen;
    bool m_display_gui;
    bool m_cursorVisible;
    float m_max_fps;

    std::vector<std::string> m_args;

    crocore::ThreadPool m_main_queue, m_background_queue;
};

//! Base class for all Events
class Event
{
protected:
    Event() {}

public:
    virtual ~Event() {}
};

//! Represents a mouse event
class MouseEvent : public Event
{
public:
    MouseEvent() : Event() {}

    MouseEvent(int the_initiator, int the_x, int the_y, unsigned int the_modifiers,
               glm::ivec2 the_wheel_inc, int the_touch_idx = 0, int the_touch_id = 0) :
            Event(),
            m_initiator(the_initiator),
            m_x(the_x),
            m_y(the_y),
            m_modifiers(the_modifiers),
            m_wheel_inc(the_wheel_inc),
            m_touch_index(the_touch_idx),
            m_touch_id(the_touch_id) {}

    //! Returns the X coordinate of the mouse event
    int get_x() const { return m_x; }

    //! Returns the Y coordinate of the mouse event
    int get_y() const { return m_y; }

    //! Returns the coordinates of the mouse event
    glm::ivec2 position() const { return glm::ivec2(m_x, m_y); }

    //! Returns the number of detents the user has wheeled through. Positive values correspond to wheel-up and negative to wheel-down.
    glm::ivec2 wheel_increment() const { return m_wheel_inc; }

    //! Returns whether the initiator for the event was the left mouse button
    bool is_left() const { return m_initiator & LEFT_DOWN; }

    //! Returns whether the initiator for the event was the right mouse button
    bool is_right() const { return m_initiator & RIGHT_DOWN; }

    //! Returns whether the initiator for the event was the middle mouse button
    bool is_middle() const { return m_initiator & MIDDLE_DOWN; }

    //! Returns whether the left mouse button was pressed during the event
    bool is_left_down() const { return m_modifiers & LEFT_DOWN; }

    //! Returns whether the right mouse button was pressed during the event
    bool is_right_down() const { return m_modifiers & RIGHT_DOWN; }

    //! Returns whether the middle mouse button was pressed during the event
    bool is_middle_down() const { return m_modifiers & MIDDLE_DOWN; }

    //! Returns whether the Shift key was pressed during the event.
    bool is_shift_down() const { return m_modifiers & SHIFT_DOWN; }

    //! Returns whether the Alt (or Option) key was pressed during the event.
    bool is_alt_down() const { return m_modifiers & ALT_DOWN; }

    //! Returns whether the Control key was pressed during the event.
    bool is_control_down() const { return m_modifiers & CTRL_DOWN; }

    //! Returns whether the meta key was pressed during the event. Maps to the Windows key on Windows and the Command key on Mac OS X.
    bool is_meta_down() const { return m_modifiers & META_DOWN; }

    //! true if this MouseEvent is generated by a touch-interface
    bool is_touch() const { return m_modifiers & TOUCH_DOWN; }

    //! the current touch id
    int touch_id() const { return m_touch_id; }

    //! the current touch id
    int touch_index() const { return m_touch_index; }

    enum
    {
        LEFT_DOWN = (1 << 0),
        RIGHT_DOWN = (1 << 1),
        MIDDLE_DOWN = (1 << 2),
        SHIFT_DOWN = (1 << 3),
        ALT_DOWN = (1 << 4),
        CTRL_DOWN = (1 << 5),
        META_DOWN = (1 << 6),
        TOUCH_DOWN = (1 << 7)
    };

private:
    int m_initiator = 0;
    int m_x = 0, m_y = 0;
    unsigned int m_modifiers;
    glm::ivec2 m_wheel_inc;
    int m_touch_index = 0;
    int m_touch_id = 0;
};

//! Represents a keyboard event
class KeyEvent : public Event
{
public:
    KeyEvent(int the_code, uint32_t the_char, uint32_t the_modifiers) :
            Event(),
            m_code(the_code),
            m_char(the_char),
            m_modifiers(the_modifiers) {}

    //! Returns the key code associated with the event, which maps into the enum listed below
    int code() const { return m_code; }

    //! Returns the ASCII character associated with the event.
    uint8_t character() const { return m_char; }

    //! Returns whether the Shift key was pressed during the event.
    bool is_shift_down() const { return m_modifiers & SHIFT_DOWN; }

    //! Returns whether the Alt (or Option) key was pressed during the event.
    bool is_alt_down() const { return m_modifiers & ALT_DOWN; }

    //! Returns whether the Control key was pressed during the event.
    bool is_control_down() const { return m_modifiers & CTRL_DOWN; }

    //! Returns whether the meta key was pressed during the event. Maps to the Windows key on Windows and the Command key on Mac OS X.
    bool is_meta_down() const { return m_modifiers & META_DOWN; }

    //! Maps a platform-native key-code to the key code enum
    static int translate_native_keycode(int the_code);

    enum
    {
        SHIFT_DOWN = (1 << 3),
        ALT_DOWN = (1 << 4),
        CTRL_DOWN = (1 << 5),
        META_DOWN = (1 << 6)
    };

private:
    int m_code = 0;
    uint32_t m_char = 0;
    uint32_t m_modifiers = 0;
};

class JoystickState
{
public:
    enum class Mapping : uint32_t
    {
        ANALOG_LEFT_H, ANALOG_LEFT_V, ANALOG_RIGHT_H, ANALOG_RIGHT_V,
        DPAD_H, DPAD_V, TRIGGER_LEFT, TRIGGER_RIGHT,
        BUTTON_A, BUTTON_B, BUTTON_X, BUTTON_Y, BUTTON_MENU, BUTTON_BACK,
        BUTTON_BUMPER_LEFT, BUTTON_BUMPER_RIGHT, BUTTON_STICK_LEFT, BUTTON_STICK_RIGHT
    };

    struct MapHash
    {
        inline size_t operator()(const Mapping &the_enum) const
        {
            std::hash<uint32_t> hasher;
            return hasher(static_cast<uint32_t>(the_enum));
        }
    };

    using ButtonMap = std::unordered_map<Mapping, uint32_t, MapHash>;

    JoystickState(const std::string &n,
                  const std::vector<uint8_t> &b,
                  const std::vector<float> &a);

    const std::string &name() const;

    const std::vector<uint8_t> &buttons() const;

    const std::vector<float> &axis() const;

    const gl::vec2 analog_left() const;

    const gl::vec2 analog_right() const;

    const gl::vec2 trigger() const;

    const gl::vec2 dpad() const;

    static ButtonMap &mapping() { return s_button_map; }

private:
    float m_dead_zone = 0.03f;
    std::string m_name;
    std::vector<uint8_t> m_buttons;
    std::vector<float> m_axis;
    static ButtonMap s_button_map;
};

struct Touch
{
    int32_t m_id = -1;
    uint32_t m_slot_index = 0;
    gl::vec2 m_position;
};

struct Key
{
    enum Type
    {
        /* The unknown key */
                _UNKNOWN = -1,

        /* Printable keys */
                _SPACE = 32,
        _APOSTROPHE = 39,  /* ' */
        _COMMA = 44,  /* , */
        _MINUS = 45,  /* - */
        _PERIOD = 46,  /* . */
        _SLASH = 47,  /* / */
        _0 = 48,
        _1 = 49,
        _2 = 50,
        _3 = 51,
        _4 = 52,
        _5 = 53,
        _6 = 54,
        _7 = 55,
        _8 = 56,
        _9 = 57,
        _SEMICOLON = 59,  /* ; */
        _EQUAL = 61,  /* = */
        _A = 65,
        _B = 66,
        _C = 67,
        _D = 68,
        _E = 69,
        _F = 70,
        _G = 71,
        _H = 72,
        _I = 73,
        _J = 74,
        _K = 75,
        _L = 76,
        _M = 77,
        _N = 78,
        _O = 79,
        _P = 80,
        _Q = 81,
        _R = 82,
        _S = 83,
        _T = 84,
        _U = 85,
        _V = 86,
        _W = 87,
        _X = 88,
        _Y = 89,
        _Z = 90,
        _LEFT_BRACKET = 91,  /* [ */
        _BACKSLASH = 92,  /* \ */
        _RIGHT_BRACKET = 93,  /* ] */
        _GRAVE_ACCENT = 96,  /* ` */
        _WORLD_1 = 161, /* non-US #1 */
        _WORLD_2 = 162, /* non-US #2 */

        /* Function keys */
                _ESCAPE = 256,
        _ENTER = 257,
        _TAB = 258,
        _BACKSPACE = 259,
        _INSERT = 260,
        _DELETE = 261,
        _RIGHT = 262,
        _LEFT = 263,
        _DOWN = 264,
        _UP = 265,
        _PAGE_UP = 266,
        _PAGE_DOWN = 267,
        _HOME = 268,
        _END = 269,
        _CAPS_LOCK = 280,
        _SCROLL_LOCK = 281,
        _NUM_LOCK = 282,
        _PRINT_SCREEN = 283,
        _PAUSE = 284,
        _F1 = 290,
        _F2 = 291,
        _F3 = 292,
        _F4 = 293,
        _F5 = 294,
        _F6 = 295,
        _F7 = 296,
        _F8 = 297,
        _F9 = 298,
        _F10 = 299,
        _F11 = 300,
        _F12 = 301,
        _F13 = 302,
        _F14 = 303,
        _F15 = 304,
        _F16 = 305,
        _F17 = 306,
        _F18 = 307,
        _F19 = 308,
        _F20 = 309,
        _F21 = 310,
        _F22 = 311,
        _F23 = 312,
        _F24 = 313,
        _F25 = 314,
        _KP_0 = 320,
        _KP_1 = 321,
        _KP_2 = 322,
        _KP_3 = 323,
        _KP_4 = 324,
        _KP_5 = 325,
        _KP_6 = 326,
        _KP_7 = 327,
        _KP_8 = 328,
        _KP_9 = 329,
        _KP_DECIMAL = 330,
        _KP_DIVIDE = 331,
        _KP_MULTIPLY = 332,
        _KP_SUBTRACT = 333,
        _KP_ADD = 334,
        _KP_ENTER = 335,
        _KP_EQUAL = 336,
        _LEFT_SHIFT = 340,
        _LEFT_CONTROL = 341,
        _LEFT_ALT = 342,
        _LEFT_SUPER = 343,
        _RIGHT_SHIFT = 344,
        _RIGHT_CONTROL = 345,
        _RIGHT_ALT = 346,
        _RIGHT_SUPER = 347,
        _MENU = 348,
        _LAST = _MENU
    };
};
}
