#pragma once

#include <type_traits>

#include <Usagi/Library/Meta/Type.hpp>

#include "../Asset.hpp"

namespace usagi
{
class TaskExecutor;
class AssetManager2;

template <typename Builder>
struct AssetBuilderProductType
{
    using ProductT = typename ExtractFirstTemplateParameter<
        decltype(std::declval<Builder>().construct_with(
            std::declval<AssetManager2 &>(),
            std::declval<TaskExecutor &>()
        ))
    >::type;
};

template <typename T>
concept AssetBuilder =
    requires(T) { typename AssetBuilderProductType<T>::ProductT; } &&
    std::is_base_of_v<Asset, typename AssetBuilderProductType<T>::ProductT> &&
    !std::is_same_v<Asset, typename AssetBuilderProductType<T>::ProductT>
;
}
