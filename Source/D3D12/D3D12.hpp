#pragma once

#include <directx/d3d12.h>
#include <directx/d3dx12.h>

#include <dxgi1_6.h>
#include <wrl/client.h>

#include <cstdint>

namespace Acrylic::D3D12
{
constexpr int FRAMECOUNT{2};
constexpr int TEXELSIZE{4};
constexpr int TEXTUREWIDTH{1024};
constexpr int TEXTUREHEIGHT{1024};

extern std::string GPUName;

// extern HWND HWnd;
// extern std::uint16_t Width;
// extern std::uint16_t Height;

// extern CD3DX12_VIEWPORT Viewport;
// extern CD3DX12_RECT ScissorRect;
// extern std::uint32_t OffsetRTV;
// extern Microsoft::WRL::ComPtr<ID3D12Device9> Device;
// extern Microsoft::WRL::ComPtr<ID3D12CommandQueue>
// CmdQueue; extern
// Microsoft::WRL::ComPtr<ID3D12CommandAllocator>
//     CmdAlloc;
// extern Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList6>
//     CmdList;
// extern Microsoft::WRL::ComPtr<IDXGISwapChain4> SwapChain;
// extern Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>
// HeapRTV; extern
// std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>>
//     BuffersRT;
// extern Microsoft::WRL::ComPtr<ID3D12PipelineState>
//     PipelineState;
// extern Microsoft::WRL::ComPtr<ID3D12RootSignature>
// RootSign;

// extern Microsoft::WRL::ComPtr<ID3D12Resource> BufferV;
// extern D3D12_VERTEX_BUFFER_VIEW BufferViewV;
//
// extern std::uint32_t FrameIndex;
// extern Microsoft::WRL::ComPtr<ID3D12Fence1> Fence;
// extern std::uint64_t FenceValue;
// extern HANDLE FenceEvent;

void Init(HINSTANCE hInst,
          int nShowCmd);
void Update();
void Render();
void Destroy();
} // namespace Suiko::Device
