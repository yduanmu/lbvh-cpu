#pragma once

#include <cstdint>
#include <vector>
#include <limits>

using std::vector;
using std::uint32_t;

struct alignas(32) Node {
	uint32_t parent;			//INVALID_U32 if root
	uint32_t l_child, r_child;	//INVALID_U32 if leaf
	uint32_t first_idx;			/* Index of 1st key within range; is idx of key if leaf.
								 * No need to store the code or prim_id; just lookup by
								 * the zcodes key index.*/
	uint32_t count;				//num keys covered (1 if leaf)
	uint32_t split;				//split index when building
	bool l_is_leaf, r_is_leaf;	//used in parallel cons
};

constexpr uint32_t INVALID_U32 = std::numeric_limits<uint32_t>::max();

inline int common_prefix(const vector<uint32_t>& zcodes, int i, int j) {
	const int n = static_cast<int>(zcodes.size());
	if(j < 0 || j >= n) {
		return -1;
	}
	if(i == j) {
		return 32;	//LCP the entire sequence
	}

	const uint32_t key_i = zcodes[static_cast<size_t>(i)];
    const uint32_t key_j = zcodes[static_cast<size_t>(j)];
    const uint32_t key_xor = key_i ^ key_j;

    if (key_xor != 0) {
        return __builtin_clz(key_xor);
    }

    //tie-break duplicate Morton codes by index. i != j here, so idx_xor != 0.
    const uint32_t idx_xor = static_cast<uint32_t>(i) ^ static_cast<uint32_t>(j);
    return 32 + __builtin_clz(idx_xor);
}

uint32_t find_split(const vector<uint32_t>& zcodes, uint32_t first, uint32_t last);

uint32_t build_tree_seq(const vector<uint32_t>& zcodes, vector<Node>& nodes,
						uint32_t first, uint32_t last,
						uint32_t parent = INVALID_U32);

