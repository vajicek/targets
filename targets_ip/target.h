#ifndef _TARGET_H
#define _TARGET_H
#pragma once

#include <string>

#include <opencv2/core/core.hpp>

struct TargetExtractorData {
	cv::Mat img;
	cv::Mat img_resized;
	cv::Mat hsv[3];
	cv::Mat warped;
	cv::Mat warped_edges;
	cv::Mat lines_drawing;

	cv::Mat smoothed;
	cv::Mat thresholded;
	cv::Mat dilated;
	cv::Mat curve_drawing;
	cv::Mat poly_drawing;

	cv::Size target_size;
	int scaled_input_size;
	explicit TargetExtractorData(cv::Size target_size, int scaled_input_size)
		: target_size(target_size), scaled_input_size(scaled_input_size) {
	}
};

void loadAndPreprocessInput(TargetExtractorData *data,
	const std::string &filename);
void extractTargetFace(TargetExtractorData *data,
	int smoothing, int dilate, int threshold);
void detectArrows(TargetExtractorData *data,
	int canny1, int canny2, int hough);

#endif	 // TARGET_H
