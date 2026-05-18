#pragma once

#include "D3D12.hpp"

#include <D3D12MemAlloc.h>
#include <DirectXCollision.h>
#include <DirectXColors.h>
#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include <directx/d3d12.h>
#include <directx/d3dx12.h>
#include <stb_image.h>

#include <windows.h>

#include <cassert>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <limits>
#include <string>
#include <string_view>
#include <vector>

namespace Acrylic::Util
{
inline auto UTF8216(std::string_view inU8,
                    std::wstring& outU16) -> bool
{
    if (inU8.empty())
    {
        outU16.clear();
        return true;
    }

    if (inU8.length() > static_cast<size_t>(std::numeric_limits<int>::max()))
    {
        return false;
    }

    const auto u8Length = static_cast<int>(inU8.length());
    const int u16Length = MultiByteToWideChar(CP_UTF8,
                                              MB_ERR_INVALID_CHARS,
                                              inU8.data(),
                                              u8Length,
                                              nullptr,
                                              0);
    if (u16Length == 0)
    {
        return false;
    }

    outU16.resize(u16Length);
    int convertedLength = MultiByteToWideChar(CP_UTF8,
                                              MB_ERR_INVALID_CHARS,
                                              inU8.data(),
                                              u8Length,
                                              outU16.data(),
                                              u16Length);
    return convertedLength == u16Length;
}

inline auto UTF1628(std::wstring_view inU16,
                    std::string& outU8) -> bool
{
    if (inU16.empty())
    {
        outU8.clear();
        return true;
    }

    if (inU16.length() > static_cast<size_t>(std::numeric_limits<int>::max()))
    {
        return false;
    }

    const auto u16Length = static_cast<int>(inU16.length());
    const int u8Length   = WideCharToMultiByte(CP_UTF8,
                                               WC_ERR_INVALID_CHARS,
                                               inU16.data(),
                                               u16Length,
                                               nullptr,
                                               0,
                                               nullptr,
                                               nullptr);
    if (u8Length == 0)
    {
        return false;
    }

    outU8.resize(u8Length);
    int convertedLength = WideCharToMultiByte(CP_UTF8,
                                              WC_ERR_INVALID_CHARS,
                                              inU16.data(),
                                              u16Length,
                                              outU8.data(),
                                              u8Length,
                                              nullptr,
                                              nullptr);
    return convertedLength == u8Length;
}

inline auto LoadBinary(const std::filesystem::path& path,
                       std::vector<std::byte>& outData) -> bool
{
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open())
    {
        return false;
    }

    auto size = file.tellg();
    outData.resize(static_cast<size_t>(size));
    file.seekg(0, std::ios::beg);

    if (!file.read(reinterpret_cast<char*>(outData.data()), size))
    {
        return false;
    }

    file.close();

    return true;
}

inline auto LoadImage(const std::filesystem::path& path,
                      std::vector<std::byte>& outData) -> bool
{
    int x{};
    int y{};
    int n{};
    unsigned char* imageData = stbi_load(path.string().c_str(), &x, &y, &n, 4);
    if (imageData == nullptr)
    {
        return false;
    }

    outData.resize(static_cast<size_t>(x) * static_cast<size_t>(y) * 4);
    memcpy(outData.data(), imageData, outData.size());
    stbi_image_free(imageData);

    return true;
}

