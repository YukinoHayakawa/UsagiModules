#pragma once

namespace usagi
{
template <ResourceBuilder Builder, typename LazyBuildArgFunc>
ResourceRequestBuilder<Builder, LazyBuildArgFunc>
HeapManager::resource(
    HeapResourceDescriptor resource_id,
    TaskExecutor *executor,
    LazyBuildArgFunc &&arg_func)
requires
    ConstructibleFromTuple<Builder, decltype(arg_func())>
    // The build params shouldn't return rvalue refs like integer literals.
    // Because at the point of using, they are much likely already out-of-scope.
    // todo: this however won't prevent forwarding refs to local variables.
    && NoRvalueRefInTuple<decltype(arg_func())>
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
ResourceAccessor<Builder> HeapManager::resource_transient(BuildArgs &&...args)
// resource builder do not receive build params during construction anymore.
// they are passed in when construct is called.
// requires std::constructible_from<Builder, BuildArgs...>                
{
    // todo: this copies values.
	auto params = [&] {
        return std::forward_as_tuple(std::forward<BuildArgs>(args)...);
    };

    auto context = allocate_request_context<Builder, decltype(params)>();

    context->arg_func = &params;

    ResourceRequestHandler handler { std::move(context) };

    return handler.process_request_transient();
}

template <ResourceBuilder Builder, typename LazyBuildArgFunc>
ResourceAccessor<Builder> HeapManager::request_resource(
    UniqueResourceRequestContext<Builder, LazyBuildArgFunc> context)
{
    ResourceRequestHandler handler { std::move(context) };

    return handler.process_request();
}
}
