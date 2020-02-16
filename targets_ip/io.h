#ifndef _IO_H
#define _IO_H
#pragma once

#include <opencv2/core/core.hpp>
#include <vector>

#include "utils.h"

void drawLines(cv::Mat color_image, std::vector<cv::Vec2f> lines, std::string filename);
std::string output(std::string filename);

#endif