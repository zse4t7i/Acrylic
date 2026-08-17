#pragma once

namespace Acrylic::Engine::State
{
struct StateRuntime
{
};

enum class StateButton : U8
{
    None = 0,
    Pressed,
    Held,
    Released,
};

struct StateInput
{
    bool isKeyboardConnected{false};
    bool isMouseConnected{false};
    bool isGamepadConnected{false};

    // Keyboard keys states
    vector<StateButton> KeyboardKeys;

    // Mouse buttons states
    StateButton MouseL{};
    StateButton MouseM{};
    StateButton MouseR{};
    StateButton Mouse4{};
    StateButton Mouse5{};
    // Mouse axes states(positon and wheel delta)
    F32 DeltaPX{};
    F32 DeltaPY{};
    F32 DeltaWY{};

    // Gamepad buttons states
    StateButton GamePadMenu{};
    StateButton GamePadView{};
    StateButton GamePadA{};
    StateButton GamePadB{};
    StateButton GamePadX{};
    StateButton GamePadY{};
    StateButton GamePadDPadUp{};
    StateButton GamePadDPadDown{};
    StateButton GamePadDPadLeft{};
    StateButton GamePadDPadRight{};
    StateButton GamePadLB{};
    StateButton GamePadRB{};
    StateButton GamePadLS{};
    StateButton GamePadRS{};
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

inline auto GetRefStateEnv() -> StateRuntime&
{
    static StateRuntime StateEnv{};
    return StateEnv;
}
} // namespace Acrylic::Engine::State
