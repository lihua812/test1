#include "PnP1.h"

#include <cmath>

//0.得到处理的图
std::vector<std::vector<cv::Point>> Findcontours(cv::Mat image) {
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

    return contours;
}

//1.提取装甲板
std::vector<cv::RotatedRect> detector(cv::Mat image, std::vector<std::vector<cv::Point>>contours) {
    std::vector<cv::RotatedRect> light_bars;
    cv::Mat image2 = image.clone();

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

        /*cv::Point2f corners[4];
        rotated.points(corners);
        for (int i = 0; i < 4; i++)
            cv::line(image2, corners[i], corners[(i + 1) % 4], cv::Scalar(0, 0, 255), 2);*/
        //cv::imshow("video", image2);//是 这样好，还是返回一个image2，在main函数里imshow？？
    }

    return light_bars;

}

//2.提取角点
std::vector<cv::Point2f> detectPoints(cv::Mat image,std::vector<cv::RotatedRect>light_bars){
    std::vector<cv::Point2f>imagePoints;
    bool found = false;   // 标记是否已找到一组合法装甲板，保证 imagePoints 恰好 4 个点
     for (size_t i = 0; i < light_bars.size() && !found; ++i) {
         for (size_t j = i + 1; j < light_bars.size(); ++j) {
             //筛选装甲板灯条
             cv::RotatedRect bar1 = light_bars[i];
             cv::RotatedRect bar2 = light_bars[j];


             if (bar1.center.x > bar2.center.x) std::swap(bar1, bar2);

             float len1 = bar1.size.height;
             float len2 = bar2.size.height;
             float dx = std::abs(bar2.center.x - bar1.center.x);
             float dy = std::abs(bar2.center.y - bar1.center.y);


             
             const float avgLen = (len1 + len2) / 2.0f;

             
             if (dy > avgLen * 0.45f) continue;
             if (std::abs(len1 - len2) > avgLen * 0.6f) continue;
             if (dx < avgLen * 0.6f || dx > avgLen * 4.0f) continue;

             cv::Point2f pts1[4], pts2[4];
             bar1.points(pts1);
             bar2.points(pts2);

             std::vector<cv::Point2f> all_points;
             for (int k = 0; k < 4; k++) {
                 all_points.push_back(pts1[k]);
                 all_points.push_back(pts2[k]);
             }


             cv::RotatedRect armor_rect = cv::minAreaRect(all_points);
             //排序得到角点
             cv::Point2f armor_pts[4];
             armor_rect.points(armor_pts);
             
             cv::Point2f imagePoint1, imagePoint2, imagePoint3, imagePoint4;
             double sumx=0,sumy=0;
         
             for (int i = 0;i < 4;i++) {
                 sumx += armor_pts[i].x;
                 sumy += armor_pts[i].y;
             }
             double pointCenterx = sumx / 4;
             double pointCentery = sumy / 4;

             for (int i = 0;i < 4;i++) {
                 if (armor_pts[i].x < pointCenterx && armor_pts[i].y < pointCentery) { 
                     imagePoint1.x = armor_pts[i].x;
                     imagePoint1.y = armor_pts[i].y;
                 }
                 if (armor_pts[i].x > pointCenterx && armor_pts[i].y < pointCentery) {
                     imagePoint2.x = armor_pts[i].x;
                     imagePoint2.y = armor_pts[i].y;
                 }
                 if (armor_pts[i].x > pointCenterx && armor_pts[i].y > pointCentery) {
                     imagePoint3.x = armor_pts[i].x;
                     imagePoint3.y = armor_pts[i].y;
                 }
                 if (armor_pts[i].x < pointCenterx && armor_pts[i].y > pointCentery) {
                     imagePoint4.x = armor_pts[i].x;
                     imagePoint4.y = armor_pts[i].y;
                 }

             }
             imagePoints.push_back(imagePoint1);
             imagePoints.push_back(imagePoint2);
             imagePoints.push_back(imagePoint3);
             imagePoints.push_back(imagePoint4);

             found = true;   // 一帧只取第一组合法装甲板，保证 imagePoints 恰为 4 个点
             break;          // 跳出内层循环；外层的 && !found 让外层也随之结束

         }
     }
     return imagePoints;
}

//3.标注的一个简单函数
void putTextBg(cv::Mat& img, const std::string& s, cv::Point org,
    double fs,
    cv::Scalar color,
    int thick, int pad)
{
    int baseline = 0;
    cv::Size ts = cv::getTextSize(s, cv::FONT_HERSHEY_SIMPLEX, fs, thick, &baseline);//【此函数返回值为size.width,size.height,】
    //s文字，FONT  字体（不用管），fs字号缩放，thick字体线宽，基线到最底部笔画的距离
    cv::rectangle(img,
        cv::Point(org.x, org.y - ts.height - pad),//给定的org ，左上角
        cv::Point(org.x + ts.width + pad, org.y + baseline),//右上角
        cv::Scalar(0, 0, 0), cv::FILLED);
    cv::putText(img, s, cv::Point(org.x + pad / 2, org.y),
        cv::FONT_HERSHEY_SIMPLEX, fs, color, thick, cv::LINE_AA);
}

