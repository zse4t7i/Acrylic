#pragma once

namespace Acrylic::Engine::Input
{
enum class ButtonState : U8
{
    None = 0,
    Pressed,
    Held,
    Released,
};

struct InputState
{
    bool isKeyboardConnected{false};
    bool isMouseConnected{false};
    bool isGamepadConnected{false};

    // Keyboard keys states
    vector<ButtonState> KeyboardKeys;

    // Mouse buttons states
    ButtonState MouseL{};
    ButtonState MouseM{};
    ButtonState MouseR{};
    ButtonState Mouse4{};
    ButtonState Mouse5{};
    // Mouse axes states(positon and wheel delta)
    F32 DeltaPX{};
    F32 DeltaPY{};
    F32 DeltaWY{};

    // Gamepad buttons states
    ButtonState GamePadMenu{};
    ButtonState GamePadView{};
    ButtonState GamePadA{};
    ButtonState GamePadB{};
    ButtonState GamePadX{};
    ButtonState GamePadY{};
    ButtonState GamePadDPadUp{};
    ButtonState GamePadDPadDown{};
    ButtonState GamePadDPadLeft{};
    ButtonState GamePadDPadRight{};
    ButtonState GamePadLB{};
    ButtonState GamePadRB{};
    ButtonState GamePadLS{};
    ButtonState GamePadRS{};
    // Gamepad axes states(left stick, right stick, triggers delta)
    F32 DeltaLT{};
    F32 DeltaRT{};
    F32 DeltaLSX{};
    F32 DeltaLSY{};
    F32 DeltaRSX{};
    F32 DeltaRSY{};
};

//==============================================================================
// External Function
//==============================================================================
void Init();
void Update();

auto GetState() -> InputState&;
} // namespace Acrylic::Engine::Input