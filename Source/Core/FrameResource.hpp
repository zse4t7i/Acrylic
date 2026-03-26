#pragma once

#include <d3d12.h>

namespace Acrylic::FrameResource
{
void Init();
void WaitForGPU();
void MoveToNext();

auto GetCurrentCA() -> ID3D12CommandAllocator*;
} // namespace Acrylic::FrameResource