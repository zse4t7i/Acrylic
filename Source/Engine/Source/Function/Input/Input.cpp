#include "Input.hpp"

#include <algorithm>

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

ComPtr<IGameInput> GI{};
Acrylic::Input::InputState IOState{};

vector<U8> ConcernedVirtualKeys{};

GameInputCallbackToken DeviceCallbackToken{};

ComPtr<IGameInputReading> PrevReadingKeyboard{};
ComPtr<IGameInputReading> PrevReadingMouse{};
ComPtr<IGameInputReading> PrevReadingGamepad{};

//==============================================================================
// Internal Function
//==============================================================================
void OnDeviceConnectionChanged(_In_ GameInputCallbackToken callbackToken,
                               _In_ void* context,
                               _In_ IGameInputDevice* device,
                               _In_ uint64_t timestamp,
                               _In_ GameInputDeviceStatus currentStatus,
                               _In_ GameInputDeviceStatus previousStatus)
{
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
                std::ranges::fill(IOState.KeyboardKeys,
                                  Acrylic::Input::ButtonState::None);
            }

            IOState.isKeyboardConnected = true;
            HR = GI->GetCurrentReading(GameInputKindKeyboard,
                                       nullptr,
                                       PrevReadingKeyboard.GetAddressOf());
            assert(SUCCEEDED(HR) &&
                   "Failed to get keyboard reading in device callback.");
            LOG_INFO("Keyboard connected.");
        }
        else if (deviceKind & GameInputKindMouse)
        {
            { // Reset the input state
                IOState.MouseL = Acrylic::Input::ButtonState::None;
                IOState.MouseM = Acrylic::Input::ButtonState::None;
                IOState.MouseR = Acrylic::Input::ButtonState::None;
                IOState.Mouse4 = Acrylic::Input::ButtonState::None;
                IOState.Mouse5 = Acrylic::Input::ButtonState::None;

                IOState.DeltaPX = 0.0F;
                IOState.DeltaPY = 0.0F;
                IOState.DeltaWY = 0.0F;
            }

            IOState.isMouseConnected = true;
            HR = GI->GetCurrentReading(GameInputKindMouse,
                                       nullptr,
                                       PrevReadingMouse.GetAddressOf());
            assert(SUCCEEDED(HR) &&
                   "Failed to get mouse reading in device callback.");
            LOG_INFO("Mouse connected.");
        }
        else if (deviceKind & GameInputKindGamepad)
        {
            { // Reset the input state
                IOState.GamePadMenu      = Acrylic::Input::ButtonState::None;
                IOState.GamePadView      = Acrylic::Input::ButtonState::None;
                IOState.GamePadA         = Acrylic::Input::ButtonState::None;
                IOState.GamePadB         = Acrylic::Input::ButtonState::None;
                IOState.GamePadX         = Acrylic::Input::ButtonState::None;
                IOState.GamePadY         = Acrylic::Input::ButtonState::None;
                IOState.GamePadDPadUp    = Acrylic::Input::ButtonState::None;
                IOState.GamePadDPadDown  = Acrylic::Input::ButtonState::None;
                IOState.GamePadDPadLeft  = Acrylic::Input::ButtonState::None;
                IOState.GamePadDPadRight = Acrylic::Input::ButtonState::None;
                IOState.GamePadLB        = Acrylic::Input::ButtonState::None;
                IOState.GamePadRB        = Acrylic::Input::ButtonState::None;
                IOState.GamePadLS        = Acrylic::Input::ButtonState::None;
                IOState.GamePadRS        = Acrylic::Input::ButtonState::None;

                IOState.DeltaLT  = 0.0F;
                IOState.DeltaRT  = 0.0F;
                IOState.DeltaLSX = 0.0F;
                IOState.DeltaLSY = 0.0F;
                IOState.DeltaRSX = 0.0F;
                IOState.DeltaRSY = 0.0F;
            }

            IOState.isGamepadConnected = true;
            HR = GI->GetCurrentReading(GameInputKindGamepad,
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
            IOState.isKeyboardConnected = false;
            PrevReadingKeyboard.Reset();
            LOG_INFO("Keyboard disconnected.");
        }
        else if (deviceKind & GameInputKindMouse)
        {
            IOState.isMouseConnected = false;
            PrevReadingMouse.Reset();
            LOG_INFO("Mouse disconnected.");
        }
        else if (deviceKind & GameInputKindGamepad)
        {
            IOState.isGamepadConnected = false;
            PrevReadingGamepad.Reset();
            LOG_INFO("Gamepad disconnected.");
        }
    }
}

