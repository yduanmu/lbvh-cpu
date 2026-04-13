#include "lbvh/comp_zorder.hpp"
#include <vector>
#include <cstdint>
#include <immintrin.h>

using std::vector;
using std::uint32_t;

// #ifdef __AVX512F__

// #else	//AVX2
struct QCent {
	vector<int> qcent_x, qcent_y, qcent_z;
};

//quantize normalized centroids
vector<int> quantize(vector<float>& centroid_x, vector<float>& centroid_y,
					 vector<float>& centroid_z, bool bit_res) {
	vector<int> qcent_x, qcent_y, qcent_z;
	size_t count = centroid_x.size();
	qcent_x.resize(count);
	qcent_y.resize(count);
	qcent_z.resize(count);
	
	float mult;
	if(bit_res) {
		mult = (1 << 21) - 1;
	} else {
		mult = (1 << 10) - 1;
	}

	//SIMD quantize loop
	size_t i = 0;
	__m256 q = _mm256_set1_ps(mult);
	for(; i + 8 <= count; i+= 8) {
		__m256 cx = _mm256_loadu_ps(centroid_x.data() + i);
		__m256 cy = _mm256_loadu_ps(centroid_y.data() + i);
		__m256 cz = _mm256_loadu_ps(centroid_z.data() + i);

		__m256 cx_result = _mm256_mul_ps(cx, q);
		__m256 cy_result = _mm256_mul_ps(cy, q);
		__m256 cz_result = _mm256_mul_ps(cz, q);
	}

	//quantize the remainder
	for(; i < count; ++i) {
		//
	}
}

//interleave

// #endif

