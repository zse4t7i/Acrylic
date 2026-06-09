#include "Config.hpp"

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
namespace Acrylic::Config
{
//==============================================================================
// External Function
//==============================================================================
void Load(const path& configPath)
{
    LOG_INFO("Acrylic::Config::Load() succeeded.");
}

void Save(const path& configPath)
{
    LOG_INFO("Acrylic::Config::Save() succeeded.");
}
} // namespace Acrylic::Config
#pragma endregion