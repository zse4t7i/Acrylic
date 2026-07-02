#pragma once

namespace Acrylic::State
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
} // namespace Acrylic::State
