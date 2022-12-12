#pragma once

#include <Usagi/Modules/Runtime/Asset/AssetPath.hpp>
#include <Usagi/Modules/Runtime/HeapManager/ResourceBuilder.hpp>

#include "TaskGraph.hpp"

namespace usagi
{
/**
 * \brief Get input task graph from the specified experiment config.
 */
class RbInputTaskGraph : public ResourceBuilderDecl<
    // bug: because resources in the heap manager are not destroyed in a topological order of the resource dependencies (unimplemented rather than by design), this reference can cause hanging ref when the referenced task graph is destroyed before this ref.
    RefCounted<TaskGraph>,  // reference to actual resource
    const AssetPath &       // experiment config path
>
{
public:
    ResourceState construct(
        ResourceConstructDelegate<ProductT> &delegate,
        const AssetPath &path) override;
};
}
