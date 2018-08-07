#pragma once

#include <cstdint>
#include <tuple>

#include "xchainer/constant.h"
#include "xchainer/index_iterator.h"
#include "xchainer/indexable_array.h"
#include "xchainer/indexer.h"
#include "xchainer/shape.h"
#include "xchainer/squash_dims.h"

namespace xchainer {
namespace native {
namespace elementwise_detail {

template <int8_t Ndim, typename Op, typename... Ts>
void ElementwiseKernel(Op op, const Indexer<Ndim>& indexer, const IndexableArray<Ts, Ndim>&... args) {
    for (auto it = indexer.It(0, 1); it; ++it) {
        op(it.raw_index(), args[it]...);
    }
}

template <int8_t Ndim, typename Op, typename... Ts, typename... Arrays>
void LaunchElementwiseKernel(Op&& op, const Shape& shape, const Axes& keep, const Arrays&... args) {
    ElementwiseKernel<Ndim, Op, Ts...>(
            op, Indexer<Ndim>{shape}, IndexableArray<Ts, Ndim>{args, GetSquashedStrides(args.strides(), keep)}...);
}

}  // namespace elementwise_detail

template <typename... Ts, typename... Arrays, typename Op>
void Elementwise(Op&& op, const Arrays&... args) {
    static_assert(sizeof...(Ts) == sizeof...(Arrays), "Data types must be specified per Array. ");

    std::tuple<Shape, Axes> squashed_result = SquashShape(args...);
    const Shape& squashed = std::get<0>(squashed_result);
    const Axes& keep = std::get<1>(squashed_result);

    // TODO(hvy): Reconsider the number of statically-optimized kernels in terms of speed and binary size trade-offs.
    switch (squashed.ndim()) {
        case 1:
            elementwise_detail::LaunchElementwiseKernel<1, Op, Ts...>(std::forward<Op>(op), squashed, keep, args...);
            break;
        case 2:
            elementwise_detail::LaunchElementwiseKernel<2, Op, Ts...>(std::forward<Op>(op), squashed, keep, args...);
            break;
        case 3:
            elementwise_detail::LaunchElementwiseKernel<3, Op, Ts...>(std::forward<Op>(op), squashed, keep, args...);
            break;
        case 4:
            elementwise_detail::LaunchElementwiseKernel<4, Op, Ts...>(std::forward<Op>(op), squashed, keep, args...);
            break;
        default:
            elementwise_detail::LaunchElementwiseKernel<kDynamicNdim, Op, Ts...>(std::forward<Op>(op), squashed, keep, args...);
            break;
    }
}

}  // namespace native
}  // namespace xchainer
