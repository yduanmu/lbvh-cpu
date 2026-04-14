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

// ------------------------------------------------------------------------------------
// Quantize normalized centroids
// ------------------------------------------------------------------------------------
QCent quantize(vector<float>& centroid_x, vector<float>& centroid_y,
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

	//vectorized quantize loop
	size_t i = 0;
	__m256 q = _mm256_set1_ps(mult);
	__m256i* out_x = reinterpret_cast<__m256i*>(qcent_x.data());
	__m256i* out_y = reinterpret_cast<__m256i*>(qcent_y.data());
	__m256i* out_z = reinterpret_cast<__m256i*>(qcent_z.data());
	for(; i + 8 <= count; i+= 8) {
		__m256 cx = _mm256_loadu_ps(centroid_x.data() + i);
		__m256 cy = _mm256_loadu_ps(centroid_y.data() + i);
		__m256 cz = _mm256_loadu_ps(centroid_z.data() + i);

		//quantized floats
		__m256 cx_qf = _mm256_mul_ps(cx, q);
		__m256 cy_qf = _mm256_mul_ps(cy, q);
		__m256 cz_qf = _mm256_mul_ps(cz, q);	

		_mm256_storeu_si256(out_x + i, _mm256_cvtps_epi32(cx_qf));
		_mm256_storeu_si256(out_y + i, _mm256_cvtps_epi32(cy_qf));
		_mm256_storeu_si256(out_z + i, _mm256_cvtps_epi32(cz_qf));
	}

	//quantize the remainder
	int multi = static_cast<int>(mult);
	for(; i < count; ++i) {
		qcent_x[i] = static_cast<int>(centroid_x[i]) * multi;	
		qcent_y[i] = static_cast<int>(centroid_y[i]) * multi;	
		qcent_z[i] = static_cast<int>(centroid_z[i]) * multi;	
	}
	return {qcent_x, qcent_y, qcent_z};
}

// ------------------------------------------------------------------------------------
// Interleave -> Z-order codes
// ------------------------------------------------------------------------------------


// #endif