void UpdateButtonState(Acrylic::Input::ButtonState& buttonState,
                       bool wasPressed,
                       bool isPressed)
{
    if (!wasPressed && isPressed)
    {
        buttonState = Acrylic::Input::ButtonState::Pressed;
    }
    else if (wasPressed && isPressed)
    {
        buttonState = Acrylic::Input::ButtonState::Held;
    }
    else if (wasPressed && !isPressed)
    {
        buttonState = Acrylic::Input::ButtonState::Released;
    }
    else if (!wasPressed && !isPressed)
    {
        buttonState = Acrylic::Input::ButtonState::None;
    }
};

void UpdateKeyboardState()
{
    ComPtr<IGameInputReading> currReadingKeyboard{};

    if (IOState.isKeyboardConnected)
    {
        HR = GI->GetCurrentReading(GameInputKindKeyboard,
                                   nullptr,
                                   currReadingKeyboard.GetAddressOf());
        assert(SUCCEEDED(HR) && "Failed to get keyboard reading.");

        // Get prevKeyCount & currKeyCount.
        const auto prevKeyCount = PrevReadingKeyboard->GetKeyCount();
        const auto currKeyCount = currReadingKeyboard->GetKeyCount();

        if (prevKeyCount == 0 && currKeyCount == 0)
        {
            // Reset the input state
            std::ranges::fill(IOState.KeyboardKeys,
                              Acrylic::Input::ButtonState::None);

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

            UpdateButtonState(IOState.KeyboardKeys[i], wasPressed, isPressed);
        }
    }

    PrevReadingKeyboard.Swap(currReadingKeyboard);
}

void UpdateMouseState()
{
    ComPtr<IGameInputReading> currReadingMouse{};

    if (IOState.isMouseConnected)
    {
        HR = GI->GetCurrentReading(GameInputKindMouse,
                                   nullptr,
                                   currReadingMouse.GetAddressOf());
        assert(SUCCEEDED(HR) && "Failed to get mouse reading.");

        // Get prevMouseState & currMouseState.
        GameInputMouseState prevMouseState{};
        PrevReadingMouse->GetMouseState(&prevMouseState);
        GameInputMouseState currMouseState{};
        currReadingMouse->GetMouseState(&currMouseState);

        // Update the state of mouse axes.
        IOState.DeltaPX = static_cast<F32>(currMouseState.positionX -
                                           prevMouseState.positionX);
        IOState.DeltaPY = static_cast<F32>(currMouseState.positionY -
                                           prevMouseState.positionY);
        IOState.DeltaWY =
            static_cast<F32>(currMouseState.wheelY - prevMouseState.wheelY);

        // Update the state of mouse buttons.
        UpdateButtonState(IOState.MouseL,
                          prevMouseState.buttons & GameInputMouseLeftButton,
                          currMouseState.buttons & GameInputMouseLeftButton);
        UpdateButtonState(IOState.MouseM,
                          prevMouseState.buttons & GameInputMouseMiddleButton,
                          currMouseState.buttons & GameInputMouseMiddleButton);
        UpdateButtonState(IOState.MouseR,
                          prevMouseState.buttons & GameInputMouseRightButton,
                          currMouseState.buttons & GameInputMouseRightButton);
        UpdateButtonState(IOState.Mouse4,
                          prevMouseState.buttons & GameInputMouseButton4,
                          currMouseState.buttons & GameInputMouseButton4);
        UpdateButtonState(IOState.Mouse5,
                          prevMouseState.buttons & GameInputMouseButton5,
                          currMouseState.buttons & GameInputMouseButton5);
    }

    PrevReadingMouse.Swap(currReadingMouse);
}

