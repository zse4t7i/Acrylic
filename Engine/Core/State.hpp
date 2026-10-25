#pragma once

namespace Acrylic::State
{
struct Runtime
{
};

enum class Button : U8
{
    None = 0,
    Pressed,
    Held,
    Released,
};

struct Input
{
    bool isKeyboardConnected{false};
    bool isMouseConnected{false};
    bool isGamepadConnected{false};

    // Keyboard keys states
    vector<Button> KeyboardKeys;

    // Mouse buttons states
    Button MouseL{};
    Button MouseM{};
    Button MouseR{};
    Button Mouse4{};
    Button Mouse5{};
    // Mouse axes states(positon and wheel delta)
    F32 DeltaPX{};
    F32 DeltaPY{};
    F32 DeltaWY{};

    // Gamepad buttons states
    Button GamePadMenu{};
    Button GamePadView{};
    Button GamePadA{};
    Button GamePadB{};
    Button GamePadX{};
    Button GamePadY{};
    Button GamePadDPadUp{};
    Button GamePadDPadDown{};
    Button GamePadDPadLeft{};
    Button GamePadDPadRight{};
    Button GamePadLB{};
    Button GamePadRB{};
    Button GamePadLS{};
    Button GamePadRS{};
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

inline auto GetRefRuntime() -> Runtime&
{
    static Runtime Instance{};
    return Instance;
}

inline auto GetRefInput() -> Input&
{
    static Input Instance{};
    return Instance;
}
} // namespace Acrylic::State
