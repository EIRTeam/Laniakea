/**************************************************************************/
/*  material_storage.h                                                    */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#pragma once

#include "godot_cpp/core/defs.hpp"
#include "godot_cpp/variant/projection.hpp"
#include "godot_cpp/variant/transform3d.hpp"

using namespace godot;
class TransformStorage {
public:
	static _FORCE_INLINE_ void store_transform(const Transform3D &p_mtx, float *p_array) {
		p_array[0] = p_mtx.basis.rows[0][0];
		p_array[1] = p_mtx.basis.rows[1][0];
		p_array[2] = p_mtx.basis.rows[2][0];
		p_array[3] = 0;
		p_array[4] = p_mtx.basis.rows[0][1];
		p_array[5] = p_mtx.basis.rows[1][1];
		p_array[6] = p_mtx.basis.rows[2][1];
		p_array[7] = 0;
		p_array[8] = p_mtx.basis.rows[0][2];
		p_array[9] = p_mtx.basis.rows[1][2];
		p_array[10] = p_mtx.basis.rows[2][2];
		p_array[11] = 0;
		p_array[12] = p_mtx.origin.x;
		p_array[13] = p_mtx.origin.y;
		p_array[14] = p_mtx.origin.z;
		p_array[15] = 1;
	}

	static _FORCE_INLINE_ void store_basis_3x4(const Basis &p_mtx, float *p_array) {
		p_array[0] = p_mtx.rows[0][0];
		p_array[1] = p_mtx.rows[1][0];
		p_array[2] = p_mtx.rows[2][0];
		p_array[3] = 0;
		p_array[4] = p_mtx.rows[0][1];
		p_array[5] = p_mtx.rows[1][1];
		p_array[6] = p_mtx.rows[2][1];
		p_array[7] = 0;
		p_array[8] = p_mtx.rows[0][2];
		p_array[9] = p_mtx.rows[1][2];
		p_array[10] = p_mtx.rows[2][2];
		p_array[11] = 0;
	}

	static _FORCE_INLINE_ void store_transform_3x3(const Basis &p_mtx, float *p_array) {
		p_array[0] = p_mtx.rows[0][0];
		p_array[1] = p_mtx.rows[1][0];
		p_array[2] = p_mtx.rows[2][0];
		p_array[3] = 0;
		p_array[4] = p_mtx.rows[0][1];
		p_array[5] = p_mtx.rows[1][1];
		p_array[6] = p_mtx.rows[2][1];
		p_array[7] = 0;
		p_array[8] = p_mtx.rows[0][2];
		p_array[9] = p_mtx.rows[1][2];
		p_array[10] = p_mtx.rows[2][2];
		p_array[11] = 0;
	}

	static _FORCE_INLINE_ void store_transform_transposed_3x4(const Transform3D &p_mtx, float *p_array) {
		p_array[0] = p_mtx.basis.rows[0][0];
		p_array[1] = p_mtx.basis.rows[0][1];
		p_array[2] = p_mtx.basis.rows[0][2];
		p_array[3] = p_mtx.origin.x;
		p_array[4] = p_mtx.basis.rows[1][0];
		p_array[5] = p_mtx.basis.rows[1][1];
		p_array[6] = p_mtx.basis.rows[1][2];
		p_array[7] = p_mtx.origin.y;
		p_array[8] = p_mtx.basis.rows[2][0];
		p_array[9] = p_mtx.basis.rows[2][1];
		p_array[10] = p_mtx.basis.rows[2][2];
		p_array[11] = p_mtx.origin.z;
	}

	static _FORCE_INLINE_ void store_camera(const Projection &p_mtx, float *p_array) {
		for (int i = 0; i < 4; i++) {
			for (int j = 0; j < 4; j++) {
				p_array[i * 4 + j] = p_mtx.columns[i][j];
			}
		}
	}

	static _FORCE_INLINE_ void store_soft_shadow_kernel(const float *p_kernel, float *p_array) {
		for (int i = 0; i < 128; i++) {
			p_array[i] = p_kernel[i];
		}
	}
};
