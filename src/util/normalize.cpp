#include "util/normalize.hpp"
#include <cstddef>
#include <cstdint>
#include <rapidobj/rapidobj.hpp>
#include <vector>
#include <iostream>
#include <limits>

using std::vector;

struct PrimitiveData {
	vector<float> centroid_x, centroid_y, centroid_z;
	vector<uint32_t> prim_id;
	vector<float> norm_x, norm_y, norm_z;

	//bounding boxes
	vector<float> min_x, min_y, min_z;
	vector<float> max_x, max_y, max_z;

	PrimitiveData(vector<float> _cx, vector<float> _cy, vector<float> _cz,
				  vector<uint32_t> _pi,
				  vector<float> _nx, vector<float> _ny, vector<float> _nz,
				  vector<float> _mix, vector<float> _miy, vector<float> _miz,
				  vector<float> _max, vector<float> _may, vector<float> _maz) :
				  centroid_x(_cx), centroid_y(_cy), centroid_z(_cz),
				  prim_id(_pi),
				  norm_x(_nx), norm_y(_ny), norm_z(_nz),
				  min_x(_mix), min_y(_miy), min_z(_miz),
				  max_x(_max), max_y(_may), max_z(_maz) {};
};

// ====================================================================================
// Calculate normals via cross product; pseudocode from wikis.khronos.org/opengl
// ====================================================================================
inline vector<float> calc_normal(vector<float> tri) {
	vector<float> u = {tri[3] - tri[0], tri[4] - tri[1], tri[5] - tri[2]}; //tri.p2 - tri.p1
	vector<float> v = {tri[6] - tri[0], tri[7] - tri[1], tri[8] - tri[2]}; //tri.p3 - tri.p1
	float x = (u[1] * v[2]) - (u[2] * v[1]); //(U.y * V.z) - (U.z * V.y)
	float y = (u[2] * v[0]) - (u[0] * v[2]); //(U.z * V.x) - (U.x * V.z)
	float z = (u[0] * v[1]) - (u[1] * v[0]); //(U.x * V.y) - (U.y * V.x)
	return {x, y, z};
}

