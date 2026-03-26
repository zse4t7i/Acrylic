#pragma once

namespace Acrylic::Scene
{
//==============================================================================
// Variable
//==============================================================================
inline constexpr int TEXELSIZE{4};
inline constexpr int TEXTUREWIDTH{1024};
inline constexpr int TEXTUREHEIGHT{1024};

//==============================================================================
// Function
//==============================================================================
void Init();
void Update();
void Render();
} // namespace Acrylic::Scene