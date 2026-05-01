#include <lbvh/cons_radix_seq.hpp>
#include <cstdint>
#include <vector>

using std::vector;
using std::uint32_t;

static_assert(alignof(Node) == 32);

// ====================================================================================
// Find splitting point via clz + binary search.
// Converted to CPU-friendly C++ from Kar12b (see README).
// ====================================================================================
uint32_t find_split(const vector<uint32_t>& zcodes, uint32_t first, uint32_t last) {
	const uint32_t first_key = zcodes[first];
	const uint32_t last_key = zcodes[last];

	//identical morton codes -> split range in middle
	if(first_key == last_key) {
		return static_cast<uint32_t>((first + last) >> 1);
	}

	const int common_prefix = __builtin_clz(first_key ^ last_key);

	/* Binary search to find where next bit differs. Looking for highest key that
	 * shares more than common_prefix bits with the first key.*/
	uint32_t split = first;	//initial guess
	int step = last - first;

	do {
		step = (step + 1) >> 1;	//exponential decrease
		const uint32_t new_split = split + step;	//proposed new position
		
		if(new_split < last) {
			const uint32_t split_key = zcodes[new_split];
			const int split_prefix = __builtin_clz(first_key ^ split_key);
			if(split_prefix > common_prefix) {
				split = new_split;	//accept proposal
			}
		}
	}
	while(step > 1);

	return split;
}

// ====================================================================================
// Recursively construct binary radix tree.
// Converted to CPU-friendly C++ from Kar12b (see README).
// ====================================================================================
uint32_t build_tree_seq(const vector<uint32_t>& zcodes, vector<Node>& nodes,
						uint32_t first, uint32_t last,
						uint32_t parent) {
	const uint32_t node_idx = static_cast<uint32_t>(nodes.size());
	nodes.push_back({});

	Node& node = nodes[node_idx];
	node.parent = parent;
	node.l_child = INVALID_U32;
	node.r_child = INVALID_U32;
	node.first_idx = first;
	node.count = last - first + 1;
	node.split = INVALID_U32;

	//if leaf
	if(first == last) {
		return node_idx;
	}

	//calculate appropriate ranges of left and right child
	const uint32_t split = static_cast<uint32_t>(
		find_split(zcodes, first, last)
	);
	node.split = split;

	const uint32_t l_child = build_tree_seq(zcodes, nodes, first, split, node_idx);
	const uint32_t r_child = build_tree_seq(zcodes, nodes, split + 1, last, node_idx);
	nodes[node_idx].l_child = l_child;	//safely use index rather than node.l_child
	nodes[node_idx].r_child = r_child;

	return node_idx;
}