inline void UploadBuffer(const std::vector<std::byte>& allocCPU,
                         D3D12MA::Allocation** ppAllocDefault,
                         D3D12MA::Allocation** ppAllocUpload,
                         ID3D12GraphicsCommandList* cmdList,
                         D3D12MA::Allocator* memAllocator)
{
    HRESULT hr{};

    D3D12MA::CALLOCATION_DESC descDefault{
        D3D12_HEAP_TYPE_DEFAULT,
        D3D12MA::ALLOCATION_FLAG_STRATEGY_MIN_MEMORY};
    D3D12MA::CALLOCATION_DESC descUpload{
        D3D12_HEAP_TYPE_UPLOAD,
        D3D12MA::ALLOCATION_FLAG_STRATEGY_MIN_MEMORY};

    auto descBuffer = CD3DX12_RESOURCE_DESC::Buffer(allocCPU.size());

    // Create a default buffer
    hr = memAllocator->CreateResource(&descDefault,
                                      &descBuffer,
                                      D3D12_RESOURCE_STATE_COMMON,
                                      nullptr,
                                      ppAllocDefault,
                                      IID_NULL,
                                      nullptr);
    assert(SUCCEEDED(hr) && "Failed to create a default buffer.");

    // Create a temporary upload buffer
    hr = memAllocator->CreateResource(&descUpload,
                                      &descBuffer,
                                      D3D12_RESOURCE_STATE_GENERIC_READ,
                                      nullptr,
                                      ppAllocUpload,
                                      IID_NULL,
                                      nullptr);
    assert(SUCCEEDED(hr) && "Failed to create a upload buffer.");

    // Map and copy CPU data to upload buffer
    void* pUpload{};
    hr = (*ppAllocUpload)->GetResource()->Map(0, nullptr, &pUpload);
    assert(SUCCEEDED(hr) && "Failed to map upload buffer.");

    memcpy(pUpload, allocCPU.data(), allocCPU.size());
    (*ppAllocUpload)->GetResource()->Unmap(0, nullptr);

    // Record copy command
    cmdList->CopyBufferRegion((*ppAllocDefault)->GetResource(),
                              0,
                              (*ppAllocUpload)->GetResource(),
                              0,
                              allocCPU.size());

    // Transition to read state
    CD3DX12_RESOURCE_BARRIER barrier =
        CD3DX12_RESOURCE_BARRIER::Transition((*ppAllocDefault)->GetResource(),
                                             D3D12_RESOURCE_STATE_COPY_DEST,
                                             D3D12_RESOURCE_STATE_GENERIC_READ);
    cmdList->ResourceBarrier(1, &barrier);
}

inline void UploadTexture(const std::vector<std::byte>& allocCPU,
                          D3D12MA::Allocation** ppAllocDefault,
                          D3D12MA::Allocation** ppAllocUpload,
                          ID3D12GraphicsCommandList* cmdList,
                          D3D12MA::Allocator* memAllocator,
                          int width,
                          int height)
{
    HRESULT hr{};

    D3D12MA::CALLOCATION_DESC descDefault{
        D3D12_HEAP_TYPE_DEFAULT,
        D3D12MA::ALLOCATION_FLAG_STRATEGY_MIN_MEMORY};
    D3D12MA::CALLOCATION_DESC descUpload{
        D3D12_HEAP_TYPE_UPLOAD,
        D3D12MA::ALLOCATION_FLAG_STRATEGY_MIN_MEMORY};

    auto descTexture =
        CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM, width, height);

    // Create a default buffer
    hr = memAllocator->CreateResource(&descDefault,
                                      &descTexture,
                                      D3D12_RESOURCE_STATE_COPY_DEST,
                                      nullptr,
                                      ppAllocDefault,
                                      IID_NULL,
                                      nullptr);
    assert(SUCCEEDED(hr) && "Failed to create a default texture.");

    // Create a temporary upload buffer
    const auto uploadBufferSize =
        GetRequiredIntermediateSize((*ppAllocDefault)->GetResource(), 0, 1);
    auto descBuffer = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);

    hr = memAllocator->CreateResource(&descUpload,
                                      &descBuffer,
                                      D3D12_RESOURCE_STATE_GENERIC_READ,
                                      nullptr,
                                      ppAllocUpload,
                                      IID_NULL,
                                      nullptr);
    assert(SUCCEEDED(hr) && "Failed to create a upload texture.");

    // Upload subresource
    D3D12_SUBRESOURCE_DATA subresource{};
    subresource.pData      = allocCPU.data();
    subresource.RowPitch   = static_cast<std::int64_t>(width) * 4;
    subresource.SlicePitch = subresource.RowPitch * height;

    UpdateSubresources(cmdList,
                       (*ppAllocDefault)->GetResource(),
                       (*ppAllocUpload)->GetResource(),
                       0,
                       0,
                       1,
                       &subresource);

    // Transition to read state
    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        (*ppAllocDefault)->GetResource(),
        D3D12_RESOURCE_STATE_COPY_DEST,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    cmdList->ResourceBarrier(1, &barrier);
}
} // namespace Acrylic::Util