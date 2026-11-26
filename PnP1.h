
#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <opencv2/opencv.hpp>


std::vector<std::vector<cv::Point>> Findcontours(cv::Mat image);

std::vector<cv::RotatedRect> detector(cv::Mat image, std::vector<std::vector<cv::Point>> contours);

std::vector<cv::Point2f> detectPoints(cv::Mat image, std::vector<cv::RotatedRect> light_bars);

void putTextBg(cv::Mat& img, const std::string& s, cv::Point org,
    double fs = 0.6,
    cv::Scalar color = cv::Scalar(0, 255, 0),
    int thick = 1, int pad = 4);

void putText1(cv::Mat frame1, cv::Mat rvec, cv::Mat tvec);

void put_frame_id(cv::Mat frame1, double fps, int frame_id);

void put_valid(cv::Mat frame1, int detect_valid, int pnp_success);

void put_err1(cv::Mat frame1, std::vector<double> errs, double meanErr);

void putText2(cv::Mat frame1);

void put_err2(cv::Mat frame1);

void put_rad1(cv::Mat frame1, double rx_rad, double ry_rad, double rz_rad);

void put_rad2(cv::Mat frame1);
