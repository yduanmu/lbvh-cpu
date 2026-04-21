#include "lbvh/comp_zorder.hpp"
#include <vector>
#include <cstdint>
#include <immintrin.h>
#include <cmath>
#include <omp.h>

using std::vector;
using std::uint32_t;
using std::uint16_t;

// #ifdef __AVX512F__

// #else	//AVX2
struct QCent {
	vector<uint16_t> qcent_x, qcent_y, qcent_z;
};

// ------------------------------------------------------------------------------------
// Quantize normalized centroids
// ------------------------------------------------------------------------------------
QCent quantize(vector<float>& centroid_x, vector<float>& centroid_y,
			   vector<float>& centroid_z, bool bit_res) {
	vector<uint16_t> qcent_x, qcent_y, qcent_z;
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
	__m128i* out_x = reinterpret_cast<__m128i*>(qcent_x.data());
	__m128i* out_y = reinterpret_cast<__m128i*>(qcent_y.data());
	__m128i* out_z = reinterpret_cast<__m128i*>(qcent_z.data());
	for(; i + 8 <= count; i+= 8) {
		__m256 cx = _mm256_loadu_ps(centroid_x.data() + i);
		__m256 cy = _mm256_loadu_ps(centroid_y.data() + i);
		__m256 cz = _mm256_loadu_ps(centroid_z.data() + i);

		//quantize floats
		__m256 cx_qf = _mm256_mul_ps(cx, q);
		__m256 cy_qf = _mm256_mul_ps(cy, q);
		__m256 cz_qf = _mm256_mul_ps(cz, q);

		//round to nearest, tie to even. 8 32-bit ints
		__m256 i32_x = _mm256_cvtps_epi32(cx_qf);
		__m256 i32_y = _mm256_cvtps_epi32(cy_qf);
		__m256 i32_z = _mm256_cvtps_epi32(cz_qf);

		//pack into 16-bit ints
		//AVX2 has no 256-bit version of packus, so must split into 2 128-bit halves
		_mm_storeu_si128(out_x + i, _mm_packus_epi32(_mm256_castsi256_si128(i32_x),
													 _mm256_extracti128_si256(i32_x, 1)));
		_mm_storeu_si128(out_y + i, _mm_packus_epi32(_mm256_castsi256_si128(i32_y),
													 _mm256_extracti128_si256(i32_y, 1)));
		_mm_storeu_si128(out_z + i, _mm_packus_epi32(_mm256_castsi256_si128(i32_z),
													 _mm256_extracti128_si256(i32_z, 1)));
	}

	//quantize the remainder
	int multi = static_cast<int>(mult);
	for(; i < count; ++i) {
		qcent_x[i] = static_cast<uint16_t>(std::round(centroid_x[i] * multi));	
		qcent_y[i] = static_cast<uint16_t>(std::round(centroid_y[i] * multi));	
		qcent_z[i] = static_cast<uint16_t>(std::round(centroid_z[i] * multi));	
	}
	return {qcent_x, qcent_y, qcent_z};
}

// ------------------------------------------------------------------------------------
// Interleave to form Z-order codes
// ------------------------------------------------------------------------------------
__m256i interleave32(uint32_t n) {
	//
}

vector<uint32_t> inter_zorder(QCent qcent) {
}

// #endif

