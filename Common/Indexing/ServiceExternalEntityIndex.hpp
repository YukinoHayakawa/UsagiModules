#pragma once

#include <type_traits>
#include <string_view>

#include <Usagi/Runtime/Service/ServiceAccess.hpp>
#include <Usagi/Runtime/Service/SimpleService.hpp>

namespace usagi
{
template <
    typename IndexDescriptor
>
struct ServiceExternalEntityIndex
{
    using SortedKeyContainer = typename IndexDescriptor::SortedKeyContainer;
    using AggregateContainer = typename IndexDescriptor::AggregateContainer;
    using SortKey = typename IndexDescriptor::SortKey;
    using Projection = typename IndexDescriptor::Projection;
    using ProjectedKey = typename IndexDescriptor::ProjectedKey;

    SortedKeyContainer index;
    AggregateContainer aggregates;

    void reset_index()
    {
        // todo perf avoid allocation overhead?
        index.clear();
    }

    template <typename EntityView>
    void visit(EntityView e)
    {
        const auto projected = Projection()(e(C<SortKey>()));
        const auto [iter, inserted] = index.emplace(projected, e.id());
        assert(inserted && "duplicated index key? index should be cleared?");
    }

    // todo thread safety
    auto & reset_aggregate(std::string_view key)
    {
        auto iter = aggregates[key];
        iter->second = ProjectedKey { };
        return iter->second;
    }

    // template <typename T>
    // auto & entity_index() requires std::is_same_v<T, IndexDescriptor>
    // {
    //     return *this;
    // }

    using ServiceT = ServiceExternalEntityIndex<IndexDescriptor>;

    auto & get_service()
    {
        return *this;
    }
};

// using ServiceExternalEntityIndex = SimpleService<ServiceExternalEntityIndex>;
template <typename IndexDescriptor>
class ServiceAlias<ServiceExternalEntityIndex<IndexDescriptor>>
{
    using ServiceT = ServiceExternalEntityIndex<IndexDescriptor>;
    ServiceT *mService = nullptr;

public:
    template <typename Runtime> 
    explicit ServiceAlias(Runtime &runtime) 
        : mService(static_cast<ServiceT *>(&runtime)) {}

    template <typename T>
    typename ServiceT::ServiceT & entity_index(T descriptor = T()) const
        requires std::is_same_v<T, IndexDescriptor>
    {
        return mService->get_service();
    }
};
}
