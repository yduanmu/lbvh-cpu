#include "lbvh/cons_radix.hpp"
#include "lbvh/cons_radix_seq.hpp"	//for shared structs and functions
#include <cstdint>
#include <vector>
#include <omp.h>
#include <array>
#include <cassert>

using std::vector;
using std::uint32_t;
using std::array;

static_assert(alignof(Node) == 32);

// ====================================================================================
// Bottom-up radix tree construction.
// ====================================================================================
static inline array<uint32_t, 2> determine_range(const vector<uint32_t>& zcodes,
								   				 const size_t n, const size_t idx) {
	/* Determine direction of range +1 or -1 by taking the sign of the
	 * LCP(i, i+1) - LCP(i, i-1). */
	assert(idx < n - 1);
	const int delta_next = common_prefix(zcodes, idx, idx + 1);
	const int delta_prev = common_prefix(zcodes, idx, idx - 1);
	int d = (delta_next - delta_prev >= 0) ? 1 : -1;

	//compute upper bound of length of range
	int delta_min = common_prefix(zcodes, idx, idx - d);
	int l_max = 2;
	while (true) {
        const int j = idx + l_max * d;
        if (j < 0 || j >= static_cast<int>(n)) {
            break;
        }
        if (common_prefix(zcodes, idx, j) <= delta_min) {
            break;
        }
        l_max <<= 1;
    }
	
	//find other end using binary search
	//for t <- {(l_max / 2), (l_max / 4), ..., 1}
	int l = 0;
	for(size_t t = l_max >> 1; t > 0; t >>= 1) {
		const int j = idx + (l + t) * d;
        if (j >= 0 && j < static_cast<int>(n)
				   && common_prefix(zcodes, idx, j) > delta_min) {
            l += t;
        }
	}

	const size_t j = idx + l * d;
	if(idx <= j) {
		return {static_cast<uint32_t>(idx), static_cast<uint32_t>(j)};
	} else {
		return {static_cast<uint32_t>(j), static_cast<uint32_t>(idx)};
	}
}

void build_tree(const vector<uint32_t>& zcodes, vector<Node>& leaf_nodes,
					vector<Node>& in_nodes, const size_t n, const size_t num_threads) {
	//leaf_nodes and in(ternal)_nodes must be properly resized prior to function call.
	omp_set_num_threads(num_threads);
	omp_set_dynamic(0);
	#pragma omp parallel
	{
		// --------------------------------------------------------------------------------
		// Construct leaf nodes.
		// --------------------------------------------------------------------------------
		#pragma omp for schedule(static)
		for(size_t i = 0; i < n; ++i) {
			leaf_nodes[i].first_idx = i;
			leaf_nodes[i].l_child = INVALID_U32;
			leaf_nodes[i].r_child = INVALID_U32;
			leaf_nodes[i].count = 1;
			leaf_nodes[i].parent = INVALID_U32;
		}

		// --------------------------------------------------------------------------------
		// Construct internal nodes.
		// --------------------------------------------------------------------------------
		#pragma omp for schedule(static)
		for(size_t idx = 0; idx < n - 1; ++idx) {
			//determine range of keys covered by current node
			array<uint32_t, 2> range = determine_range(zcodes, n, idx);
			uint32_t first = range[0];
			uint32_t last = range[1];
			in_nodes[idx].first_idx = first;
			in_nodes[idx].count = last - first + 1;
			in_nodes[idx].l_is_leaf = false;
			in_nodes[idx].r_is_leaf = false;
			in_nodes[idx].parent = INVALID_U32;

			const uint32_t split = static_cast<uint32_t>(
				find_split(zcodes, first, last)
			);

			//select children
			uint32_t child_a;
			if(split == first) {	//sub-range has only 1 child -> leaf.
				child_a = first;
				leaf_nodes[child_a].parent = idx;
				in_nodes[idx].l_is_leaf = true;
			} else {
				//child is another internal node, which may or may not exist.
				child_a = split;
				in_nodes[child_a].parent = idx;
			}
			in_nodes[idx].l_child = child_a;

			uint32_t child_b;
			if(split + 1 == last) {
				child_b = last;
				leaf_nodes[child_b].parent = idx;
				in_nodes[idx].r_is_leaf = true;
			} else {
				child_b = split + 1;
				in_nodes[child_b].parent = idx;
			}
			in_nodes[idx].r_child = child_b;
		}
	}
}

