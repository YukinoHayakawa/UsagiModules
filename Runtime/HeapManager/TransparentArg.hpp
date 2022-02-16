#pragma once

#include <utility>

namespace usagi
{
// TransparentArg won't participate in resource build param hashing.
template <typename Ref>
class TransparentArg
{
    Ref mArgRef;

public:
    template <typename T>
    TransparentArg(T arg) : mArgRef(std::forward<T>(arg))
    {
    }

    operator Ref() const
    {
        return mArgRef;
    }

    operator Ref()
    {
        return mArgRef;
    }
};

template <typename T>
TransparentArg(T &&arg) -> TransparentArg<T &&>;
}
