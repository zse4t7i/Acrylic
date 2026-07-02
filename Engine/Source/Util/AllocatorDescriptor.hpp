#pragma once

namespace Acrylic
{
class AllocatorDescriptor
{
  public:
    void Init(ID3D12Device* device,
              D3D12_DESCRIPTOR_HEAP_TYPE type,
              U32 capacity);

    auto GetPtrHeap() const -> ID3D12DescriptorHeap*;

    auto Index2HandleCPU(U32 index) -> CD3DX12_CPU_DESCRIPTOR_HANDLE;
    auto Index2HandleGPU(U32 index) -> CD3DX12_GPU_DESCRIPTOR_HANDLE;

  private:
    ComPtr<ID3D12DescriptorHeap> mHeap;
    CD3DX12_CPU_DESCRIPTOR_HANDLE mBaseCPU{};
    CD3DX12_GPU_DESCRIPTOR_HANDLE mBaseGPU{};

    U32 mSizeDescriptor{0};
};

// For CbvSrvUav. When a resource is created, request a free bindless index.
// When a resource is destroyed, release the index so that it can be reused by
// another resource. The main idea is to somewhat automate getting CbvSrvUav
// descriptors. We do not care where the descriptor is in the heap so long as we
// have its index, we can reference it in the shader.
class AllocatorCSU : public AllocatorDescriptor
{
  public:
    void Init(ID3D12Device* device, U32 capacity);

    auto NextFreeIndex() -> U32;
    void ReleaseIndex(U32 index);

  private:
    AllocatorCSU() = default;

    std::queue<U32> mFreeIndices;

    // Used for validation. Could put in debug builds only.
    std::unordered_set<U32> mUsedIndices;
};
} // namespace Acrylic
