#ifndef _UTILS_H
#define _UTILS_H
#pragma once

#include <vector>

#include <opencv2/core/mat.hpp>

std::vector<int> get_random_n_tuple(int count, int max);
float dist(const cv::Vec2f &a, const cv::Vec2f &b);

struct Line {
	cv::Vec2f a_, b_;
	cv::Vec2f dir() { return a_ - b_; }
	cv::Vec2f norm() { return cv::Vec2f(a_[1] - b_[1], b_[0] - a_[0]); }
	Line(cv::Vec2f a, cv::Vec2f b): a_(a), b_(b) {
	}
};

Line createLineFromSlope(cv::Vec2f slope_line);

float cosAngleLines(Line a, Line b);

bool intersection(Line a, Line b, cv::Vec2f *r);

template<typename G>
auto mean(G element_getter, size_t len) {
	auto sum = 0;
	for (int i = 0; i < len; i++) {
		sum += element_getter(i);
	}
	return sum / len;
}

template<typename G>
auto std_dev(G element_getter, size_t len) {
	auto mean_val = mean(element_getter, len);
	auto var_val = mean([&](int i) { return element_getter(i) - mean_val; }, len);
	return sqrt(var_val);
}

void sameAs(cv::Mat *target, const cv::Mat &source);

void zeroSameAs(cv::Mat *target, const cv::Mat &source);

void showStack(std::vector<cv::Mat*> input_images, size_t cols, bool wait = true);

cv::Size2i getSizeKeepRatio(const cv::Mat &s, const int width, const int height);

void drawLineByAngleOffset(cv::Mat* m, float angle, float shift);

#endif
