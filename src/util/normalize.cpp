#include "util/normalize.hpp"
#include <cstddef>
#include <cstdint>
#include <rapidobj/rapidobj.hpp>
#include <vector>
#include <iostream>
#include <limits>

using std::vector;

// ====================================================================================
// Calculate normals via cross product
// ====================================================================================
inline Fl3 calc_normal(float x0, float y0, float z0,
		               float x1, float y1, float z1,
					   float x2, float y2, float z2) {
	float ux = x1 - x0;
	float uy = y1 - y0;
	float uz = z1 - z0;
	float vx = x2 - x0;
	float vy = y2 - y0;
	float vz = z2 - z0;
	return {
		(uy * vz) - (uz * vy),
		(uz * vx) - (ux * vz),
		(ux * vy) - (uy * vx)
	};
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
	centroid_x.resize(num_tris);
	centroid_y.resize(num_tris);
	centroid_z.resize(num_tris);
	vector<float> bmin_x, bmin_y, bmin_z; //b for bounding volumes
	bmin_x.resize(num_tris);
	bmin_y.resize(num_tris);
	bmin_z.resize(num_tris);
	vector<float> bmax_x, bmax_y, bmax_z;
	bmax_x.resize(num_tris);
	bmax_y.resize(num_tris);
	bmax_z.resize(num_tris);
	vector<uint32_t> prim_id;
	prim_id.resize(num_tris);
	vector<float> norm_x, norm_y, norm_z; //want face normals so ignoring OBJ normals
	norm_x.resize(num_tris);
	norm_y.resize(num_tris);
	norm_z.resize(num_tris);
	//all of the above elements (centroid, min, max, normal) is one per primitive (triangle)

	uint32_t prim_counter = 0;

	// ------------------------------------------------------------------------------------
	// For each 3 vertices of a face of a shape: compute centroid, bounds, and
	// normal vector.
	// ------------------------------------------------------------------------------------
	// should prepare for SIMD batching by restructing into a single for loop of 
	// for (size_t i = 0; i < num_tris; i++)
	for(const auto& shape : result.shapes) {
		const auto& mesh = shape.mesh;
		size_t index_offset = 0;

		for(size_t face = 0; face < mesh.num_face_vertices.size(); ++face) {
			//get Index object of each of the 3 vertices
			const rapidobj::Index& i0 = mesh.indices[index_offset + 0];
			const rapidobj::Index& i1 = mesh.indices[index_offset + 1];
			const rapidobj::Index& i2 = mesh.indices[index_offset + 2];
			
			//using Index, get pointers to positions of each of the 3 vertices
			assert(i0.position_index >= 0 && "Negative or invalid OBJ index");
			assert(i1.position_index >= 0 && "Negative or invalid OBJ index");
			assert(i2.position_index >= 0 && "Negative or invalid OBJ index");
			const float* p0 = &attr.positions[3 * i0.position_index];
			const float* p1 = &attr.positions[3 * i1.position_index];
			const float* p2 = &attr.positions[3 * i2.position_index];

			//compute centroids
			float cx = (p0[0] + p1[0] + p2[0]) * (1.0f / 3.0f);
			float cy = (p0[1] + p1[1] + p2[1]) * (1.0f / 3.0f);
			float cz = (p0[2] + p1[2] + p2[2]) * (1.0f / 3.0f);

			//normalize centroids
			cx = (cx - min_x) * inv_dx;
			cy = (cy - min_y) * inv_dy;
			cz = (cz - min_z) * inv_dz;

			centroid_x[prim_counter] = cx;
			centroid_y[prim_counter] = cy;
			centroid_z[prim_counter] = cz;

			//compute bounds
			bmin_x[prim_counter] = std::min({p0[0], p1[0], p2[0]});
			bmin_y[prim_counter] = std::min({p0[1], p1[1], p2[1]});
			bmin_z[prim_counter] = std::min({p0[2], p1[2], p2[2]});
			bmax_x[prim_counter] = std::max({p0[0], p1[0], p2[0]});
			bmax_y[prim_counter] = std::max({p0[1], p1[1], p2[1]});
			bmax_z[prim_counter] = std::max({p0[2], p1[2], p2[2]});

			//compute normals
			Fl3 n = calc_normal(p0[0], p0[1], p0[2],
								p1[0], p1[1], p1[2],
								p2[0], p2[1], p2[2]);
			norm_x[prim_counter] = n.x;
			norm_y[prim_counter] = n.y;
			norm_z[prim_counter] = n.z;

			prim_id[prim_counter] = prim_counter;
			++prim_counter;

			index_offset += 3;
		}
	}	

	return PrimitiveData(centroid_x, centroid_y, centroid_z,
			             prim_id,
						 norm_x, norm_y, norm_z,
						 bmin_x, bmin_y, bmin_z,
						 bmax_x, bmax_y, bmax_z);
}
