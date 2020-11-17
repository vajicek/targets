#include "video.h"

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/videoio.hpp>

void captureCameraImage(std::string source,
	cv::Mat *frame,
	std::function<void(const cv::Mat&)> fnc) {
	cv::VideoCapture capture(source.c_str());
	while (capture.isOpened()) {
		capture.grab();
		capture.retrieve(*frame);
		if (!frame->empty()) {
			fnc(*frame);
		}
		if (cv::waitKey(100) == 27) {
			break;
		}
	}
}
