#include <opencv2/core/core.hpp>

#include "utils.h"
#include "video.h"

int main(int argc, char** argv) {
	captureCameraImage("http://100.112.117.170:9999/video");
	captureCameraImage("/dev/video0");

	// Mat image = loadInput(TEST_IMAGE);
	// Mat small_image = uniformResize(image, 512);

	// benchmarkEdges(small_image);
	// dumpLinesToTable(benchmarkLines(small_image));
	// detectRectagle(small_image);

	return 0;
}
