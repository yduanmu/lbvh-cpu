#include <cstdint>
#include <vector>
#include <algorithm>
#include <chrono>
#include <limits>

using std::vector;
using std::uint32_t;
using std::chrono::duration_cast;
using std::chrono::microseconds;
using std::chrono::steady_clock;

static constexpr uint32_t INVALID_U32 = std::numeric_limits<uint32_t>::max();

struct alignas(32) Node {
	uint32_t parent;			//INVALID_U32 if root
	uint32_t l_child, r_child;	//INVALID_U32 if leaf
	uint32_t key_offset;		//index of 1st key within range
	uint32_t count;				//num keys covered (1 if leaf)
	uint32_t split;				//split index when building
};

static_assert(alignof(Node) == 32);

// ====================================================================================
// Helpers.
// ====================================================================================
inline bool isLeaf(const Node& n) {
	return(n.count == 1);
}

uint32_t find_split(const vector<uint32_t>& zcodes, const vector<uint32_t>& prim_ids,
					uint32_t first, uint32_t last) {
	//
}

// ====================================================================================
// Recursively construct binary radix tree.
// Converted to CPU-friendly C++ from Kar12b (see README).
// ====================================================================================


