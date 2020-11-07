#include <iostream>

#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE TargetTest

#include <boost/test/unit_test.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc.hpp>

#include "target.h"

using cv::Matx33f;
using cv::Vec2f;
using cv::Vec2i;
using cv::Vec3b;
using cv::Vec3f;

cv::Mat load_data() {
	cv::Mat img = imread(
		"/home/vajicek/src/targets/targets_ip/testdata/img0001_scaled.jpg",
		cv::IMREAD_COLOR);
	return img;
}

struct Camera {
	Matx33f m;
	Camera(float object_distance, float screen_resolution, Vec2f shift) : m {
		object_distance * screen_resolution, 0, shift[0],
		0, object_distance * screen_resolution, shift[1],
		0, 0, 1} {
	}
};

struct ModelProjection {
	Camera camera;
	Target model;

	Vec2f project_and_log(Vec2f model_coord) {
		Vec3f model_3d_coord = model.get_target_point(model_coord).value();

		std::cout << "model_coord = " << model_coord << "\n";
		std::cout << "model_3d_coord = " << model_3d_coord << "\n";

		auto projected_coord = camera.m * model_3d_coord;
		Vec2f projected_coord_2d {projected_coord[0] / projected_coord[2],
			projected_coord[1] / projected_coord[2]};
		std::cout << "projected_coord_2d = " << projected_coord_2d << "\n";
		return projected_coord_2d;
	}
};



BOOST_AUTO_TEST_CASE(test_project_modelspace_to_imagespace) {
	auto camera_image = load_data();

	Target target_model{{0.0f, 0.0f, 240.0f},  // center
		{-1.0f, 0.0f, -1.0f},				   // normal
		{0.0f, 1.0f, 0.0f},					   // up
		120.0f};							   // target base
	Camera camera{240, 128,
		{camera_image.cols / 2.0f, camera_image.rows / 2.0f}};
	ModelProjection model_projection{camera, target_model};

	model_projection.project_and_log({-1, -1});
	model_projection.project_and_log({1, -1});
	model_projection.project_and_log({1, 1});
	model_projection.project_and_log({-1, 1});

	for (float y = -1; y < 1; y += 0.1) {
		for (float x = -1; x < 1; x += 0.1) {
			auto image_coord = model_projection.project_and_log({x, y});
			cv::drawMarker(camera_image,
				cv::Point(image_coord[0], image_coord[1]),
				cv::Scalar(0, 128, 0));
		}
	}

	imshow("opencv", camera_image);
	cv::waitKey(0);

}

BOOST_AUTO_TEST_CASE(test_target_model_metric) {
	Target better_target_model{{10.0f, -10.0f, 120.0f},	 // center
		{-0.1f, 0.0f, -1.0f},							 // normal
		{0.0f, 1.0f, 0.0f},								 // up
		120.0f};										 // target base

	Target target_model{
		{0.0f, 0.0f, 120.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, 120.0f};

	auto camera_image = load_data();

	// visualize_metric(camera_image, target_model, 180);

	auto lower_metric = compute_metric(camera_image, better_target_model, 180);
	auto higher_metric = compute_metric(camera_image, target_model, 180);
	BOOST_CHECK(lower_metric < higher_metric);
}

BOOST_AUTO_TEST_CASE(test_target_raycast) {
	Target target{{0.0f, 0.0f, 120.0f},	 // center
		{0.5f, 0.0f, -1.0f},			 // normal
		{0.0f, 1.0f, 0.0f},				 // up
		120.0f};						 // target base

	auto color = target.cast_ray_color({0, 0, 0}, {0, 0, 1}).value();

	BOOST_CHECK_EQUAL(color[0], 0.0f);
	BOOST_CHECK_EQUAL(color[1], 1.0f);
	BOOST_CHECK_EQUAL(color[2], 1.0f);
}

BOOST_AUTO_TEST_CASE(test_mat_convert) {
	const float float_val = 0.5;
	cv::Mat m_f(1, 1, CV_32FC1);
	m_f.at<float>() = {float_val};
	auto mat_float_val = m_f.at<float>(0, 0);
	BOOST_CHECK_EQUAL(mat_float_val, float_val);

	cv::Mat m_u8(1, 1, CV_8UC1);
	m_f.convertTo(m_u8, CV_8UC1, 255.0f, 0.0f);
	auto mat_u8_val = m_u8.at<uint8_t>(0, 0);
	BOOST_CHECK_EQUAL(
		mat_u8_val, cv::saturate_cast<uchar>(float_val * 255.0f + 0.0f));
	BOOST_CHECK_EQUAL(
		mat_u8_val, static_cast<uchar>(std::round(float_val * 255.0f + 0.0f)));
}

BOOST_AUTO_TEST_CASE(test_mat_casting) {
	cv::Mat m_3f(1, 1, CV_32FC3);
	m_3f.at<Vec3f>() = {1, 0.2f, 3};
	auto val = m_3f.at<Vec3f>(0, 0);
	BOOST_CHECK_EQUAL(val[0], 1.0f);
	BOOST_CHECK_EQUAL(val[1], 0.2f);
	BOOST_CHECK_EQUAL(val[2], 3.0f);

	cv::Mat m_3u8(1, 1, CV_8UC3);
	m_3f.convertTo(m_3u8, CV_8UC3, 255.0, 0);

	auto mat_3u8_val = m_3u8.at<Vec3b>(0, 0);
	Vec3b expected{255, 51, 255};
	BOOST_CHECK_EQUAL(mat_3u8_val[0], expected[0]);
	BOOST_CHECK_EQUAL(mat_3u8_val[1], expected[1]);
	BOOST_CHECK_EQUAL(mat_3u8_val[2], expected[2]);
}

BOOST_AUTO_TEST_CASE(plane_ray_intersection) {
	Vec3f center{0, 0, 120};
	Vec3f up{0, 1, 0};
	Vec3f normal{0, 0, -1};
	Vec3f origin{0, 0, 0};
	Vec3f direction{0.1, 0.2, 1};
	auto a = plane_ray_intersection_2d_composed(
		center, up, normal, origin, direction);
	auto b =
		plane_ray_intersection_2d_direct(center, up, normal, origin, direction);

	BOOST_CHECK_EQUAL(a, b);
}

/*
Fitting Parameterized 3-D Models to Images
http://www.ai.mit.edu/courses/6.899/papers/pami91.pdf

3D Parametric Models
https://cse291-i.github.io/WI18/LectureSlides/L10_Learning_for_parametric_models.pdf
*/
