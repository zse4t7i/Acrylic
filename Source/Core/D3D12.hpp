#pragma once

#include <D3D12MemAlloc.h>
#include <d3d12.h>

namespace Acrylic::D3D12
{
void Init();
// Waits for the SwapChain to be ready for the next frame.
void WaitForSwapChain();
void Resize();
// Present w/ V-Sync.
void PresentSync();
// Present w/o V-Sync.
void PresentTear();

auto GetDevice() -> ID3D12Device9*;
auto GetMemAllocator() -> D3D12MA::Allocator*;
auto GetCmdQueue() -> ID3D12CommandQueue*;
auto GetCmdList() -> ID3D12GraphicsCommandList6*;
auto GetCurrentRT() -> ID3D12Resource*;
auto GetCurrentRTV() -> D3D12_CPU_DESCRIPTOR_HANDLE;

} // namespace Acrylic::D3D12
