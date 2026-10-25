#pragma once
#include <unordered_map>

namespace whiskers {

enum class Key {
    Unknown = 0,
    A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
    Num0, Num1, Num2, Num3, Num4, Num5, Num6, Num7, Num8, Num9,
    Space, Enter, Escape, Left, Right, Up, Down,
    Shift, Ctrl, Alt
};

enum class ButtonState {
    Released = 0,
    Pressed = 1
};

class Input {
public:
    static Input& getInstance();
    
    void update();
    bool isKeyPressed(Key key) const;
    bool isKeyJustPressed(Key key) const;
    bool isKeyJustReleased(Key key) const;
    
    void setKeyState(Key key, ButtonState state);
    
private:
    Input() = default;
    
    std::unordered_map<Key, ButtonState> m_currentState;
    std::unordered_map<Key, ButtonState> m_previousState;
    
    static Key sdlKeyToKey(int sdlKey);
};

}  // namespace whiskers
