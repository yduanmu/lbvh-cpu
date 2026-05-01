#pragma once

#include <cstdint>
#include <vector>
#include <limits>

using std::vector;
using std::uint32_t;

struct alignas(32) Node {
	uint32_t parent;			//INVALID_U32 if root
	uint32_t l_child, r_child;	//INVALID_U32 if leaf
	uint32_t first_idx;			/* Index of 1st key within range; is only key if leaf.
								 * No need to store the code or prim_id; just lookup
								 * by the zcodes key index.*/
	uint32_t count;				//num keys covered (1 if leaf)
	uint32_t split;				//split index when building
};

constexpr uint32_t INVALID_U32 = std::numeric_limits<uint32_t>::max();

uint32_t build_tree_seq(const vector<uint32_t>& zcodes, vector<Node>& nodes,
						uint32_t first, uint32_t last,
						uint32_t parent = INVALID_U32);

