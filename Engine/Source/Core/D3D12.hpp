#pragma once

namespace Acrylic::D3D12
{
//==============================================================================
// External Constexpr
//==============================================================================
inline constexpr int BUFFERCOUNT{3};
inline constexpr int FRAMECOUNT{2};
//==============================================================================
// External Function
//==============================================================================
void Init();
void WaitForCmdExecuted();
void WaitForFrameAvailable();
// Waits for the SwapChain to be ready for the next frame.
void WaitForBufferAvailable();
void Resize();
// Present w/ V-Sync.
void PresentSync();
// Present w/o V-Sync.
void PresentTear();
// void Exit();

auto GetDevice() -> ID3D12Device9 *;
auto GetCmdQueue() -> ID3D12CommandQueue *;
auto GetMemAllocator() -> D3D12MA::Allocator *;
auto GetCurrentRT() -> ID3D12Resource *;
auto GetCurrentRTV() -> D3D12_CPU_DESCRIPTOR_HANDLE;
auto GetFrameIndex() -> int;
auto GetStrideCSU() -> U32;
} // namespace Acrylic::D3D12
