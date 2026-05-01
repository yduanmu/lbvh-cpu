#pragma once
#include "lbvh/cons_radix_seq.hpp"
#include "util/normalize.hpp"
#include <limits>

constexpr float INVALID_MAX_FL = std::numeric_limits<float>::max();
constexpr float INVALID_MIN_FL = std::numeric_limits<float>::min();

void compute_aabb_seq(vector<Node>& nodes,
					  const uint32_t idx, const PrimitiveData& prim_data,
					  const bool first);

void compute_aabb_seq(vector<Node>& leaf_nodes, vector<Node>& in_nodes,
					  const uint32_t idx, const PrimitiveData& prim_data,
					  const bool first);

