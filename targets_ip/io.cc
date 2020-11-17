#include "io.h"

#include <iostream>
#include <string>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include <boost/filesystem.hpp>

#include "utils.h"

namespace fs = boost::filesystem;

cv::Mat loadImage(const std::string &filename) {
	cv::Mat image = cv::imread(filename.c_str());
	if (!image.data) {
		std::cerr <<  "Could not open or find the image" << std::endl;
		abort();
	}
	return image;
}

void storeImage(const cv::Mat &mat, const std::string &filename) {
	cv::imwrite(filename, mat);
}

void drawLines(cv::Mat color_image, std::vector<cv::Vec2f> lines, std::string filename) {
	for (size_t i = 0; i < lines.size(); i++) {
		Line line1 = createLineFromSlope(lines[i]);
		cv::line(color_image,
			cv::Point(line1.a_), cv::Point(line1.b_),
			cv::Scalar(0, 0, 255), 3, cv::LINE_AA);
	}
	cv::imwrite(filename, color_image);
}
