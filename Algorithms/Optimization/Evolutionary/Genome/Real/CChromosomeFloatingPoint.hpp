#pragma once

#include <array>
#include <concepts>

namespace usagi
{
/**
 * \brief Chromosome encoded with floating point numbers. Note that floating
 * point numbers are not the same thing as real numbers.
 * \tparam Gene Gene type.
 * \tparam Size Chromosome length.
 */
template <std::floating_point Gene, std::size_t Size>
struct CChromosomeFloatingPoint : std::array<Gene, Size>
{
};
}
