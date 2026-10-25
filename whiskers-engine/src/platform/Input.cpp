#include "whiskers/platform/Input.h"
#include <SDL2/SDL.h>

namespace whiskers {

Input& Input::getInstance() {
    static Input instance;
    return instance;
}

void Input::update() {
    // Store previous state
    m_previousState = m_currentState;
}

bool Input::isKeyPressed(Key key) const {
    auto it = m_currentState.find(key);
    return it != m_currentState.end() && it->second == ButtonState::Pressed;
}

bool Input::isKeyJustPressed(Key key) const {
    auto currentIt = m_currentState.find(key);
    auto previousIt = m_previousState.find(key);
    
    bool currentPressed = (currentIt != m_currentState.end() && currentIt->second == ButtonState::Pressed);
    bool previousPressed = (previousIt != m_previousState.end() && previousIt->second == ButtonState::Pressed);
    
    return currentPressed && !previousPressed;
}

bool Input::isKeyJustReleased(Key key) const {
    auto currentIt = m_currentState.find(key);
    auto previousIt = m_previousState.find(key);
    
    bool currentPressed = (currentIt != m_currentState.end() && currentIt->second == ButtonState::Pressed);
    bool previousPressed = (previousIt != m_previousState.end() && previousIt->second == ButtonState::Pressed);
    
    return !currentPressed && previousPressed;
}

void Input::setKeyState(Key key, ButtonState state) {
    m_currentState[key] = state;
}

Key Input::sdlKeyToKey(int sdlKey) {
    switch (sdlKey) {
        case SDLK_a: return Key::A;
        case SDLK_b: return Key::B;
        case SDLK_c: return Key::C;
        case SDLK_d: return Key::D;
        case SDLK_e: return Key::E;
        case SDLK_f: return Key::F;
        case SDLK_g: return Key::G;
        case SDLK_h: return Key::H;
        case SDLK_i: return Key::I;
        case SDLK_j: return Key::J;
        case SDLK_k: return Key::K;
        case SDLK_l: return Key::L;
        case SDLK_m: return Key::M;
        case SDLK_n: return Key::N;
        case SDLK_o: return Key::O;
        case SDLK_p: return Key::P;
        case SDLK_q: return Key::Q;
        case SDLK_r: return Key::R;
        case SDLK_s: return Key::S;
        case SDLK_t: return Key::T;
        case SDLK_u: return Key::U;
        case SDLK_v: return Key::V;
        case SDLK_w: return Key::W;
        case SDLK_x: return Key::X;
        case SDLK_y: return Key::Y;
        case SDLK_z: return Key::Z;
        case SDLK_0: return Key::Num0;
        case SDLK_1: return Key::Num1;
        case SDLK_2: return Key::Num2;
        case SDLK_3: return Key::Num3;
        case SDLK_4: return Key::Num4;
        case SDLK_5: return Key::Num5;
        case SDLK_6: return Key::Num6;
        case SDLK_7: return Key::Num7;
        case SDLK_8: return Key::Num8;
        case SDLK_9: return Key::Num9;
        case SDLK_SPACE: return Key::Space;
        case SDLK_RETURN: return Key::Enter;
        case SDLK_ESCAPE: return Key::Escape;
        case SDLK_LEFT: return Key::Left;
        case SDLK_RIGHT: return Key::Right;
        case SDLK_UP: return Key::Up;
        case SDLK_DOWN: return Key::Down;
        case SDLK_LSHIFT:
        case SDLK_RSHIFT: return Key::Shift;
        case SDLK_LCTRL:
        case SDLK_RCTRL: return Key::Ctrl;
        case SDLK_LALT:
        case SDLK_RALT: return Key::Alt;
        default: return Key::Unknown;
    }
}

}  // namespace whiskers
