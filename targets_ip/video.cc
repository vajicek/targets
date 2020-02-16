#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/videoio.hpp>
#include <iostream>

#include "utils.h"
#include "video.h"

using namespace cv;
using namespace std;

// Examples
//  captureCameraImage("http://100.112.117.170:9999/video");
//  captureCameraImage("/dev/video0");
void captureCameraImage(string source) {
	VideoCapture capture(source.c_str());
	int i = 0;

	//namedWindow("Display window", WINDOW_AUTOSIZE);
	while (capture.isOpened()) {
		Mat edge_image;
		Mat frame;
		cout << "capture.read" << endl;
		cout << "frame " << i << endl;
		capture.grab();
		capture.retrieve(frame);
		cout << "fps: " << capture.get(CV_CAP_PROP_FPS) << endl;
		//auto ret = capture.read(frame);
		if (frame.empty()) {
			cout << "frame.empty" << endl;
		} else {
			cout << "call imshow" << endl;
			Canny(frame, edge_image, 50, 200, 3, false);
			//imshow("frame", frame);
			imshow("edges", edge_image);
		}
		if (waitKey(25) & 0xFF == int('q')) {
			break;
		}
		i++;
	}
}
