#pragma once

#include <vector>
#include <cstdint>
#include <optional>
#include <string>

using std::vector;
using std::uint32_t;

struct PrimitiveData {
	vector<float> centroid_x, centroid_y, centroid_z;
	vector<uint32_t> prim_id;
	vector<float> norm_x, norm_y, norm_z;	//plane normal

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

struct Fl3 {
	float x, y, z;
};

inline Fl3 calc_normal(float x0, float y0, float z0,
		               float x1, float y1, float z1,
					   float x2, float y2, float z2);

std::optional <PrimitiveData> load_tri_obj(const std::string& path);

