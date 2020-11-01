#include <iostream>

#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE TargetTest

#include "target.h"

#include <boost/test/unit_test.hpp>
#include <opencv2/highgui/highgui.hpp>

using cv::Vec2f;
using cv::Vec2i;
using cv::Vec3f;
using cv::Vec3b;

cv::Mat load_data() {
	cv::Mat img = imread("/home/vajicek/src/targets/targets_ip/testdata/img0001_scaled.jpg",
		cv::IMREAD_COLOR);
	return img;
}

BOOST_AUTO_TEST_CASE(test_target_model_derivatives) {
	const cv::Vec3f center {10.0f, -10.0f, 120.0f};
	const cv::Vec3f up {0.0f, 1.0f, 0.0f};
	const cv::Vec3f normal {-0.1f, 0.0f, -1.0f};
	const cv::Vec3f origin {0.0f, 0.0f, 0.0f};
	const cv::Vec3f direction {0.1f, 0.1f, 1.0f};

	auto t_dc0_analytic = plane_ray_intersection_2d_composed_dc(
		center, up, normal,
		origin, direction,
		0);

	float epsilon = 0.01;

	auto t_dc0_p = plane_ray_intersection_2d_composed(
		center + cv::Vec3f {epsilon, 0, 0}, up, normal,
		origin, direction);

	auto t_dc0_m = plane_ray_intersection_2d_composed(
		center - cv::Vec3f {epsilon, 0, 0}, up, normal,
		origin, direction);

	auto t_dc0_numeric = (t_dc0_p - t_dc0_m) * (0.5 / epsilon);

	std::cout << "t_dc0_analytic= " << t_dc0_analytic << "\n";
	std::cout << "t_dc0_numeric= " << t_dc0_numeric << "\n";
}

BOOST_AUTO_TEST_CASE(test_target_model_metric) {
	Target better_target_model {
		{10.0f, -10.0f, 120.0f},  // center
		{-0.1f, 0.0f, -1.0f},  // normal
		{0.0f, 1.0f, 0.0f},  // up
		120.0f};  // target base

	Target target_model {
		{0.0f, 0.0f, 120.0f},
		{0.0f, 0.0f, 0.0f},
		{0.0f, 1.0f, 0.0f},
		120.0f};

	auto camera_image = load_data();

	// visualize_metric(camera_image, target_model, 180);

	auto lower_metric = compute_metric(camera_image, better_target_model, 180);
	auto higher_metric = compute_metric(camera_image, target_model, 180);
	BOOST_CHECK(lower_metric < higher_metric);
}

BOOST_AUTO_TEST_CASE(test_target_raycast) {
	Target target {
		{0.0f, 0.0f, 120.0f},  // center
		{0.5f, 0.0f, -1.0f},  // normal
		{0.0f, 1.0f, 0.0f},  // up
		120.0f};  // target base

	auto color = target.cast_ray_color({ 0, 0, 0 }, {0, 0, 1}).value();

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
	BOOST_CHECK_EQUAL(mat_u8_val,
		cv::saturate_cast<uchar>(float_val * 255.0f + 0.0f));
	BOOST_CHECK_EQUAL(mat_u8_val,
		static_cast<uchar>(std::round(float_val * 255.0f + 0.0f)));
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
	Vec3b expected {255, 51, 255};
	BOOST_CHECK_EQUAL(mat_3u8_val[0], expected[0]);
	BOOST_CHECK_EQUAL(mat_3u8_val[1], expected[1]);
	BOOST_CHECK_EQUAL(mat_3u8_val[2], expected[2]);
}

BOOST_AUTO_TEST_CASE(plane_ray_intersection) {
	Vec3f center {0, 0, 120};
	Vec3f up {0, 1, 0};
	Vec3f normal {0, 0, -1};
	Vec3f origin {0, 0, 0};
	Vec3f direction {0.1, 0.2, 1};
	auto a = plane_ray_intersection_2d_composed(
		center, up, normal, origin, direction);
	auto b = plane_ray_intersection_2d_direct(
		center, up, normal, origin, direction);

	BOOST_CHECK_EQUAL(a, b);
}

/*
Fitting Parameterized 3-D Models to Images
http://www.ai.mit.edu/courses/6.899/papers/pami91.pdf

3D Parametric Models
https://cse291-i.github.io/WI18/LectureSlides/L10_Learning_for_parametric_models.pdf
*/
