#include "target.h"

#include <algorithm>
#include <optional>
#include <vector>
#include <iostream>

#include "opencv2/core.hpp"
#include "opencv2/core/core.hpp"
#include <opencv2/core/mat.hpp>

const cv::Vec3f YELLOW(0, 1, 1);
const cv::Vec3f RED(0, 0, 1);
const cv::Vec3f BLUE(1, 0, 0);
const cv::Vec3f BLACK(0, 0, 0);
const cv::Vec3f WHITE(1, 1, 1);
const cv::Vec3f MISS(0.1, 0.1, 0.1);

const std::vector<cv::Vec3f> COLORS {YELLOW, RED, BLUE, BLACK, WHITE, MISS};

cv::Vec2f plane_ray_intersection(
		const cv::Vec3f &center,
		const cv::Vec3f &up,
		const cv::Vec3f &normal,
		const cv::Vec3f &origin,
		const cv::Vec3f &direction) {
	// origin - center = x * binorm + y * up - t * direction;
	// v = m * x;

	auto binorm = normal.cross(up);

	cv::Matx31f v { (origin - center).val };
	cv::Matx33f m {
		binorm[0], up[0], -direction[0],
		binorm[1], up[1], -direction[1],
		binorm[2], up[2], -direction[2]};
	auto m_inv = m.inv();
	auto x = m_inv * v;

	return cv::Vec2f { x.val[0], x.val[1] };
}

bool is_in_target(const cv::Vec2f &coord, float base_2) {
	return coord[0] > -base_2 && coord[0] < base_2 &&
		coord[1] > -base_2 && coord[1] < base_2;
}

std::optional<uint64> Target::cast_ray_section(const cv::Vec3f &origin,
	const cv::Vec3f &direction) const {
	auto coord_intersection = plane_ray_intersection(
		center, up, normal, origin, direction);
	float base_2 = base / 2;
	if (!is_in_target(coord_intersection, base_2)) {
		return std::nullopt;
	}
	const float center_distance {cv::sqrt(coord_intersection.dot(coord_intersection))};
	const float section_width { base_2 / 5 };
	const uint64 section_index { static_cast<uint64>(center_distance / section_width)};
	return section_index;
}

std::optional<cv::Vec3f> Target::cast_ray_color(const cv::Vec3f &origin,
		const cv::Vec3f &direction) const {
	auto section_index { cast_ray_section(origin, direction) };
	if (!section_index.has_value()) {
		return std::nullopt;
	}
	auto color_index { std::min(section_index.value(), COLORS.size() - 1) };
	return COLORS[color_index];
}
