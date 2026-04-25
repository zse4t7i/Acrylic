#pragma once

#include <directx/d3d12.h>

namespace Acrylic::Frame
{
void Init();
void WaitForGPU();
void MoveToNext();

auto GetCurrentCA() -> ID3D12CommandAllocator*;
} // namespace Acrylic::Frame