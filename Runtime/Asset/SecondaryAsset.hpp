#pragma once

#include <typeinfo>

#include <Usagi/Library/Memory/Noncopyable.hpp>
#include <Usagi/Runtime/ErrorHandling.hpp>

namespace usagi
{
class SecondaryAsset : Noncopyable
{
public:
    virtual ~SecondaryAsset() = default;

    const std::type_info & type() const
    {
        return typeid(*this);
    }

    template <typename DerivedT>
    // Return type declared as pointer to avoid unwanted copying of objects.
    DerivedT * as()
    {
        auto ptr = dynamic_cast<DerivedT *>(this);
        if(ptr == nullptr)
            USAGI_THROW(std::bad_cast());
        return ptr;
    }
};

template <std::move_constructible T>
class SecondaryAssetAdapter
    : public SecondaryAsset
    , public T
{
public:
    SecondaryAssetAdapter(T t)
        : T(std::move(t))
    {
    }
};
}
