#include "Input.hpp"
#include "Log.hpp"
#include "State.hpp"

using namespace GameInput::v3;

#pragma region Internal
namespace
{
//==============================================================================
// Internal Variable
//==============================================================================
// Return result for assertion.
HRESULT HR{};
bool BR{};

F32 DeadZone{0.2F};

ComPtr<IGameInput> InputStream{};

vector<U8> ConcernedVirtualKeys{};

GameInputCallbackToken CallbackToken{};

ComPtr<IGameInputReading> PrevReadingKeyboard{};
ComPtr<IGameInputReading> PrevReadingMouse{};
ComPtr<IGameInputReading> PrevReadingGamepad{};

//==============================================================================
// Internal Function
//==============================================================================
void OnDeviceConnectionChanged(_In_ GameInputCallbackToken /*callbackToken*/,
                               _In_ void* /*context*/,
                               _In_ IGameInputDevice* device,
                               _In_ U64 /*timestamp*/,
                               _In_ GameInputDeviceStatus currentStatus,
                               _In_ GameInputDeviceStatus previousStatus)
{
    auto& stateInput  = Acrylic::State::GetRefInput();
    bool wasConnected = previousStatus & GameInputDeviceConnected;
    bool isConnected  = currentStatus & GameInputDeviceConnected;

    const GameInputDeviceInfo* deviceInfo{};
    device->GetDeviceInfo(&deviceInfo);
    GameInputKind deviceKind = deviceInfo->supportedInput;

    // Connected
    if (isConnected && !wasConnected)
    {
        if (deviceKind & GameInputKindKeyboard)
        {
            { // Reset the input state
                std::ranges::fill(stateInput.KeyboardKeys,
                                  Acrylic::State::Button::None);
            }

            stateInput.isKeyboardConnected = true;
            HR                             = InputStream->GetCurrentReading(
                GameInputKindKeyboard,
                nullptr,
                PrevReadingKeyboard.GetAddressOf());
            assert(SUCCEEDED(HR) &&
                   "Failed to get keyboard reading in device callback.");
            LOG_INFO("Keyboard connected.");
        }
        else if (deviceKind & GameInputKindMouse)
        {
            { // Reset the input state
                stateInput.MouseL = Acrylic::State::Button::None;
                stateInput.MouseM = Acrylic::State::Button::None;
                stateInput.MouseR = Acrylic::State::Button::None;
                stateInput.Mouse4 = Acrylic::State::Button::None;
                stateInput.Mouse5 = Acrylic::State::Button::None;

                stateInput.DeltaPX = 0.0F;
                stateInput.DeltaPY = 0.0F;
                stateInput.DeltaWY = 0.0F;
            }

            stateInput.isMouseConnected = true;
            HR =
                InputStream->GetCurrentReading(GameInputKindMouse,
                                               nullptr,
                                               PrevReadingMouse.GetAddressOf());
            assert(SUCCEEDED(HR) &&
                   "Failed to get mouse reading in device callback.");
            LOG_INFO("Mouse connected.");
        }
        else if (deviceKind & GameInputKindGamepad)
        {
            { // Reset the input state
                stateInput.GamePadMenu      = Acrylic::State::Button::None;
                stateInput.GamePadView      = Acrylic::State::Button::None;
                stateInput.GamePadA         = Acrylic::State::Button::None;
                stateInput.GamePadB         = Acrylic::State::Button::None;
                stateInput.GamePadX         = Acrylic::State::Button::None;
                stateInput.GamePadY         = Acrylic::State::Button::None;
                stateInput.GamePadDPadUp    = Acrylic::State::Button::None;
                stateInput.GamePadDPadDown  = Acrylic::State::Button::None;
                stateInput.GamePadDPadLeft  = Acrylic::State::Button::None;
                stateInput.GamePadDPadRight = Acrylic::State::Button::None;
                stateInput.GamePadLB        = Acrylic::State::Button::None;
                stateInput.GamePadRB        = Acrylic::State::Button::None;
                stateInput.GamePadLS        = Acrylic::State::Button::None;
                stateInput.GamePadRS        = Acrylic::State::Button::None;

                stateInput.DeltaLT  = 0.0F;
                stateInput.DeltaRT  = 0.0F;
                stateInput.DeltaLSX = 0.0F;
                stateInput.DeltaLSY = 0.0F;
                stateInput.DeltaRSX = 0.0F;
                stateInput.DeltaRSY = 0.0F;
            }

            stateInput.isGamepadConnected = true;
            HR                            = InputStream->GetCurrentReading(
                GameInputKindGamepad,
                nullptr,
                PrevReadingGamepad.GetAddressOf());
            assert(SUCCEEDED(HR) &&
                   "Failed to get gamepad reading in device callback.");
            LOG_INFO("Gamepad connected.");
        }
    }
    // Disconnected
    else if (!isConnected && wasConnected)
    {
        if (deviceKind & GameInputKindKeyboard)
        {
            stateInput.isKeyboardConnected = false;
            PrevReadingKeyboard.Reset();
            LOG_INFO("Keyboard disconnected.");
        }
        else if (deviceKind & GameInputKindMouse)
        {
            stateInput.isMouseConnected = false;
            PrevReadingMouse.Reset();
            LOG_INFO("Mouse disconnected.");
        }
        else if (deviceKind & GameInputKindGamepad)
        {
            stateInput.isGamepadConnected = false;
            PrevReadingGamepad.Reset();
            LOG_INFO("Gamepad disconnected.");
        }
    }
}

void UpdateButtonState(Acrylic::State::Button& buttonState,
                       bool wasPressed,
                       bool isPressed)
{
    if (!wasPressed && isPressed)
    {
        buttonState = Acrylic::State::Button::Pressed;
    }
    else if (wasPressed && isPressed)
    {
        buttonState = Acrylic::State::Button::Held;
    }
    else if (wasPressed && !isPressed)
    {
        buttonState = Acrylic::State::Button::Released;
    }
    else if (!wasPressed && !isPressed)
    {
        buttonState = Acrylic::State::Button::None;
    }
};

void UpdateKeyboardState()
{
    auto& stateInput = Acrylic::State::GetRefInput();
    ComPtr<IGameInputReading> currReadingKeyboard{};

    if (stateInput.isKeyboardConnected)
    {
        HR = InputStream->GetCurrentReading(GameInputKindKeyboard,
                                            nullptr,
                                            currReadingKeyboard.GetAddressOf());
        assert(SUCCEEDED(HR) && "Failed to get keyboard reading.");

        // Get prevKeyCount & currKeyCount.
        const auto prevKeyCount = PrevReadingKeyboard->GetKeyCount();
        const auto currKeyCount = currReadingKeyboard->GetKeyCount();

        if (prevKeyCount == 0 && currKeyCount == 0)
        {
            // Reset the input state
            std::ranges::fill(stateInput.KeyboardKeys,
                              Acrylic::State::Button::None);

            PrevReadingKeyboard.Swap(currReadingKeyboard);
            return;
        }

        // Get prevKeyStates & currKeyStates.
        // Keyboards rarely support more than 12-16 keys at once.
        array<GameInputKeyState, 16> prevKeyStates{};
        PrevReadingKeyboard->GetKeyState(prevKeyCount, prevKeyStates.data());
        array<GameInputKeyState, 16> currKeyStates{};
        currReadingKeyboard->GetKeyState(currKeyCount, currKeyStates.data());

        // Update the state of concerned keys.
        for (auto i = 0; i < ConcernedVirtualKeys.size(); i++)
        {
            auto vk = ConcernedVirtualKeys[i];

            bool wasPressed{false};
            for (auto j = 0; j < prevKeyCount; j++)
            {
                if (prevKeyStates[j].virtualKey == vk)
                {
                    wasPressed = true;
                    break;
                }
            }

            bool isPressed{false};
            for (auto j = 0; j < currKeyCount; j++)
            {
                if (currKeyStates[j].virtualKey == vk)
                {
                    isPressed = true;
                    break;
                }
            }

            UpdateButtonState(stateInput.KeyboardKeys[i],
                              wasPressed,
                              isPressed);
        }
    }

    PrevReadingKeyboard.Swap(currReadingKeyboard);
}

void UpdateMouseState()
{
    auto& stateInput = Acrylic::State::GetRefInput();
    ComPtr<IGameInputReading> currReadingMouse{};

    if (stateInput.isMouseConnected)
    {
        HR = InputStream->GetCurrentReading(GameInputKindMouse,
                                            nullptr,
                                            currReadingMouse.GetAddressOf());
        assert(SUCCEEDED(HR) && "Failed to get mouse reading.");

        // Get prevMouseState & currMouseState.
        GameInputMouseState prevMouseState{};
        PrevReadingMouse->GetMouseState(&prevMouseState);
        GameInputMouseState currMouseState{};
        currReadingMouse->GetMouseState(&currMouseState);

        // Update the state of mouse axes.
        stateInput.DeltaPX = static_cast<F32>(currMouseState.positionX -
                                              prevMouseState.positionX);
        stateInput.DeltaPY = static_cast<F32>(currMouseState.positionY -
                                              prevMouseState.positionY);
        stateInput.DeltaWY =
            static_cast<F32>(currMouseState.wheelY - prevMouseState.wheelY);

        // Update the state of mouse buttons.
        UpdateButtonState(stateInput.MouseL,
                          prevMouseState.buttons & GameInputMouseLeftButton,
                          currMouseState.buttons & GameInputMouseLeftButton);
        UpdateButtonState(stateInput.MouseM,
                          prevMouseState.buttons & GameInputMouseMiddleButton,
                          currMouseState.buttons & GameInputMouseMiddleButton);
        UpdateButtonState(stateInput.MouseR,
                          prevMouseState.buttons & GameInputMouseRightButton,
                          currMouseState.buttons & GameInputMouseRightButton);
        UpdateButtonState(stateInput.Mouse4,
                          prevMouseState.buttons & GameInputMouseButton4,
                          currMouseState.buttons & GameInputMouseButton4);
        UpdateButtonState(stateInput.Mouse5,
                          prevMouseState.buttons & GameInputMouseButton5,
                          currMouseState.buttons & GameInputMouseButton5);
    }

    PrevReadingMouse.Swap(currReadingMouse);
}

void UpdateGamepadState()
{
    auto& stateInput = Acrylic::State::GetRefInput();
    ComPtr<IGameInputReading> currReadingGamepad{};

    if (stateInput.isGamepadConnected)
    {
        HR = InputStream->GetCurrentReading(GameInputKindGamepad,
                                            nullptr,
                                            currReadingGamepad.GetAddressOf());
        assert(SUCCEEDED(HR) && "Failed to get gamepad reading.");

        // Get prevGamepadState & currGamepadState.
        GameInputGamepadState prevGamepadState{};
        PrevReadingGamepad->GetGamepadState(&prevGamepadState);
        GameInputGamepadState currGamepadState{};
        currReadingGamepad->GetGamepadState(&currGamepadState);

        // Update the state of gamepad axes.
        stateInput.DeltaLT  = currGamepadState.leftTrigger > DeadZone
                                  ? currGamepadState.leftTrigger
                                  : 0.0F;
        stateInput.DeltaRT  = currGamepadState.rightTrigger > DeadZone
                                  ? currGamepadState.rightTrigger
                                  : 0.0F;
        stateInput.DeltaLSX = currGamepadState.leftThumbstickX > DeadZone
                                  ? currGamepadState.leftThumbstickX
                                  : 0.0F;
        stateInput.DeltaLSY = currGamepadState.leftThumbstickY > DeadZone
                                  ? currGamepadState.leftThumbstickY
                                  : 0.0F;
        stateInput.DeltaRSX = currGamepadState.rightThumbstickX > DeadZone
                                  ? currGamepadState.rightThumbstickX
                                  : 0.0F;
        stateInput.DeltaRSY = currGamepadState.rightThumbstickY > DeadZone
                                  ? currGamepadState.rightThumbstickY
                                  : 0.0F;

        // Update the state of gamepad buttons.
        UpdateButtonState(stateInput.GamePadMenu,
                          prevGamepadState.buttons & GameInputGamepadMenu,
                          currGamepadState.buttons & GameInputGamepadMenu);
        UpdateButtonState(stateInput.GamePadView,
                          prevGamepadState.buttons & GameInputGamepadView,
                          currGamepadState.buttons & GameInputGamepadView);
        UpdateButtonState(stateInput.GamePadA,
                          prevGamepadState.buttons & GameInputGamepadA,
                          currGamepadState.buttons & GameInputGamepadA);
        UpdateButtonState(stateInput.GamePadB,
                          prevGamepadState.buttons & GameInputGamepadB,
                          currGamepadState.buttons & GameInputGamepadB);
        UpdateButtonState(stateInput.GamePadX,
                          prevGamepadState.buttons & GameInputGamepadX,
                          currGamepadState.buttons & GameInputGamepadX);
        UpdateButtonState(stateInput.GamePadY,
                          prevGamepadState.buttons & GameInputGamepadY,
                          currGamepadState.buttons & GameInputGamepadY);
        UpdateButtonState(stateInput.GamePadDPadUp,
                          prevGamepadState.buttons & GameInputGamepadDPadUp,
                          currGamepadState.buttons & GameInputGamepadDPadUp);
        UpdateButtonState(stateInput.GamePadDPadDown,
                          prevGamepadState.buttons & GameInputGamepadDPadDown,
                          currGamepadState.buttons & GameInputGamepadDPadDown);
        UpdateButtonState(stateInput.GamePadDPadLeft,
                          prevGamepadState.buttons & GameInputGamepadDPadLeft,
                          currGamepadState.buttons & GameInputGamepadDPadLeft);
        UpdateButtonState(stateInput.GamePadDPadRight,
                          prevGamepadState.buttons & GameInputGamepadDPadRight,
                          currGamepadState.buttons & GameInputGamepadDPadRight);
        UpdateButtonState(
            stateInput.GamePadLB,
            prevGamepadState.buttons & GameInputGamepadLeftShoulder,
            currGamepadState.buttons & GameInputGamepadLeftShoulder);
        UpdateButtonState(
            stateInput.GamePadRB,
            prevGamepadState.buttons & GameInputGamepadRightShoulder,
            currGamepadState.buttons & GameInputGamepadRightShoulder);
        UpdateButtonState(
            stateInput.GamePadLS,
            prevGamepadState.buttons & GameInputGamepadLeftThumbstick,
            currGamepadState.buttons & GameInputGamepadLeftThumbstick);
        UpdateButtonState(
            stateInput.GamePadRS,
            prevGamepadState.buttons & GameInputGamepadRightThumbstick,
            currGamepadState.buttons & GameInputGamepadRightThumbstick);
    }

    PrevReadingGamepad.Swap(currReadingGamepad);
}
} // namespace
#pragma endregion

