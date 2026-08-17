#pragma once

#include "State.hpp"

namespace Acrylic::Engine::Input
{
//==============================================================================
// External Function
//==============================================================================
void Init();
void Update();

auto GetState() -> State::StateInput&;
} // namespace Acrylic::Engine::Input