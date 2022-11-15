#pragma once

#include <iterator>

#include <Usagi/Entity/Component.hpp>
#include <Usagi/Entity/detail/ComponentFilter.hpp>

namespace usagi
{
/**
 * \brief Transform one component to another by applying a binary operation.
 * \tparam BinaryTransformOp The transform operator.
 * \tparam SourceComponent The component to be read from.
 * \tparam TargetComponent The component to be written to.
 * \tparam SourceProjection Projection applied to source.
 * \tparam TargetProjection Projection applied to target.
 */
template <
    Component SourceComponent,
    Component TargetComponent,
    typename BinaryTransformOp
    // typename SourceProjection = std::identity,
    // typename TargetProjection = std::identity
> requires
    // transform op is able to read from source and write to target.
    std::is_invocable_v<
        BinaryTransformOp,
        const SourceComponent &,
        TargetComponent &
    >
    // transform op is able to read from projected source and write to projected
    // target.
    // std::is_invocable_v<
    //     BinaryTransformOp,
    //     const std::invoke_result_t<SourceProjection, const SourceComponent &> &,
    //     std::invoke_result_t<TargetProjection, TargetComponent &> &
    // >
struct EntityVisitorTransformSingleComponent
{
    using WriteAccess = C<TargetComponent>;

    BinaryTransformOp op;
    // SourceProjection src_proj;
    // TargetProjection dst_proj;

    void operator()(auto &&entity_view)
    {
        const auto &src_comp = entity_view(C<SourceComponent>());
        auto &dst_comp = entity_view.add_component(C<TargetComponent>());
        // op(std::invoke(src_proj, src_comp), std::invoke(dst_proj, dst_comp));
        op(src_comp, dst_comp);
    }
};
}
