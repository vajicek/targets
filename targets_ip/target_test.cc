#include <iostream>
#include <vector>
#include <algorithm>

#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE TargetTest

#include "utils.h"
#include "opt.h"
#include "target.h"
#include "preproc.h"
#include <boost/test/unit_test.hpp>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc.hpp>

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

BOOST_AUTO_TEST_CASE(test_blur_input, *disabled()) {
	auto camera_image = load_data();
	//cv::blur(camera_image, camera_image, cv::Size(8, 8));

	cv::Mat a = edges(camera_image, 500, 550);
	cv::blur(a, a, cv::Size(8, 8));

	//cv::imshow("opencv", camera_image);
	cv::imshow("opencv", a);
	cv::waitKey(0);
}

BOOST_AUTO_TEST_CASE(test_masking/*, *disabled()*/) {
	auto img = load_data();

	showStack({&img, &img, &img}, 2);

	// cv::Mat res;
	// cv::hconcat(camera_image, camera_image, res);

	// cv::imshow("opencv", res);
	// cv::waitKey(0);
}
