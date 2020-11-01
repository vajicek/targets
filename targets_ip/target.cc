#include "target.h"

#include <algorithm>
#include <optional>
#include <vector>
#include <iostream>

#include "opencv2/core.hpp"
#include "opencv2/core/core.hpp"
#include <opencv2/core/mat.hpp>
#include <opencv2/highgui/highgui.hpp>

using cv::Vec2f;
using cv::Vec2i;
using cv::Vec3f;
using cv::Vec3b;

const Vec3f YELLOW(0, 1, 1);
const Vec3f RED(0, 0, 1);
const Vec3f BLUE(1, 0, 0);
const Vec3f BLACK(0.1, 0.1, 0.1);
const Vec3f WHITE(1, 1, 1);
const Vec3f MISS(0.1, 0.1, 0.1);

const std::vector<Vec3f> COLORS {YELLOW, RED, BLUE, BLACK, WHITE, MISS};

Vec3f plane_ray_intersection_3d(
		const Vec3f &center,
		const Vec3f &normal,
		const Vec3f &origin,
		const Vec3f &direction) {
	auto offset = center.dot(normal);
	auto ray_scale = (offset - origin.dot(normal)) / direction.dot(normal);
	return origin + ray_scale * direction;
}

/*
cx a
cy b
cz c

nx d
ny e
nz f

ox g
oy h
oz i

dx j
dy k
dz l

ux m
uy n
uz o
*/


//g + j * ((a*d+b*e+c*f)-(g*d+h*e+i*f))/(j*d+k*e+l*f) - a
//h + k * ((a*d+b*e+c*f)-(g*d+h*e+i*f))/(j*d+k*e+l*f) - b
//i + l * ((a*d+b*e+c*f)-(g*d+h*e+i*f))/(j*d+k*e+l*f) - c

//binorm = def x mno = e*o - f*n, f*m - d*o, d*n - e*m

// 2d_x = (O + D*(C.N - O.N)/(D.N) - C) . N x U
// (e*o - f*n) * g + (e*o - f*n) * j * ((a*d + b*e + c*f) - (g*d + h*e + i*f)) / (j*d + k*e + l*f) - a * (e*o - f*n) +
// (f*m - d*o) * h + (f*m - d*o) * k * ((a*d + b*e + c*f) - (g*d + h*e + i*f)) / (j*d + k*e + l*f) - b * (f*m - d*o) +
// (d*n - e*m) * i + (d*n - e*m) * l * ((a*d + b*e + c*f) - (g*d + h*e + i*f)) / (j*d + k*e + l*f) - c * (d*n - e*m)

// 2d_y = (O + D*(C.N - O.N)/(D.N) - C) . U
// m*g + m*j * ((a*d + b*e + c*f) - (g*d + h*e + i*f)) / (j*d + k*e + l*f) - a*m +
// n*h + n*k * ((a*d + b*e + c*f) - (g*d + h*e + i*f)) / (j*d + k*e + l*f) - b*n +
// o*i + o*l * ((a*d + b*e + c*f) - (g*d + h*e + i*f)) / (j*d + k*e + l*f) - c*o

Vec2f plane_ray_intersection_2d_composed_dc(
		const Vec3f &center,
		const Vec3f &up,
		const Vec3f &normal,
		const Vec3f &origin,
		const Vec3f &direction,
		const int dc) {
	float a = center[0];
	float b = center[1];
	float c = center[2];

	float d = normal[0];
	float e = normal[1];
	float f = normal[2];

	float g = origin[0];
	float h = origin[1];
	float i = origin[2];

	float j = direction[0];
	float k = direction[1];
	float l = direction[2];

	float m = up[0];
	float n = up[1];
	float o = up[2];

	switch (dc) {
		case 0:
			return {
				((e*o - f*n) * j + (f*m - d*o) * k + (d*n - e*m) * l) * d / (j*d + k*e + l*f) - (e*o - f*n),
				d*(m*j + n*k + o*l) / (j*d + k*e + l*f) - m
			};
		case 1:
			return {
				((e*o - f*n) * j + (f*m - d*o) * k + (d*n - e*m) * l) * e / (j*d + k*e + l*f) - (f*m - d*o),
				e*(o*l + n*k + o*l) / (j*d + k*e + l*f) - n
			};
		case 2:
			return {
				((e*o - f*n) * j + (f*m - d*o) * k + (d*n - e*m) * l) * f / (j*d + k*e + l*f) - (d*n - e*m),
				f*(m*j + n*k + o*l) / (j*d + k*e + l*f) - o
			};
	}
	throw new std::runtime_error("invalid dc parameter");
}

Vec2f plane_ray_intersection_2d_composed_dn(
		const Vec3f &center,
		const Vec3f &up,
		const Vec3f &normal,
		const Vec3f &origin,
		const Vec3f &direction,
		const int dn) {
	switch (dn) {
		case 0:
			return {
				0,
				0
			};
		case 1:
			return {
				0,
				0
			};
		case 2:
			return {
				0,
				0
			};
	}
	throw new std::runtime_error("invalid dn parameter");
}

Vec2f plane_ray_intersection_2d_composed(
		const Vec3f &center,
		const Vec3f &up,
		const Vec3f &normal,
		const Vec3f &origin,
		const Vec3f &direction) {
	auto intersection = plane_ray_intersection_3d(center, normal, origin, direction);
	auto intersection_local = intersection - center;
	auto binorm = normal.cross(up);
	return { intersection_local.dot(binorm), intersection_local.dot(up) };
}

