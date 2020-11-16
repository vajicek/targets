#include <algorithm>
#include <iostream>
#include <vector>

#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE TargetTest

#include <boost/test/unit_test.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc.hpp>

#include "opt.h"
#include "target_model.h"
#include "utils.h"

using cv::Mat;
using cv::Matx33f;
using cv::Vec2f;
using cv::Vec2i;
using cv::Vec3b;
using cv::Vec3f;
using boost::unit_test::disabled;

Mat load_data() {
	Mat img = imread(
		"/home/vajicek/src/targets/targets_ip/testdata/img0001_scaled.jpg",
		cv::IMREAD_COLOR);
	return img;
}

void show(Mat *camera_image, const ModelProjection &model_projection) {
	for (float y = -1; y < 1; y += 0.05) {
		for (float x = -1; x < 1; x += 0.05) {
			auto target_color =
				model_projection.model.target_color({x, y}).value_or(
					Vec3f{1, 1, 1});
			auto image_coord = model_projection.project({x, y});
			auto int_coord = Vec2i(image_coord[1], image_coord[0]);
			auto image_color = camera_image->at<Vec3b>(int_coord);
			cv::circle(*camera_image,
				cv::Point(image_coord[0], image_coord[1]),
				2,
				cv::Scalar(target_color[0] * 255,
					target_color[1] * 255,
					target_color[2] * 255),
				-1);
		}
	}

	const float section_width = 1.0 / 5;

	for (float t = 0; t < 1; t += 0.01) {
		Vec2f direction {cosf(t * 2 * M_PI), sinf(t * 2 * M_PI)};
		for (int circle = 0; circle < 4; circle++) {
			const double circle_radius = (circle + 1) * section_width;
			const Vec2f circle_point {direction * circle_radius};
			auto image_coord = model_projection.project(circle_point);
			cv::circle(*camera_image,
				cv::Point(image_coord[0], image_coord[1]),
				2,
				cv::Scalar(0, 255, 0),
				-1);
		}
	}

	imshow("opencv", *camera_image);
	cv::waitKey(0);
}

BOOST_AUTO_TEST_CASE(test_optimize_model, *disabled()) {
	auto camera_image = load_data();

	Target target = fit_target_model_to_image(camera_image);
	Camera camera{26, 10, {camera_image.cols / 2.0f, camera_image.rows / 2.0f}};
	ModelProjection model_projection{camera, target};

	show(&camera_image, model_projection);
}

BOOST_AUTO_TEST_CASE(test_sample_modelspace_in_imagespace, *disabled()) {
	Mat camera_image = load_data();

	Camera camera{26, 10, {camera_image.cols / 2.0f, camera_image.rows / 2.0f}};
	Target target_model{{20.0f, -20.0f, 300.0f},   // center
		{0.0f, 0.1f, 0.0f},						   // rotation
		120.0f};								   // target base
	ModelProjection model_projection{camera, target_model};
	SystemModel model{camera_image};
	std::cout << "model.value = " << model.value(target_model) << "\n";

	show(&camera_image, model_projection);
}

BOOST_AUTO_TEST_CASE(test_project_modelspace_to_imagespace) {
	auto camera_image = load_data();

	Target target_model{
		Vec3f{0.0f, 0.0f, 240.0f}, Vec3f{0.0f, 0.0f, 0.0f}, 120.0f};
	Camera camera{240, 1, {camera_image.cols / 2.0f, camera_image.rows / 2.0f}};
	ModelProjection model_projection{camera, target_model};
}

BOOST_AUTO_TEST_CASE(test_mat_convert) {
	const float float_val = 0.5;
	Mat m_f(1, 1, CV_32FC1);
	m_f.at<float>() = {float_val};
	auto mat_float_val = m_f.at<float>(0, 0);
	BOOST_CHECK_EQUAL(mat_float_val, float_val);

	Mat m_u8(1, 1, CV_8UC1);
	m_f.convertTo(m_u8, CV_8UC1, 255.0f, 0.0f);
	auto mat_u8_val = m_u8.at<uint8_t>(0, 0);
	BOOST_CHECK_EQUAL(
		mat_u8_val, cv::saturate_cast<uchar>(float_val * 255.0f + 0.0f));
	BOOST_CHECK_EQUAL(
		mat_u8_val, static_cast<uchar>(std::round(float_val * 255.0f + 0.0f)));
}

BOOST_AUTO_TEST_CASE(test_mat_casting) {
	Mat m_3f(1, 1, CV_32FC3);
	m_3f.at<Vec3f>() = {1, 0.2f, 3};
	auto val = m_3f.at<Vec3f>(0, 0);
	BOOST_CHECK_EQUAL(val[0], 1.0f);
	BOOST_CHECK_EQUAL(val[1], 0.2f);
	BOOST_CHECK_EQUAL(val[2], 3.0f);

	Mat m_3u8(1, 1, CV_8UC3);
	m_3f.convertTo(m_3u8, CV_8UC3, 255.0, 0);

	auto mat_3u8_val = m_3u8.at<Vec3b>(0, 0);
	Vec3b expected{255, 51, 255};
	BOOST_CHECK_EQUAL(mat_3u8_val[0], expected[0]);
	BOOST_CHECK_EQUAL(mat_3u8_val[1], expected[1]);
	BOOST_CHECK_EQUAL(mat_3u8_val[2], expected[2]);
}

/*
Fitting Parameterized 3-D Models to Images
http://www.ai.mit.edu/courses/6.899/papers/pami91.pdf

3D Parametric Models
https://cse291-i.github.io/WI18/LectureSlides/L10_Learning_for_parametric_models.pdf
*/
