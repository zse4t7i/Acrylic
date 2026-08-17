#include "State.hpp"
#include "Log.hpp"

#pragma region Internal
namespace
{
//==============================================================================
// Internal Variable
//==============================================================================
Acrylic::Engine::State::StateInput IOState{};

//==============================================================================
// Internal Function
//==============================================================================

} // namespace
#pragma endregion

#pragma region External
namespace Acrylic::Engine::State
{
//==============================================================================
// External Function
//==============================================================================
void Init()
{
    LOG_INFO("Acrylic::Engine::State::Init() succeeded.");
}
} // namespace Acrylic::Engine::State
#pragma endregion