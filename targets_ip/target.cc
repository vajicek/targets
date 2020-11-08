#include "opt.h"
#include "target.h"

#include <algorithm>
#include <iostream>
#include <optional>
#include <vector>

#include <opencv2/core.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/core/mat.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc.hpp>

using cv::Mat;
using cv::Matx33f;
using cv::Vec2f;
using cv::Vec2i;
using cv::Vec3b;
using cv::Vec3f;

const Vec3f YELLOW(0, 0.81, 1);
const Vec3f RED(0, 0.12, 1);
const Vec3f BLUE(1, 0.75, 0.25);
const Vec3f BLACK(0.1, 0.1, 0.1);
const Vec3f WHITE(1, 1, 1);
const Vec3f MISS(1, 1, 1);

const std::vector<Vec3f> COLORS{YELLOW, RED, BLUE, BLACK, WHITE, MISS};

bool is_in_target(const Vec2f &coord, float base_2) {
	return coord[0] > -base_2 && coord[0] < base_2 && coord[1] > -base_2 &&
		   coord[1] < base_2;
}

Vec3f get_vec3f(const gsl_vector *v, int offset) {
	return {static_cast<float>(gsl_vector_get(v, offset)),
		static_cast<float>(gsl_vector_get(v, offset + 1)),
		static_cast<float>(gsl_vector_get(v, offset + 2))};
}

Vec3f get_vec3f(const std::vector<double> &vector, int offset) {
	return Vec3f{static_cast<float>(vector[offset + 0]),
		static_cast<float>(vector[offset + 1]),
		static_cast<float>(vector[offset + 2])};
}

Matx33f eulerAnglesToRotationMatrix(const Vec3f &theta) {
	// clang-format off
	const Matx33f rx = Matx33f {
		1, 0, 0,
		0, std::cos(theta[0]), -std::sin(theta[0]),
		0, std::sin(theta[0]), std::cos(theta[0])};
	const Matx33f ry = Matx33f {
		std::cos(theta[1]), 0, std::sin(theta[1]),
		0, 1, 0,
		-std::sin(theta[1]), 0, std::cos(theta[1])};
	const Matx33f rz = Matx33f {
		std::cos(theta[2]), -std::sin(theta[2]), 0,
		std::sin(theta[2]), std::cos(theta[2]), 0,
		0, 0, 1};
	return rz * ry * rx;
	// clang-format off
}

//-----------------------------------------------------------------------------

Target::Target(
	const cv::Vec3f &center, const cv::Vec3f &euler_angles, float base)
	: center(center),
		base(base),
		rotation_matrix(eulerAnglesToRotationMatrix(euler_angles)),
		normal{rotation_matrix * Vec3f{0, 0, -1}},
		up{rotation_matrix * Vec3f{0, 1, 0}},
		binormal(cv::normalize(normal.cross(up))) {
}

std::optional<uint64> Target::target_section(const cv::Vec2f &point) const {
	if (!is_in_target(point, 1)) {
		return std::nullopt;
	}
	const float center_distance{cv::sqrt(point.dot(point))};
	const float section_width{1.0f / 5};
	const uint64 section_index{
		static_cast<uint64>(center_distance / section_width)};
	return section_index;
}

std::optional<Vec3f> Target::target_color(const cv::Vec2f &point) const {
	auto section_index{target_section(point)};
	if (!section_index.has_value()) {
		return std::nullopt;
	}
	auto color_index{std::min(section_index.value(), COLORS.size() - 1)};
	return COLORS[color_index];
}

std::optional<cv::Vec3f> Target::get_target_point(
	const cv::Vec2f &point) const {
	auto point3 = center + binormal * point[0] * base + up * point[1] * base;
	return std::optional(point3);
}

//-----------------------------------------------------------------------------

Vec2f ModelProjection::project(Vec2f model_coord) const {
	Vec3f model_3d_coord = model.get_target_point(model_coord).value();
	auto projected_coord = camera.projection_matrix * model_3d_coord;
	return {projected_coord[0] / projected_coord[2],
		projected_coord[1] / projected_coord[2]};
}

Vec2f ModelProjection::project_and_log(Vec2f model_coord, bool log) const {
	Vec3f model_3d_coord = model.get_target_point(model_coord).value();

	if (log) {
		std::cout << "model_coord = " << model_coord << "\n";
		std::cout << "model_3d_coord = " << model_3d_coord << "\n";
	}

	auto projected_coord = camera.projection_matrix * model_3d_coord;
	Vec2f projected_coord_2d{projected_coord[0] / projected_coord[2],
		projected_coord[1] / projected_coord[2]};
	if (log) {
		std::cout << "projected_coord_2d = " << projected_coord_2d << "\n";
	}
	return projected_coord_2d;
}

//-----------------------------------------------------------------------------

// clang-format off
Camera::Camera(float object_distance, float scale, Vec2f shift)
	: projection_matrix{
		object_distance * scale, 0, shift[0],
		0, object_distance * scale, shift[1],
		0, 0, 1} {
}
// clang-format on

//-----------------------------------------------------------------------------

float SystemModel::value(const Target &target_model) const {
	const Vec2f shift{camera_image.cols / 2.0f, camera_image.rows / 2.0f};
	const Camera camera{26, 10, shift};
	const ModelProjection model_projection{camera, target_model};
	float total_sample_fit_cost = 0.0f;
	int sample_count = 0;
	for (float y = -1; y < 1; y += 0.005) {
		for (float x = -1; x < 1; x += 0.005) {
			auto target_model_color =
				model_projection.model.target_color({x, y}).value_or(
					Vec3f{1, 1, 1});
			auto image_coord = model_projection.project({x, y});
			auto int_coord = Vec2i(image_coord[1], image_coord[0]);
			if (int_coord[0] < 0 || int_coord[1] >= camera_image.cols ||
				int_coord[1] < 0 || int_coord[0] >= camera_image.rows) {
				total_sample_fit_cost += 3;
				continue;
			}
			auto image_color = camera_image.at<Vec3b>(int_coord);
			auto image_color_float = Vec3f(image_color) / 255.0f;
			auto diff = image_color_float - target_model_color;
			total_sample_fit_cost += diff.dot(diff);
			sample_count++;
		}
	}
	return total_sample_fit_cost / sample_count;
}

double target_model_fit_cost(const gsl_vector *v, void *params) {
	auto &model = *reinterpret_cast<SystemModel *>(params);
	return model.value(Target{get_vec3f(v, 0), get_vec3f(v, 3), 120.0f});
}

Target fit_target_model_to_image(Mat *camera_image) {
	cv::blur(*camera_image, *camera_image, cv::Size(10, 10));
	SystemModel model{*camera_image};

	std::vector<double> result;

	optimize({0, 0, 300, 0, 0, 0},
		{1.0, 1.0, 1.0, 0.001, 0.01, 0.01},
		&model,
		target_model_fit_cost,
		&result);

	return Target{get_vec3f(result, 0), get_vec3f(result, 3), 120.0f};
}
