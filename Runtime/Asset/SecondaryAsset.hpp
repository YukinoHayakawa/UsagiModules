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
    DerivedT & as() requires std::is_base_of_v<SecondaryAsset, DerivedT>
    {
        auto ptr = dynamic_cast<DerivedT *>(this);
        if(ptr == nullptr)
            USAGI_THROW(std::bad_cast());
        return *ptr;
    }
};

template <std::move_constructible T>
class SecondaryAssetAdapter
    : public SecondaryAsset
    , protected T
{
public:
    template <typename... Args>
    SecondaryAssetAdapter(Args &&... args)
        : T(std::forward<Args>(args)...)
    {
    }

    const T & value() const
    {
        return *this;
    }
};
}
