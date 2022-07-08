#pragma once

#include <any>
#include <deque>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <shared_mutex>
#include <typeindex>

#include <Usagi/Library/Memory/PoolAllocator.hpp>
#include <Usagi/Library/Meta/Tuple.hpp>

#include "details/ResourceBuilder.hpp"
#include "details/ResourceRequestBuilder.hpp"
#include "details/ResourceRequestContext.hpp"

namespace usagi
{
class Heap;
class Task;
class TaskExecutor;
class ResourceBuildTaskBase;

namespace details::heap_manager
{
// Avoid header dependency on TaskExecutor.
void submit_build_task(TaskExecutor *executor, std::unique_ptr<Task> task);
void run_build_task_synced(std::unique_ptr<ResourceBuildTaskBase> task);

template <ResourceBuilder Builder, typename... Args>
HeapResourceDescriptor make_resource_descriptor(Args &&...args);
    // requires std::constructible_from<Builder, Args...>;

template <ResourceBuilder Builder, typename Tuple>
HeapResourceDescriptor make_resource_descriptor_from_tuple(Tuple &&tuple);
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
 *
 * A heap is a generalized runtime resource management unit. It can be a region
 * of managed memory, a factory, etc. 
 */
// Tried to make this a template to allow different storage impl, but failed
// because many places require a pointer to the base type of the HeapManager.
// In that case, it would be impossible to resolve heap address at compile
// time.
class HeapManager : Noncopyable
{
    // ********************************************************************* //    
    //                             Heap Management                           //
    // ********************************************************************* //

    std::shared_mutex mHeapMutex;
    // std::map<HeapResourceIdT, std::unique_ptr<Heap>> mHeaps;
    std::map<std::type_index, std::any> mHeaps;

public:
    template <typename HeapT, typename... Args>
    HeapT * add_heap(Args &&...args);

    // overload that takes a constructed heap. allows polymorphism.
    template <typename HeapT>
    HeapT * add_heap(std::unique_ptr<HeapT> heap);

    template <typename HeapT>
    HeapT * locate_heap();

    // ********************************************************************* //    
    //                       Resource Entry Management                       //
    // ********************************************************************* //
private:
    std::mutex mEntryMapMutex;

    struct ResourceEntryComparator
    {
        using is_transparent = void;

        bool operator()(const std::unique_ptr<ResourceEntryBase> &lhs, 
            const std::unique_ptr<ResourceEntryBase> &rhs) const;
        bool operator()(const HeapResourceDescriptor &lhs, 
            const std::unique_ptr<ResourceEntryBase> &rhs) const;
        bool operator()(const std::unique_ptr<ResourceEntryBase> &lhs, 
            const HeapResourceDescriptor &rhs) const;
    };

    // todo manage the memory of resource entries
    std::set<
        std::unique_ptr<ResourceEntryBase>,
        ResourceEntryComparator
    > mResourceEntries;
    using ResourceEntryIt = decltype(mResourceEntries)::iterator;

    template <ResourceBuilder Builder>
    auto make_accessor_nolock(
        HeapResourceDescriptor descriptor,
        // typename Builder::TargetHeapT *heap,
        bool is_fallback)
    -> ResourceAccessor<Builder>;

    template <typename Builder, typename LazyBuildArgFunc>
    friend struct ResourceRequestHandler;

    constexpr static HeapResourceIdT DummyBuilderId = -1;
    std::atomic<HeapResourceIdT> mUniqueResourceIdCounter = 0;

public:
    HeapResourceDescriptor make_unique_descriptor();

private:
    // ********************************************************************* //    
    //                       Resource Request Handling                       //
    // ********************************************************************* //

    // PoolAllocator has an internal mutex.
    // std::mutex mRequestContextPoolMutex;
    // References to elements inside std::deque won't be affected when
    // inserting at the end.
    PoolAllocator<ResourceRequestContextBlock, std::deque> mRequestContextPool;

    template <ResourceBuilder Builder, typename LazyBuildArgFunc>
    UniqueResourceRequestContext<Builder, LazyBuildArgFunc>
    allocate_request_context();
    void deallocate_request_context(const ResourceBuildContextCommon &context); 

    friend struct details::heap_manager::RequestContextDeleter;

public:
    virtual ~HeapManager();

    // ********************************************************************* //    
    //                          Async Resource Request                       //
    // ********************************************************************* //

    /**
     * \brief Request an access proxy to certain resource.
     * \tparam Builder The builder type used to build the resource
     * when it could not be found in the cache.
     * \tparam LazyBuildArgFunc Functor type that returns a tuple
     * containing the build params.
     * \param resource_id The cache id of the requested resource. If the
     * requester does not know it, leave it empty and the id can be accessed
     * via the returned accessor.
     * \param executor Task executor.
     * \param arg_func A callable object that returns a tuple of
     * parameters that will be passed to the builder when the requested
     * resource could not be found. The object will not be called if the
     * resource is found in the cache.
     * \return An accessor that can be used to access or wait for the resource.
     */
    template <ResourceBuilder Builder, typename LazyBuildArgFunc>
    ResourceRequestBuilder<Builder, LazyBuildArgFunc> resource(
        HeapResourceDescriptor resource_id,
        TaskExecutor *executor,
        LazyBuildArgFunc &&arg_func)
    requires
        ConstructibleFromTuple<Builder, decltype(arg_func())>
        && NoRvalueRefInTuple<decltype(arg_func())>;

    // ********************************************************************* //    
    //                       Transient Resource Request                      //
    // ********************************************************************* //

    /*
     * Transient resource: The resource is temporary and only intended to be
     * used by the current frame. Examples include GPU command buffers/
     * semaphores/etc. It is allocated via the heap manager to benefit from
     * the reference counting mechanism universally applied to all resources.
     * It is supposed to have a very short lifetime, and is very likely
     * to be recycled within several frames. To optimize for these scenarios,
     * transient resources are built on the calling thread and no fallback
     * will be used if it is failed to construct.
     *
     * todo: still adds too much overhead compared with direct allocation from GpuDevice, etc. Lots of value copying, etc.
     *
     * Note: If the user only wants to construct the resource synchronously,
     * use `resource()` and pass in a synchronized task executor.
     */
    template <ResourceBuilder Builder, typename... BuildArgs>
    ResourceAccessor<Builder> resource_transient(BuildArgs &&... args);
    // requires std::constructible_from<Builder, BuildArgs...>;

    template <typename Builder, typename LazyBuildArgFunc>
    friend class ResourceRequestBuilder;

private:
    template <typename ResourceBuilderT>
    friend class ResourceConstructDelegate;

    template <ResourceBuilder Builder, typename LazyBuildArgFunc>
    ResourceAccessor<Builder> request_resource(
        UniqueResourceRequestContext<Builder, LazyBuildArgFunc> context);
};
}

#include "details/HeapManagerImpl_Heap.hpp"
#include "details/HeapManagerImpl_Accessor.hpp"
#include "details/HeapManagerImpl_Context.hpp"
#include "details/HeapManagerImpl_RequestHandler.hpp"
#include "details/HeapManagerImpl_Request.hpp"
#include "details/HeapManagerImpl_Build.hpp"
#include "details/HeapManagerImpl_Util.hpp"
#include "details/ResourceRequestBuilderImpl.hpp"
#include "details/ResourceConstructDelegateImpl.hpp"
