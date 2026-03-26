#pragma once

#include <DirectXCollision.h>
#include <DirectXColors.h>
#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include <stb_image.h>

#include <windows.h>

#include <cassert>
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
    std::ifstream file(path.string().c_str(), std::ios::binary | std::ios::ate);
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
} // namespace Acrylic::Util