#include <iostream>
#include <unistd.h>
#include <string>
#include <optional>
#include <chrono>
#include <cstdint>

//for output
#include <bitset>
#include <fstream>

#include "util/normalize.hpp"
#include "lbvh/comp_zorder.hpp"
#include "lbvh/sort_zorder.hpp"
#include "lbvh/sort_zorder_seq.hpp"
#include "lbvh/cons_radix_seq.hpp"

using std::cout;
using std::cerr;
using std::endl;
using std::chrono::duration_cast;
using std::chrono::microseconds;
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

	bool logging = true;
};

void usage() {
	cerr << "Usage. All flags default TRUE (parallel). Toggle for FALSE (sequential):\n"
			"-f <file name>\n"
			"-t <number of threads>\n"
			"\t(default 16. 1 means sequential)\n"
			"-c Z-order (Morton) code computation\n"
			"-s radix sort\n"
			"-r binary radix tree construction\n"
			"-b BVH construction\n"
			"-l toggle logging to FALSE (improves execution time)\n"
			"-h print this message\n";

	exit(0);
}

Config parse_args(int argc, char** argv) {
	Config cfg;
	int ch;
	while ((ch = getopt(argc, argv, "f:t:csrblh")) != -1) {
		try {
			switch(static_cast<char>(ch)){
				case 'f':
					cfg.filename = optarg;
					break;
				case 't':
					cfg.num_threads = static_cast<size_t>(std::stoi(optarg));
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
				case 'l':
					cfg.logging = false;
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
// BFS tree traversal for printing.
// ====================================================================================
void print_tree(const vector<Node>& nodes, const vector<uint32_t>& zcodes,
				const uint32_t index, std::ofstream& tree) {
	tree << "IDX: " << index << "\n";

	//if leaf
	if(nodes[index].count == 1) {
		tree << "\tLEAF: " << std::bitset<32>(zcodes[nodes[index].first_idx]) << endl;
	}

	if(nodes[index].l_child != INVALID_U32) {
		tree << "\tLEFT: " << nodes[index].l_child << "\n";
	}
	if(nodes[index].r_child != INVALID_U32) {
		tree << "\tRIGHT: " << nodes[index].r_child << "\n";
	}

	if(nodes[index].l_child != INVALID_U32) {
		print_tree(nodes, zcodes, nodes[index].l_child, tree);
	}
	if(nodes[index].r_child != INVALID_U32) {
		print_tree(nodes, zcodes, nodes[index].r_child, tree);
	}
}

// ====================================================================================
// Main.
// ====================================================================================
int main(int argc, char** argv) {
	auto cfg = parse_args(argc, argv);
	// --------------------------------------------------------------------------------
	// parsing using normalize.cpp > rapidobj
	// @TODO: create vector<uint32_t> prim_ids.
	// --------------------------------------------------------------------------------
	std::optional <PrimitiveData> prim_data = load_tri_obj("models/" + cfg.filename);
	if(!prim_data) {
		cerr << "See above error" << endl;
		return 1;
	}
	cout << "normalize complete" << endl;

	// --------------------------------------------------------------------------------
	// comp_zorder
	// @TODO: keep vector<uint32_t> prim_ids.
	// --------------------------------------------------------------------------------
	auto t0 = steady_clock::now();
	//@TODO: comp_zorder needs a sequential ver.
	//@TODO: move timer to the code itself to avoid function call/return overhead.
	QCent qcent = quantize(prim_data->centroid_x, prim_data->centroid_y,
						   prim_data->centroid_z, cfg.num_threads);
	vector<uint32_t> zcodes = inter_zorder(qcent, cfg.num_threads);
	auto t1 = steady_clock::now();
	auto elapsed = duration_cast<microseconds>(t1 - t0);
	cout << "comp_zorder PAR complete: " << elapsed.count() << "us" << endl;

	// --------------------------------------------------------------------------------
	// sort_zorder
	// @TODO: keep vector<uint32_t> prim_ids.
	// --------------------------------------------------------------------------------
	if(!cfg.sort_zorder || cfg.num_threads <= 1) {
		radix_sort_seq(zcodes);
	} else {
		radix_sort(zcodes, cfg.num_threads);
	}

	//logging
	if(cfg.logging) {
		std::ofstream sorted_keys("tests/sorted_keys.txt");
		if(sorted_keys.is_open()) {
			for(uint32_t k : zcodes) {
				sorted_keys << std::bitset<32>(k) << "\n";
			}
			sorted_keys.close();
		} else {
			cerr << "Unable to open tests/sorted_keys.txt" << std::flush;
		}
		cout << "\tnum keys: " << zcodes.size() << endl;
	}

	// --------------------------------------------------------------------------------
	// cons_radix
	// --------------------------------------------------------------------------------
	vector<Node> nodes;

	//binary tree w/ N leaves has N - 1 internal nodes
	nodes.reserve(zcodes.size() * 2 - 1);

	if(!cfg.cons_radix || cfg.num_threads <= 1) {
		t0 = steady_clock::now();
		build_tree_seq(zcodes, nodes, 0, static_cast<uint32_t>(zcodes.size() - 1));
		t1 = steady_clock::now();
		elapsed = duration_cast<microseconds>(t1 - t0);
		cout << "cons_radix_SEQ complete: " << elapsed.count() << "us" << endl;
	}

	//logging w/ BFS
	if(cfg.logging) {
		std::ofstream tree("tests/tree.txt");
		if(tree.is_open()) {
			print_tree(nodes, zcodes, 0, tree);
			tree.close();
		} else {
			cerr << "Unable to open tests/tree.txt" << std::flush;
		}
	}

	//if radix tree is constructed correctly, zcodes == BFS(nodes).
	//in other words, they must be in the same order.
	if(cfg.logging) {
		vector<uint32_t> tree_zcodes;
		tree_zcodes.reserve(zcodes.size());

		std::ifstream tree("tests/tree.txt");
		if(tree.is_open()) {
			std::string line;
			while(std::getline(tree, line)) {
				std::size_t pos = line.find("LEAF:");
				if(pos != std::string::npos) {
					//extract bit sequence
					std::string bits = line.substr(pos + 5);

					//trim whitespace
					bits.erase(0, bits.find_first_not_of(" \t"));
            		bits.erase(bits.find_last_not_of(" \t\r\n") + 1);

					uint32_t value = static_cast<uint32_t>(
							std::stoul(bits, nullptr, 2)
					);
					tree_zcodes.push_back(value);
				}
			}

		tree.close();
		} else {
			cerr << "Unable to open tests/tree.txt" << endl;
		}

		bool unequal = false;
		for(size_t i = 0; i < zcodes.size(); ++i) {
			if(zcodes[i] != tree_zcodes[i]) {
				unequal = true;
				cout << "\tInequality at zcodes [" << i << "] = "
					 << zcodes[i] << endl;
				break;
			}
		}
		if(!unequal) {
			cout << "\tradix tree IS valid" << endl;
		} else {
			cout << "\tradix tree NOT valid" << endl;
		}
	}

	return 0;
}

