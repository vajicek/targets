#include <utils.h>
#include <iostream>

#include <opencv2/highgui/highgui.hpp>

using cv::Vec2f;
using cv::Point2f;
using cv::Point;
using std::vector;

/*
bool intersection(Line a, Line b, Vec2f &r) {
	Vec2f a_dir = a.norm();
	Vec2f b_dir = b.norm();
	float cross = Point2f(a_dir).cross(Point2f(b_dir));
	if (abs(cross) < 1e-8) {
		return false;
	}
	float t1 = Point2f(a.a_ - b.a_).dot(Point2f(b.norm())) / cross;
	r = a.a_ + a_dir * t1;
	return true;
}
*/

bool intersection(Line a, Line b, Vec2f *r) {
	Vec2f a_norm = a.norm();
	float c1 = a_norm.dot(a.a_);

	Vec2f b_norm = b.norm();
	float c2 = b_norm.dot(b.a_);

	float cross = Point2f(a_norm).cross(Point2f(b_norm));
	if (abs(cross) < 1e-8) {
		return false;
	}
	*r = Vec2f(b_norm[1] * c1 - a_norm[1] * c2, a_norm[0] * c2 - b_norm[0] * c1) / cross;
	return true;
}


// O(n^2)
vector<int> get_random_n_tuple(int count, int max) {
	static unsigned int seed = 123;
	vector<int> result;
	while(result.size() < count) {
		int num = rand_r(&seed) % max;
		if (find(result.begin(), result.end(), num) == result.end()) {
			result.push_back(num);
		}
	}
	return result;
}

float dist(const Vec2f &a, const Vec2f &b) {
	Vec2f diff = a - b;
	return sqrt(diff.dot(diff));
}

Line createLineFromSlope(cv::Vec2f slope_line) {
	float rho = slope_line[0], theta = slope_line[1];
	Point pt1, pt2;
	double a = cos(theta), b = sin(theta);
	double x0 = a*rho, y0 = b*rho;
	return Line(Vec2f(cvRound(x0 + 1000*(-b)), cvRound(y0 + 1000*(a))),
		Vec2f(cvRound(x0 - 1000*(-b)), cvRound(y0 - 1000*(a))));
}

float cosAngleLines(Line a, Line b) {
	Point2f d1 = Point2f(normalize(a.a_ - a.b_));
	return Point2f(normalize(b.a_ - b.b_)).dot(d1);
}

void sameAs(cv::Mat *target, const cv::Mat &source) {
	target->create(source.rows, source.cols, source.type());
}

void zeroSameAs(cv::Mat *target, const cv::Mat &source) {
	sameAs(target, source);
	(*target) *= 0;
}

void showStack(std::vector<cv::Mat*> input_images, size_t cols) {
	int rows = (input_images.size() + cols - 1) / cols;

	std::vector<cv::Mat> row_mats;
	for (size_t i = 0; i < rows; i++) {
		std::vector<cv::Mat> col_mats;
		for(size_t j = 0; j < cols; j++) {
			auto index = i * cols + j;
			if (index >= input_images.size()) {
				cv::Mat mat;
				sameAs(&mat, *input_images[0]);
				col_mats.push_back(mat);
			} else {
				col_mats.push_back(*input_images[index]);
			}
		}
		cv::Mat row_mat;
		cv::hconcat(col_mats.data(), col_mats.size(), row_mat);
		row_mats.push_back(row_mat);
	}
	cv::Mat result;
	cv::vconcat(row_mats.data(), row_mats.size(), result);

	cv::imshow("opencv", result);
	cv::waitKey(0);
}
