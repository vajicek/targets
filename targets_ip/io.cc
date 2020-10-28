#include "io.h"

#include <iostream>
#include <string>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include <boost/format.hpp>
#include <boost/filesystem.hpp>

#include "utils.h"

using cv::Mat;
using cv::Point;
using cv::Scalar;
using cv::imread;
using cv::line;
using cv::Vec2f;
using std::vector;
using std::string;
using std::endl;
using std::cerr;
namespace fs = boost::filesystem;

const char* TEST_IMAGE = "testdata/img0001.jpg";
const char* OUTPUT_DIR = "output/";

string output(string filename) {
	return (fs::path(OUTPUT_DIR) / filename).string();
}

Mat loadInput(const string &filename) {
	Mat image = imread(filename.c_str());
	if (!image.data) {
		cerr <<  "Could not open or find the image" << endl;
		abort();
	}
	return image;
}

void drawLines(Mat color_image, vector<Vec2f> lines, string filename) {
	for(size_t i = 0; i < lines.size(); i++) {
		Line line1 = createLineFromSlope(lines[i]);
		line(color_image, Point(line1.a_), Point(line1.b_), Scalar(0, 0, 255), 3, cv::LINE_AA);
	}
	imwrite(filename, color_image);
}

void show() {
	// namedWindow("Display window", WINDOW_AUTOSIZE);
	// Mat edge_image = edges(small_image);
	// imshow("Display window", edge_image);
	// waitKey(0);
}
