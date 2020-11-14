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
	return coord[0] >= -base_2 && coord[0] <= base_2 && coord[1] >= -base_2 &&
		   coord[1] <= base_2;
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

std::optional<Vec3f> img_color(const Mat &camera_image,
	const ModelProjection &model_projection,
	const Vec2f &model_coord) {
	auto image_coord = model_projection.project(model_coord);
	auto int_coord = Vec2i(image_coord[1], image_coord[0]);
	if (int_coord[0] < 0 || int_coord[1] >= camera_image.cols ||
		int_coord[1] < 0 || int_coord[0] >= camera_image.rows) {
		return std::nullopt;
	}
	auto image_color = camera_image.at<Vec3b>(int_coord);
	return std::optional(Vec3f(image_color) / 255.0f);
}

float dota(const std::optional<Vec3f> &v) {
	if (v.has_value()) {
		return v.value().dot(v.value());
	}
	return 0;
}

float sample_model_edges(const Mat &camera_image, const ModelProjection &model_projection) {
	const float section_width = 1.0 / 5;
	float total_sample_fit_cost = 0.0f;
	int sample_count = 0;
	for (float t = 0; t < 1; t += 0.01) {
		total_sample_fit_cost += dota(img_color(camera_image, model_projection, Vec2f{t, 0}));
		total_sample_fit_cost += dota(img_color(camera_image, model_projection, Vec2f{t, 1}));
		total_sample_fit_cost += dota(img_color(camera_image, model_projection, Vec2f{0, t}));
		total_sample_fit_cost += dota(img_color(camera_image, model_projection, Vec2f{1, t}));
		sample_count += 4;

		Vec2f direction {cosf(t * 2 * M_PI), sinf(t * 2 * M_PI)};
		for (int circle = 0; circle < 4; circle ++) {
			const double circle_radius = (circle + 1) * section_width;
			const Vec2f circle_point {direction * circle_radius};
			total_sample_fit_cost += dota(img_color(camera_image, model_projection, circle_point));
		}
		sample_count += 4;
	}
	return total_sample_fit_cost / sample_count;
}

float sample_model_area_error(const Mat &camera_image, const ModelProjection &model_projection) {
	float total_sample_fit_cost = 0.0f;
	int sample_count = 0;
	for (float y = -1; y <= 1; y += 0.01) {
		for (float x = -1; x <= 1; x += 0.01) {
			const Vec2f model_coord {x, y};
			auto target_model_color = model_projection.model.target_color(model_coord);
			auto image_color_float = img_color(camera_image, model_projection, model_coord);
			if (!image_color_float.has_value() || !target_model_color.has_value()) {
				std::cout << "should not happen " << model_coord
					<< " " << image_color_float.has_value()
					<< " " << target_model_color.has_value() << "\n";
				total_sample_fit_cost += 3;
			} else {
				auto diff = image_color_float.value() - target_model_color.value();
				// std::cout << model_coord << " -> "
				// 	<< image_color_float.value() << ","
				// 	<< target_model_color.value() << "\n";
				total_sample_fit_cost += diff.dot(diff);
			}
			sample_count++;
		}
	}
	return total_sample_fit_cost / sample_count;
}

float SystemModel::value(const Target &target_model) const {
	const Vec2f shift{camera_image.cols / 2.0f, camera_image.rows / 2.0f};
	const Camera camera{26, 10, shift};
	const ModelProjection model_projection{camera, target_model};

	float edge_cost = sample_model_edges(camera_image_edges, model_projection);
	float area_error_cost = sample_model_area_error(camera_image, model_projection);

	std::cout
		<< "area_error_cost = " << area_error_cost
		<< " edge_cost = " << edge_cost << "\n";

	return area_error_cost - 1.0*edge_cost;
}

double target_model_fit_cost(const gsl_vector *v, void *params) {
	auto &model = *reinterpret_cast<SystemModel *>(params);
	return model.value(Target{get_vec3f(v, 0), get_vec3f(v, 3), 120.0f});
}

Target fit_target_model_to_image(const Mat &camera_image) {
	cv::Mat camera_image_edges;
	cv::Mat blurred_camera_image = camera_image.clone();

	imshow("opencv", camera_image);
	cv::waitKey(0);


	cv::blur(blurred_camera_image, blurred_camera_image, cv::Size(5, 5));
	imshow("opencv", blurred_camera_image);
	cv::waitKey(0);

	cv::Canny(camera_image, camera_image_edges, 150, 400, 3, true);
	imshow("opencv", camera_image_edges);
	cv::waitKey(0);

	cv::blur(camera_image_edges, camera_image_edges, cv::Size(8, 8));
	imshow("opencv", camera_image_edges);
	cv::waitKey(0);

	SystemModel model{blurred_camera_image, camera_image_edges};

	std::vector<double> result;

	optimize({0, 0, 300, 0, 0, 0},
		{1.0, 1.0, 1.0, 0.001, 0.01, 0.01},
		&model,
		target_model_fit_cost,
		&result);

	return Target{get_vec3f(result, 0), get_vec3f(result, 3), 120.0f};
}
