#pragma once

class UinputKeyboard {
public:
    UinputKeyboard();
    ~UinputKeyboard();

    bool isValid() const { return m_fd >= 0; }
    void tap(int keycode);

private:
    void emitEvent(int type, int code, int value);
    int m_fd = -1;
};