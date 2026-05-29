#pragma once

namespace Acrylic::Timer
{
void Init();
void Update();

auto GetDeltaTime() -> FP64;
auto GetTotalTime() -> FP64;
auto GetFPS() -> FP32;
auto GetFrameCount() -> U64;
}