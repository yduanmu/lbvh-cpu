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
#include "lbvh/cons_radix.hpp"
#include "lbvh/cons_bvh_seq.hpp"

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
// DFS in-order tree traversal for printing.
// ====================================================================================
void print_tree(const vector<Node>& nodes, const vector<uint32_t>& zcodes,
				const uint32_t idx, std::ofstream& tree, bool first) {
	if(!first && idx == 0) {
		return;
	}

	if(nodes[idx].l_child != INVALID_U32) {
		print_tree(nodes, zcodes, nodes[idx].l_child, tree, false);
	}

	//if leaf
	if(nodes[idx].count == 1) {
		tree << std::bitset<32>(zcodes[nodes[idx].first_idx]) << endl;
	}

	if(nodes[idx].r_child != INVALID_U32) {
		print_tree(nodes, zcodes, nodes[idx].r_child, tree, false);
	}
}

void print_tree_ext(const vector<Node>& leaf_nodes, const vector<Node>& in_nodes,
					const vector<uint32_t>& zcodes,
					const uint32_t idx, std::ofstream& tree, const bool first) {
	if(!first && idx == 0) {
		return;
	}
	const uint32_t l_child = in_nodes[idx].l_child;
	const uint32_t r_child = in_nodes[idx].r_child;

	if(l_child != INVALID_U32) {
		if(in_nodes[idx].l_is_leaf) {
			tree << std::bitset<32>(zcodes[leaf_nodes[l_child].first_idx]) << endl;
		} else {
			print_tree_ext(leaf_nodes, in_nodes, zcodes, l_child, tree, false);
		}
	}

	if(r_child != INVALID_U32) {
		if(in_nodes[idx].r_is_leaf) {
			tree << std::bitset<32>(zcodes[leaf_nodes[r_child].first_idx]) << endl;
		} else {
			print_tree_ext(leaf_nodes, in_nodes, zcodes, r_child, tree, false);
		}
	}
}

// ====================================================================================
// Main.
// ====================================================================================
int main(int argc, char** argv) {
	auto cfg = parse_args(argc, argv);
	// --------------------------------------------------------------------------------
	// Parse using normalize.cpp > rapidobj.
	// --------------------------------------------------------------------------------
	std::optional<PrimitiveData> prim_data = load_tri_obj("models/" + cfg.filename);
	if(!prim_data) {
		cerr << "See above error" << endl;
		return 1;
	}
	cout << "normalize complete" << endl;

	// --------------------------------------------------------------------------------
	// Compute Morton keys.
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
	// Sort Morton keys.
	// prim_id indices are the sorted positions. The value at each index are the
	// original positions.
	// --------------------------------------------------------------------------------
	if(!cfg.sort_zorder || cfg.num_threads <= 1) {
		radix_sort_seq(zcodes, prim_data->prim_id);
	} else {
		radix_sort(zcodes, cfg.num_threads, prim_data->prim_id);
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
	// Construct binary radix tree.
	// --------------------------------------------------------------------------------
	vector<Node> nodes;
	vector<Node> leaf_nodes, in_nodes;

	if(!cfg.cons_radix || cfg.num_threads <= 1) {
		//binary tree w/ N leaves has N - 1 internal nodes
		nodes.reserve(zcodes.size() * 2 - 1);
		t0 = steady_clock::now();
		build_tree_seq(zcodes, nodes, 0, static_cast<uint32_t>(zcodes.size() - 1));
		t1 = steady_clock::now();
		elapsed = duration_cast<microseconds>(t1 - t0);
		cout << "cons_radix_SEQ complete: " << elapsed.count() << "us" << endl;
	} else {
		leaf_nodes.resize(zcodes.size());
		in_nodes.resize(zcodes.size() - 1);
		t0 = steady_clock::now();
		build_tree(zcodes, leaf_nodes, in_nodes, zcodes.size(), cfg.num_threads);
		t1 = steady_clock::now();
		elapsed = duration_cast<microseconds>(t1 - t0);
		cout << "cons_radix PAR complete: " << elapsed.count() << "us" << endl;
	}

	//logging w/ DFS
	if(cfg.logging) {
		if(!cfg.cons_radix || cfg.num_threads <= 1) {
			std::ofstream tree("tests/tree.txt");
			if(tree.is_open()) {
				print_tree(nodes, zcodes, 0, tree, true);
				tree.close();
			} else {
				cerr << "Unable to open tests/tree.txt" << std::flush;
			}
		} else {
			std::ofstream tree("tests/tree.txt");
			if(tree.is_open()) {
				print_tree_ext(leaf_nodes, in_nodes, zcodes, 0, tree, true);
				tree.close();
			} else {
				cerr << "Unable to open tests/tree.txt" << std::flush;
			}
		}
	}

	//check if radix tree constructed properly
	if(cfg.logging) {
		vector<uint32_t> tree_zcodes;
		tree_zcodes.reserve(zcodes.size());

		std::ifstream tree("tests/tree.txt");
		if(tree.is_open()) {
			std::string line;
			while(std::getline(tree, line)) {
				uint32_t value = static_cast<uint32_t>(
						std::stoul(line, nullptr, 2));
				tree_zcodes.push_back(value);
			}
			tree.close();
		} else {
			cerr << "Unable to open tests/tree.txt" << std::flush;
		}

		bool unequal = false;
		for(size_t i = 0; i < zcodes.size(); ++i) {
			if(zcodes[i] != tree_zcodes[i]) {
				unequal = true;
				cout << "\tInequality at zcodes [" << i << "] = "
					 << std::bitset<32>(zcodes[i]) << endl;
				break;
			}
		}
		if(!unequal) {
			cout << "\tradix tree IS valid" << endl;
		} else {
			cout << "\tradix tree NOT valid" << endl;
		}
	}
	
	// --------------------------------------------------------------------------------
	// Bounding box calculation.
	// --------------------------------------------------------------------------------
	if(leaf_nodes.size() != 0 && in_nodes.size() != 0) {
		compute_aabb_seq(leaf_nodes, in_nodes, 0, prim_data.value(), true);
	}

	return 0;
}

