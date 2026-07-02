#include "AllocatorDescriptor.hpp"

namespace Acrylic
{

void AllocatorDescriptor::Init(ID3D12Device* device,
                               D3D12_DESCRIPTOR_HEAP_TYPE type,
                               UINT capacity)
{
    assert(mHeap == nullptr);

    D3D12_DESCRIPTOR_HEAP_DESC heapDesc;
    heapDesc.NumDescriptors = capacity;
    heapDesc.Type           = type;
    heapDesc.Flags          = type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV ||
                                      type == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER
                                  ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE
                                  : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    heapDesc.NodeMask       = 0;

    HRESULT hr =
        device->CreateDescriptorHeap(&heapDesc,
                                     IID_PPV_ARGS(mHeap.GetAddressOf()));
    assert(SUCCEEDED(hr) && "Failed to create descriptor heap.");

    mSizeDescriptor = device->GetDescriptorHandleIncrementSize(type);
}

auto AllocatorDescriptor::GetPtrHeap() const -> ID3D12DescriptorHeap*
{
    return mHeap.Get();
}

auto AllocatorDescriptor::Index2HandleCPU(uint32_t index)
    -> CD3DX12_CPU_DESCRIPTOR_HANDLE
{
    auto hcpu = CD3DX12_CPU_DESCRIPTOR_HANDLE(
        mHeap->GetCPUDescriptorHandleForHeapStart());
    hcpu.Offset(index, mSizeDescriptor);
    return hcpu;
}

auto AllocatorDescriptor::Index2HandleGPU(uint32_t index)
    -> CD3DX12_GPU_DESCRIPTOR_HANDLE
{
    auto hgpu = CD3DX12_GPU_DESCRIPTOR_HANDLE(
        mHeap->GetGPUDescriptorHandleForHeapStart());
    hgpu.Offset(index, mSizeDescriptor);
    return hgpu;
}

void AllocatorCSU::Init(ID3D12Device* device, UINT capacity)
{
    AllocatorDescriptor::Init(device,
                              D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
                              capacity);

    for (UINT i = 0; i < capacity; ++i)
    {
        mFreeIndices.push(i);
    }

    mUsedIndices.clear();
}

uint32_t AllocatorCSU::NextFreeIndex()
{
    assert(!mFreeIndices.empty());

    const uint32_t index = mFreeIndices.front();

    mUsedIndices.insert(index);

    mFreeIndices.pop();

    return index;
}

void AllocatorCSU::ReleaseIndex(uint32_t index)
{
    // If a resource is destroyed, we can reuse its index.

    auto it = mUsedIndices.find(index);

    // Make sure we are releasing a used index.
    assert(it != std::end(mUsedIndices));

    mUsedIndices.erase(it);

    mFreeIndices.push(index);
}



} // namespace Acrylic
