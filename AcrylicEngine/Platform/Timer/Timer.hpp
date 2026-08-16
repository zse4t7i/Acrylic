#pragma once

namespace Acrylic::Engine::Timer
{
void Init();
void Update();

auto GetFrameTime() -> F64;
auto GetTotalTime() -> F64;
auto GetFPS() -> F32;
auto GetFrameCount() -> U64;
} // namespace Acrylic::Engine::Timer