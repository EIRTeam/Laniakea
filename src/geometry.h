#pragma once

#include "godot_cpp/variant/rect2.hpp"
#include "godot_cpp/variant/vector2.hpp"

using namespace godot;

class Geometry {
public:
	static bool segment_intersects_segment(const Vector2 &p_from_a, const Vector2 &p_to_a, const Vector2 &p_from_b, const Vector2 &p_to_b, Vector2 *r_result) {
		Vector2 B = p_to_a - p_from_a;
		Vector2 C = p_from_b - p_from_a;
		Vector2 D = p_to_b - p_from_a;

		real_t ABlen = B.dot(B);
		if (ABlen <= 0) {
			return false;
		}
		Vector2 Bn = B / ABlen;
		C = Vector2(C.x * Bn.x + C.y * Bn.y, C.y * Bn.x - C.x * Bn.y);
		D = Vector2(D.x * Bn.x + D.y * Bn.y, D.y * Bn.x - D.x * Bn.y);

		// Fail if C x B and D x B have the same sign (segments don't intersect).
		if ((C.y < (real_t)-CMP_EPSILON && D.y < (real_t)-CMP_EPSILON) || (C.y > (real_t)CMP_EPSILON && D.y > (real_t)CMP_EPSILON)) {
			return false;
		}

		// Fail if segments are parallel or colinear.
		// (when A x B == zero, i.e (C - D) x B == zero, i.e C x B == D x B)
		if (Math::is_equal_approx(C.y, D.y)) {
			return false;
		}

		real_t ABpos = D.x + (C.x - D.x) * D.y / (D.y - C.y);

		// Fail if segment C-D crosses line A-B outside of segment A-B.
		if ((ABpos < 0) || (ABpos > 1)) {
			return false;
		}

		// Apply the discovered position to line A-B in the original coordinate system.
		if (r_result) {
			*r_result = p_from_a + B * ABpos;
		}

		return true;
	}

	static inline bool is_point_in_circle(const Vector2 &p_point, const Vector2 &p_circle_pos, real_t p_circle_radius) {
		return p_point.distance_squared_to(p_circle_pos) <= p_circle_radius * p_circle_radius;
	}

	static real_t segment_intersects_circle(const Vector2 &p_from, const Vector2 &p_to, const Vector2 &p_circle_pos, real_t p_circle_radius) {
		Vector2 line_vec = p_to - p_from;
		Vector2 vec_to_line = p_from - p_circle_pos;

		// Create a quadratic formula of the form ax^2 + bx + c = 0
		real_t a, b, c;

		a = line_vec.dot(line_vec);
		b = 2 * vec_to_line.dot(line_vec);
		c = vec_to_line.dot(vec_to_line) - p_circle_radius * p_circle_radius;

		// Solve for t.
		real_t sqrtterm = b * b - 4 * a * c;

		// If the term we intend to square root is less than 0 then the answer won't be real,
		// so it definitely won't be t in the range 0 to 1.
		if (sqrtterm < 0) {
			return -1;
		}

		// If we can assume that the line segment starts outside the circle (e.g. for continuous time collision detection)
		// then the following can be skipped and we can just return the equivalent of res1.
		sqrtterm = Math::sqrt(sqrtterm);
		real_t res1 = (-b - sqrtterm) / (2 * a);
		real_t res2 = (-b + sqrtterm) / (2 * a);

		if (res1 >= 0 && res1 <= 1) {
			return res1;
		}
		if (res2 >= 0 && res2 <= 1) {
			return res2;
		}
		return -1;
	}

	static bool segment_intersects_rect(const Vector2 &p_from, const Vector2 &p_to, const Rect2 &p_rect) {
		if (p_rect.has_point(p_from) || p_rect.has_point(p_to)) {
			return true;
		}

		const Vector2 rect_points[4] = {
			p_rect.position,
			p_rect.position + Vector2(p_rect.size.x, 0),
			p_rect.position + p_rect.size,
			p_rect.position + Vector2(0, p_rect.size.y)
		};

		// Check if any of the rect's edges intersect the segment.
		for (int i = 0; i < 4; i++) {
			if (segment_intersects_segment(p_from, p_to, rect_points[i], rect_points[(i + 1) % 4], nullptr)) {
				return true;
			}
		}

		return false;
	}

	static float distance_to_rect_squared(const Vector2 &p_point, const Vector2 &p_rect_center, float rect_extents) {
		float dx = Math::max(Math::abs(p_point.x - p_rect_center.x) - rect_extents / 2.0, 0.0);
		float dy = Math::max(Math::abs(p_point.y - p_rect_center.y) - rect_extents / 2.0, 0.0);
		return dx * dx + dy * dy;
	}

	static Vector2 get_closest_point_to_segment(const Vector2 &p_point, const Vector2 &p_segment_a, const Vector2 &p_segment_b) {
		Vector2 p = p_point - p_segment_a;
		Vector2 n = p_segment_b - p_segment_a;
		real_t l2 = n.length_squared();
		if (l2 < 1e-20f) {
			return p_segment_a; // Both points are the same, just give any.
		}

		real_t d = n.dot(p) / l2;

		if (d <= 0.0f) {
			return p_segment_a; // Before first point.
		} else if (d >= 1.0f) {
			return p_segment_b; // After first point.
		} else {
			return p_segment_a + n * d; // Inside.
		}
	}

	static real_t get_distance_to_segment(const Vector2 &p_point, const Vector2 &p_segment_a, const Vector2 &p_segment_b) {
		return p_point.distance_to(get_closest_point_to_segment(p_point, p_segment_a, p_segment_b));
	}

	static real_t get_distance_to_segment_squared(const Vector2 &p_point, const Vector2 &p_segment_a, const Vector2 &p_segment_b) {
		return p_point.distance_squared_to(get_closest_point_to_segment(p_point, p_segment_a, p_segment_b));
	}
};
