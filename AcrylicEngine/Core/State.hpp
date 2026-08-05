#pragma once

namespace Acrylic::Engine::State
{
struct StateRuntimeEnvironment
{
};

//==============================================================================
// External Function
//==============================================================================
void Init();

inline auto GetRefStateEnv() -> StateRuntimeEnvironment&
{
    static StateRuntimeEnvironment StateEnv{};
    return StateEnv;
}
} // namespace Acrylic::Engine::State
