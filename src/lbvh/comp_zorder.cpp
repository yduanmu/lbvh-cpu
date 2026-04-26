#include "lbvh/comp_zorder.hpp"
#include <vector>
#include <cstdint>
#include <immintrin.h>
#include <cmath>
#include <omp.h>

using std::vector;
using std::uint32_t;
using std::uint16_t;

// ====================================================================================
// Discretize & Quantize.
// ====================================================================================
QCent quantize(const vector<float>& centroid_x, const vector<float>& centroid_y,
			   const vector<float>& centroid_z, int num_thr) {
	vector<uint16_t> qcent_x, qcent_y, qcent_z;
	size_t n = centroid_x.size();
	qcent_x.resize(n);
	qcent_y.resize(n);
	qcent_z.resize(n);
	
	float mult = (1 << 10) - 1; //multiply by 1023 to quantize to 10-bit resolution

	//vectorized quantize loop
	size_t limit = n - (n % 8);
	__m256 q = _mm256_set1_ps(mult);

	omp_set_num_threads(num_thr);
	#pragma omp parallel for schedule(static)
	for(size_t i = 0; i < limit; i += 8) {
		__m256 cx = _mm256_loadu_ps(centroid_x.data() + i);
		__m256 cy = _mm256_loadu_ps(centroid_y.data() + i);
		__m256 cz = _mm256_loadu_ps(centroid_z.data() + i);

		//quantize floats
		__m256 cx_qf = _mm256_mul_ps(cx, q);
		__m256 cy_qf = _mm256_mul_ps(cy, q);
		__m256 cz_qf = _mm256_mul_ps(cz, q);

		//round to nearest, tie to even. 8 32-bit ints
		__m256i i32_x = _mm256_cvtps_epi32(cx_qf);
		__m256i i32_y = _mm256_cvtps_epi32(cy_qf);
		__m256i i32_z = _mm256_cvtps_epi32(cz_qf);

		//pack into 16-bit ints
		//AVX2 has no 256-bit version of packus, so must split into 2 128-bit halves
		_mm_storeu_si128(reinterpret_cast<__m128i*>(qcent_x.data()) + i,
						 _mm_packus_epi32(_mm256_castsi256_si128(i32_x),
										  _mm256_extracti128_si256(i32_x, 1)));
		_mm_storeu_si128(reinterpret_cast<__m128i*>(qcent_y.data() + i),
						 _mm_packus_epi32(_mm256_castsi256_si128(i32_y),
										  _mm256_extracti128_si256(i32_y, 1)));
		_mm_storeu_si128(reinterpret_cast<__m128i*>(qcent_z.data() + i),
						 _mm_packus_epi32(_mm256_castsi256_si128(i32_z),
										  _mm256_extracti128_si256(i32_z, 1)));
	}

	//sequentially quantize the remainder
	int multi = static_cast<int>(mult);
	for(size_t i = limit; i < n; ++i) {
		qcent_x[i] = static_cast<uint16_t>(std::round(centroid_x[i] * multi));	
		qcent_y[i] = static_cast<uint16_t>(std::round(centroid_y[i] * multi));	
		qcent_z[i] = static_cast<uint16_t>(std::round(centroid_z[i] * multi));	
	}

	return {qcent_x, qcent_y, qcent_z};
}

// ====================================================================================
// Interleave to form Z-order codes
// ====================================================================================
inline __m256i expand_bits(__m256i v) {
    const __m256i m1 = _mm256_set1_epi32(0x030000FF);
    const __m256i m2 = _mm256_set1_epi32(0x0300F00F);
    const __m256i m3 = _mm256_set1_epi32(0x030C30C3);
    const __m256i m4 = _mm256_set1_epi32(0x09249249);

    v = _mm256_and_si256(_mm256_or_si256(v, _mm256_slli_epi32(v, 16)), m1);
    v = _mm256_and_si256(_mm256_or_si256(v, _mm256_slli_epi32(v, 8)),  m2);
    v = _mm256_and_si256(_mm256_or_si256(v, _mm256_slli_epi32(v, 4)),  m3);
    v = _mm256_and_si256(_mm256_or_si256(v, _mm256_slli_epi32(v, 2)),  m4);

    return v;
}

vector<uint32_t> inter_zorder(const QCent& qcent, int num_thr) {
	vector<uint32_t> zcodes;
	size_t n = qcent.x.size();
	zcodes.resize(n);

	size_t limit = n - (n % 8);
	omp_set_num_threads(num_thr);
	#pragma omp parallel for schedule(static)
	for(size_t i = 0; i < limit; i += 8) {
		/* Our centroids are now in 16-bit ints with each a 10-bit value, but
		 * expand_bits() expects 32-bit ints instead, so we must widen them. AVX2
		 * widens 8 16->32-bit ints at a time, and expand_bits() processes 3x8 32-bit
		 * at a time. */

		__m128i buf16 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(qcent.x.data() + i));
		__m256i x = _mm256_cvtepu16_epi32(buf16);

		buf16 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(qcent.y.data() + i));
		__m256i y = _mm256_cvtepu16_epi32(buf16);

		buf16 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(qcent.z.data() + i));
		__m256i z = _mm256_loadu_epi32(qcent.z.data() + i);

		//expand out
		x = expand_bits(x);
        y = expand_bits(y);
        z = expand_bits(z);

		//interleave together
		//@TODO check if it's slli (higher address )or srli (lower address)
		y = _mm256_slli_epi32(y, 1);
		z = _mm256_slli_epi32(z, 2);
		_mm256_storeu_si256(reinterpret_cast<__m256i*>(zcodes.data() + i),
							_mm256_or_si256(x,_mm256_or_si256(y, z)));
	}

	//sequential remainder loop
	for(size_t i = limit; i < n; ++i) {
		uint32_t x = qcent.x[i];
		uint32_t y = qcent.y[i];
		uint32_t z = qcent.z[i];

		//expand out
		x = (x | (x << 16)) & 0x030000FF;
		y = (y | (y << 16)) & 0x030000FF;
		z = (z | (z << 16)) & 0x030000FF;

		x = (x | (x << 8)) & 0x0300F00F;
		y = (y | (y << 8)) & 0x0300F00F;
		z = (z | (z << 8)) & 0x0300F00F;

		x = (x | (x << 4)) & 0x030C30C3;
		y = (y | (y << 4)) & 0x030C30C3;
		z = (z | (z << 4)) & 0x030C30C3;

		x = (x | (x << 2)) & 0x09249249;
		y = (y | (y << 2)) & 0x09249249;
		z = (z | (z << 2)) & 0x09249249;

		//interleave together
		zcodes[i] = x | (y << 1) | (z << 2);
	}
	return zcodes;
}

