#ifndef _TARGET_MODEL_H
#define _TARGET_MODEL_H
#pragma once

#include <optional>

#include <opencv2/core/core.hpp>

class Target {
 private:
	cv::Vec3f center;
	float base;
	cv::Matx33f rotation_matrix;
	cv::Vec3f normal;
	cv::Vec3f up;
	cv::Vec3f binormal;

 public:
	explicit Target(const cv::Vec3f &center, const cv::Vec3f &euler_angles, float base);

	std::optional<cv::Vec3f> get_target_point(const cv::Vec2f &point) const;
	std::optional<uint64> target_section(const cv::Vec2f &point) const;
	std::optional<cv::Vec3f> target_color(const cv::Vec2f &point) const;
};

struct Camera {
	cv::Matx33f projection_matrix;
	Camera(float object_distance, float scale, cv::Vec2f shift);
};

struct ModelProjection {
	Camera camera;
	Target model;

	cv::Vec2f project(cv::Vec2f model_coord) const;
};

struct SystemModel {
	cv::Mat camera_image;
	cv::Mat camera_image_edges;

	float value(const Target &target_model) const;
};

Target fit_target_model_to_image(const cv::Mat &camera_image);

#endif	 // _TARGET_MODEL_H
