#include <iostream>
#include <unistd.h>
#include <string>
// #include <chrono>
#include "util/normalize.hpp"

using std::cout;
using std::cerr;
using std::endl;

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
// main
// ====================================================================================

int main(int argc, char** argv) {
	//parsing using normalize.cpp > rapidobj
	auto cfg = parse_args(argc, argv);
	std::optional <PrimitiveData> prim_data = load_tri_obj("models/" + cfg.filename);
	if(!prim_data) {
		cout << "See above error" << endl << std::flush;
		return 1;
	}
	
	for(size_t i = 0; i < prim_data->prim_id.size(); ++i) {
		cout << "ID " << prim_data->prim_id[i] << ":\n"
			 << "\t centroid: ( " << prim_data->centroid_x[i] << ", "
			 				      << prim_data->centroid_y[i] << ", "
								  << prim_data->centroid_z[i] << ")\n"
			 << "\t normal: (" << prim_data->norm_x[i] << ", "
			 				   << prim_data->norm_y[i] << ", "
							   << prim_data->norm_z[i] << ")\n"
			 << "\t min: (" << prim_data->min_x[i] << ", "
			 				<< prim_data->min_y[i] << ", "
							<< prim_data->min_z[i] << ")\n"
			 << "\t max: (" << prim_data->max_x[i] << ", "
			 				<< prim_data->max_y[i] << ", "
							<< prim_data->max_z[i] << ")" << endl;
	}

	//only thread 0 should time things
	// std::chrono::steady_clock::time_point start;
	// start = std::chrono::steady_clock::now();
	// auto end = std::chrono::steady_clock::now();
	// std::chrono::duration<double> elapsed = end - start;
	
	return 0;
}

