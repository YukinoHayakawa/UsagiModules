#pragma once

namespace usagi
{
// TransparentArg won't participate in resource build param hashing.
template <typename UnderlyingT>
class TransparentArg
{
};
}
