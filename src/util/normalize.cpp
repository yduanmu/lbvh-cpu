#include "util/normalize.hpp"
#include <cstddef>
#include <rapidobj/rapidobj.hpp>
#include <vector>
#include <iostream>

using std::vector;

struct PrimitiveData {
	vector<float> centroid_x, centroid_y, centroid_z;
	vector<uint32_t> prim_id;
	vector<float> min_x, min_y, min_z;
	vector<float> max_x, max_y, max_z;
};

PrimitiveData load_tri_obj(const std::string& path) {
	//parse obj
	PrimitiveData prim_data;
	rapidobj::Result result = rapidobj::ParseFile(path);
	if(result.error) {
		std::cerr << "Parse error: " << result.error.code.message() << std::endl;
		return prim_data;
	}

	//triangulate
	bool success = rapidobj::Triangulate(result);
	if(!success) {
		std::cerr << "Triangulation error: " << result.error.code.message() << std::endl;
		return prim_data;
	}

	//for each 3 vertices of a face of a shape, compute centroid & bounds
	const auto& attr = result.attributes;
	for(const auto& shape : result.shapes) {
		const auto& mesh = shape.mesh;
		size_t index_offset = 0;

		for(size_t face = 0; face < mesh.num_face_vertices.size(); ++face) {
			//3 after triangulation
			const rapidobj::Index& i0 = mesh.indices[index_offset + 0];
			const rapidobj::Index& i1 = mesh.indices[index_offset + 1];
			const rapidobj::Index& i2 = mesh.indices[index_offset + 2];

			for(int v = 0; v < 3; ++v) {
				
			}
		}
	}

	return prim_data;
}
