// TestCore.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <stdio.h>
#include <Windows.h>
#include <filesystem>
#include "modelLoader/include/modelLoader.h"
#include "core/include/coreUtils.h"
#include "core/include/yoloOnnxInfer.h"

#include "actions_detector.h"
#include "geometry.h"

namespace plugin = nx::analytics::lcd_vision_solutions;

std::mutex m_thread_mutex;
plugin::ActionDetector* m_ActionDetector;
OnnxLoader* modelLoader;

void mainThread()
{
	printf(cv::getBuildInformation().c_str());

	fs::path video_dir = fs::current_path() / fs::path("test_video");

	fs::path inputVideo = video_dir / fs::path("metro.mp4");
	cv::VideoCapture cap;
	//cap = cv::VideoCapture(rtsp, cv::CAP_GSTREAMER);
	cap = cv::VideoCapture(inputVideo.string(), cv::CAP_ANY);

	fs::path save_video_dir = video_dir / fs::path("outputVideo.mp4");

	int width = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_WIDTH));
	int height = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_HEIGHT));
	int fps = cap.get(cv::CAP_PROP_FPS);

	float resizeCof = 1.0;

	cv::VideoWriter writer(save_video_dir.string(), cv::VideoWriter::fourcc('m', 'p', '4', 'v'),
		fps, cv::Size(width * resizeCof, height * resizeCof), true);

	// Set assumeFPS
	int assumeFPS = fps;
	int skip = fps / assumeFPS;

	/*cv::Mat frame;*/
	bool start = false;
	// Create a window
	static const std::string kWinName = "Test Core with Video";
	cv::namedWindow(kWinName);
	float confTh = 0.3;
	float nmsTh = 0.3;
	int index = 0;
	cv::Mat frame;

	// Tracking settings
	// int maxTrack = 30; // 30 Objects
	int maxTrackAge = 60; // 60 Frame

	// Init Tracker
	m_ActionDetector->initTracker(assumeFPS, maxTrackAge);
	// Set ROIs
	m_ActionDetector->setROIs({ "0$0$9999$0$9999$9999$0$9999" });

	while (1)
	{
		try
		{
			cap >> frame;
			if (frame.empty())
			{
				break;
			}

			if (index % skip == 0)
			{
				cv::resize(frame, frame, cv::Size(), resizeCof, resizeCof, cv::INTER_LINEAR);
				cv::Mat oriFrame = frame.clone();

				 m_thread_mutex.lock();

				auto start = std::chrono::high_resolution_clock::now();

				auto detections = m_ActionDetector->runTest(frame);

				auto end = std::chrono::high_resolution_clock::now();
				std::chrono::duration<double> inferenceTime = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

				 m_thread_mutex.unlock();

				 // Draw Prediction on Frame
				 m_ActionDetector->drawPrediction(oriFrame, detections);

				std::string inferenceTimeText = "Inference Time: " + std::to_string(inferenceTime.count()) + " ms";
				cv::putText(oriFrame, inferenceTimeText, cv::Point(10, 30), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 255, 0), 2);

				writer.write(oriFrame);
				cv::imshow(kWinName, oriFrame);
			}
			else
			{
				//Detector.getTrack(frame);
			}
			index++;
			//cv::imshow(kWinName, frame);
			char c = (char)cv::waitKey(10);
			if (c == 27)
			{
				break;
			}
			/*cap.release();*/
		}
		catch (const std::exception&)
		{
			writer.release();
			cap.release();
			printf("Error");
			cv::destroyAllWindows();
		}
	}
	writer.release();
	cap.release();
	printf("Done");
	cv::destroyAllWindows();
}

int main()
{
	modelLoader = new OnnxLoader();
	bool status;

	// Load Original model (comment it if use Encrypt model)
	OnnxLoader::Model m_Detmodel;
	fs::path m_DetmodelName = fs::current_path() / fs::path("coco.onnx");
	status = modelLoader->loadModel(m_DetmodelName, "cuda", "0", m_Detmodel);
	std::cout << "Load model " << m_DetmodelName.string() << ": " << status << std::endl;

	OnnxLoader::Model m_ClipVisualmodel;
	fs::path m_ClipVisualmodelName = fs::current_path() / fs::path("clip_visualL.onnx");
	status = modelLoader->loadModel(m_ClipVisualmodelName, "cuda", "0", m_ClipVisualmodel);
	std::cout << "Load model " << m_ClipVisualmodelName.string() << ": " << status << std::endl;

	//load text description from text (instead of DB)
	m_ActionDetector = new plugin::ActionDetector(fs::current_path(), m_Detmodel, m_ClipVisualmodel, false);

	// Run Thread
	std::thread t1(mainThread);
	t1.join();


	return 0;
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
