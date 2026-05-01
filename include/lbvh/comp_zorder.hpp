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

/**
 * @brief	Quantize normalized centroids to 10-bit resolution (1024 discrete values).
 *
 * @param[in]	centroid_x	All x-coordinates of all centroids.
 * @param[in]	centroid_y	All y-coordinates of all centroids.
 * @param[in]	centroid_z	All z-coordinates of all centroids.
 * @param[in]	num_thr		Number of OpenMP threads.
 *
 * @return	A QCent struct containing uint32_t quantized centroid coordinates (x, y, z)
 * 			with each dimension in its own vector. Each 32-bit int contains a
 * 			10-bit value.
 */
QCent quantize(const vector<float>& centroid_x, const vector<float>& centroid_y,
			   const vector<float>& centroid_z, int num_thr);

/**
 * @brief	Interleaves 3 unsigned 32-bit ints into a single Z-order (Morton) code.
 *
 * @param[in]	qcent	All quantized centroids, unsigned 16-bit ints with 10-bit values.
 * @param[in]	num_thr	Number of OpenMP threads.
 *
 * @reurn	Morton keys, possibly with duplicates.
 */
vector<uint32_t> inter_zorder(const QCent& qcent, int num_thr);

