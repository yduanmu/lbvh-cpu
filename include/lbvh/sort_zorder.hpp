#pragma once

#include <cstdint>
#include <vector>
#include <array>

using std::vector;
using std::uint32_t;
using std::array;

struct alignas(64) Count {
	array<size_t, 256> local = {0};
};

void prefix_sums(vector<Count>& offset, vector<Count>& count, size_t num_thr);

void radix_sort(vector<uint32_t>& zcodes, size_t num_thr);

