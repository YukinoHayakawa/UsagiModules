#pragma once

#include <Usagi/Library/Graph/AdjacencyList.hpp>

#include "TaskProfile.hpp"

namespace usagi
{
using TaskGraph /*[[deprecated]]*/ = graph::AdjacencyList<TaskProfile, double>;
}
