#include "lbvh/cons_bvh_seq.hpp"
#include "lbvh/cons_radix_seq.hpp"
#include "util/normalize.hpp"
#include <vector>
#include <cstdint>
#include <algorithm>

using std::vector;
using std::uint32_t;

// ====================================================================================
// Post-order traversal to compute AABBs.
// ====================================================================================
void compute_aabb_seq(vector<Node>& nodes,
					  const uint32_t idx, const PrimitiveData& prim_data,
					  const bool first) {
	const uint32_t l_child = nodes[idx].l_child;
	const uint32_t r_child = nodes[idx].r_child;

	//if not root node second visit, traverse.
	if(!(!first && idx == 0)) {
		if(l_child != INVALID_U32) {
			compute_aabb_seq(nodes, l_child, prim_data, false);
		}
		if(r_child != INVALID_U32) {
			compute_aabb_seq(nodes, r_child, prim_data, false);
		}
	}

	//visit node
	vector<float> mins(6, INVALID_MAX_FL);	//[0, 2] = L, [3, 5] = R
	vector<float> maxs(6, INVALID_MIN_FL);

	//if leaf
	if(nodes[idx].count == 1) {
		nodes[idx].min_x = prim_data.min_x[prim_data.prim_id[idx]];
		nodes[idx].min_y = prim_data.min_y[prim_data.prim_id[idx]];
		nodes[idx].min_z = prim_data.min_z[prim_data.prim_id[idx]];

		nodes[idx].max_x = prim_data.max_x[prim_data.prim_id[idx]];
		nodes[idx].max_y = prim_data.max_y[prim_data.prim_id[idx]];
		nodes[idx].max_z= prim_data.max_z[prim_data.prim_id[idx]];
	} else {
		if(l_child != INVALID_U32) {
			mins[0] = prim_data.min_x[prim_data.prim_id[l_child]];
			mins[1] = prim_data.min_y[prim_data.prim_id[l_child]];
			mins[2] = prim_data.min_z[prim_data.prim_id[l_child]];

			maxs[0] = prim_data.max_x[prim_data.prim_id[l_child]];
			maxs[1] = prim_data.max_y[prim_data.prim_id[l_child]];
			maxs[2] = prim_data.max_z[prim_data.prim_id[l_child]];
		}
		if(r_child != INVALID_U32) {
			mins[3] = prim_data.min_x[prim_data.prim_id[r_child]];
			mins[4] = prim_data.min_y[prim_data.prim_id[r_child]];
			mins[5] = prim_data.min_z[prim_data.prim_id[r_child]];

			maxs[3] = prim_data.max_x[prim_data.prim_id[r_child]];
			maxs[4] = prim_data.max_y[prim_data.prim_id[r_child]];
			maxs[5] = prim_data.max_z[prim_data.prim_id[r_child]];
		}

		nodes[idx].min_x = std::min(mins[0], mins[3]);
		nodes[idx].min_y = std::min(mins[1], mins[4]);
		nodes[idx].min_z = std::min(mins[2], mins[5]);

		nodes[idx].max_x = std::max(maxs[0], maxs[3]);
		nodes[idx].max_y = std::max(maxs[1], maxs[4]);
		nodes[idx].max_z = std::max(maxs[2], maxs[5]);
	}
}
void compute_aabb_seq(vector<Node>& leaf_nodes, vector<Node>& in_nodes,
					  const uint32_t idx, const PrimitiveData& prim_data,
					  const bool first) {
	const uint32_t l_child = in_nodes[idx].l_child;
	const uint32_t r_child = in_nodes[idx].r_child;

	//if not root node second visit, traverse.
	if(!(!first && idx == 0)) {
		if(l_child != INVALID_U32) {
			compute_aabb_seq(leaf_nodes, in_nodes, l_child, prim_data, false);
		}
		if(r_child != INVALID_U32) {
			compute_aabb_seq(leaf_nodes, in_nodes, r_child, prim_data, false);
		}
	}

	//visit node
	//prim_id indices are sorted positions; value @ index are original positions.
	vector<float> mins(6, INVALID_MAX_FL);	//[0, 2] = L, [3, 5] = R
	vector<float> maxs(6, INVALID_MIN_FL);
	if(l_child != INVALID_U32) {
		if(in_nodes[idx].l_is_leaf) {
			mins[0] = prim_data.min_x[prim_data.prim_id[l_child]];
			mins[1] = prim_data.min_y[prim_data.prim_id[l_child]];
			mins[2] = prim_data.min_z[prim_data.prim_id[l_child]];

			maxs[0] = prim_data.max_x[prim_data.prim_id[l_child]];
			maxs[1] = prim_data.max_y[prim_data.prim_id[l_child]];
			maxs[2] = prim_data.max_z[prim_data.prim_id[l_child]];

			leaf_nodes[l_child].min_x = mins[0];
			leaf_nodes[l_child].min_y = mins[1];
			leaf_nodes[l_child].min_z = mins[2];

			leaf_nodes[l_child].max_x = maxs[0];
			leaf_nodes[l_child].max_y = maxs[1];
			leaf_nodes[l_child].max_z = maxs[2];

			in_nodes[l_child].min_x = mins[0];
			in_nodes[l_child].min_y = mins[1];
			in_nodes[l_child].min_z = mins[2];

			in_nodes[l_child].max_x = maxs[0];
			in_nodes[l_child].max_y = maxs[1];
			in_nodes[l_child].max_z = maxs[2];
		}

		mins[0] = in_nodes[l_child].min_x;
		mins[1] = in_nodes[l_child].min_y;
		mins[2] = in_nodes[l_child].min_z;

		maxs[0] = in_nodes[l_child].max_x;
		maxs[1] = in_nodes[l_child].max_y;
		maxs[2] = in_nodes[l_child].max_z;
	}
	if(r_child != INVALID_U32) {
		if(in_nodes[idx].r_is_leaf) {
			mins[3] = prim_data.min_x[prim_data.prim_id[r_child]];
			mins[4] = prim_data.min_y[prim_data.prim_id[r_child]];
			mins[5] = prim_data.min_z[prim_data.prim_id[r_child]];

			maxs[3] = prim_data.max_x[prim_data.prim_id[r_child]];
			maxs[4] = prim_data.max_y[prim_data.prim_id[r_child]];
			maxs[5] = prim_data.max_z[prim_data.prim_id[r_child]];

			leaf_nodes[r_child].min_x = mins[3];
			leaf_nodes[r_child].min_y = mins[4];
			leaf_nodes[r_child].min_z = mins[5];

			leaf_nodes[r_child].max_x = maxs[3];
			leaf_nodes[r_child].max_y = maxs[4];
			leaf_nodes[r_child].max_z = maxs[5];

			in_nodes[r_child].min_x = mins[3];
			in_nodes[r_child].min_y = mins[4];
			in_nodes[r_child].min_z = mins[5];

			in_nodes[r_child].max_x = maxs[3];
			in_nodes[r_child].max_y = maxs[4];
			in_nodes[r_child].max_z = maxs[5];
		}

		mins[3] = in_nodes[r_child].min_x;
		mins[4] = in_nodes[r_child].min_y;
		mins[5] = in_nodes[r_child].min_z;

		maxs[3] = in_nodes[r_child].max_x;
		maxs[4] = in_nodes[r_child].max_y;
		maxs[5] = in_nodes[r_child].max_z;
	}

	in_nodes[idx].min_x = std::min(mins[0], mins[3]);
	in_nodes[idx].min_y = std::min(mins[1], mins[4]);
	in_nodes[idx].min_z = std::min(mins[2], mins[5]);

	in_nodes[idx].max_x = std::max(maxs[0], maxs[3]);
	in_nodes[idx].max_y = std::max(maxs[1], maxs[4]);
	in_nodes[idx].max_z = std::max(maxs[2], maxs[5]);
}

