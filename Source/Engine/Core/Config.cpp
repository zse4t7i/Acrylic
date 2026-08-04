#include "Config.hpp"
#include "Log.hpp"

#pragma region Internal
namespace
{
//==============================================================================
// Internal Variable
//==============================================================================
// Return result for assertion.
HRESULT HR{};
bool BR{};

//==============================================================================
// Internal Function
//==============================================================================

} // namespace
#pragma endregion

#pragma region External
namespace Acrylic::Engine::Config
{
//==============================================================================
// External Function
//==============================================================================
void Load(const Path& configPath)
{
    LOG_INFO("Acrylic::Engine::Config::Load() succeeded.");
}

void Save(const Path& configPath)
{
    LOG_INFO("Acrylic::Engine::Config::Save() succeeded.");
}
} // namespace Acrylic::Engine::Config
#pragma endregion