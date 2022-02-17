#pragma once

#include <map>
#include <memory>
#include <mutex>
#include <shared_mutex>

#include <Usagi/Library/Meta/Tuple.hpp>

#include "details/ResourceBuilder.hpp"
#include "details/ResourceRequestBuilder.hpp"

namespace usagi
{
class Heap;
class Task;
class TaskExecutor;

namespace details::heap_manager
{
// Avoid introducing TaskExecutor by including this header.

void submit_build_task(TaskExecutor *executor, std::unique_ptr<Task> task);
void run_build_task_synced(std::unique_ptr<Task> task);
}

/*
 * HeapManager is a generalization of AssetManager. Object construction and
 * dependency management have generally the same idea: Object are constructed
 * via so called ResourceBuilders, and if the building process of an object
 * involves a request to another object, a dependency edge is created to
 * record that dependency relationship. It is assumed that the building process
 * of a resource is stable so it always refers to the same set of other
 * resources.
 * Unlike AssetManager, HeapManager doesn't have explicit definitions of
 * AssetPackage or Asset and such. Objects are arranged around Heaps. Each heap
 * is a memory allocator that can contain certain types of objects. The
 * derivation of an object is done via certain transfer function called by
 * the corresponding builder. For example, graphics subsystem may have two
 * heaps, one for staging buffers, one for device local buffers. Requesting
 * an image from device local heap will cause its builder to seek for a
 * predecessor heap that can provide the source content it needs, which in
 * this case is the staging heap. Then, it calls a transfer function that
 * copies the image from staging heap to the device local heap. The most
 * derived type of the heaps are provided to the transfer function, thus it can
 * interact with whatever graphics API it requires to perform the copy
 * operation. This process is transparent to the user, but scheduling of the
 * building/copying tasks are handled by schedulers provided by the user.
 *
 * Design decisions to make:
 *
 * todo: Store resource handles in HeapManger or individual heaps?
 * - Dependency management has to be done in heap manager as it's the only
 *   place where the information of resources are complete.
 *   - But, it's also possible to distribute edges in heaps, despite it would
 *     be rather messy.
 *   - If certain objects are unloaded by a heap, the heap must notify the
 *     heap manager to update dependency edges.
 *     - todo: What to do for the removed resource node?
 *       - For dependency chain A->B->C (-> means derivation), if B is unloaded
 *         due to insufficient memory on its heap, should the node be preserved
 *         and made into a tomb, or A shall be directly linked to C?
 *         Unloading B does not invalidates C, unless the content of A is
 *         changed so B is invalidated.
 */
// Tried to make this a template to allow different storage impl, but failed
// because many places require a pointer to the base type of the HeapManager.
// In that case, it would be impossible to resolve heap address at compile
// time.
class HeapManager : Noncopyable
{
    // Heap Management

    std::shared_mutex mHeapMutex;
    std::map<HeapResourceIdT, std::unique_ptr<Heap>> mHeaps;
    // std::map<std::uint64_t, ...> mResources;

    // template <ResourceBuilder T>
    // constexpr static bool IsResourceBuilderSupported =
        // HeapStorageImpl::template IsResourceBuilderSupported<T>;

    // Resource Entry Management

    std::mutex mEntryMapMutex;
    std::map<HeapResourceDescriptor, ResourceEntry> mResourceEntries;
    using ResourceEntryIt = decltype(mResourceEntries)::iterator;

    // Resource Request Handling

    template <typename ResourceBuilderT, typename BuildParamTupleFunc>
    friend class ResourceRequestBuilder;

    template <
        ResourceBuilder ResourceBuilderT,
        typename TargetHeapT = typename ResourceBuilderT::TargetHeap
    >
    auto make_accessor_nolock(
        HeapResourceDescriptor descriptor,
        TargetHeapT *heap,
        bool is_fallback)
    -> ResourceAccessor<ResourceBuilderT>;

    template <ResourceBuilder ResourceBuilderT, typename... Args>
    static HeapResourceDescriptor make_resource_descriptor(Args &&...args)
        requires std::constructible_from<ResourceBuilderT, Args...>;

public:
    virtual ~HeapManager();

    /**
     * \brief Request an access proxy to certain resource.
     * \tparam ResourceBuilderT The builder type used to build the resource
     * when it could not be found in the cache.
     * \tparam BuildParamTupleFuncT Functor type that returns a tuple
     * containing the build params.
     * \param resource_cache_id The cache id of the requested resource. If the
     * requester does not know it, leave it empty and the id can be accessed
     * via the returned accessor.
     * \param executor Task executor.
     * \param lazy_build_params A callable object that returns a tuple of
     * parameters that will be passed to the builder when the requested
     * resource could not be found. The object will not be called if the
     * resource is found in the cache.
     * \return An accessor that can be used to access or wait for the resource.
     */
    template <
        ResourceBuilder ResourceBuilderT,
        typename BuildParamTupleFuncT
    >
    auto resource(
        HeapResourceDescriptor resource_cache_id,
        TaskExecutor *executor,
        BuildParamTupleFuncT &&lazy_build_params)
    -> ResourceRequestBuilder<ResourceBuilderT, BuildParamTupleFuncT>
    requires
        ConstructibleFromTuple<ResourceBuilderT, decltype(lazy_build_params())>
        && NoRvalueRefInTuple<decltype(lazy_build_params())>;
        // && IsResourceBuilderSupported<ResourceBuilderT>;

    // Immediate resource is constructed on the calling thread without using
    // any task executor and no fallback is used. The ResourceBuilder is
    // always constructed.
    template <
        ResourceBuilder ResourceBuilderT,
        typename... BuildArgs
    >
    auto resource_immediate(BuildArgs &&... args)
    -> ResourceAccessor<ResourceBuilderT>
    requires
        std::constructible_from<ResourceBuilderT, BuildArgs...>;
        // && IsResourceBuilderSupported<ResourceBuilderT>;

    template <typename HeapT, typename... Args>
    HeapT * add_heap(Args &&...args);

    template <typename HeapT>
    HeapT * locate_heap();

private:
    template <
        ResourceBuilder ResourceBuilderT,
        typename BuildParamTupleFunc
    >
    auto request_resource(
        const ResourceBuildOptions &options,
        TaskExecutor *executor,
        BuildParamTupleFunc &&param_func)
    -> ResourceAccessor<ResourceBuilderT>;
};
}

#include "details/HeapManagerImpl_Heap.hpp"
#include "details/HeapManagerImpl_Accessor.hpp"
#include "details/HeapManagerImpl_Request.hpp"
#include "details/HeapManagerImpl_Build.hpp"
#include "details/HeapManagerImpl_Util.hpp"
#include "details/ResourceRequestBuilderImpl.hpp"
#include "details/ResourceConstructDelegateImpl.hpp"
