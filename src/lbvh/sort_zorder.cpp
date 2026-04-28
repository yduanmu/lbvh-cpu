#include "lbvh/sort_zorder.hpp"
#include <cstdint>
#include <omp.h>
#include <vector>

using std::vector;
using std::uint32_t;

// ====================================================================================
// Radix sort parallelized from eloj's radix_sort_u32.c implementation.
// ====================================================================================
void prefix_sums(vector<Count>& offset, vector<Count>& count, size_t num_thr){
	//starts are the beginning offsets of each bucket.
	size_t start = 0;

	for(size_t bucket = 0; bucket < 256; ++bucket) {
		//runs are the per-thread offsets of each bucket.
		size_t run = start;

		for(size_t t = 0; t < num_thr; ++t){
			/* Offset = run. They decide what slice of the current bucket each thread
			 * owns. */
			offset[t].local[bucket] = run;

			/* Increase run by amount of elements in each histogram bucket to
			 * prepare for the next thread's offset calculation. */
			run += count[t].local[bucket];
		}

		//prepare for offset calculation of next bucket
		start = run;
	}
}

void radix_sort(vector<uint32_t>& zcodes, size_t num_thr) {
	size_t n = zcodes.size();
	vector<uint32_t> zcodes_aux;
	zcodes_aux.resize(n);

	/* Per-thread histograms. Aligned to prevent false sharing. One vector per pass;
	 * within each vector is the per-thread histogram vector. */
	vector<Count> count0, count1, count2, count3;
	count0.resize(num_thr);
	count1.resize(num_thr);
	count2.resize(num_thr);
	count3.resize(num_thr);

	//offset arrays
	vector<Count> offset0, offset1, offset2, offset3;
	offset0.resize(num_thr);
	offset1.resize(num_thr);
	offset2.resize(num_thr);
	offset3.resize(num_thr);

	omp_set_dynamic(0);
	omp_set_num_threads(num_thr);
	
	// --------------------------------------------------------------------------------
	// Sort in 4 passes in LSB order.
	// --------------------------------------------------------------------------------
	#pragma omp parallel
	{
		int tid = omp_get_thread_num();

		// ------------------------------- Pass 0. ------------------------------------
		/* Each thread only mutates in its own slice. These offsets are calculated by
		 * histogramming the current pass and taking the prefix sums. */

		//histograms
		#pragma omp for schedule(static)
		for(size_t i = 0; i < n; ++i ) {
			/* Mask keys into octets to prepare for 4 passes of 8 bits each, then
			 * histogram each of the octet arrays to prepare for the 4 passes. */
			uint32_t key = zcodes[i];
			++count0[tid].local[key & 0xFF];
		}

		/* Prefix sum: histogram of digit counts is now tranformed to memory address
		 * offsets for the complete vector of keys. */
		#pragma omp single
		{
			prefix_sums(offset0, count0, num_thr);
		}

		/* Now sort the current pass. */
		#pragma omp for schedule(static)
		for(size_t i = 0; i < n; ++i) {
			uint32_t key = zcodes[i];
			uint8_t digit = key & 0xFF;
			size_t dest = offset0[tid].local[digit] ++;
			zcodes_aux[dest] = zcodes[i];
		}
		
		// ------------------------------- Pass 1. ------------------------------------
		#pragma omp for schedule(static)
		for(size_t i = 0; i < n; ++i ) {
			uint32_t key = zcodes_aux[i];
			++count1[tid].local[(key >> 8) & 0xFF];
		}

		#pragma omp single
		{
			prefix_sums(offset1, count1, num_thr);
		}

		#pragma omp for schedule(static)
		for(size_t i = 0; i < n; ++i) {
			uint32_t key = zcodes_aux[i];
			uint8_t digit = (key >> 8) & 0xFF;
			size_t dest = offset1[tid].local[digit] ++;
			zcodes[dest] = zcodes_aux[i];
		}

		// ------------------------------- Pass 2. ------------------------------------
		#pragma omp for schedule(static)
		for(size_t i = 0; i < n; ++i ) {
			uint32_t key = zcodes[i];
			++count2[tid].local[(key >> 16) & 0xFF];
		}

		#pragma omp single
		{
			prefix_sums(offset2, count2, num_thr);
		}

		#pragma omp for schedule(static)
		for(size_t i = 0; i < n; ++i) {
			uint32_t key = zcodes[i];
			uint8_t digit = (key >> 16) & 0xFF;
			size_t dest = offset2[tid].local[digit] ++;
			zcodes_aux[dest] = zcodes[i];
		}

		// ------------------------------- Pass 3. ------------------------------------
		#pragma omp for schedule(static)
		for(size_t i = 0; i < n; ++i ) {
			uint32_t key = zcodes_aux[i];
			++count3[tid].local[(key >> 24) & 0xFF];
		}

		#pragma omp single
		{
			prefix_sums(offset3, count3, num_thr);
		}

		#pragma omp for schedule(static)
		for(size_t i = 0; i < n; ++i) {
			uint32_t key = zcodes_aux[i];
			uint8_t digit = (key >> 24) & 0xFF;
			size_t dest = offset3[tid].local[digit] ++;
			zcodes[dest] = zcodes_aux[i];
		}

	}
}

