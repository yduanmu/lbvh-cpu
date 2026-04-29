/* 
 * MIT License
 * Copyright (c) 2018-2021 Eddy L O Jansson
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "lbvh/sort_zorder_seq.hpp"
#include <cstdint>
#include <vector>
#include <array>
#include <iostream>
#include <chrono>

using std::vector;
using std::uint32_t;
using std::array;
using std::chrono::duration_cast;
using std::chrono::milliseconds;
using std::chrono::steady_clock;

// ====================================================================================
// Sequential radix sort from eloj's radix_sort_u32.c, which I converted to C++.
// ====================================================================================
void radix_sort_seq(vector<uint32_t>& zcodes) {
	auto t0 = steady_clock::now();

	size_t n = zcodes.size();
	vector<uint32_t> zcodes_aux;
	zcodes_aux.resize(n);

	//Histograms. 1 vector per pass.
	array<size_t, 256> count0 = {0};
	array<size_t, 256> count1 = {0};
	array<size_t, 256> count2 = {0};
	array<size_t, 256> count3 = {0};

	// --------------------------------------------------------------------------------
	// Generate histograms.
	// --------------------------------------------------------------------------------
	for(size_t i = 0; i < n; ++i) {
		uint32_t key = zcodes[i];
		++count0[key & 0xFF];
		++count1[(key >> 8) & 0xFF];
		++count2[(key >> 16) & 0xFF];
		++count3[(key >> 24) & 0xFF];
	}
	
	// --------------------------------------------------------------------------------
	// Calculate prefix sums.
	// --------------------------------------------------------------------------------
	size_t a0 = 0;
	size_t a1 = 0;
	size_t a2 = 0;
	size_t a3 = 0;
	for(size_t i = 0; i < 256; ++i) {
		size_t b0 = count0[i];
		size_t b1 = count1[i];
		size_t b2 = count2[i];
		size_t b3 = count3[i];
		count0[i] = a0;
		count1[i] = a1;
		count2[i] = a2;
		count3[i] = a3;
		a0 += b0;
		a1 += b1;
		a2 += b2;
		a3 += b3;
	}

	// --------------------------------------------------------------------------------
	// Sort in 4 passes in LSB order.
	// --------------------------------------------------------------------------------
	for(size_t i = 0; i < n; ++i) {
		uint32_t key = zcodes[i];
		size_t dest = count0[key & 0xFF]++;
		zcodes_aux[dest] = zcodes[i];
	}

	for(size_t i = 0; i < n; ++i) {
		uint32_t key = zcodes_aux[i];
		size_t dest = count1[(key >> 8) & 0xFF]++;
		zcodes[dest] = zcodes_aux[i];
	}

	for(size_t i = 0; i < n; ++i) {
		uint32_t key = zcodes[i];
		size_t dest = count2[(key >> 16) & 0xFF]++;
		zcodes_aux[dest] = zcodes[i];
	}

	for(size_t i = 0; i < n; ++i) {
		uint32_t key = zcodes_aux[i];
		size_t dest = count3[(key >> 24) & 0xFF]++;
		zcodes[dest] = zcodes_aux[i];
	}

	auto t1 = steady_clock::now();
	auto elapsed = duration_cast<milliseconds>(t1 - t0);
	std::cout << "sort_zorder_SEQ complete: " << elapsed.count() << std::endl;
} 