Vec2f plane_ray_intersection_2d_direct(
		const Vec3f &center,
		const Vec3f &up,
		const Vec3f &normal,
		const Vec3f &origin,
		const Vec3f &direction) {
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

	return Vec2f { x.val[0], x.val[1] };
}

bool is_in_target(const Vec2f &coord, float base_2) {
	return coord[0] > -base_2 && coord[0] < base_2 &&
		coord[1] > -base_2 && coord[1] < base_2;
}

std::optional<uint64> Target::cast_ray_section(const Vec3f &origin,
	const Vec3f &direction) const {
	auto coord_intersection = plane_ray_intersection_2d_direct(
		center, up, normal, origin, direction);
	float base_2 = base / 2;
	if (!is_in_target(coord_intersection, base_2)) {
		return std::nullopt;
	}
	const float center_distance { cv::sqrt(coord_intersection.dot(coord_intersection)) };
	const float section_width { base_2 / 5 };
	const uint64 section_index { static_cast<uint64>(center_distance / section_width) };
	return section_index;
}

std::optional<Vec3f> Target::cast_ray_color(const Vec3f &origin,
		const Vec3f &direction) const {
	auto section_index { cast_ray_section(origin, direction) };
	if (!section_index.has_value()) {
		return std::nullopt;
	}
	auto color_index { std::min(section_index.value(), COLORS.size() - 1) };
	return COLORS[color_index];
}

void sample_space(const Target &target,
		const Vec2i &count,
		const float focal_length,
		const std::function<void(const Vec2i&,
			const std::optional<Vec3f>&)> &process_sample) {
	const Vec2f start {-count[0] / 2.0f, -count[1] / 2.0f};
	const Vec2f step {1.0, 1.0};
	for (int y = 0; y < count[1]; y++) {
		for (int x = 0; x < count[0]; x++) {
			const Vec2i int_coord { x, y };
			const Vec2f coord { static_cast<float>(x), static_cast<float>(y) };
			const Vec2f projection_plane_cooord { (start + coord).mul(step) };
			const Vec3f direction = cv::normalize(Vec3f { projection_plane_cooord[0],
				projection_plane_cooord[1],
				focal_length });
			auto model_color = target.cast_ray_color({ 0, 0, 0 }, direction);
			process_sample(int_coord, model_color);
		}
	}
}

void sample_image(const Target &target,
		const cv::Mat &camera_image,
		const float focal_length,
		const std::function<void(const Vec2i&,
			const std::optional<Vec3f>&,
			const Vec3f&)> &process_image_sample) {
	cv::Mat camera_image_float;
	camera_image.convertTo(camera_image_float, CV_32FC3);

	const Vec2i count {camera_image_float.cols, camera_image_float.rows};
	sample_space(target, count, focal_length,
		[&camera_image_float, &process_image_sample](
				const Vec2i& int_coord,
				const std::optional<Vec3f>& model_color) {
			auto camera_image_color_float =
				camera_image_float.at<Vec3f>(int_coord[1], int_coord[0]);
			process_image_sample(int_coord, model_color, camera_image_color_float);
		});
}

void visualize_model(const Target &target,
		const Vec2f &start,
		const Vec2f &step,
		const Vec2i &count,
		const float focal_length) {
	cv::Mat model_image(count[1], count[0], CV_32FC3);

	sample_space(target, count, focal_length,
		[&model_image](const Vec2i& int_coord, const std::optional<Vec3f>& model_color) {
			model_image.at<Vec3f>(int_coord[1], int_coord[0]) =
				model_color.value_or(Vec3f { 0.1f, 0.0f, 0.1f });
		});

	cv::Mat model_image_3b;
	model_image.convertTo(model_image_3b, CV_8UC3, 255.0, +128);
	imshow("opencv", model_image_3b);
	cv::waitKey(0);
}

void visualize_metric(const cv::Mat &camera_image,
		const Target &target,
		const float focal_length,
		const int field_height = 32) {
	cv::Mat metric_samples(camera_image.rows, camera_image.cols, CV_32FC3);
	sample_image(target, camera_image, focal_length,
		[&metric_samples, &field_height](const Vec2i& int_coord,
				const std::optional<Vec3f>& model_color,
				const Vec3f& camera_image_color_float) {
			metric_samples.at<Vec3f>(int_coord[1], int_coord[0]) = camera_image_color_float;
			if (model_color.has_value()) {
				auto diff = camera_image_color_float - 255 * model_color.value();
				auto dist = std::sqrt(diff.dot(diff));
				if ((int_coord[0] / field_height + int_coord[1] / field_height) % 2 == 0) {
					metric_samples.at<Vec3f>(int_coord[1], int_coord[0]) = Vec3f {dist};
				}
			}
		});

	cv::Mat metric_samples_3b;
	metric_samples.convertTo(metric_samples_3b, CV_8UC3);
	imshow("opencv", metric_samples_3b);
	cv::waitKey(0);
}

double compute_metric(const cv::Mat &camera_image,
		const Target &target,
		const float focal_length) {
	double total = 0;
	int samples = 0;
	sample_image(target, camera_image, focal_length,
		[&total, &samples](const Vec2i& int_coord,
				const std::optional<Vec3f>& model_color,
				const Vec3f& camera_image_color_float) {
			if (model_color.has_value()) {
				auto diff = camera_image_color_float - 255 * model_color.value();
				auto dist = std::sqrt(diff.dot(diff));
				total += dist;
				samples++;
			}
		});
	return total / samples;
}
