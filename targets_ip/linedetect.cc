#include <iostream>
#include <string>
#include <vector>
#include <tuple>

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include <boost/format.hpp>

#include "utils.h"
#include "io.h"
#include "preproc.h"

using cv::Vec2f;
using cv::Mat;
using std::vector;
using std::tuple;
using std::endl;
using std::get;
using std::cout;

// Benchmark locate edges in the image. Combines canny and hough.
vector<Vec2f> houghLinesOnEdges(const Mat &image, double t1, double t2) {
	Mat edge_image = edges(image, t1, t2);
	vector<Vec2f> lines;
	HoughLines(edge_image, lines, 1, CV_PI/180, 100, 0, 0);

	Mat color_image;
	cvtColor(edge_image, color_image, cv::COLOR_GRAY2BGR);
	drawLines(color_image, lines, output(boost::str(boost::format("lines_%1%_%2%.png") % t1 % t2)));
	return lines;
}

vector<tuple<double, double, vector<Vec2f>>> benchmarkLines(const Mat &image) {
	vector<tuple<double, double, vector<Vec2f>>> retval;
	for(auto t1 : vector<double>{50, 150, 350, 400, 500, 600, 700}) {
		for(auto t2 : vector<double>{150, 250, 450, 500, 600, 700, 800}) {
			auto lines = houghLinesOnEdges(image, t1, t2);
			retval.push_back(tuple(t1, t2, lines));
		}
	}
	return retval;
}

void dumpLinesToTable(vector<tuple<double, double, vector<Vec2f>>> lines) {
	cout << "t1,t2,count,dist,ang" << endl;
	for (auto line_set : lines) {
		for (auto line : get<2>(line_set)) {
			cout << get<0>(line_set) << "," << get<1>(line_set) << ","
				<< lines.size() << "," << line.val[0] << "," << line.val[1] << endl;
		}
	}
}
