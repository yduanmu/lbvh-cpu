#pragma once

#include "lbvh/cons_radix_seq.hpp"
#include <cstdint>
#include <vector>

using std::vector;
using std::uint32_t;

void build_tree(const vector<uint32_t>& zcodes,
				vector<Node>& leaf_nodes, vector<Node>& in_nodes, const size_t n);

