#pragma once

namespace usagi
{
template <ResourceBuilder Builder, typename LazyBuildArgFunc>
ResourceRequestContext<Builder, LazyBuildArgFunc> &
HeapManager::allocate_request_context()
{
    const auto index = mRequestContextPool.allocate();
    auto &block = mRequestContextPool.at(index);

    using ConcreteContextT = ResourceRequestContext<
        Builder, LazyBuildArgFunc
    >;
    static_assert(sizeof(ConcreteContextT) <= sizeof(block));

    auto &context = reinterpret_cast<ConcreteContextT &>(block);
    std::construct_at<ConcreteContextT>(&context);

    context.context_index = index;
    context.manager = this;

    return context;
}
}
