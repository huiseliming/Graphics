#include "GameInput.h"
#include "../Utility/Utility.h"
#include <XInput.h>
#pragma comment(lib, "xinput9_1_0.lib")
#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")

GameInput::GameInput()
{
    KbmInitialize();
    GlobalVariable<GameInput>::Set(this);
}

GameInput::~GameInput()
{
    KbmShutdown();
}

void GameInput::Update(float frameDelta)
{
    memcpy(m_Buttons[1], m_Buttons[0], sizeof(m_Buttons[0]));
    memset(m_Buttons[0], 0, sizeof(m_Buttons[0]));
    memset(m_Analogs, 0, sizeof(m_Analogs));

    XINPUT_STATE newInputState;
    if (ERROR_SUCCESS == XInputGetState(0, &newInputState))
    {
        if (newInputState.Gamepad.wButtons & (1 << 0)) m_Buttons[0][kDPadUp] = true;
        if (newInputState.Gamepad.wButtons & (1 << 1)) m_Buttons[0][kDPadDown] = true;
        if (newInputState.Gamepad.wButtons & (1 << 2)) m_Buttons[0][kDPadLeft] = true;
        if (newInputState.Gamepad.wButtons & (1 << 3)) m_Buttons[0][kDPadRight] = true;
        if (newInputState.Gamepad.wButtons & (1 << 4)) m_Buttons[0][kStartButton] = true;
        if (newInputState.Gamepad.wButtons & (1 << 5)) m_Buttons[0][kBackButton] = true;
        if (newInputState.Gamepad.wButtons & (1 << 6)) m_Buttons[0][kLThumbClick] = true;
        if (newInputState.Gamepad.wButtons & (1 << 7)) m_Buttons[0][kRThumbClick] = true;
        if (newInputState.Gamepad.wButtons & (1 << 8)) m_Buttons[0][kLShoulder] = true;
        if (newInputState.Gamepad.wButtons & (1 << 9)) m_Buttons[0][kRShoulder] = true;
        if (newInputState.Gamepad.wButtons & (1 << 12)) m_Buttons[0][kAButton] = true;
        if (newInputState.Gamepad.wButtons & (1 << 13)) m_Buttons[0][kBButton] = true;
        if (newInputState.Gamepad.wButtons & (1 << 14)) m_Buttons[0][kXButton] = true;
        if (newInputState.Gamepad.wButtons & (1 << 15)) m_Buttons[0][kYButton] = true;

        m_Analogs[kAnalogLeftTrigger] = newInputState.Gamepad.bLeftTrigger / 255.0f;
        m_Analogs[kAnalogRightTrigger] = newInputState.Gamepad.bRightTrigger / 255.0f;
        m_Analogs[kAnalogLeftStickX] = FilterAnalogInput(newInputState.Gamepad.sThumbLX, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
        m_Analogs[kAnalogLeftStickY] = FilterAnalogInput(newInputState.Gamepad.sThumbLY, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
        m_Analogs[kAnalogRightStickX] = FilterAnalogInput(newInputState.Gamepad.sThumbRX, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE);
        m_Analogs[kAnalogRightStickY] = FilterAnalogInput(newInputState.Gamepad.sThumbRY, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE);
    }

    KbmUpdate();

    for (uint32_t i = 0; i < kNumKeys; ++i)
    {
        m_Buttons[0][i] = (m_Keybuffer[m_DXKeyMapping[i]] & 0x80) != 0;
    }

    for (uint32_t i = 0; i < 8; ++i)
    {
        if (m_MouseState.rgbButtons[i] > 0) m_Buttons[0][kMouse0 + i] = true;
    }

    m_Analogs[kAnalogMouseX] = (float)m_MouseState.lX * .0018f;
    m_Analogs[kAnalogMouseY] = (float)m_MouseState.lY * -.0018f;

    if (m_MouseState.lZ > 0)
        m_Analogs[kAnalogMouseScroll] = 1.0f;
    else if (m_MouseState.lZ < 0)
        m_Analogs[kAnalogMouseScroll] = -1.0f;

    // Update time duration for buttons pressed
    for (uint32_t i = 0; i < kNumDigitalInputs; ++i)
    {
        if (m_Buttons[0][i])
        {
            if (!m_Buttons[1][i])
                m_HoldDuration[i] = 0.0f;
            else
                m_HoldDuration[i] += frameDelta;
        }
    }

    for (uint32_t i = 0; i < kNumAnalogInputs; ++i)
    {
        m_AnalogsTC[i] = m_Analogs[i] * frameDelta;
    }
}

float GameInput::FilterAnalogInput(int val, int deadZone)
{
    if (val < 0)
    {
        if (val > -deadZone)
            return 0.0f;
        else
            return (val + deadZone) / (32768.0f - deadZone);
    }
    else
    {
        if (val < deadZone)
            return 0.0f;
        else
            return (val - deadZone) / (32767.0f - deadZone);
    }
}

void GameInput::KbmBuildKeyMapping()
{
    m_DXKeyMapping[GameInput::kKey_escape] = 1;
    m_DXKeyMapping[GameInput::kKey_1] = 2;
    m_DXKeyMapping[GameInput::kKey_2] = 3;
    m_DXKeyMapping[GameInput::kKey_3] = 4;
    m_DXKeyMapping[GameInput::kKey_4] = 5;
    m_DXKeyMapping[GameInput::kKey_5] = 6;
    m_DXKeyMapping[GameInput::kKey_6] = 7;
    m_DXKeyMapping[GameInput::kKey_7] = 8;
    m_DXKeyMapping[GameInput::kKey_8] = 9;
    m_DXKeyMapping[GameInput::kKey_9] = 10;
    m_DXKeyMapping[GameInput::kKey_0] = 11;
    m_DXKeyMapping[GameInput::kKey_minus] = 12;
    m_DXKeyMapping[GameInput::kKey_equals] = 13;
    m_DXKeyMapping[GameInput::kKey_back] = 14;
    m_DXKeyMapping[GameInput::kKey_tab] = 15;
    m_DXKeyMapping[GameInput::kKey_q] = 16;
    m_DXKeyMapping[GameInput::kKey_w] = 17;
    m_DXKeyMapping[GameInput::kKey_e] = 18;
    m_DXKeyMapping[GameInput::kKey_r] = 19;
    m_DXKeyMapping[GameInput::kKey_t] = 20;
    m_DXKeyMapping[GameInput::kKey_y] = 21;
    m_DXKeyMapping[GameInput::kKey_u] = 22;
    m_DXKeyMapping[GameInput::kKey_i] = 23;
    m_DXKeyMapping[GameInput::kKey_o] = 24;
    m_DXKeyMapping[GameInput::kKey_p] = 25;
    m_DXKeyMapping[GameInput::kKey_lbracket] = 26;
    m_DXKeyMapping[GameInput::kKey_rbracket] = 27;
    m_DXKeyMapping[GameInput::kKey_return] = 28;
    m_DXKeyMapping[GameInput::kKey_lcontrol] = 29;
    m_DXKeyMapping[GameInput::kKey_a] = 30;
    m_DXKeyMapping[GameInput::kKey_s] = 31;
    m_DXKeyMapping[GameInput::kKey_d] = 32;
    m_DXKeyMapping[GameInput::kKey_f] = 33;
    m_DXKeyMapping[GameInput::kKey_g] = 34;
    m_DXKeyMapping[GameInput::kKey_h] = 35;
    m_DXKeyMapping[GameInput::kKey_j] = 36;
    m_DXKeyMapping[GameInput::kKey_k] = 37;
    m_DXKeyMapping[GameInput::kKey_l] = 38;
    m_DXKeyMapping[GameInput::kKey_semicolon] = 39;
    m_DXKeyMapping[GameInput::kKey_apostrophe] = 40;
    m_DXKeyMapping[GameInput::kKey_grave] = 41;
    m_DXKeyMapping[GameInput::kKey_lshift] = 42;
    m_DXKeyMapping[GameInput::kKey_backslash] = 43;
    m_DXKeyMapping[GameInput::kKey_z] = 44;
    m_DXKeyMapping[GameInput::kKey_x] = 45;
    m_DXKeyMapping[GameInput::kKey_c] = 46;
    m_DXKeyMapping[GameInput::kKey_v] = 47;
    m_DXKeyMapping[GameInput::kKey_b] = 48;
    m_DXKeyMapping[GameInput::kKey_n] = 49;
    m_DXKeyMapping[GameInput::kKey_m] = 50;
    m_DXKeyMapping[GameInput::kKey_comma] = 51;
    m_DXKeyMapping[GameInput::kKey_period] = 52;
    m_DXKeyMapping[GameInput::kKey_slash] = 53;
    m_DXKeyMapping[GameInput::kKey_rshift] = 54;
    m_DXKeyMapping[GameInput::kKey_multiply] = 55;
    m_DXKeyMapping[GameInput::kKey_lalt] = 56;
    m_DXKeyMapping[GameInput::kKey_space] = 57;
    m_DXKeyMapping[GameInput::kKey_capital] = 58;
    m_DXKeyMapping[GameInput::kKey_f1] = 59;
    m_DXKeyMapping[GameInput::kKey_f2] = 60;
    m_DXKeyMapping[GameInput::kKey_f3] = 61;
    m_DXKeyMapping[GameInput::kKey_f4] = 62;
    m_DXKeyMapping[GameInput::kKey_f5] = 63;
    m_DXKeyMapping[GameInput::kKey_f6] = 64;
    m_DXKeyMapping[GameInput::kKey_f7] = 65;
    m_DXKeyMapping[GameInput::kKey_f8] = 66;
    m_DXKeyMapping[GameInput::kKey_f9] = 67;
    m_DXKeyMapping[GameInput::kKey_f10] = 68;
    m_DXKeyMapping[GameInput::kKey_numlock] = 69;
    m_DXKeyMapping[GameInput::kKey_scroll] = 70;
    m_DXKeyMapping[GameInput::kKey_numpad7] = 71;
    m_DXKeyMapping[GameInput::kKey_numpad8] = 72;
    m_DXKeyMapping[GameInput::kKey_numpad9] = 73;
    m_DXKeyMapping[GameInput::kKey_subtract] = 74;
    m_DXKeyMapping[GameInput::kKey_numpad4] = 75;
    m_DXKeyMapping[GameInput::kKey_numpad5] = 76;
    m_DXKeyMapping[GameInput::kKey_numpad6] = 77;
    m_DXKeyMapping[GameInput::kKey_add] = 78;
    m_DXKeyMapping[GameInput::kKey_numpad1] = 79;
    m_DXKeyMapping[GameInput::kKey_numpad2] = 80;
    m_DXKeyMapping[GameInput::kKey_numpad3] = 81;
    m_DXKeyMapping[GameInput::kKey_numpad0] = 82;
    m_DXKeyMapping[GameInput::kKey_decimal] = 83;
    m_DXKeyMapping[GameInput::kKey_f11] = 87;
    m_DXKeyMapping[GameInput::kKey_f12] = 88;
    m_DXKeyMapping[GameInput::kKey_numpadenter] = 156;
    m_DXKeyMapping[GameInput::kKey_rcontrol] = 157;
    m_DXKeyMapping[GameInput::kKey_divide] = 181;
    m_DXKeyMapping[GameInput::kKey_sysrq] = 183;
    m_DXKeyMapping[GameInput::kKey_ralt] = 184;
    m_DXKeyMapping[GameInput::kKey_pause] = 197;
    m_DXKeyMapping[GameInput::kKey_home] = 199;
    m_DXKeyMapping[GameInput::kKey_up] = 200;
    m_DXKeyMapping[GameInput::kKey_pgup] = 201;
    m_DXKeyMapping[GameInput::kKey_left] = 203;
    m_DXKeyMapping[GameInput::kKey_right] = 205;
    m_DXKeyMapping[GameInput::kKey_end] = 207;
    m_DXKeyMapping[GameInput::kKey_down] = 208;
    m_DXKeyMapping[GameInput::kKey_pgdn] = 209;
    m_DXKeyMapping[GameInput::kKey_insert] = 210;
    m_DXKeyMapping[GameInput::kKey_delete] = 211;
    m_DXKeyMapping[GameInput::kKey_lwin] = 219;
    m_DXKeyMapping[GameInput::kKey_rwin] = 220;
    m_DXKeyMapping[GameInput::kKey_apps] = 221;
}

void GameInput::KbmZeroInputs()
{
    memset(&m_MouseState, 0, sizeof(DIMOUSESTATE2));
    memset(m_Keybuffer, 0, sizeof(m_Keybuffer));
}

void GameInput::KbmInitialize()
{
    KbmBuildKeyMapping();
    if (FAILED(DirectInput8Create(GetModuleHandle(nullptr), DIRECTINPUT_VERSION, IID_IDirectInput8, (void**)&m_pDirectInput, nullptr)))
        ASSERT(false, "DirectInput8 initialization failed.");

    if (FAILED(m_pDirectInput->CreateDevice(GUID_SysKeyboard, &m_pKeyboard, nullptr)))
        ASSERT(false, "Keyboard CreateDevice failed.");
    if (FAILED(m_pKeyboard->SetDataFormat(&c_dfDIKeyboard)))
        ASSERT(false, "Keyboard SetDataFormat failed.");
    if (FAILED(m_pKeyboard->SetCooperativeLevel(GlobalVariable<Window>::Get()->GetHWND(), DISCL_FOREGROUND | DISCL_NONEXCLUSIVE)))
        ASSERT(false, "Keyboard SetCooperativeLevel failed.");

    DIPROPDWORD dipdw;
    dipdw.diph.dwSize = sizeof(DIPROPDWORD);
    dipdw.diph.dwHeaderSize = sizeof(DIPROPHEADER);
    dipdw.diph.dwObj = 0;
    dipdw.diph.dwHow = DIPH_DEVICE;
    dipdw.dwData = 10;
    if (FAILED(m_pKeyboard->SetProperty(DIPROP_BUFFERSIZE, &dipdw.diph)))
        ASSERT(false, "Keyboard set buffer size failed.");

    if (FAILED(m_pDirectInput->CreateDevice(GUID_SysMouse, &m_pMouse, nullptr)))
        ASSERT(false, "Mouse CreateDevice failed.");
    if (FAILED(m_pMouse->SetDataFormat(&c_dfDIMouse2)))
        ASSERT(false, "Mouse SetDataFormat failed.");
    if (FAILED(m_pMouse->SetCooperativeLevel(GlobalVariable<Window>::Get()->GetHWND(), DISCL_FOREGROUND | DISCL_NONEXCLUSIVE)))
        ASSERT(false, "Mouse SetCooperativeLevel failed.");

    KbmZeroInputs();
}

void GameInput::KbmShutdown()
{
    if (m_pKeyboard)
    {
        m_pKeyboard->Unacquire();
        m_pKeyboard->Release();
        m_pKeyboard = nullptr;
    }
    if (m_pMouse)
    {
        m_pMouse->Unacquire();
        m_pMouse->Release();
        m_pMouse = nullptr;
    }
    if (m_pDirectInput)
    {
        m_pDirectInput->Release();
        m_pDirectInput = nullptr;
    }
}

void GameInput::KbmUpdate()
{
    HWND foreground = GetForegroundWindow();
    bool visible = IsWindowVisible(foreground) != 0;

    if (foreground != GlobalVariable<Window>::Get()->GetHWND() // wouldn't be able to acquire
        || !visible)
    {
        KbmZeroInputs();
    }
    else
    {
        m_pMouse->Acquire();
        m_pMouse->GetDeviceState(sizeof(DIMOUSESTATE2), &m_MouseState);
        m_pKeyboard->Acquire();
        m_pKeyboard->GetDeviceState(sizeof(m_Keybuffer), m_Keybuffer);
    }
}

