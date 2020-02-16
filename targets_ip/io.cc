#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <iostream>
#include <string>

#include <boost/format.hpp>
#include <boost/filesystem.hpp>

#include "utils.h"

using namespace cv;
using namespace std;
namespace fs = boost::filesystem;

string TEST_IMAGE = "testdata/img0001.jpg";
string OUTPUT_DIR = "output/";

std::string output(std::string filename) {
	return (fs::path(OUTPUT_DIR) / filename).string();
}

Mat loadInput(const string &filename) {
	Mat image = imread(filename.c_str(), CV_LOAD_IMAGE_COLOR);
	if (!image.data) {
		cerr <<  "Could not open or find the image" << endl ;
		abort();
	}
	return image;
}

void drawLines(Mat color_image, vector<Vec2f> lines, string filename) {
	for(size_t i = 0; i < lines.size(); i++) {
		Line line1 = createLineFromSlope(lines[i]);
		line(color_image, Point(line1.a_), Point(line1.b_), Scalar(0,0,255), 3, CV_AA);
	}
	imwrite(filename, color_image);
}

void show() {
	//namedWindow("Display window", WINDOW_AUTOSIZE);
	//Mat edge_image = edges(small_image);
	//imshow("Display window", edge_image);
	//waitKey(0);
}
