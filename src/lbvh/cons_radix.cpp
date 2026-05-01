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
static array<uint32_t, 2> determine_range(const vector<uint32_t>& zcodes,
								   		  const size_t n, const size_t idx) {
	/* Determine direction of range +1 or -1 by taking the sign of the
	 * LCP(i, i+1) - LCP(i, i-1). */
	assert(idx < n);
	const int i = zcodes[idx];
	const int lcp1 = __builtin_clz(i ^ zcodes[idx + 1]);
	const int lcp2 = __builtin_clz(i ^ zcodes[idx - 1]);
	int d = -1;	//1 if POS, -1 if NEG
	if(lcp1 - lcp2 >= 0) {
		d = 1;
	}

	//compute upper bound of length of range
	int delta_min = __builtin_clz(i ^ zcodes[(idx + d)]);
	int l_max = 2;
	do {
		l_max <<= 1;
	} while(__builtin_clz(i ^ zcodes[idx + (l_max * d)]) > delta_min);

	//find other end using binary search
	//for t <- {(l_max / 2), (l_max / 4), ..., 1}
	int l = 0;
	for(size_t t = l_max >> 1; t > 0; t >>= 1) {
		if(__builtin_clz(i ^ zcodes[idx + (l + t) * d]) > delta_min) {
			l += t;
		}
	}
	const int j = i + l * d;

	if(i <= j) {
		return {static_cast<uint32_t>(i), static_cast<uint32_t>(j)};
	} else {
		return {static_cast<uint32_t>(j), static_cast<uint32_t>(i)};
	}
}

void build_tree(const vector<uint32_t>& zcodes,
				vector<Node>& leaf_nodes, vector<Node>& in_nodes, const size_t n) {
	//leaf_nodes and in(ternal)_nodes must be properly resized prior to function call.
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

			const uint32_t split = static_cast<uint32_t>(
				find_split(zcodes, first, last)
			);

			//select children
			uint32_t child_a;
			if(split == first) {	//sub-range has only 1 child -> leaf.
				child_a = leaf_nodes[split].first_idx;
			} else {
				//child is another internal node, which may or may not exist.
				child_a = split;
			}

			uint32_t child_b;
			if(split + 1 == last) {
				child_b = leaf_nodes[split + 1].first_idx;
			} else {
				child_b = split + 1;
			}

			/* Record child relationships. Only gives index. Know whether to check
			 * leaf_nodes or in_nodes for child by looking at current in_node's
			 * count. */
			in_nodes[idx].l_child = child_a;
			in_nodes[idx].r_child = child_b;

			//record the child's parent relationships
			if(split == first) {
				leaf_nodes[child_a].parent = idx;
			} else {
				in_nodes[child_a].parent = idx;
			}
			if(split + 1 == last) {
				leaf_nodes[child_b].parent = idx;
			} else {
				in_nodes[child_b].parent = idx;
			}
		}
	}
}

