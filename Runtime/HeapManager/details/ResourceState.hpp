#pragma once

#include <cstdint>

namespace usagi
{
class ResourceState
{
    using ValueT = std::uint16_t;

    ValueT mValue;

    constexpr ResourceState(ValueT value)
        : mValue(value)
    {
    }

    constexpr static ValueT BASE_STATE_MASK     { 0b1111'1111'0000'0000 };

public:
#define USAGI_CVAL_DECL static const ResourceState
    //                                                ' Primary '   Sub   '

    // In absent/failed states, the heap should not have the resource.
    USAGI_CVAL_DECL ABSENT                    ; // { 0b0000'0001'0000'0000 };
    USAGI_CVAL_DECL ABSENT_FIRST_REQUEST      ; // { 0b0000'0001'0000'0001 };
    USAGI_CVAL_DECL ABSENT_EVICTED            ; // { 0b0000'0001'0000'0010 };

    USAGI_CVAL_DECL PREPARING                 ; // { 0b0000'0010'0000'0000 };
    USAGI_CVAL_DECL SCHEDULED                 ; // { 0b0000'0100'0000'0000 };
    USAGI_CVAL_DECL BUILDING                  ; // { 0b0000'1000'0000'0000 };
    USAGI_CVAL_DECL WAITING_DEPENDENCY        ; // { 0b0001'0000'0000'0000 };

    USAGI_CVAL_DECL FAILED                    ; // { 0b0010'0000'0000'0000 };
    USAGI_CVAL_DECL FAILED_INVALID_DATA       ; // { 0b0010'0000'0000'0001 };
    USAGI_CVAL_DECL FAILED_INSUFFICIENT_MEMORY; // { 0b0010'0000'0000'0010 };
    USAGI_CVAL_DECL FAILED_MISSING_DEPENDENCY ; // { 0b0010'0000'0000'0100 };

    USAGI_CVAL_DECL READY                     ; // { 0b0100'0000'0000'0000 };

#undef USAGI_CVAL_DECL

    // Comparators

    friend bool operator==(const ResourceState &lhs, const ResourceState &rhs)
    {
        return lhs.mValue == rhs.mValue;
    }

    friend bool operator!=(const ResourceState &lhs, const ResourceState &rhs)
    {
        return !(lhs == rhs);
    }

    // State queries

    ResourceState base_state() const
    {
        return { static_cast<ValueT>(mValue & BASE_STATE_MASK) };
    }

    bool ready() const
    {
        return mValue == READY;
    }

    bool failed() const
    {
        return mValue & FAILED.mValue;
    }
};
}
