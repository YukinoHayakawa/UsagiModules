#pragma once

#include <cassert>
#include <cstdint>

namespace usagi
{
template <typename T, typename Traits>
concept RefCountTraits = requires(Traits traits, T *t)
{
    // Atomic operation. Returns the new ref count immediately after
    // the increment.
    { Traits::increment_reference(t) } -> std::uint32_t;
    // Similar idea as above.
    { Traits::decrement_reference(t) } -> std::uint32_t;
    { Traits::free(t) };
};

template <
    typename T,
    typename RcTraits
> requires(RefCountTraits<T, RcTraits>)
class RefCounted
{
protected:
    T *mValue = nullptr;

    void share_from(const RefCounted &other)
    {
        decrement();

        // The current object shares the ownership with another instance.
        // The caller must ensure that the lifetime other instance exceeds
        // the execution of this constructor.
        // see: https://www.reddit.com/r/cpp/comments/79ak5d/a_bug_in_stdshared_ptr/

        mValue = other.mValue;
        const auto ref = RcTraits::increment_reference(mValue);

        assert(ref > 1);
    }

    void move_from(RefCounted &&other)
    {
        decrement();

        // As the copy constructor, the other instance must not die before
        // this function finishes.

        mValue = other.mValue;
        other.mValue = nullptr;
    }

    void decrement()
    {
        if(mValue == nullptr) return;

        // Because the decrement of reference count is an atomic operation,
        // if multiple RefCounted instances try to do that at the same time,
        // one instance will receive 0 and release the resource.
        const auto ref = RcTraits::decrement_reference(mValue);
        if(ref == 0)
            RcTraits::free(mValue);
    }

public:
    // No resource is held
    RefCounted() = default;

    // Construct new reference counted handle
    RefCounted(T *value)
    {
        mValue = value;
        // May share the ownership with other instances
        RcTraits::increment_reference(value);

        // Before this constructor finishes, no other instances shall
        // be created by sharing the resource of this instance.
    }

    RefCounted(const RefCounted &other)
    {
        share_from(std::forward<decltype(other)>(other));
    }

    RefCounted(RefCounted &&other) noexcept
    {
        move_from(std::forward<decltype(other)>(other));
    }

    ~RefCounted()
    {
        // Unlike constructors, destructors of multiple instances might
        // be called in different threads at the same time. Only one
        // instance will be able to release the resource must be guaranteed.

        decrement();
    }

    RefCounted & operator=(const RefCounted &other)
    {
        if(this == &other)
            return *this;

        share_from(std::forward<decltype(other)>(other));

        return *this;
    }

    RefCounted & operator=(RefCounted &&other) noexcept
    {
        if(this == &other)
            return *this;

        move_from(std::forward<decltype(other)>(other));

        return *this;
    }

    operator bool() const
    {
        return mValue != nullptr;
    }

    T * get()
    {
        return mValue;
    }

    decltype(auto) operator*()
    {
        return *mValue;
    }

    decltype(auto) operator->()
    {
        return mValue;
    }
};
}
