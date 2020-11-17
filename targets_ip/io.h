#ifndef _IO_H
#define _IO_H
#pragma once

#include <vector>
#include <string>
#include <opencv2/core/core.hpp>

#include "utils.h"

void drawLines(cv::Mat color_image, std::vector<cv::Vec2f> lines, std::string filename);
cv::Mat loadImage(const std::string &filename);
void storeImage(const cv::Mat &mat, const std::string &filename);

#endif
