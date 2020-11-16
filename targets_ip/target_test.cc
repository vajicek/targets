#include <iostream>
#include <vector>
#include <algorithm>
#include <string>

#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE TargetTest

#include "opt.h"
#include "preproc.h"
#include "target.h"
#include "utils.h"

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
		"/home/vajicek/src/targets/targets_ip/testdata/img0001.jpg",
		cv::IMREAD_COLOR);
	return img;
}

BOOST_AUTO_TEST_CASE(test_ordering_by_direction/*, *disabled()*/) {
	BOOST_CHECK(vector2Angle({1, -1}) > vector2Angle({1, 1}));			// 01:30
	BOOST_CHECK(vector2Angle({1, 1}) > vector2Angle({-1, 1}));			// 04:30
	BOOST_CHECK(vector2Angle({-1, 1}) > vector2Angle({-1, -1}));		// 07:30
	BOOST_CHECK(vector2Angle({1, -1}) > vector2Angle({-1, -1}));		// 10:30
}

BOOST_AUTO_TEST_CASE(test_interactive_threshold, *disabled()) {
	cv::namedWindow("opencv", 1);

	int threshold = 240;
	cv::createTrackbar("trackbar_thresh", "opencv", &threshold, 255, NULL, NULL);

	int smoothing = 3;
	cv::createTrackbar("smoothing", "opencv", &smoothing, 10, NULL, NULL);

	int dilate = 3;
	cv::createTrackbar("dilate", "opencv", &dilate, 5, NULL, NULL);

	int canny1 = 50;
	cv::createTrackbar("canny1", "opencv", &canny1, 1000, NULL, NULL);

	int canny2 = 200;
	cv::createTrackbar("canny2", "opencv", &canny2, 1000, NULL, NULL);

	int hough = 50;
	cv::createTrackbar("hough", "opencv", &hough, 300, NULL, NULL);

	TargetExtractorData data(cv::Size(512, 512), 512);

	loadAndPreprocessInput(&data, "/home/vajicek/src/targets/targets_ip/testdata/img0001.jpg");

	while (true) {
		extractTargetFace(&data, smoothing, dilate, threshold);
		detectArrows(&data, canny1, canny2, hough);

		showStack({&data.warped, &data.warped_edges, &data.lines_drawing}, 3, false);

		// showStack({&data.hsv[2], &data.smoothed, &data.thresholded,
		// 	&data.dilated, &data.curve_drawing, &data.poly_drawing,
		// 	&data.warped, &data.warped_edges, &data.lines_drawing}, 3, false);

		if (cv::waitKey(100) == 27) {
			break;
		}
	}
}

BOOST_AUTO_TEST_CASE(test_ip_pipeline, *disabled()) {
	auto img = load_data();

	cv::Mat imgResized;
	cv::resize(img, imgResized, getSizeKeepRatio(img, 0, 512));

	cv::Mat bgr[3];
	cv::split(imgResized, bgr);

	cv::Mat imgYUV;
	cv::cvtColor(imgResized, imgYUV, cv::COLOR_BGR2YUV);

	cv::Mat yuv[3];
	cv::split(imgYUV, yuv);

	cv::Mat imgEqualizedY;
	cv::equalizeHist(yuv[0], imgEqualizedY);

	showStack({&bgr[0], &bgr[1], &bgr[2],
		&yuv[0], &yuv[1], &yuv[2],
		&imgEqualizedY}, 3);

	cv::Mat imgEqualizedYUV;
	cv::Mat yuv_array[] = {imgEqualizedY, yuv[1], yuv[2]};
	cv::merge(yuv_array, 3, imgEqualizedYUV);

	cv::Mat equalized_bgr[3];
	cv::equalizeHist(bgr[0], equalized_bgr[0]);
	cv::equalizeHist(bgr[1], equalized_bgr[1]);
	cv::equalizeHist(bgr[2], equalized_bgr[2]);

	cv::Mat imgEqualizedBGR2;
	cv::Mat bgr_array[] = {equalized_bgr[0], equalized_bgr[1], equalized_bgr[2]};
	cv::merge(equalized_bgr, 3, imgEqualizedBGR2);

	cv::Mat imgEqualizedBGR;
	cv::cvtColor(imgEqualizedYUV, imgEqualizedBGR, cv::COLOR_YUV2BGR);

	showStack({&imgResized, &imgEqualizedBGR, &imgEqualizedBGR2}, 3);
}
