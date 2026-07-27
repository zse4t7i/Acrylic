#pragma once

#include <stb_image.h>

namespace Acrylic::Engine::Util
{
inline auto UTF8216(string_view inUTF8, wstring& outUTF16) -> bool
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
    const int u16Length = MultiByteToWideChar(CP_UTF8,
                                              MB_ERR_INVALID_CHARS,
                                              inUTF8.data(),
                                              u8Length,
                                              nullptr,
                                              0);
    if (u16Length == 0)
    {
        return false;
    }

    outUTF16.resize(u16Length);
    int convertedLength = MultiByteToWideChar(CP_UTF8,
                                              MB_ERR_INVALID_CHARS,
                                              inUTF8.data(),
                                              u8Length,
                                              outUTF16.data(),
                                              u16Length);
    return convertedLength == u16Length;
}

inline auto UTF1628(wstring_view inUTF16, string& outUTF8) -> bool
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
    const int u8Length   = WideCharToMultiByte(CP_UTF8,
                                               WC_ERR_INVALID_CHARS,
                                               inUTF16.data(),
                                               u16Length,
                                               nullptr,
                                               0,
                                               nullptr,
                                               nullptr);
    if (u8Length == 0)
    {
        return false;
    }

    outUTF8.resize(u8Length);
    int convertedLength = WideCharToMultiByte(CP_UTF8,
                                              WC_ERR_INVALID_CHARS,
                                              inUTF16.data(),
                                              u16Length,
                                              outUTF8.data(),
                                              u8Length,
                                              nullptr,
                                              nullptr);
    return convertedLength == u8Length;
}

