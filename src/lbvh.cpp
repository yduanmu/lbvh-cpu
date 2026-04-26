#include <iostream>
#include <unistd.h>
#include <string>
#include <bitset>
#include <optional>
#include <chrono>
#include <cstdint>
#include <random>
#include <fstream>
#include <string>
#include "util/normalize.hpp"
#include "lbvh/comp_zorder.hpp"

using std::cout;
using std::cerr;
using std::endl;
using std::chrono::duration_cast;
using std::chrono::milliseconds;
using std::chrono::steady_clock;
using std::vector;
using std::uint32_t;

// ====================================================================================
// Command-line config
// ====================================================================================
struct Config {
	std::string filename;
	unsigned int num_threads = 16; // if 1, sequential

	//parallel default
	bool comp_zorder = true;
	bool sort_zorder = true;
	bool cons_radix = true;
	bool cons_bvh = true;
};

void usage() {
	cerr << "usage:\n"
			"\t-f <file name>\n"
			"\t-t <number of threads>\n"
			"\t 	(default 16. 1 means sequential)\n"
			"\t		(for sequential BVH construction, see sah.cpp)\n"
			"\t-c default true flag for parallel Z-order code computation\n"
	"\t-s default true flag for parallel radix sort\n"
	"\t 	(if false, uses sequential radix sort)"
			"\t-r default true flag for parallel construction of binary radix tre>\n"
			"\t-b default true flag for parallel BVH construction\n"
			"\t-h print this message";

	exit(0);
}

Config parse_args(int argc, char** argv) {
	Config cfg;
	int ch;
	while ((ch = getopt(argc, argv, "f:t:csrbh")) != -1) {
		try {
			switch(static_cast<char>(ch)){
				case 'f':
					cfg.filename = optarg;
					break;
				case 't':
					cfg.num_threads = std::stoi(optarg);
					break;
				case 'c':
					cfg.comp_zorder = false;
					break;
				case 's':
					cfg.sort_zorder = false;
					break;
				case 'r':
					cfg.cons_radix = false;
					break;
				case 'b':
					cfg.cons_bvh = false;
break;
				case 'h':
				case '?':
					usage();
			}
		} catch (std::exception const& ex) {
		cerr << "invalid argument to -" << static_cast<char>(ch)
			 << ": " << optarg << endl;
		usage();
		}
	}
	return cfg;
}

// ====================================================================================
// Cheap and easy testcases.
// ====================================================================================
// ------------------------------------------------------------------------------------
// Test comp_zorder.cpp
// ------------------------------------------------------------------------------------
vector<float> generate_centroids(size_t n, unsigned int seed, std::string fn) {
	vector<float> val;
	val.resize(n);
	std::mt19937 prng(seed);
	std::uniform_real_distribution<float> dist(0.0, 3.0);

	//generate random and then write
	std::ofstream centroids("tests/comp_zorder/" + fn + ".txt");
	if(centroids.is_open()) {
		for(size_t i = 0; i < n; ++i) {
			val[i] = dist(prng);
			centroids << val[i] << "\n";
		}
	} else {
		cerr << "Unable to open " + fn + ".txt";
	}
	return val;
}