#pragma region External
namespace Acrylic::Input
{
void Init()
{
    auto& stateInput = Acrylic::State::GetRefInput();

    // Init the keys we want to listen to.
    ConcernedVirtualKeys =
        {'W', 'S', 'A', 'D', VK_UP, VK_DOWN, VK_LEFT, VK_RIGHT, VK_LSHIFT};

    // Reset the input state
    stateInput.KeyboardKeys.assign(ConcernedVirtualKeys.size(),
                                   Acrylic::State::Button::None);

    // Create GameInput and register device callback.
    HR = GameInputCreate(InputStream.GetAddressOf());
    assert(SUCCEEDED(HR) && "Failed to create Microsoft.GameInput.");
    HR = InputStream->RegisterDeviceCallback(
        nullptr,
        GameInputKindKeyboard | GameInputKindMouse | GameInputKindGamepad,
        GameInputDeviceConnected,
        GameInputAsyncEnumeration,
        nullptr,
        OnDeviceConnectionChanged,
        &CallbackToken);
    assert(SUCCEEDED(HR) && "Failed to register device callback.");

    LOG_INFO("Acrylic::Input::Init() succeeded.");
}

void Update()
{
    UpdateKeyboardState();
    UpdateMouseState();
    UpdateGamepadState();
}
} // namespace Acrylic::Input
#pragma endregion