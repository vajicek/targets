#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <iostream>
#include <string>
#include <cstdlib>
#include <cmath>
#include <functional>

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
	for(auto t1: vector<double>{50, 150, 350, 400, 500, 600, 700}) {
		for(auto t2: vector<double>{150, 250, 450, 500, 600, 700, 800}) {
			Mat edge_image = edges(image, t1, t2);
			imwrite(output(boost::str(boost::format("edges_%1%.png") % t2)), edge_image);
		}
	}
}

struct RansacRectangleFromLines {
	vector<Vec2f> lines_;

	RansacRectangleFromLines(vector<Vec2f> lines): lines_(lines) {
	}

	vector<Vec2f> get_intersections(vector<int> lines4) {
		vector<Vec2f> intersections;
		for (int i = 0; i < lines4.size() - 1; i++) {
			for (int j = i + 1; j < lines4.size(); j++) {
				Vec2f intersection_point;
				Line a = createLineFromSlope(lines_[lines4[i]]);
				Line b = createLineFromSlope(lines_[lines4[j]]);
				float abs_cos_i_j = abs(cosAngleLines(a, b));
				if (abs_cos_i_j > cos(30.0 * M_PI / 180.0)) {
					// lines almost parallel
					continue;
				}

				if (intersection(a, b, intersection_point)){
					intersections.push_back(intersection_point);
				}
			}
		}
		return intersections;
	}

	bool is_rectangle(vector<int> lines4) {
		vector<Vec2f> intersections = get_intersections(lines4);
		if (intersections.size() != 4) {
			return false;
		}

		Vec2f sum;
		for (Vec2f intersection_point : intersections) {
			sum += intersection_point;
		}
		Vec2f center = sum.mul(1.0f / intersections.size());

		sort(intersections.begin(), intersections.end(),
			[center](const Vec2f &a, const Vec2f &b) -> bool {
				return atan2(a[0] - center[0], a[1] - center[1]) > atan2(b[0] - center[0], b[1] - center[1]);
			});


		float dist_mean = mean([&](int i){ return dist(center, intersections[i]); }, (size_t)intersections.size());
		float dist_std_dev = std_dev([&](int i){ return dist(center, intersections[i]); }, intersections.size());
		bool is_rect = dist_std_dev < 0.9 * dist_mean;

		for (Vec2f intersection_point : intersections) {
			cout << intersection_point << "(" << atan2(intersection_point[0], intersection_point[1]) << "), ";
		}
		cout << " " << is_rect << "\n";

		return is_rect;
	}

	void find_rectangle() {
		// attempts
		int attempts = lines_.size() * 100;
		for (int i = 0; i < attempts; i++) {
			//select four lines
			auto four = get_random_n_tuple(4, lines_.size());
			sort(four.begin(), four.end());
			if (is_rectangle(four)) {
				cout << four[0] << ", " << four[1] << ", " << four[2] << ", " << four[3] << "\n";
			}
		}
	}
};

bool detectRectagle(const Mat &image) {
	Mat edge_image = edges(image, 500, 600);
	vector<Vec2f> lines;
	HoughLines(edge_image, lines, 1, CV_PI/180, 100, 0, 0);

	if (lines.size() < 4) {
		return false;
	}

	Line a = createLineFromSlope(lines[0]);
	Line b = createLineFromSlope(lines[1]);

	line(image, Point(a.a_), Point(a.b_), Scalar(0,0,255), 3, CV_AA);
	line(image, Point(b.a_), Point(b.b_), Scalar(0,0,255), 3, CV_AA);

	Vec2f intersection_point;
	intersection(a, b, intersection_point);
	cout << intersection_point << "\n";

	circle(image, Point(intersection_point[0], intersection_point[1]), 10, Scalar(255, 0, 0), -1);

	imwrite(output("points.png"), image);

	//RansacRectangleFromLines ransacRectangleFromLines(lines);
	//ransacRectangleFromLines.find_rectangle();

	//auto lines4 = get_random_n_tuple(4, lines.size());
	//vector<Vec2f> intersections = ransacRectangleFromLines.get_intersections(lines4);

	//for(Vec2f a: intersections) {
	//	cout << a << "\n";
	//	circle(image, Point(a[1], a[1]), 10, Scalar( 255, 0, 0 ), -1);
	//}

	return true;
}

void drawLines(Mat color_image, vector<Vec2f> lines, string filename) {
	for(size_t i = 0; i < lines.size(); i++) {
		Line line1 = createLineFromSlope(lines[i]);
		line(color_image, Point(line1.a_), Point(line1.b_), Scalar(0,0,255), 3, CV_AA);
	}
	imwrite(filename, color_image);
}

// Benchmark locate edges in the image. Combines canny and hough.
vector<Vec2f> houghLinesOnEdges(const Mat &image, double t1, double t2) {
	Mat edge_image = edges(image, t1, t2);
	vector<Vec2f> lines;
	HoughLines(edge_image, lines, 1, CV_PI/180, 100, 0, 0);

	Mat color_image;
	cvtColor(edge_image, color_image, COLOR_GRAY2BGR);
	drawLines(color_image, lines, output(boost::str(boost::format("lines_%1%_%2%.png") % t1 % t2)));
	return lines;
}

vector<tuple<double, double, vector<Vec2f>>> benchmarkLines(const Mat &image) {
	vector<tuple<double, double, vector<Vec2f>>> retval;
	for(auto t1: vector<double>{50, 150, 350, 400, 500, 600, 700}) {
		for(auto t2: vector<double>{150, 250, 450, 500, 600, 700, 800}) {
			auto lines = houghLinesOnEdges(image, t1, t2);
			retval.push_back(tuple(t1, t2, lines));
		}
	}
	return retval;
}

void dumpLinesToTable(vector<tuple<double, double, vector<Vec2f>>> lines) {
	cout << "t1,t2,count,dist,ang" << endl;
	for (auto line_set: lines) {
		for (auto line : get<2>(line_set)) {
			cout << get<0>(line_set) << "," << get<1>(line_set) << "," << lines.size() << "," << line.val[0] << "," << line.val[1] << endl;
		}
	}
}

void show() {
	//namedWindow("Display window", WINDOW_AUTOSIZE);
	//Mat edge_image = edges(small_image);
	//imshow("Display window", edge_image);
	//waitKey(0);
}

int main(int argc, char** argv) {
	Mat image = loadInput(TEST_IMAGE);
	Mat small_image = uniformResize(image, 512);

	benchmarkEdges(small_image);
	dumpLinesToTable(benchmarkLines(small_image));
	//detectRectagle(small_image);

	return 0;
}
