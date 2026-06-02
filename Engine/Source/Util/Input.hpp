#pragma once

namespace Acrylic::Input
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

    //Keyboard keys states
    vector<ButtonState> KeyboardKeys;

    //Mouse buttons states
    ButtonState MouseL{};
    ButtonState MouseM{};
    ButtonState MouseR{};
    ButtonState Mouse4{};
    ButtonState Mouse5{};
    // Mouse axes states(positon and wheel delta)
    FP32 DeltaPX{};
    FP32 DeltaPY{};
    FP32 DeltaWY{};

    //Gamepad buttons states
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
    FP32 DeltaLT{};
    FP32 DeltaRT{};
    FP32 DeltaLSX{};
    FP32 DeltaLSY{};
    FP32 DeltaRSX{};
    FP32 DeltaRSY{};
};

//==============================================================================
// External Function
//==============================================================================
void Init();
void Update();

auto GetState() -> InputState&;
} // namespace Acrylic::Input