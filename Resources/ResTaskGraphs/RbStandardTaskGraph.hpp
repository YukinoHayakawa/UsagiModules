#pragma once

#include <Usagi/Modules/Runtime/Asset/AssetPath.hpp>
#include <Usagi/Modules/Runtime/HeapManager/ResourceBuilder.hpp>

#include "TaskGraph.hpp"

namespace usagi
{
class RbStandardTaskGraph : public ResourceBuilderDecl<
    TaskGraph,
    const AssetPath &,  // asset path
    std::size_t         // task amount limit
>
{
public:
    ResourceState construct(
        ResourceConstructDelegate<ProductT> &delegate,
        const AssetPath &path,
        std::size_t max_tasks) override;
};
}
