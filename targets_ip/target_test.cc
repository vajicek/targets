#include <iostream>

#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE UtilsTest

#include "target.h"

#include <boost/test/unit_test.hpp>
#include <opencv2/highgui/highgui.hpp>

using cv::Vec2f;
using cv::Vec2i;
using cv::Vec3f;
using cv::Vec3b;

void sample(const Target &target,
		const Vec2f &start,
		const Vec2f &step,
		const Vec2i &count) {
	cv::Mat m(count[1], count[0], CV_32FC3);
	for (int y = 0; y < count[1]; y++) {
		for (int x = 0; x < count[0]; x++) {
			Vec2f coord { static_cast<float>(x), static_cast<float>(y) };
			Vec2f p { start + coord.mul(step) };
			Vec3f dir = cv::normalize(Vec3f { p[0], p[1], 120 });
			auto color = target.cast_ray_color({ 0, 0, 0 }, dir);
			m.at<Vec3f>(y, x) = color.value_or(Vec3f { 0.1f, 0.0f, 0.1f });
		}
	}
	cv::Mat dst;
	m.convertTo(dst, CV_8UC3, 255.0, +128);
	imshow("opencv", dst);
	cv::waitKey(0);
}

BOOST_AUTO_TEST_CASE(test_target) {
	Target target {
		{0.0f, 0.0f, 120.0f},  // center
		{0.5f, 0.0f, -1.0f},  // normal
		{0.0f, 1.0f, 0.0f},  // up
		120.0f};  // target base

	sample(target, Vec2f(-128, -128), Vec2f(1, 1), Vec2i(256, 256));

	// auto color = target.cast_ray({0, 0, 0}, {0.1, 0.1, 1});

	// std::cout << color.value() << "\n";

	// Vec2f r;
	// BOOST_CHECK(intersection(Line(Vec2f(0, 0), Vec2f(1, 0)),
	// 	Line(Vec2f(0, 0), Vec2f(0, 1)), &r));
	// BOOST_CHECK_SMALL(dist(r, Vec2f(0, 0)), 0.0001f);
	// BOOST_CHECK(intersection(Line(Vec2f(444, 1025), Vec2f(549, -973)),
	// 	Line(Vec2f(0, 0), Vec2f(0, 1)), &r));
	// BOOST_CHECK_SMALL(dist(r, Vec2f(0, 0)), 0.0001f);
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
