#include <iostream>
#include <unistd.h>
#include <string>
#include <chrono>

using std::cout;
using std::cerr;
using std::endl;

// ===============================================================
// Command-line config
// ===============================================================
struct Config {
	unsigned int num_threads = 16; // if 1, sequential
	
	//sequential default
	bool comp_zorder = true;
	bool sort_zorder = true; // if false, use std::sort
	bool cons_radix = true;
	bool cons_bvh = true;
};

void usage() {
	cerr << "usage:\n"
			"\t-t <number of threads>\n"
			"\t 	(default 16. 1 means sequential)\n"
			"\t		(for sequential BVH construction, see sah.cpp)\n"
			"\t-c default true flag for parallel Z-order code computation\n"
			"\t-s default true flag for parallel radix sort\n"
			"\t 	(if false, uses std::sort)"
			"\t-r default true flag for parallel construction of binary radix tre>\n"
			"\t-b default true flag for parallel BVH construction\n"
			"\t-h print this message";
	
	exit(0);
}

Config parse_args(int argc, char** argv) {
	Config cfg;
	int ch;
	while ((ch = getopt(argc, argv, "t:csrbh")) != -1) {
		try {
			switch(static_cast<char>(ch)){
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

// ===============================================================
// main
// ===============================================================

int main(int argc, char** argv) {
	auto cfg = parse_args(argc, argv);

	//only thread 0 should time things
	std::chrono::steady_clock::time_point start;
	start = std::chrono::steady_clock::now();
	auto end = std::chrono::steady_clock::now();
	std::chrono::duration<double> elapsed = end - start;
}
