#include "target.h"

#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc.hpp>

#include "utils.h"

void loadAndPreprocessInput(TargetExtractorData *data,
	const std::string &filename) {
	data->img = imread(filename, cv::IMREAD_COLOR);

	cv::resize(data->img, data->img_resized,
		getSizeKeepRatio(data->img, 0, data->scaled_input_size));

	cv::Mat imgHSV;
	cv::cvtColor(data->img_resized, imgHSV, cv::COLOR_BGR2HSV);

	cv::split(imgHSV, data->hsv);
}

double vector2Angle(cv::Point2f a) { return atan2(a.x, a.y); }

double pointCenter2Angle(cv::Point2f a, cv::Point2f center) {
	return atan2(a.x - center.x, a.y - center.y);
}

void warpPolygonToSquare(TargetExtractorData *data) {
	if (data->poly.size() != 4) {
		return;
	}
	std::vector<cv::Point2f> source{
		cv::Point2f{data->poly[0]}, cv::Point2f{data->poly[1]},
		cv::Point2f{data->poly[2]}, cv::Point2f{data->poly[3]}};

	cv::Point2f center = (source[0] + source[1] + source[2] + source[3]) / 4;

	std::sort(std::begin(source), std::end(source), [center](auto a, auto b) {
		return pointCenter2Angle(a, center) > pointCenter2Angle(b, center);
	});

	cv::Mat warp_mat = cv::getPerspectiveTransform(source,
		std::vector<cv::Point2f>{
			cv::Point2i{data->target_size.width, 0},
			cv::Point2i{data->target_size.width, data->target_size.height},
			cv::Point2i{0, data->target_size.height},
			cv::Point2i{0, 0}});
	cv::warpPerspective(data->hsv[2], data->warped, warp_mat, data->target_size,
		cv::INTER_LINEAR, cv::BORDER_CONSTANT, cv::Scalar());
}

void extractTargetFace(TargetExtractorData *data,
	int smoothing, int dilate, int threshold) {
	const cv::Mat *preprocessed = &data->hsv[2];

	if (smoothing) {
		cv::blur(data->hsv[2], data->smoothed, cv::Size(smoothing, smoothing));
		preprocessed = &data->smoothed;
	}

	cv::threshold(*preprocessed, data->thresholded, threshold, 255, cv::THRESH_BINARY);
	preprocessed = &data->thresholded;

	if (dilate) {
		cv::dilate(*preprocessed, data->dilated,
			cv::getStructuringElement(cv::MORPH_RECT, cv::Size(dilate, dilate)));
		preprocessed = &data->dilated;
	}

	std::vector<std::vector<cv::Point>> contours;
	cv::findContours(*preprocessed, contours, cv::RETR_LIST, cv::CHAIN_APPROX_NONE);

	std::sort(std::begin(contours), std::end(contours),
		[](auto a, auto b) { return cv::contourArea(a) > cv::contourArea(b); });

	zeroSameAs(&data->curve_drawing, data->thresholded);
	cv::drawContours(data->curve_drawing, contours, 0, cv::Scalar(255, 255, 255));

	cv::approxPolyDP(contours[0], data->poly, 30.0, true);

	zeroSameAs(&data->poly_drawing, data->thresholded);
	cv::polylines(data->poly_drawing, {data->poly}, true, cv::Scalar(255, 255, 255));

	warpPolygonToSquare(data);
}

void detectArrows(TargetExtractorData *data,
	int canny1, int canny2, int hough) {
	cv::Canny(data->warped, data->warped_edges, canny1, canny2, 3);

	std::vector<cv::Vec4i> lines;
	cv::HoughLinesP(data->warped_edges, lines, 1, 0.01, hough, 30, 10);

	zeroSameAs(&data->lines_drawing, data->warped);
	for (size_t i = 0; i < lines.size(); i++) {
		cv::line(data->lines_drawing,
			cv::Point(lines[i][0], lines[i][1]),
			cv::Point(lines[i][2], lines[i][3]),
			cv::Scalar(255, 255, 255), 1, 8);
	}
}
