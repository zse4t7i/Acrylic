#pragma once

namespace Acrylic
{
class AllocatorCSU
{
  public:
    void Init(ID3D12Device* device, U32 capacity);

    auto GetPtrHeap() -> ID3D12DescriptorHeap*
    {
        assert(mHeap != nullptr && "AllocatorCSU hasn't been initialized.");
        return mHeap.Get();
    };

    auto Index2HandleCPU(U32 index) -> D3D12_CPU_DESCRIPTOR_HANDLE
    {
        return CD3DX12_CPU_DESCRIPTOR_HANDLE(mBaseCPU,
                                             static_cast<I32>(index),
                                             mSizeOfDescriptor);
    };
    auto Index2HandleGPU(U32 index) -> D3D12_GPU_DESCRIPTOR_HANDLE
    {

        return CD3DX12_GPU_DESCRIPTOR_HANDLE(mBaseGPU,
                                             static_cast<I32>(index),
                                             mSizeOfDescriptor);
    };
    auto HandleCPU2Index(D3D12_CPU_DESCRIPTOR_HANDLE handleCPU) const -> U32
    {
        return static_cast<U32>((handleCPU.ptr - mBaseCPU.ptr) /
                                mSizeOfDescriptor);
    };
    auto HandleGPU2Index(D3D12_GPU_DESCRIPTOR_HANDLE handleGPU) const -> U32
    {
        return static_cast<U32>((handleGPU.ptr - mBaseGPU.ptr) /
                                mSizeOfDescriptor);
    };

    auto AllocateIndex() -> U32;
    void FreeIndex(U32 index);

  private:
    ComPtr<ID3D12DescriptorHeap> mHeap;
    D3D12_CPU_DESCRIPTOR_HANDLE mBaseCPU{};
    D3D12_GPU_DESCRIPTOR_HANDLE mBaseGPU{};

    U32 mSizeOfDescriptor{0};

    queue<U32> mUsableIndices;
    unordered_set<U32> mUsedIndices;
};

inline void AllocatorCSU::Init(ID3D12Device* device, U32 capacity)
{
    assert(mHeap == nullptr && "AllocatorCSU has already been initialized.");

    D3D12_DESCRIPTOR_HEAP_DESC descHeap{
        .Type{D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV},
        .NumDescriptors{capacity},
        .Flags{D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE}};

    HRESULT hr =
        device->CreateDescriptorHeap(&descHeap,
                                     IID_PPV_ARGS(mHeap.GetAddressOf()));
    assert(SUCCEEDED(hr) && "Failed to create descriptor heap.");

    mBaseCPU = mHeap->GetCPUDescriptorHandleForHeapStart();
    mBaseGPU = mHeap->GetGPUDescriptorHandleForHeapStart();

    mSizeOfDescriptor = device->GetDescriptorHandleIncrementSize(
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    for (auto i = 0; i < capacity; i++)
    {
        mUsableIndices.push(i);
    }

    mUsedIndices.clear();
}

inline auto AllocatorCSU::AllocateIndex() -> U32
{
    assert(!mUsableIndices.empty() && "Out of index.");

    auto index = mUsableIndices.front();
    mUsableIndices.pop();

    mUsedIndices.insert(index);

    return index;
}

inline void AllocatorCSU::FreeIndex(U32 index)
{
    auto it = mUsedIndices.find(index);

    assert(it != mUsedIndices.end() && "Index is already freed.");

    mUsedIndices.erase(it);

    mUsableIndices.push(index);
}
} // namespace Acrylic
