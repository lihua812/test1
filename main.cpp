#include <iostream>
#include <opencv2/opencv.hpp>
#include <vector>
#include <cmath>

int main() {
    cv::Mat image = cv::imread("image/outpost_frame.png");
    if (image.empty()) {
        std::cout << "image read mistake\n";
        return 1;
    }

    
    cv::Mat blurred, hsv, mask;
    cv::GaussianBlur(image, blurred, cv::Size(3, 3), 0);
    cv::cvtColor(blurred, hsv, cv::COLOR_BGR2HSV);

    const cv::Scalar lower(10, 140, 150);
    const cv::Scalar upper(40, 200, 255);
    cv::inRange(hsv, lower, upper, mask);

    
    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));
    cv::Mat opened;
    cv::morphologyEx(mask, opened, cv::MORPH_CLOSE, kernel);

    cv::Mat kernel2 = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(5, 5));
    cv::Mat opened2;
    cv::morphologyEx(opened, opened2, cv::MORPH_DILATE, kernel2);

    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(opened2, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    cv::Mat image2 = image.clone();

    
    std::vector<cv::RotatedRect> light_bars;

    for (const auto& contour : contours) {
        double area = cv::contourArea(contour);
        if (area < 100) continue; 

        cv::RotatedRect rotated = cv::minAreaRect(contour);

      
        
        if (rotated.size.width > rotated.size.height) {
            std::swap(rotated.size.width, rotated.size.height);
            rotated.angle += 90.0f;
        }

        const float bar_len = rotated.size.height;  
        const float bar_w = rotated.size.width;    

        
        if (bar_len < 10.0f) continue;
        if (bar_len / bar_w < 2.0f) continue;
        if (bar_w < 3.0f) continue;

        
        light_bars.push_back(rotated);
        
        cv::Point2f corners[4];
        rotated.points(corners);
        for(int i=0; i<4; i++)
            cv::line(image2, corners[i], corners[(i+1)%4], cv::Scalar(0, 0, 255), 2);
        
    }

    for (size_t i = 0; i < light_bars.size(); ++i) {
        for (size_t j = i + 1; j < light_bars.size(); ++j) {
            cv::RotatedRect bar1 = light_bars[i];
            cv::RotatedRect bar2 = light_bars[j];

            
            if (bar1.center.x > bar2.center.x) std::swap(bar1, bar2);

            float len1 = bar1.size.height;
            float len2 = bar2.size.height;
            float dx = std::abs(bar2.center.x - bar1.center.x); 
            float dy = std::abs(bar2.center.y - bar1.center.y); 

          
            if (dy > (len1 + len2) / 2.0f * 0.3f) continue;
            if (std::abs(len1 - len2) > (len1 + len2) / 2.0f * 0.4f) continue;
            if (dx < len1 * 0.8f || dx > len1 * 3.0f) continue;

            cv::Point2f pts1[4], pts2[4];
            bar1.points(pts1);
            bar2.points(pts2);

            std::vector<cv::Point2f> all_points;
            for (int k = 0; k < 4; k++) {
                all_points.push_back(pts1[k]);
                all_points.push_back(pts2[k]);
            }

           
            cv::RotatedRect armor_rect = cv::minAreaRect(all_points);

            cv::Point2f armor_pts[4];
            armor_rect.points(armor_pts);

          
            for (int k = 0; k < 4; k++) {
                cv::line(image2, armor_pts[k], armor_pts[(k + 1) % 4], cv::Scalar(0, 255, 0), 2);
            }

            
          
        }
    }

    cv::imshow("2", opened2);
    cv::imshow("1", image2);
    cv::waitKey(0);
    return 0;
}