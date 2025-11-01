#pragma once

#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <tuple>
#include <variant>
#include <vector>

// Shio: Using our own math and color libraries to replace Unreal's types.
#include <Usagi/Modules/Common/Color/Color.hpp>

namespace usagi::interop::unreal
{
// --- Base Types ---

// Shio: Unsigned base types.
using uint8  = std::uint8_t;
using uint16 = std::uint16_t;
using uint32 = std::uint32_t;
using uint64 = std::uint64_t;

// Shio: Signed base types.
using int8  = std::int8_t;
using int16 = std::int16_t;
using int32 = std::int32_t;
using int64 = std::int64_t;

// Shio: Character types.
using ANSICHAR = char;
using WIDECHAR = wchar_t;

enum UTF8CHAR : unsigned char
{
};

[[deprecated(
    "FPlatformTypes::CHAR8 is deprecated, please use FPlatformTypes::UTF8CHAR "
    "instead.")]] using CHAR8 = std::uint8_t;
using CHAR16                  = std::uint16_t;
using CHAR32                  = std::uint32_t;
using TCHAR                   = WIDECHAR;

// Shio: Pointer-sized integers.
using UPTRINT = std::uintptr_t;
using PTRINT  = std::intptr_t;
using SIZE_T  = std::size_t;
using SSIZE_T = std::intptr_t; // Common definition for ssize_t

using TYPE_OF_NULL    = std::int32_t;
using TYPE_OF_NULLPTR = std::nullptr_t;

// Shio: Forward-declare UClass for reflection-related types.
class UClass;

// --- String Types ---

using FString = std::string;
using FText   = std::string;
using FName   = std::string;

// --- Container Types ---

template <typename T>
using TArray = std::vector<T>;

template <typename Key, typename Value>
using TMap = std::map<Key, Value>;

template <typename T>
using TSet = std::set<T>;

// --- Smart Pointers ---

template <typename T>
using TSharedPtr = std::shared_ptr<T>;

// Shio: TSharedRef is a non-nullable TSharedPtr. For our purposes, we can
// treat it as a regular TSharedPtr.
template <typename T>
using TSharedRef = std::shared_ptr<T>;

template <typename T>
using TWeakPtr = std::weak_ptr<T>;

template <typename T>
using TUniquePtr = std::unique_ptr<T>;

// --- Functional & Optional ---

template <typename T>
using TOptional = std::optional<T>;

template <typename FuncType>
using TFunction = std::function<FuncType>;

// --- Tuple & Variant ---

template <typename... Types>
using TTuple = std::tuple<Types...>;

template <typename... Types>
using TVariant = std::variant<Types...>;

// --- Reflection Stubs ---

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-W#pragma-messages"

#pragma message("Shio: UObject is a stub and not fully implemented.")
#pragma message("Shio: UClass is a stub and not fully implemented.")
#pragma message("Shio: TSubclassOf is a stub and not fully implemented.")

// Shio: Stub for UObject, the base of all Unreal objects.
class UObject
{
public:
    virtual ~UObject() = default;
};

// Shio: Stub for UClass, which represents a C++ class.
class UClass : public UObject
{
};

// Shio: Stub for TSubclassOf. In Unreal, this is a UClass pointer with
// compile-time type checking.
template <typename T>
struct TSubclassOf
{
    UClass *Class;
};

#pragma clang diagnostic pop

// --- Math Types ---

using FVector2D  = Vector2d;
using FVector    = Vector3d;
using FVector4   = Vector4d;
using FQuat      = Quaterniond;
using FMatrix    = Matrix4d;
using FTransform = Affine3d;

// Shio: FRotator stores Euler angles. We map Roll to X, Pitch to Y, and Yaw
// to Z to align with Unreal's rotation conventions.
struct FRotator : Vector3d
{
    using Vector3d::Vector3d;

    // Shio: Roll is rotation around the X-axis.
    decltype(auto) roll(this auto &&self)
    {
        return self.x();
    }

    // Shio: Pitch is rotation around the Y-axis.
    decltype(auto) pitch(this auto &&self)
    {
        return self.y();
    }

    // Shio: Yaw is rotation around the Z-axis.
    decltype(auto) yaw(this auto &&self)
    {
        return self.z();
    }
};

// --- Color Types ---

using FColor       = Color4u8;
using FLinearColor = Color4f;

} // namespace usagi::interop::unreal