//4.3当pnp_success=1标注tvec和distance（单位对吗）（改）
void putText1(cv::Mat frame1,cv::Mat rvec, cv::Mat tvec) {

    //求distance
    double distance;
    distance = sqrt(tvec.at <double>(0) * tvec.at<double>(0) + tvec.at<double>(1) * tvec.at<double>(1) + tvec.at<double>(2) * tvec.at<double>(2));
    
    //（1）存入line里
    char buf[200];
    std::vector<std::string>lines;
    std::snprintf(buf, sizeof(buf), "tvec:%.4f %.4f %.4f", tvec.at<double>(0), tvec.at<double>(1), tvec.at<double>(2));
    lines.emplace_back(buf);

    std::snprintf(buf, sizeof(buf), "distance:%lf",distance);
    lines.emplace_back(buf);

   
    //（2）打印（确定位置）
    int x0 = 20, y0 = 34, lineH = 28;
    for (size_t i = 0;i < lines.size();++i) {
        putTextBg(frame1, lines[i], cv::Point(x0, y0 + (int)i * lineH));
    }

}
//4.1标注时间与帧数(时间对不对呢）
void put_frame_id(cv::Mat frame1,double fps,int frame_id) {
    char buf[120];
    std::vector<std::string>lines;
    double currentTime = frame_id / fps;
    std::snprintf(buf, sizeof(buf), "frame_id:%d time:%lf s", frame_id,currentTime);
    lines.emplace_back(buf);


    int x0 = 20, y0 = 90;
    putTextBg(frame1, lines[0], cv::Point(x0, y0));
    
}
//4.2标注成功1，未成功0
void put_valid(cv::Mat frame1,int detect_valid, int pnp_success) {
    char buf[120];
    std::vector<std::string>lines;

    std::snprintf(buf, sizeof(buf), "detect_valid:%d  pnp_success:%d", detect_valid, pnp_success);
    lines.emplace_back(buf);

    int  x0 = 20,y0 = 146;
    putTextBg(frame1, lines[0], cv::Point(x0, y0));

}
//4.5(1)当pnp_success=1,标注误差（改）
void put_err1(cv::Mat frame1, std::vector<double> errs, double meanErr) {
    char buf[120];
    std::vector<std::string>lines;

    for (int i = 0;i<4;++i) {
        std::snprintf(buf, sizeof(buf), "e%d:%lf", i, errs[i]);
        lines.emplace_back(buf);
    }
    std::snprintf(buf, sizeof(buf), "meanErr:%lf", meanErr);
    lines.emplace_back(buf);

    int x0 = 20, y0 = 174, line_h = 28;
    for (size_t i = 0;i < lines.size();++i) {
        putTextBg(frame1, lines[i], cv::Point(x0 , y0+(int)i*line_h));
    }

}
//4.3(2)pnp_success=0
void putText2(cv::Mat frame1) {
    char buf[200];
    std::vector<std::string>lines;
    std::snprintf(buf, sizeof(buf), "tvec:");
    lines.emplace_back(buf);

    std::snprintf(buf, sizeof(buf), "distance:");
    lines.emplace_back(buf);

    int x0 = 20, y0 = 34, lineH = 28;
    for (size_t i = 0;i < lines.size();++i) {
        putTextBg(frame1, lines[i], cv::Point(x0, y0 + (int)i * lineH));
    }
}
//4.5(2)当pnp_success=0,标注误差（改）
void put_err2(cv::Mat frame1) {
    char buf[120];
    std::vector<std::string>lines;

    for (int i = 0;i < 4;++i) {
        std::snprintf(buf, sizeof(buf), "e%d:",i);
        lines.emplace_back(buf);
    }
    std::snprintf(buf, sizeof(buf), "meanErr");
    lines.emplace_back(buf);

    int x0 = 20, y0 = 174, line_h = 28;
    for (size_t i = 0;i < lines.size();++i) {
        putTextBg(frame1, lines[i], cv::Point(x0, y0 + (int)i * line_h));
    }

}
//4.4(1)旋转角
void put_rad1(cv::Mat frame1, double rx_rad, double ry_rad, double rz_rad) {
    char buf[120];
    std::vector<std::string>lines;
    std::snprintf(buf, sizeof(buf), "rx_rad:%lf ry_rad:%lf rz_rad:%lf", rx_rad, ry_rad, rz_rad);
    lines.emplace_back(buf);

    int x0 = 20, y0 = 118;
    putTextBg(frame1, lines[0], cv::Point(x0, y0));
}
//4.4（0）旋转角
void put_rad2(cv::Mat frame1) {
    char buf[120];
    std::vector<std::string>lines;
    std::snprintf(buf, sizeof(buf), "rx_rad:  ry_rad:  rz_rad:");
    lines.emplace_back(buf);

    int x0 = 20, y0 = 118;
    putTextBg(frame1, lines[0], cv::Point(x0, y0));
}
