#include "FrameResource.hpp"
#include "D3D12.hpp"
#include "Log.hpp"

#include <d3d12.h>
#include <d3dx12/d3dx12.h>

#include <wrl/client.h>

#include <array>
#include <cassert>
#include <cstdint>

using Microsoft::WRL::ComPtr;

namespace
{
//==============================================================================
// Variable
//==============================================================================
HRESULT hr{};
bool br{};

HANDLE EventFence{};
ComPtr<ID3D12Fence1> Fence{};

constexpr int FRAMECOUNT{2};
std::array<ComPtr<ID3D12CommandAllocator>, FRAMECOUNT> FrameCAs{};
std::array<std::uint64_t, FRAMECOUNT> FrameFVs{0, 0};
int IndexFrame{0};
//==============================================================================
// Function
//==============================================================================

} // namespace

namespace Acrylic::FrameResource
{
void Init()
{
    for (int i = 0; i < 2; i++)
    {
        hr = Acrylic::D3D12::GetDevice()->CreateCommandAllocator(
            D3D12_COMMAND_LIST_TYPE_DIRECT,
            IID_PPV_ARGS(FrameCAs[i].GetAddressOf()));
        assert(SUCCEEDED(hr) && "Failed to create command allocator.");
    }

    hr = Acrylic::D3D12::GetDevice()->CreateFence(
        FrameFVs[IndexFrame]++,
        D3D12_FENCE_FLAG_NONE,
        IID_PPV_ARGS(Fence.GetAddressOf()));
    assert(SUCCEEDED(hr) && "Failed to create fence.");

    EventFence = CreateEventW(nullptr, false, false, nullptr);
    assert(EventFence != nullptr && "Failed to create fence event.");

    LOG_INFO("Acrylic::FrameResource::Init() succeeded.");
}

void WaitForGPU()
{
    hr = Acrylic::D3D12::GetCmdQueue()->Signal(Fence.Get(),
                                               FrameFVs[IndexFrame]);
    assert(SUCCEEDED(hr) && "Failed to signal command queue.");

    hr = Fence->SetEventOnCompletion(FrameFVs[IndexFrame]++, EventFence);
    assert(SUCCEEDED(hr) && "Failed to set event on completion.");
    WaitForSingleObject(EventFence, INFINITE);
}

void MoveToNext()
{
    const auto currentFV = FrameFVs[IndexFrame];

    hr = Acrylic::D3D12::GetCmdQueue()->Signal(Fence.Get(), currentFV);
    assert(SUCCEEDED(hr) && "Failed to signal command queue.");

    IndexFrame = (IndexFrame + 1) % FRAMECOUNT;

    if (Fence->GetCompletedValue() < FrameFVs[IndexFrame])
    {
        hr = Fence->SetEventOnCompletion(FrameFVs[IndexFrame], EventFence);
        assert(SUCCEEDED(hr) && "Failed to set event on completion.");
        WaitForSingleObject(EventFence, INFINITE);
    }

    FrameFVs[IndexFrame] = currentFV + 1;
}
//==============================================================================
// Accessors
//==============================================================================
auto GetCurrentCA() -> ID3D12CommandAllocator*
{
    return FrameCAs[IndexFrame].Get();
}
} // namespace Acrylic::FrameResource