#ifndef _VIDEO_H
#define _VIDEO_H
#pragma once

#include <string>
#include <functional>

#include <opencv2/core/core.hpp>

// Examples
//  captureCameraImage("http://100.112.117.170:9999/video");
//  captureCameraImage("/dev/video0");
void captureCameraImage(std::string source,
	cv::Mat *frame,
	std::function<void(const cv::Mat&)> fnc);

#endif
