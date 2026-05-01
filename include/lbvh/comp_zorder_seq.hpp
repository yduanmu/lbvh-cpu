#pragma once

#include "lbvh/comp_zorder.hpp"

//exactly the same as parallel version, but without OpenMP.
QCent quantize(const vector<float>& centroid_x, const vector<float>& centroid_y,
			   const vector<float>& centroid_z);
vector<uint32_t> inter_zorder(const QCent& qcent);