void test_comp_zorder(const vector<float>& cent_x, const vector<float>& cent_y,
			   		  const vector<float>& cent_z, int num_thr) {
	size_t n = cent_x.size();

	QCent qcent;
	qcent.x.resize(n);
	qcent.y.resize(n);
	qcent.z.resize(n);

	vector<uint32_t> zcodes;
	zcodes.resize(n);

	//quantize
	auto t0 = steady_clock::now();
	qcent = quantize(cent_x, cent_y, cent_z, num_thr);
	auto t1 = steady_clock::now();
	auto elapsed = duration_cast<milliseconds>(t1 - t0);
	cout << "finished quantize(): " << elapsed.count() << endl;

	//write quantized values
	std::ofstream quantized("tests/comp_zorder/quantized.txt");
	if(quantized.is_open()) {
		for(size_t i = 0; i < n; ++i) {
			quantized << qcent.x[i] << "	"
					  << qcent.y[i] << "	"
					  << qcent.z[i] << "\n";
		}
	} else {
		cerr << "Unable to open quantized.txt";
	}

	//write quantized values in binary
	std::ofstream qbinary("tests/comp_zorder/qbinary.txt");
	if(qbinary.is_open()) {
		for(size_t i = 0; i < n; ++i) {
			qbinary << std::bitset<16>(qcent.x[i]) << "		"
					<< std::bitset<16>(qcent.y[i]) << "		"
					<< std::bitset<16>(qcent.z[i]) << "\n";
		}
	}

	//morton codes
	t0 = steady_clock::now();
	zcodes = inter_zorder(qcent, num_thr);
	t1 = steady_clock::now();
	elapsed = duration_cast<milliseconds>(t1 - t0);
	cout << "morton complete: " << elapsed.count() << endl;

	//write morton codes in binary
	std::ofstream morton("tests/comp_zorder/morton.txt");
	if(morton.is_open()) {
		for(size_t i = 0; i < n; ++i) {
			morton << std::bitset<32>(zcodes[i]) << "\n";
		}
	} else {
		cerr << "Unable to open morton.txt";
	}
}

// ====================================================================================
// Main.
// ====================================================================================
int main(int argc, char** argv) {
	auto cfg = parse_args(argc, argv);
	// --------------------------------------------------------------------------------
	// For testcases.
	// --------------------------------------------------------------------------------
	size_t n = 15;	//number of centroids

	vector<float> cent_x, cent_y, cent_z;
	cent_x.resize(n);
	cent_y.resize(n);
	cent_z.resize(n);
	cent_x = generate_centroids(n, 0, "cent_x");
	cent_y = generate_centroids(n, 1, "cent_y");
	cent_z = generate_centroids(n, 2, "cent_z");

	test_comp_zorder(cent_x, cent_y, cent_z, cfg.num_threads);

	// --------------------------------------------------------------------------------
	// For full LBVH.
	// --------------------------------------------------------------------------------
	//parsing using normalize.cpp > rapidobj
	// std::optional <PrimitiveData> prim_data = load_tri_obj("models/" + cfg.filename);
	// if(!prim_data) {
	// 	cerr << "See above error" << endl;
	// 	return 1;
	// }
	//
	// for(size_t i = 0; i < prim_data->prim_id.size(); ++i) {
	// 	cout << "ID " << prim_data->prim_id[i] << ":\n"
	// 		 << "\t centroid: ( " << prim_data->centroid_x[i] << ", "
	// 		 				      << prim_data->centroid_y[i] << ", "
	// 							  << prim_data->centroid_z[i] << ")\n"
	// 		 << "\t normal: (" << prim_data->norm_x[i] << ", "
	// 		 				   << prim_data->norm_y[i] << ", "
	// 						   << prim_data->norm_z[i] << ")\n"
	// 		 << "\t min: (" << prim_data->min_x[i] << ", "
	// 		 				<< prim_data->min_y[i] << ", "
	// 						<< prim_data->min_z[i] << ")\n"
	// 		 << "\t max: (" << prim_data->max_x[i] << ", "
	// 		 				<< prim_data->max_y[i] << ", "
	// 						<< prim_data->max_z[i] << ")" << endl;
	// }
	//
	// //bool bit_res = false if 10 bit resolution, true if 21.
	// auto t0 = steady_clock::now();
	// if(cfg.num_threads > 1) {
	// 	QCent qcent = quantize(prim_data->centroid_x, prim_data->centroid_y,
	// 						   prim_data->centroid_z, cfg.num_threads);
	// 	vector<uint32_t> zcodes = inter_zorder(qcent, cfg.num_threads);
	// }
	// auto t1 = steady_clock::now();
	// auto elapsed = duration_cast<milliseconds>(t1 - t0);
	// cout << "comp_zorder complete: " << elapsed.count() << endl;

	return 0;
}

