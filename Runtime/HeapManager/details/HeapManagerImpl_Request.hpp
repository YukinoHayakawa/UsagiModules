#pragma once

#include <Usagi/Modules/Runtime/Executive/TaskExecutorSynchronized.hpp>

namespace usagi
{
template <ResourceBuilder Builder, typename LazyBuildArgFunc>
ResourceRequestBuilder<Builder, LazyBuildArgFunc>
HeapManager::resource(
    HeapResourceDescriptor resource_id,
    TaskExecutor *executor,
    LazyBuildArgFunc &&arg_func)
// requires
    // The build params shouldn't return rvalue refs like integer literals
    // inside a lambda.
    // Because at the point of using, they are much likely already out-of-scope.
    // todo: this however won't prevent forwarding refs to local variables.
    // todo: the handling of l/r refs should be revised.
    // NoRvalueRefInTuple<decltype(arg_func())>
{
    auto context = allocate_request_context<Builder, LazyBuildArgFunc>();

    context->executor = executor;
    context->arg_func = &arg_func;
    context->options.requested_resource = resource_id;

    // The request really happens when RequestBuilder.make_request()
    // is called.
    return ResourceRequestBuilder { std::move(context) };
}

// request to a transient resource doesn't require resource id and executor,
// because a transient resource is only supposed to be requested once,
// and it will be built on the requesting thread. the resource id will be
// computed when creating the building task.
template <ResourceBuilder Builder, typename... BuildArgs>
ResourceAccessor<typename Builder::ProductT>
HeapManager::resource_transient(BuildArgs &&...args)
{
    // safe to forward arguments here because they have storage on the calling
    // stack.
	auto params = [&] {
        return std::forward_as_tuple(std::forward<BuildArgs>(args)...);
    };

    auto context = allocate_request_context<Builder, decltype(params)>();

    TaskExecutorSynchronized executor;
    context->executor = &executor;
    context->arg_func = &params;

    ResourceRequestHandler handler { std::move(context) };

    return handler.process_request_transient();
}

template <ResourceBuilder Builder, typename LazyBuildArgFunc>
ResourceAccessor<typename Builder::ProductT> HeapManager::request_resource(
    UniqueResourceRequestContext<Builder, LazyBuildArgFunc> context)
{
    ResourceRequestHandler handler { std::move(context) };

    return handler.process_request();
}
}