void UpdateGamepadState()
{
    ComPtr<IGameInputReading> currReadingGamepad{};

    if (IOState.isGamepadConnected)
    {
        HR = GI->GetCurrentReading(GameInputKindGamepad,
                                   nullptr,
                                   currReadingGamepad.GetAddressOf());
        assert(SUCCEEDED(HR) && "Failed to get gamepad reading.");

        // Get prevGamepadState & currGamepadState.
        GameInputGamepadState prevGamepadState{};
        PrevReadingGamepad->GetGamepadState(&prevGamepadState);
        GameInputGamepadState currGamepadState{};
        currReadingGamepad->GetGamepadState(&currGamepadState);

        // Update the state of gamepad axes.
        IOState.DeltaLT  = currGamepadState.leftTrigger > DeadZone
                               ? currGamepadState.leftTrigger
                               : 0.0F;
        IOState.DeltaRT  = currGamepadState.rightTrigger > DeadZone
                               ? currGamepadState.rightTrigger
                               : 0.0F;
        IOState.DeltaLSX = currGamepadState.leftThumbstickX > DeadZone
                               ? currGamepadState.leftThumbstickX
                               : 0.0F;
        IOState.DeltaLSY = currGamepadState.leftThumbstickY > DeadZone
                               ? currGamepadState.leftThumbstickY
                               : 0.0F;
        IOState.DeltaRSX = currGamepadState.rightThumbstickX > DeadZone
                               ? currGamepadState.rightThumbstickX
                               : 0.0F;
        IOState.DeltaRSY = currGamepadState.rightThumbstickY > DeadZone
                               ? currGamepadState.rightThumbstickY
                               : 0.0F;

        // Update the state of gamepad buttons.
        UpdateButtonState(IOState.GamePadMenu,
                          prevGamepadState.buttons & GameInputGamepadMenu,
                          currGamepadState.buttons & GameInputGamepadMenu);
        UpdateButtonState(IOState.GamePadView,
                          prevGamepadState.buttons & GameInputGamepadView,
                          currGamepadState.buttons & GameInputGamepadView);
        UpdateButtonState(IOState.GamePadA,
                          prevGamepadState.buttons & GameInputGamepadA,
                          currGamepadState.buttons & GameInputGamepadA);
        UpdateButtonState(IOState.GamePadB,
                          prevGamepadState.buttons & GameInputGamepadB,
                          currGamepadState.buttons & GameInputGamepadB);
        UpdateButtonState(IOState.GamePadX,
                          prevGamepadState.buttons & GameInputGamepadX,
                          currGamepadState.buttons & GameInputGamepadX);
        UpdateButtonState(IOState.GamePadY,
                          prevGamepadState.buttons & GameInputGamepadY,
                          currGamepadState.buttons & GameInputGamepadY);
        UpdateButtonState(IOState.GamePadDPadUp,
                          prevGamepadState.buttons & GameInputGamepadDPadUp,
                          currGamepadState.buttons & GameInputGamepadDPadUp);
        UpdateButtonState(IOState.GamePadDPadDown,
                          prevGamepadState.buttons & GameInputGamepadDPadDown,
                          currGamepadState.buttons & GameInputGamepadDPadDown);
        UpdateButtonState(IOState.GamePadDPadLeft,
                          prevGamepadState.buttons & GameInputGamepadDPadLeft,
                          currGamepadState.buttons & GameInputGamepadDPadLeft);
        UpdateButtonState(IOState.GamePadDPadRight,
                          prevGamepadState.buttons & GameInputGamepadDPadRight,
                          currGamepadState.buttons & GameInputGamepadDPadRight);
        UpdateButtonState(
            IOState.GamePadLB,
            prevGamepadState.buttons & GameInputGamepadLeftShoulder,
            currGamepadState.buttons & GameInputGamepadLeftShoulder);
        UpdateButtonState(
            IOState.GamePadRB,
            prevGamepadState.buttons & GameInputGamepadRightShoulder,
            currGamepadState.buttons & GameInputGamepadRightShoulder);
        UpdateButtonState(
            IOState.GamePadLS,
            prevGamepadState.buttons & GameInputGamepadLeftThumbstick,
            currGamepadState.buttons & GameInputGamepadLeftThumbstick);
        UpdateButtonState(
            IOState.GamePadRS,
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
    // Init the keys we want to listen to.
    ConcernedVirtualKeys =
        {'W', 'S', 'A', 'D', VK_UP, VK_DOWN, VK_LEFT, VK_RIGHT, VK_LSHIFT};

    // Reset the input state
    IOState.KeyboardKeys.assign(ConcernedVirtualKeys.size(),
                                Acrylic::Input::ButtonState::None);

    // Create GameInput and register device callback.
    HR = GameInputCreate(GI.GetAddressOf());
    assert(SUCCEEDED(HR) && "Failed to create Microsoft.GameInput.");
    HR = GI->RegisterDeviceCallback(nullptr,
                                    GameInputKindKeyboard | GameInputKindMouse |
                                        GameInputKindGamepad,
                                    GameInputDeviceConnected,
                                    GameInputAsyncEnumeration,
                                    nullptr,
                                    OnDeviceConnectionChanged,
                                    &DeviceCallbackToken);
    assert(SUCCEEDED(HR) && "Failed to register device callback.");

    LOG_INFO("Acrylic::D3D12::Input() succeeded.");
}

void Update()
{
    UpdateKeyboardState();
    UpdateMouseState();
    UpdateGamepadState();
}
//==============================================================================
// Accessors
//==============================================================================
auto GetState() -> InputState&
{
    return IOState;
}
} // namespace Acrylic::Input
#pragma endregion