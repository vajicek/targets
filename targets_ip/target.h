#ifndef _TARGET_H
#define _TARGET_H
#pragma once

#include <optional>

#include "opencv2/core/core.hpp"

class Target {
 public:
	cv::Vec3f center;
	cv::Vec3f normal;
	cv::Vec3f up;
	float base;

	std::optional<cv::Vec3f> cast_ray_color(const cv::Vec3f &origin,
		const cv::Vec3f &direction) const;
	std::optional<uint64> cast_ray_section(const cv::Vec3f &origin,
		const cv::Vec3f &direction) const;
};

double compute_metric(const cv::Mat &camera_image,
		const Target &target,
		const float focal_length);

#endif  // _TARGET_H
