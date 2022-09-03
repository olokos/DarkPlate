#pragma once
#include <DarkHelp.hpp>

void  __declspec(dllexport) draw_label(const std::string& txt, cv::Mat& mat, const cv::Point& tl, const double factor = 1.0);

/// This is the 2nd stage detection.  By the time this is called, we have a smaller Roi, we no longer have the full frame.
void __declspec(dllexport) process_plate(DarkHelp::NN& nn, cv::Mat& plate, cv::Mat& output);

/** Process a single license plate located within the given prediction. This means we build a RoI and apply it the rectangle to both the frame and the output image.*/
void __declspec(dllexport) process_plate(DarkHelp::NN& nn, cv::Mat& frame, const DarkHelp::PredictionResult& prediction, cv::Mat& output_frame);

cv::Mat __declspec(dllexport)  process_frame(DarkHelp::NN& nn, cv::Mat& frame);

void __declspec(dllexport) process_File(DarkHelp::NN& nn, const std::string& filename);

int __declspec(dllexport) main(int argc, char* argv[]);

std::vector<std::string> __declspec(dllexport) getListOfRecognitionsFromVideo(std::string & filenameToProcess);
