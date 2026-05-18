#include "Renderer.hpp"
#include "D3D12.hpp"
#include "Frame.hpp"
#include "Log.hpp"
#include "Util.hpp"
#include "Window.hpp"

#include <D3D12MemAlloc.h>
#include <DirectXCollision.h>
#include <DirectXColors.h>
#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include <directx/d3d12.h>
#include <directx/d3dx12.h>

#include <windows.h>
#include <wrl/client.h>

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <vector>

using Microsoft::WRL::ComPtr;

namespace
{
//==============================================================================
// Variable
//==============================================================================
struct Vertex
{
    DirectX::XMFLOAT3 Position{};
    DirectX::XMFLOAT2 UV{};
};

struct ConstantBuffer
{
    DirectX::XMFLOAT4 Offset{};
    std::array<float, 60> padding{};
};
static_assert((sizeof(ConstantBuffer) % 256) == 0,
              "Constant Buffer size must be 256-byte aligned");

HRESULT hr{};
bool br{};

ID3D12Device9* Device;
D3D12MA::Allocator* MemAllocator;
ID3D12CommandQueue* CmdQueue;
ID3D12GraphicsCommandList6* CmdList;

ComPtr<ID3D12RootSignature> RS{};
ComPtr<ID3D12PipelineState> PSO{};

ComPtr<ID3D12DescriptorHeap> HeapCSU{};
std::uint32_t OffsetCSU{0};
D3D12_VERTEX_BUFFER_VIEW VBV{};
ComPtr<D3D12MA::Allocation> AllocVB{};
ComPtr<D3D12MA::Allocation> AllocCB{};
ComPtr<D3D12MA::Allocation> AllocTexture{};
ConstantBuffer CBData{};
void* pCBMemBegin{};

//==============================================================================
// Function
//=============================================================================

void populateCmdList()
{

}
} // namespace

namespace Acrylic::Renderer
{
void Init()
{

    LOG_INFO("Acrylic::Renderer::Init() succeeded.");
}
void Update()
{
    const float translationSpeed = 0.005f;
    const float offsetBounds     = 1.25f;

    CBData.Offset.x += translationSpeed;
    if (CBData.Offset.x > offsetBounds)
    {
        CBData.Offset.x = -offsetBounds;
    }
    memcpy(pCBMemBegin, &CBData, sizeof(CBData));
}

void Render()
{

}
} // namespace Acrylic::Renderer