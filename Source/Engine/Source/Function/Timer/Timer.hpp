#pragma once

namespace Acrylic::Timer
{
void Init();
void Update();

auto GetDeltaTime() -> F64;
auto GetTotalTime() -> F64;
auto GetFPS() -> F32;
auto GetFrameCount() -> U64;
} // namespace Acrylic::Timer