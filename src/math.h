#pragma once

#include "godot_cpp/core/defs.hpp"
#include "godot_cpp/variant/quaternion.hpp"
#include "godot_cpp/variant/vector2.hpp"

using namespace godot;

namespace LNMath {

static constexpr float AV_2_RPM = 60.0f / Math_TAU;
static constexpr float RPM_2_AV = Math_TAU / 60.0f;

static _FORCE_INLINE_ float smoothmin(float p_a, float p_b, float p_k) {
	float h = CLAMP(0.5f + 0.5f * (p_a - p_b) / p_k, 0.0f, 1.0f);
	return Math::lerp(p_a, p_b, h) - p_k * h * (1.0f - h);
}

static _FORCE_INLINE_ Vector3 quat_log(Quaternion q, float eps = 1e-8f) {
	float length = Math::sqrt(q.x * q.x + q.y * q.y + q.z * q.z);

	if (length < eps) {
		return Vector3(q.x, q.y, q.z);
	} else {
		float halfangle = Math::acos(CLAMP(q.w, -1.0f, 1.0f));
		return halfangle * (Vector3(q.x, q.y, q.z) / length);
	}
}

static _FORCE_INLINE_ Quaternion quat_inv(const Quaternion &p_x) {
	return Quaternion(p_x.x, p_x.y, p_x.z, -p_x.w);
}
static _FORCE_INLINE_ Quaternion quat_abs(const Quaternion &p_x) {
	return p_x.w < 0.0 ? -p_x : p_x;
}

static _FORCE_INLINE_ Quaternion quat_normalize(Quaternion q, const float eps = 1e-8f) {
	return q / (q.length() + eps);
}

static _FORCE_INLINE_ Quaternion quat_exp(Vector3 v, float eps = 1e-8f) {
	float halfangle = sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);

	if (halfangle < eps) {
		return quat_normalize(Quaternion(v.x, v.y, v.z, 1.0f));
	} else {
		float c = cosf(halfangle);
		float s = sinf(halfangle) / halfangle;
		return Quaternion(s * v.x, s * v.y, s * v.z, c);
	}
}

static _FORCE_INLINE_ Quaternion quat_from_scaled_angle_axis(Vector3 v, float eps = 1e-8f) {
	return quat_exp(v / 2.0f, eps);
}

static _FORCE_INLINE_ Vector3 quat_to_scaled_angle_axis(Quaternion q, float eps = 1e-8f) {
	return 2.0f * quat_log(q);
}

static _FORCE_INLINE_ Quaternion quat_mul(Quaternion q, Quaternion p) {
	return Quaternion(
			p.w * q.x + p.x * q.w - p.y * q.z + p.z * q.y,
			p.w * q.y + p.x * q.z + p.y * q.w - p.z * q.x,
			p.w * q.z - p.x * q.y + p.y * q.x + p.z * q.w,
			p.w * q.w - p.x * q.x - p.y * q.y - p.z * q.z);
}

static inline void quat_to_angle_axis(Quaternion q, float &angle, Vector3 &axis, float eps = 1e-8f) {
	float length = Math::sqrt(q.x * q.x + q.y * q.y + q.z * q.z);

	if (length < eps) {
		angle = 0.0f;
		axis = Vector3(1.0f, 0.0f, 0.0f);
	} else {
		angle = 2.0f * Math::acos(CLAMP(q.w, -1.0f, 1.0f));
		axis = Vector3(q.x, q.y, q.z) / length;
	}
}

static _FORCE_INLINE_ Quaternion quat_from_angle_axis(float angle, Vector3 axis) {
	float c = cosf(angle / 2.0f);
	float s = sinf(angle / 2.0f);
	return Quaternion(s * axis.x, s * axis.y, s * axis.z, c);
}

static _FORCE_INLINE_ float fast_negexp(float x) {
	return 1.0f / (1.0f + x + 0.48f * x * x + 0.235f * x * x * x);
}

static _FORCE_INLINE_ int32_t circle_intersect(const Vector2 &p_center_0, const float p_radius_0, const Vector2 &p_center_1, const float p_radius_1, Vector2 &r_out0, Vector2 &r_out1, const float p_eps = 0.001f) {
	// -------------------------------------------------------------------------
	// Robust circle-circle intersection (2D)
	// -------------------------------------------------------------------------
	// Standard derivation:
	//  - Let d = |C2 - C1|
	//  - If circles intersect, the intersection chord's midpoint P2 lies on the line C1->C2 at distance 'a' from C1
	//  - a = (R1^2 - R2^2 + d^2) / (2d)
	//  - h^2 = R1^2 - a^2  (height from P2 to each intersection point along the perpendicular)
	//
	// Numerical notes:
	//  - We explicitly handle degenerate cases (coincident centers, near-tangent).
	//  - We clamp small negative values of h^2 to zero to reduce floating-point noise.
	const Vector2 dir_vec = (p_center_1 - p_center_0);
	const float d = dir_vec.length();

	// Degenerate: same (or extremely close) centers.
	if (d < p_eps) {
		if (Math::is_equal_approx(p_radius_0, p_radius_1)) {
			// Coincident circles: infinitely many intersection points.
			return -1;
		} else {
			// Concentric circles with different radii: no intersection.
			return 0;
		}
	}

	// No intersection if too far apart or one fully inside the other without touching.
	// We apply tolerance Eps to make the test robust near the boundary.
	if (d > p_radius_0 + p_radius_1 + p_eps || d < Math::abs(p_radius_0 - p_radius_1) - p_eps) {
		return 0;
	}

	// Distance from C1 to the chord midpoint P2 along the line C1->C2.
	const float a = (p_radius_0 * p_radius_0 - p_radius_1 * p_radius_1 + d * d) / (2.0f * d);

	// Height squared from P2 to intersections.
	float h2 = p_radius_0 * p_radius_0 - a * a;

	// Clamp tiny negative values caused by floating-point error.
	if (h2 < 0.0f && h2 > -1e-3f) {
		h2 = 0.0f;
	}
	// If still negative, treat as no intersection (safety).
	if (h2 < 0.0f) {
		return 0;
	}

	const float h = Math::sqrt(h2);

	// Unit direction from C1 to C2.
	const Vector2 dir = dir_vec / d;
	// Chord midpoint.
	const Vector2 P2 = p_center_0 + a * dir;

	// Perpendicular vector in 2D (rotated 90 degrees).
	const Vector2 perp(-dir.y, dir.x);

	// Two symmetric intersection points.
	r_out0 = P2 + h * perp;
	r_out1 = P2 - h * perp;

	// If h is ~0 the circles are tangent (one intersection).
	return (h <= p_eps) ? 1 : 2;
}

