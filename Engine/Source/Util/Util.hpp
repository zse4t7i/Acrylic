#pragma once

#include <stb_image.h>

namespace Acrylic::Util
{
inline auto UTF8216(string_view inUTF8, wstring &outUTF16) -> bool
{
    if (inUTF8.empty())
    {
        outUTF16.clear();
        return true;
    }

    if (inUTF8.length() > I32_MAX)
    {
        return false;
    }

    const auto u8Length = static_cast<int>(inUTF8.length());
    const int u16Length = MultiByteToWideChar(
        CP_UTF8, MB_ERR_INVALID_CHARS, inUTF8.data(), u8Length, nullptr, 0);
    if (u16Length == 0)
    {
        return false;
    }

    outUTF16.resize(u16Length);
    int convertedLength =
        MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, inUTF8.data(),
                            u8Length, outUTF16.data(), u16Length);
    return convertedLength == u16Length;
}

inline auto UTF1628(wstring_view inUTF16, string &outUTF8) -> bool
{
    if (inUTF16.empty())
    {
        outUTF8.clear();
        return true;
    }

    if (inUTF16.length() > I32_MAX)
    {
        return false;
    }

    const auto u16Length = static_cast<int>(inUTF16.length());
    const int u8Length =
        WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, inUTF16.data(),
                            u16Length, nullptr, 0, nullptr, nullptr);
    if (u8Length == 0)
    {
        return false;
    }

    outUTF8.resize(u8Length);
    int convertedLength = WideCharToMultiByte(
        CP_UTF8, WC_ERR_INVALID_CHARS, inUTF16.data(), u16Length,
        outUTF8.data(), u8Length, nullptr, nullptr);
    return convertedLength == u8Length;
}

inline auto LoadBinary(const path &path, vector<Byte> &outData) -> bool
{
    ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open())
    {
        return false;
    }

    auto size = file.tellg();
    outData.resize(static_cast<size_t>(size));
    file.seekg(0, std::ios::beg);

    if (!file.read(reinterpret_cast<char *>(outData.data()), size))
    {
        return false;
    }

    file.close();

    return true;
}

inline auto LoadImage(const path &path, vector<Byte> &outData) -> bool
{
    int x{};
    int y{};
    int n{};
    unsigned char *imageData = stbi_load(path.string().c_str(), &x, &y, &n, 4);
    if (imageData == nullptr)
    {
        return false;
    }

    outData.resize(static_cast<size_t>(x) * static_cast<size_t>(y) * 4);
    memcpy(outData.data(), imageData, outData.size());
    stbi_image_free(imageData);

    return true;
}

inline void UploadBuffer(const vector<Byte> &allocCPU,
                         D3D12MA::Allocation **ppAllocDefault,
                         D3D12MA::Allocation **ppAllocUpload,
                         ID3D12GraphicsCommandList *cmdList,
                         D3D12MA::Allocator *memAllocator)
{
    HRESULT hr{};

    D3D12MA::CALLOCATION_DESC descDefault{
        D3D12_HEAP_TYPE_DEFAULT, D3D12MA::ALLOCATION_FLAG_STRATEGY_MIN_MEMORY};
    D3D12MA::CALLOCATION_DESC descUpload{
        D3D12_HEAP_TYPE_UPLOAD, D3D12MA::ALLOCATION_FLAG_STRATEGY_MIN_MEMORY};

    auto descBuffer = CD3DX12_RESOURCE_DESC::Buffer(allocCPU.size());

    // Create a default buffer
    hr = memAllocator->CreateResource(&descDefault, &descBuffer,
                                      D3D12_RESOURCE_STATE_COMMON, nullptr,
                                      ppAllocDefault, IID_NULL, nullptr);
    assert(SUCCEEDED(hr) && "Failed to create a default buffer.");

    // Create a temporary upload buffer
    hr = memAllocator->CreateResource(
        &descUpload, &descBuffer, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
        ppAllocUpload, IID_NULL, nullptr);
    assert(SUCCEEDED(hr) && "Failed to create a upload buffer.");

    // Map and copy CPU data to upload buffer
    void *pUpload{};
    hr = (*ppAllocUpload)->GetResource()->Map(0, nullptr, &pUpload);
    assert(SUCCEEDED(hr) && "Failed to map upload buffer.");

    memcpy(pUpload, allocCPU.data(), allocCPU.size());
    (*ppAllocUpload)->GetResource()->Unmap(0, nullptr);

    // Record copy command
    cmdList->CopyBufferRegion((*ppAllocDefault)->GetResource(), 0,
                              (*ppAllocUpload)->GetResource(), 0,
                              allocCPU.size());

    // Transition to read state
    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        (*ppAllocDefault)->GetResource(), D3D12_RESOURCE_STATE_COPY_DEST,
        D3D12_RESOURCE_STATE_GENERIC_READ);
    cmdList->ResourceBarrier(1, &barrier);
}

inline void UploadTexture(const vector<Byte> &allocCPU,
                          D3D12MA::Allocation **ppAllocDefault,
                          D3D12MA::Allocation **ppAllocUpload,
                          ID3D12GraphicsCommandList *cmdList,
                          D3D12MA::Allocator *memAllocator, int width,
                          int height)
{
    HRESULT hr{};

    D3D12MA::CALLOCATION_DESC descDefault{
        D3D12_HEAP_TYPE_DEFAULT, D3D12MA::ALLOCATION_FLAG_STRATEGY_MIN_MEMORY};
    D3D12MA::CALLOCATION_DESC descUpload{
        D3D12_HEAP_TYPE_UPLOAD, D3D12MA::ALLOCATION_FLAG_STRATEGY_MIN_MEMORY};

    auto descTexture =
        CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM, width, height);

    // Create a default buffer
    hr = memAllocator->CreateResource(&descDefault, &descTexture,
                                      D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
                                      ppAllocDefault, IID_NULL, nullptr);
    assert(SUCCEEDED(hr) && "Failed to create a default texture.");

    // Create a temporary upload buffer
    const auto uploadBufferSize =
        GetRequiredIntermediateSize((*ppAllocDefault)->GetResource(), 0, 1);
    auto descBuffer = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);

    hr = memAllocator->CreateResource(
        &descUpload, &descBuffer, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
        ppAllocUpload, IID_NULL, nullptr);
    assert(SUCCEEDED(hr) && "Failed to create a upload texture.");

    // Upload subresource
    D3D12_SUBRESOURCE_DATA subresource{};
    subresource.pData = allocCPU.data();
    subresource.RowPitch = static_cast<I64>(width) * 4;
    subresource.SlicePitch = subresource.RowPitch * height;

    UpdateSubresources(cmdList, (*ppAllocDefault)->GetResource(),
                       (*ppAllocUpload)->GetResource(), 0, 0, 1, &subresource);

    // Transition to read state
    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        (*ppAllocDefault)->GetResource(), D3D12_RESOURCE_STATE_COPY_DEST,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    cmdList->ResourceBarrier(1, &barrier);
}
} // namespace Acrylic::Util