// ====================================================================================
// Parses OBJ using rapidobj and returns only necessary information to build LBVH.
// ====================================================================================
std::optional <PrimitiveData> load_tri_obj(const std::string& path) {
	//parse obj
	rapidobj::Result result = rapidobj::ParseFile(path);
	if(result.error) {
		std::cerr << "Parse error: " << result.error.code.message() << std::endl;
		return std::nullopt;
	}

	//triangulate
	bool success = rapidobj::Triangulate(result);
	if(!success) {
		std::cerr << "Triangulation error: " << result.error.code.message() << std::endl;
		return std::nullopt;
	}

	const auto& attr = result.attributes;

	constexpr float INF = std::numeric_limits<float>::infinity();
	constexpr float NINF = -std::numeric_limits<float>::infinity();
	float min_x = INF;
	float min_y = INF;
	float min_z = INF;
	float max_x = NINF;
	float max_y = NINF;
	float max_z = NINF;

	//compute bounds for use when normalizing centroids
	for(size_t i = 0; i < attr.positions.size(); i+=3) {
		float x = attr.positions[i + 0];
		float y = attr.positions[i + 1];
		float z = attr.positions[i + 2];

		min_x = std::min(min_x, x);
		min_y = std::min(min_y, y);
		min_z = std::min(min_z, z);
		max_x = std::max(max_x, x);
		max_y = std::max(max_y, y);
		max_z = std::max(max_z, z);
	}

	//compute scale (inverse dimension size) for normalizing centroids to 0-1 range
	float inv_dx = (max_x > min_x) ? 1.0f / (max_x - min_x) : 0.0f;	//guard against div by 0
	float inv_dy = (max_y > min_y) ? 1.0f / (max_y - min_y) : 0.0f;
	float inv_dz = (max_z > min_z) ? 1.0f / (max_z - min_z) : 0.0f;
	
	//optimization
	int num_tris = 0;
	for (const auto& shape : result.shapes) {
		num_tris += static_cast<int>(shape.mesh.indices.size() / 3);
	}
	vector<float> centroid_x, centroid_y, centroid_z;
	centroid_x.reserve(num_tris);
	centroid_y.reserve(num_tris);
	centroid_z.reserve(num_tris);
	vector<float> bmin_x, bmin_y, bmin_z; //b for bounding volumes
	bmin_x.reserve(num_tris);
	bmin_y.reserve(num_tris);
	bmin_z.reserve(num_tris);
	vector<float> bmax_x, bmax_y, bmax_z;
	bmax_x.reserve(num_tris);
	bmax_y.reserve(num_tris);
	bmax_z.reserve(num_tris);
	vector<uint32_t> prim_id;
	prim_id.reserve(num_tris);
	vector<float> norm_x, norm_y, norm_z;
	norm_x.reserve(num_tris);
	norm_y.reserve(num_tris);
	norm_z.reserve(num_tris);
	//all of the above elements (centroid, min, max, normal) is one per primitive (triangle)

	//rapidobj allows normal vector vector to be empty
	bool compute_norms = true;
	if(!attr.normals.empty()) { //register normals
		compute_norms = false;
		for(size_t i = 0; i < attr.normals.size(); i+=3) {
			norm_x.push_back(attr.normals[i]);
			norm_y.push_back(attr.normals[i + 1]);
			norm_z.push_back(attr.normals[i + 2]);
		}
	}

	// ------------------------------------------------------------------------------------
	// For each 3 vertices of a face of a shape: compute centroid, bounds, and
	// normal vector.
	// ------------------------------------------------------------------------------------
	for(const auto& shape : result.shapes) {
		const auto& mesh = shape.mesh;
		size_t index_offset = 0;

		for(size_t face = 0; face < mesh.num_face_vertices.size(); ++face) {
			//get Index object of each of the 3 vertices
			const rapidobj::Index& i0 = mesh.indices[index_offset + 0];
			const rapidobj::Index& i1 = mesh.indices[index_offset + 1];
			const rapidobj::Index& i2 = mesh.indices[index_offset + 2];
			
			//using Index, get pointers to positions of each of the 3 vertices
			const float* p0 = &attr.positions[3 * i0.position_index];
			const float* p1 = &attr.positions[3 * i1.position_index];
			const float* p2 = &attr.positions[3 * i2.position_index];

			//compute centroids
			float cx = (p0[0] + p1[0] + p2[0]) * (1.0f/3.0f);
			float cy = (p0[1] + p1[1] + p2[1]) * (1.0f / 3.0f);
			float cz = (p0[2] + p1[2] + p2[2]) * (1.0f / 3.0f);

			//normalize centroids
			cx = (cx - min_x) * inv_dx;
			cy = (cy - min_y) * inv_dy;
			cz = (cz - min_z) * inv_dz;

			centroid_x.push_back(cx);
			centroid_y.push_back(cy);
			centroid_z.push_back(cz);

			//compute bounds
			float temp = std::min(p0[0], p1[0]);
			float mix = std::min(temp, p2[0]);
			temp = std::min(p0[1], p1[1]);
			float miy = std::min(temp, p2[1]);
			temp = std::min(p0[2], p1[2]);
			float miz = std::min(temp, p2[2]);
			temp = std::max(p0[0], p1[0]);
			float max = std::max(temp, p2[0]);
			temp = std::max(p0[1], p1[1]);
			float may = std::max(temp, p2[1]);
			temp = std::max(p0[2], p1[2]);
			float maz = std::max(temp, p2[2]);

			bmin_x.push_back(mix);
			bmin_y.push_back(miy);
			bmin_z.push_back(miz);
			bmax_x.push_back(max);
			bmax_y.push_back(may);
			bmax_z.push_back(maz);

			//compute normals
			if(compute_norms) {
				vector<float> n = calc_normal({p0[0], p0[1], p0[2],
							 				   p1[0], p1[1], p1[2],
							 				   p2[0], p2[1], p2[2]});
				norm_x.push_back(n[0]);
				norm_y.push_back(n[1]);
				norm_z.push_back(n[2]);
			}

			prim_id.push_back(static_cast<uint32_t>(index_offset / size_t(3))); //probably safe

			index_offset += 3;
		}
	}	

	return PrimitiveData(centroid_x, centroid_y, centroid_z,
			             prim_id,
						 norm_x, norm_y, norm_z,
						 bmin_x, bmin_y, bmin_z,
						 bmax_x, bmax_y, bmax_z);
}
