#pragma once

#include <vector>
#include <cstdint>
#include <immintrin.h>

using std::vector;
using std::uint32_t;
using std::uint16_t;

struct QCent {
	vector<uint16_t> x, y, z;
};

QCent quantize(vector<float>& centroid_x, vector<float>& centroid_y,
			   vector<float>& centroid_z, int num_thr);

inline __m256i expand_bits(__m256i v);

vector<uint32_t> inter_zorder(QCent qcent, int num_thr);

