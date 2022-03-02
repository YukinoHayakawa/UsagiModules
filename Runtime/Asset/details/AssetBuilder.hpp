#pragma once

#include <type_traits>

#include <Usagi/Library/Meta/Template.hpp>

#include "../Asset.hpp"

namespace usagi
{
class AssetRequestProxy;
class TaskExecutor;
class AssetManager2;

/*
 * If the compiler complains about incomplete definition of AssetRequestProxy,
 * include AssetRequestProxy.hpp at the site where the following two templates
 * are used.
 */
template <typename Builder>
struct AssetBuilderProductType
{
    using ProductT = typename ExtractFirstTemplateParameter<
        typename ExtractSecondTemplateParameter<
            decltype(std::declval<Builder>().construct_with(
                std::declval<AssetRequestProxy &>()
            ))
        >::type
    >::type;
};

template <typename T>
concept AssetBuilder =
    requires(T) { typename AssetBuilderProductType<T>::ProductT; } &&
    std::is_base_of_v<Asset, typename AssetBuilderProductType<T>::ProductT> &&
    !std::is_same_v<Asset, typename AssetBuilderProductType<T>::ProductT>
;
}
