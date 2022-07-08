#pragma once

#include <cstdint>

namespace usagi
{
// todo maybe status is the more suitable word
class ResourceState
{
    using ValueT = std::uint16_t;

    [[maybe_unused]]
    ValueT mValue = ABSENT.mValue;

    explicit constexpr ResourceState(ValueT value)
        : mValue(value)
    {
    }

    constexpr static ValueT BASE_STATE_MASK     { 0b1111'1111'0000'0000 };

public:
    constexpr ResourceState() = default;

#define USAGI_CVAL_DECL static const ResourceState

    // In absent/failed states, the heap should not have the resource.
    USAGI_CVAL_DECL ABSENT                    ;
    USAGI_CVAL_DECL ABSENT_FIRST_REQUEST      ;
    USAGI_CVAL_DECL ABSENT_EVICTED            ;

    USAGI_CVAL_DECL PREPARING                 ;
    USAGI_CVAL_DECL SCHEDULED                 ;
    USAGI_CVAL_DECL BUILDING                  ;
    USAGI_CVAL_DECL WAITING_DEPENDENCY        ;

    USAGI_CVAL_DECL FAILED                    ;
    USAGI_CVAL_DECL FAILED_INVALID_DATA       ;
    USAGI_CVAL_DECL FAILED_INSUFFICIENT_MEMORY;
    USAGI_CVAL_DECL FAILED_MISSING_DEPENDENCY ;
    USAGI_CVAL_DECL FAILED_BUSY               ;

    // todo add partially ready (e.g. progressively decoded resources)
    USAGI_CVAL_DECL READY                     ;

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
        return ResourceState { static_cast<ValueT>(mValue & BASE_STATE_MASK) };
    }

    bool ready() const
    {
        return mValue == READY.mValue;
    }

    bool failed() const
    {
        return mValue & FAILED.mValue;
    }
};
}
