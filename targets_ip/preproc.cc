#include "preproc.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include <boost/format.hpp>

#include "utils.h"
#include "io.h"

using cv::Mat;
using cv::Size;
using std::vector;
using std::cerr;

Mat uniformResize(const Mat &image, int width) {
	int new_width = width;
	int new_height = width * image.size[0] / static_cast<double>(image.size[1]);

	cerr << "original size: " << image.size[0] << "," << image.size[1] << "\n";
	cerr << "new size: " << new_height << "," << new_width << "\n";

	Mat small_image;
	resize(image, small_image, Size(new_width, new_height));
	return small_image;
}

Mat edges(const Mat &image, double t1, double t2) {
	Mat edges;
	Canny(image, edges, t1, t2, 3, true);
	return edges;
}

/// output images filtered with differently configured edge detector
void benchmarkEdges(const Mat &image) {
	for(auto t1 : vector<double>{50, 150, 350, 400, 500, 600, 700}) {
		for(auto t2 : vector<double>{150, 250, 450, 500, 600, 700, 800}) {
			Mat edge_image = edges(image, t1, t2);
			imwrite(output(boost::str(boost::format("edges_%1%.png") % t2)), edge_image);
		}
	}
}
