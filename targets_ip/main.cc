#include <iostream>
#include <map>
#include <functional>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>

#include <boost/program_options.hpp>

#include "io.h"
#include "target.h"
#include "utils.h"
#include "video.h"

namespace po = boost::program_options;

enum class Action {
	NONE,
	HELP,
	EXTRACT_TARGET,
	STREAM
};

struct Operations {
	std::string input_file;
	std::string output_file;
	Action action = Action::NONE;
};

Action stringToAction(std::string action_str) {
	if (action_str == "target") {
		return Action::EXTRACT_TARGET;
	} else if (action_str == "stream") {
		return Action::STREAM;
	}
	return Action::NONE;
}

void configureOperations(
	Operations *operations, const po::variables_map &variables_map) {
	if (variables_map.count("input")) {
		operations->input_file = variables_map["input"].as<std::string>();
	}

	if (variables_map.count("output")) {
		operations->output_file = variables_map["output"].as<std::string>();
	}

	if (variables_map.count("action")) {
		operations->action = stringToAction(variables_map["action"].as<std::string>());
	}

	if (variables_map.count("help")) {
		operations->action == Action::HELP;
	}
}

void setupOperationsFromArguments(Operations* operations, int argc, char** argv) {
	po::options_description options_description("Allowed options");
	options_description.add_options()
		("help", "produce help message")
		("action", po::value<std::string>(), "set action")
		("output", po::value<std::string>(), "set output file")
		("input", po::value<std::string>(), "set input file");

	auto parsed_options = po::parse_command_line(argc, argv, options_description);

	po::variables_map variables_map;
	po::store(parsed_options, variables_map);
	po::notify(variables_map);

	configureOperations(operations, variables_map);

	if (operations->action == Action::NONE || operations->action == Action::HELP) {
		std::cout << options_description << "\n";
		exit(0);
	}
}

void extractTarget(Operations* operations) {
	const cv::Size target_size(256, 256);
	const int scaled_input_size = 256;

	const int smoothing = 3;
	const int dilate = 3;
	const int threshold = 240;

	TargetExtractorData data(target_size, scaled_input_size);
	loadAndPreprocessInput(&data, operations->input_file);
	extractTargetFace(&data, smoothing, dilate, threshold);
	storeImage(data.warped, operations->output_file);
}

void stream(Operations* operations) {
	const cv::Size target_size(256, 256);
	const int scaled_input_size = 256;
	TargetExtractorData data(target_size, scaled_input_size);

	captureCameraImage("/dev/video0", &data.img,
		[&data](const cv::Mat &frame) {
			const int smoothing = 3;
			const int dilate = 3;
			const int threshold = 200;

			preprocessInput(&data);
			extractTargetFace(&data, smoothing, dilate, threshold);

			if (data.poly.size() == 4) {
				cv::imshow("opencv", data.warped);
			} else {
				showStack({&data.hsv[2],
					&data.smoothed,
					&data.thresholded,
					&data.dilated,
					&data.curve_drawing,
					&data.poly_drawing}, 3, false);
			}
		});
}

void runOperations(Operations *operations) {
	std::map<Action, std::function<void(Operations*)>> actions_map {
		{Action::EXTRACT_TARGET, extractTarget},
		{Action::STREAM, stream}
	};
	actions_map[operations->action](operations);
}

int main(int argc, char** argv) {
	Operations operations;
	setupOperationsFromArguments(&operations, argc, argv);
	runOperations(&operations);
	return 0;
}