static _FORCE_INLINE_ int circle_intersect_2d(
		const Vector2 &p_center_a,
		float p_radius_a,
		const Vector2 &p_center_b,
		float p_radius_b,
		Vector2 &r_p0,
		Vector2 &r_p1,
		float p_eps = 1e-6f) {
	const Vector2 delta = p_center_b - p_center_a;
	const float dist = delta.length();

	// No intersection: circles too far apart or one inside the other.
	if (dist > p_radius_a + p_radius_b + p_eps) {
		return 0;
	}
	if (dist < Math::abs(p_radius_a - p_radius_b) - p_eps) {
		return 0;
	}
	// No intersection: coincident circles.
	if (dist < p_eps) {
		return 0;
	}

	// Distance from CenterA to the radical line (the chord connecting the two intersection points).
	const float A = (p_radius_a * p_radius_a - p_radius_b * p_radius_b + dist * dist) / (2.0f * dist);

	// Distance from the radical line to either intersection point (half-chord length).
	const float H2 = p_radius_a * p_radius_a - A * A;
	const float H = (H2 > 0.0f) ? Math::sqrt(H2) : 0.0f;

	// Point on the line AB closest to both intersection points.
	const Vector2 delta_n = delta / dist;
	const Vector2 mid = p_center_a + delta_n * A;

	// Perpendicular to AB, scaled by H.
	const Vector2 perp = Vector2(-delta_n.y, delta_n.x) * H;

	r_p0 = mid + perp;
	r_p1 = mid - perp;

	// Tangent: both points are the same, report as 1 solution.
	return (H < p_eps) ? 1 : 2;
}

static _FORCE_INLINE_ Vector2 to_plane_2d(const Vector3 &p_point, const Vector3 &p_plane_origin, const Vector3 &p_plane_axis_right, const Vector3 &p_plane_axis_up) {
	// Convert to plane-local coordinates by projecting the relative vector onto the plane basis.
	// Assumes AxisRight/AxisUp form an orthonormal basis (or close enough).
	const Vector3 rel = p_point - p_plane_origin;
	return Vector2(
			rel.dot(p_plane_axis_right),
			rel.dot(p_plane_axis_up));
}

static _FORCE_INLINE_ void make_plane_basis(const Vector3 &p_plane_normal, Vector3 &p_plane_axis_right, Vector3 &p_plane_axis_up) {
	const Vector3 Hint = (Math::abs(p_plane_normal.dot(Vector3(0, 1, 0))) < 0.99f)
			? Vector3(0, 1, 0)
			: Vector3(1, 0, 0);

	p_plane_axis_right = p_plane_normal.cross(Hint).normalized();
	p_plane_axis_up = p_plane_normal.cross(p_plane_axis_right).normalized();
}

static _FORCE_INLINE_ Vector2 to_plane_2d(const Vector3 &p_point, const Vector3 &p_plane_origin, const Vector3 &p_plane_normal) {
	Vector3 AxisRight, AxisUp;
	make_plane_basis(p_plane_normal, AxisRight, AxisUp);

	const Vector3 Rel = p_point - p_plane_origin;
	return Vector2(Rel.dot(AxisRight), Rel.dot(AxisUp));
}

static _FORCE_INLINE_ Vector3 from_plane_2d(const Vector2 &p_point, const Vector3 &p_plane_origin, const Vector3 &p_plane_normal) {
	Vector3 plane_axis_right, plane_axis_up;
	make_plane_basis(p_plane_normal, plane_axis_right, plane_axis_up);

	return p_plane_origin + plane_axis_right * p_point.x + plane_axis_up * p_point.y;
}

static Vector3 _FORCE_INLINE_ from_plane_2d(const Vector2 &p_point, const Vector3 &p_plane_origin, const Vector3 &p_plane_axis_right, const Vector3 &p_plane_axis_up) {
	// Reconstruct 3D point from plane coordinates using the provided basis.
	return p_plane_origin + (p_plane_axis_right * p_point.x + p_plane_axis_up * p_point.y);
}

}; //namespace LNMath
