#pragma once

namespace Acrylic::Engine
{
struct AllocationDynamic
{
    void* AddressCPU{};
    D3D12_GPU_VIRTUAL_ADDRESS AddressGPU{};
};

// One instance per Acrylic::Engine::D3D12::FRAMECOUNT.
// Reset() is only safe to call after
// Acrylic::Engine::D3D12::WaitForFrameResourceAvailable().
class AllocatorDynamic
{
  public:
    void Init(D3D12MA::Allocator* alctrGPU, U32 size = 64 * 1024);

    // Alignment defaults to CBV alignment (256). Pass 4 for index data,
    // 16 for vertex data if you want tighter packing for dynamic geometry.
    auto Allocate(U32 size, U32 alignment = 256) -> AllocationDynamic;

    void Reset()
    {
        mAllocSize = 0;
    };

  private:
    ComPtr<D3D12MA::Allocation> mAlloc;
    U32 mAllocCapacity{0};
    U32 mAllocSize{0};

    void* mBaseCPU{};
    D3D12_GPU_VIRTUAL_ADDRESS mBaseGPU{};
};

inline void AllocatorDynamic::Init(D3D12MA::Allocator* alctrGPU, U32 size)
{
    mAllocCapacity = size;

    D3D12MA::CALLOCATION_DESC descUpload{
        D3D12_HEAP_TYPE_UPLOAD,
        D3D12MA::ALLOCATION_FLAG_STRATEGY_MIN_MEMORY};
    auto descRes = CD3DX12_RESOURCE_DESC::Buffer(mAllocCapacity);

    if (alctrGPU->IsGPUUploadHeapSupported())
    {
        descUpload.HeapType = D3D12_HEAP_TYPE_GPU_UPLOAD;
    }

    HRESULT hr = alctrGPU->CreateResource(&descUpload,
                                          &descRes,
                                          D3D12_RESOURCE_STATE_GENERIC_READ,
                                          nullptr,
                                          mAlloc.GetAddressOf(),
                                          IID_NULL,
                                          nullptr);
    assert(SUCCEEDED(hr) && "Failed to create allocDynamic buffer.");

    CD3DX12_RANGE readRange{0, 0};
    hr = mAlloc->GetResource()->Map(0, &readRange, &mBaseCPU);
    assert(SUCCEEDED(hr) && "Failed to map allocDynamic buffer.");

    mBaseGPU = mAlloc->GetResource()->GetGPUVirtualAddress();

    assert(reinterpret_cast<U64>(mBaseCPU) %
                   D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT ==
               0 &&
           "AllocatorDynamic::mBaseCPU is not aligned to "
           "D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT.");
    assert(mBaseGPU % D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT == 0 &&
           "AllocatorDynamic::mBaseGPU is not aligned to "
           "D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT.");
};

inline auto AllocatorDynamic::Allocate(U32 size, U32 alignment)
    -> AllocationDynamic
{
    U32 alignedAllocSize = (mAllocSize + alignment - 1) & ~(alignment - 1);

    assert(alignedAllocSize + size <= mAllocCapacity &&
           "AllocatorDynamic exhausted for this frame slot — grow "
           "mAllocCapacity or allocate less per frame.");

    AllocationDynamic alloc{
        .AddressCPU{static_cast<Byte*>(mBaseCPU) + alignedAllocSize},
        .AddressGPU{mBaseGPU + alignedAllocSize},
    };
    mAllocSize = alignedAllocSize + size;
    return alloc;
};
} // namespace Acrylic::Engine
