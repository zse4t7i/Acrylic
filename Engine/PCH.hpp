#pragma once

#define UNICODE
#define _UNICODE
// Use the C++ standard templated min/max.
#define NOMINMAX
// DirectX apps don't need GDI.
#define NODRAWTEXT
#define NOGDI
#define NOBITMAP
// Include <mcx.h> if you need this.
#define NOMCX
// Include <winsvc.h> if you need this.
#define NOSERVICE
// WinHelp is deprecated.
#define NOHELP
// Exclude rarely-used stuff from Windows headers.
#define WIN32_LEAN_AND_MEAN

// Third-party Library
#include <D3D12MemAlloc.h>
#include <DirectXCollision.h>
#include <DirectXColors.h>
#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include <GameInput.h>
#include <directx/d3d12.h>
#include <directx/d3dx12.h>
#include <entt/entt.hpp>
#include <nlohmann/json.hpp>

#include <dxgi1_6.h>
#include <windows.h>
#include <wrl/client.h>
#ifdef DEBUG
#include <dxgidebug.h>
#endif

// Standard Library
#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <future>
#include <limits>
#include <memory>
#include <optional>
#include <queue>
#include <stack>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using DirectX::XMFLOAT3;
using DirectX::XMFLOAT4;
using DirectX::XMFLOAT4X4;
using Microsoft::WRL::ComPtr;
using std::array;
using std::future;
using std::ifstream;
using std::optional;
using std::queue;
using std::shared_ptr;
using std::stack;
using std::string;
using std::string_view;
using std::unique_ptr;
using std::unordered_map;
using std::unordered_set;
using std::vector;
using std::wstring;
using std::wstring_view;

using Byte = std::byte;
using Path = std::filesystem::path;
using JSON = nlohmann::json;

// Integer 8
using U8 = std::uint8_t;
inline constexpr U8 U8_MAX{std::numeric_limits<U8>::max()};
inline constexpr U8 U8_MIN{std::numeric_limits<U8>::min()};
using I8 = std::int8_t;
inline constexpr I8 I8_MAX{std::numeric_limits<I8>::max()};
inline constexpr I8 I8_MIN{std::numeric_limits<I8>::min()};

// Integer 16
using U16 = std::uint16_t;
inline constexpr U16 U16_MAX{std::numeric_limits<U16>::max()};
inline constexpr U16 U16_MIN{std::numeric_limits<U16>::min()};
using I16 = std::int16_t;
inline constexpr I16 I16_MAX{std::numeric_limits<I16>::max()};
inline constexpr I16 I16_MIN{std::numeric_limits<I16>::min()};

// Integer 32
using U32 = std::uint32_t;
inline constexpr U32 U32_MAX{std::numeric_limits<U32>::max()};
inline constexpr U32 U32_MIN{std::numeric_limits<U32>::min()};
using I32 = std::int32_t;
inline constexpr I32 I32_MAX{std::numeric_limits<I32>::max()};
inline constexpr I32 I32_MIN{std::numeric_limits<I32>::min()};

// Integer 64
using U64 = std::uint64_t;
inline constexpr U64 U64_MAX{std::numeric_limits<U64>::max()};
inline constexpr U64 U64_MIN{std::numeric_limits<U64>::min()};
using I64 = std::int64_t;
inline constexpr I64 I64_MAX{std::numeric_limits<I64>::max()};
inline constexpr I64 I64_MIN{std::numeric_limits<I64>::min()};

// Floating-point 32
using F32 = float;
inline constexpr F32 F32_MAX{std::numeric_limits<F32>::max()};
inline constexpr F32 F32_MIN{std::numeric_limits<F32>::min()};
// Floating-point 64
using F64 = double;
inline constexpr F64 F64_MAX{std::numeric_limits<F64>::max()};
inline constexpr F64 F64_MIN{std::numeric_limits<F64>::min()};