inline auto LoadBinary(const Path& path, vector<Byte>& outData) -> bool
{
    ifstream file(path, std::ios::binary | std::ios::ate);
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

inline auto LoadImage(const Path& path,
                      vector<Byte>& outData,
                      int& outWidth,
                      int& outHeight) -> bool
{
    int n{};
    unsigned char* imageData =
        stbi_load(path.string().c_str(), &outWidth, &outHeight, &n, 4);
    if (imageData == nullptr)
    {
        return false;
    }

    outData.resize(static_cast<size_t>(outWidth) *
                   static_cast<size_t>(outHeight) * 4);
    memcpy(outData.data(), imageData, outData.size());
    stbi_image_free(imageData);

    return true;
}

inline void CreateRTV2D(ID3D12Device* device,
                        ID3D12Resource* resource,
                        DXGI_FORMAT format,
                        U32 mipSlice,
                        D3D12_CPU_DESCRIPTOR_HANDLE hDescriptor)
{
    D3D12_RENDER_TARGET_VIEW_DESC descRTV{};
    descRTV.ViewDimension        = D3D12_RTV_DIMENSION_TEXTURE2D;
    descRTV.Format               = format;
    descRTV.Texture2D.MipSlice   = mipSlice;
    descRTV.Texture2D.PlaneSlice = 0;
    device->CreateRenderTargetView(resource, &descRTV, hDescriptor);
}

inline void CreateDSV(ID3D12Device* device,
                      ID3D12Resource* resource,
                      D3D12_DSV_FLAGS flags,
                      DXGI_FORMAT format,
                      U32 mipSlice,
                      D3D12_CPU_DESCRIPTOR_HANDLE hDescriptor)
{
    D3D12_DEPTH_STENCIL_VIEW_DESC descDSV{};
    descDSV.Flags              = flags;
    descDSV.ViewDimension      = D3D12_DSV_DIMENSION_TEXTURE2D;
    descDSV.Format             = format;
    descDSV.Texture2D.MipSlice = mipSlice;
    device->CreateDepthStencilView(resource, &descDSV, hDescriptor);
}

inline void CreateSRV2D(ID3D12Device* device,
                        ID3D12Resource* resource,
                        DXGI_FORMAT format,
                        U32 mipLevels,
                        D3D12_CPU_DESCRIPTOR_HANDLE hDescriptor)
{
    D3D12_SHADER_RESOURCE_VIEW_DESC descSRV{};
    descSRV.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    descSRV.ViewDimension           = D3D12_SRV_DIMENSION_TEXTURE2D;
    descSRV.Texture2D.MostDetailedMip     = 0;
    descSRV.Texture2D.ResourceMinLODClamp = 0.0F;
    descSRV.Format                        = format;
    descSRV.Texture2D.MipLevels           = mipLevels;
    device->CreateShaderResourceView(resource, &descSRV, hDescriptor);
}

inline void CreateSRV2DArray(ID3D12Device* device,
                             ID3D12Resource* resource,
                             DXGI_FORMAT format,
                             U32 mipLevels,
                             U32 arraySize,
                             D3D12_CPU_DESCRIPTOR_HANDLE hDescriptor)
{
    D3D12_SHADER_RESOURCE_VIEW_DESC descSRV{};
    descSRV.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    descSRV.ViewDimension           = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
    descSRV.Texture2DArray.MostDetailedMip     = 0;
    descSRV.Texture2DArray.FirstArraySlice     = 0;
    descSRV.Texture2DArray.ArraySize           = arraySize;
    descSRV.Texture2DArray.ResourceMinLODClamp = 0.0F;
    descSRV.Format                             = format;
    descSRV.Texture2DArray.MipLevels           = mipLevels;
    device->CreateShaderResourceView(resource, &descSRV, hDescriptor);
}

inline void CreateSRVCube(ID3D12Device* device,
                          ID3D12Resource* resource,
                          DXGI_FORMAT format,
                          U32 mipLevels,
                          D3D12_CPU_DESCRIPTOR_HANDLE hDescriptor)
{
    D3D12_SHADER_RESOURCE_VIEW_DESC descSRV{};
    descSRV.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    descSRV.ViewDimension           = D3D12_SRV_DIMENSION_TEXTURECUBE;
    descSRV.TextureCube.MostDetailedMip     = 0;
    descSRV.TextureCube.ResourceMinLODClamp = 0.0F;
    descSRV.Format                          = format;
    descSRV.TextureCube.MipLevels           = mipLevels;
    device->CreateShaderResourceView(resource, &descSRV, hDescriptor);
}

inline void CreateSRVBuffer(ID3D12Device* device,
                            U64 firstElement,
                            U32 elementCount,
                            U32 elementByteSize,
                            ID3D12Resource* resource,
                            D3D12_CPU_DESCRIPTOR_HANDLE hDescriptor)
{
    D3D12_SHADER_RESOURCE_VIEW_DESC descSRV{};
    descSRV.Format                  = DXGI_FORMAT_UNKNOWN; // structured buffer
    descSRV.ViewDimension           = D3D12_SRV_DIMENSION_BUFFER;
    descSRV.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    descSRV.Buffer.FirstElement     = firstElement;
    descSRV.Buffer.NumElements      = elementCount;
    descSRV.Buffer.StructureByteStride = elementByteSize;
    descSRV.Buffer.Flags               = D3D12_BUFFER_SRV_FLAG_NONE;
    device->CreateShaderResourceView(resource, &descSRV, hDescriptor);
}

inline void CreateUAV2D(ID3D12Device* device,
                        ID3D12Resource* resource,
                        DXGI_FORMAT format,
                        U32 mipSlice,
                        D3D12_CPU_DESCRIPTOR_HANDLE hDescriptor)
{
    D3D12_UNORDERED_ACCESS_VIEW_DESC descUAV{};
    descUAV.Format             = format;
    descUAV.ViewDimension      = D3D12_UAV_DIMENSION_TEXTURE2D;
    descUAV.Texture2D.MipSlice = mipSlice;
    device->CreateUnorderedAccessView(resource, nullptr, &descUAV, hDescriptor);
}

inline void CreateUAVBuffer(ID3D12Device* device,
                            U64 firstElement,
                            U32 elementCount,
                            U32 elementByteSize,
                            U64 counterOffset,
                            ID3D12Resource* resource,
                            ID3D12Resource* counterResource,
                            D3D12_CPU_DESCRIPTOR_HANDLE hDescriptor)
{
    D3D12_UNORDERED_ACCESS_VIEW_DESC descUAV{};
    descUAV.Format              = DXGI_FORMAT_UNKNOWN; // structured buffer
    descUAV.ViewDimension       = D3D12_UAV_DIMENSION_BUFFER;
    descUAV.Buffer.FirstElement = firstElement;
    descUAV.Buffer.NumElements  = elementCount;
    descUAV.Buffer.StructureByteStride  = elementByteSize;
    descUAV.Buffer.CounterOffsetInBytes = counterOffset;
    descUAV.Buffer.Flags                = D3D12_BUFFER_UAV_FLAG_NONE;
    device->CreateUnorderedAccessView(resource,
                                      counterResource,
                                      &descUAV,
                                      hDescriptor);
}

inline void CreateSampler(ID3D12Device* device)
{
    constexpr D3D12_SAMPLER_DESC descSampler{
        .Filter         = D3D12_FILTER_MIN_MAG_MIP_POINT,
        .AddressU       = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
        .AddressV       = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
        .AddressW       = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
        .MipLODBias     = 0.0F,
        .MaxAnisotropy  = 16,
        .ComparisonFunc = D3D12_COMPARISON_FUNC_NONE,
        .BorderColor    = {0.0F, 0.0F, 0.0F, 0.0F},
        .MinLOD         = 0.0F,
        .MaxLOD         = F32_MAX};

    auto descPointWarp     = descSampler;
    descPointWarp.Filter   = D3D12_FILTER_MIN_MAG_MIP_POINT;
    descPointWarp.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    descPointWarp.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    descPointWarp.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;

    auto descPointClamp     = descSampler;
    descPointClamp.Filter   = D3D12_FILTER_MIN_MAG_MIP_POINT;
    descPointClamp.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    descPointClamp.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    descPointClamp.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;

    auto descLinearWarp     = descSampler;
    descLinearWarp.Filter   = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    descLinearWarp.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    descLinearWarp.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    descLinearWarp.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;

    auto descLinearClamp     = descSampler;
    descLinearClamp.Filter   = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    descLinearClamp.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    descLinearClamp.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    descLinearClamp.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;

    auto descAnisotropicWarp          = descSampler;
    descAnisotropicWarp.Filter        = D3D12_FILTER_ANISOTROPIC;
    descAnisotropicWarp.AddressU      = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    descAnisotropicWarp.AddressV      = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    descAnisotropicWarp.AddressW      = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    descAnisotropicWarp.MaxAnisotropy = 8;

    auto descAnisotropicClamp          = descSampler;
    descAnisotropicClamp.Filter        = D3D12_FILTER_ANISOTROPIC;
    descAnisotropicClamp.AddressU      = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    descAnisotropicClamp.AddressV      = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    descAnisotropicClamp.AddressW      = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    descAnisotropicClamp.MaxAnisotropy = 8;

    auto descShadow     = descSampler;
    descShadow.Filter   = D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
    descShadow.AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
    descShadow.AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
    descShadow.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
    descShadow.ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

    array<D3D12_SAMPLER_DESC, 7> descSamplers{
        descPointWarp,
        descPointClamp,
        descLinearWarp,
        descLinearClamp,
        descAnisotropicWarp,
        descAnisotropicClamp,
        descShadow,
    };

    ComPtr<ID3D12DescriptorHeap> heap;

    D3D12_DESCRIPTOR_HEAP_DESC descHeap{};
    descHeap.NumDescriptors = descSamplers.size();
    descHeap.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
    descHeap.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    descHeap.NodeMask       = 0;

    HRESULT hr =
        device->CreateDescriptorHeap(&descHeap,
                                     IID_PPV_ARGS(heap.GetAddressOf()));
    assert(SUCCEEDED(hr) && "Failed to create descriptor heap.");

    CD3DX12_CPU_DESCRIPTOR_HANDLE handleSampler{
        heap->GetCPUDescriptorHandleForHeapStart()};
    auto sizeSampler = device->GetDescriptorHandleIncrementSize(
        D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);

    for (const auto& descSampler : descSamplers)
    {
        device->CreateSampler(&descSampler, handleSampler);
        handleSampler.Offset(1, sizeSampler);
    }
}
} // namespace Acrylic::Engine::Util