#pragma once

#include <cstdint>
#include <memory>

namespace usagi
{
/*
 * We are using xxHash64 results as asset access keys. The algorithm is made
 * for speed instead for security. Therefore, it would be rather easy to
 * make up collisions. Using data size to disambiguate won't help much either.
 * So we simply use the hash and throw errors when any collision is found.
 */
using AssetHashId = std::uint64_t;
using AssetReference = std::shared_ptr<void>;
}
