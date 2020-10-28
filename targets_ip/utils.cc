#include <utils.h>
#include <iostream>

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
