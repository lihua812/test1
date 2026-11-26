#include "PnP1.h"

int main() {
    //数据
    std::vector<cv::Point2f>imagePoints;
    std::vector<cv::Point3f> objectPoints = {
        {-0.0675f, -0.028f, 0},
        { 0.0675f, -0.028f, 0},
        { 0.0675f,  0.028f, 0},
        {-0.0675f,  0.028f, 0}
    };

    cv::Mat cameraMatrix = (cv::Mat_<double>(3, 3) <<
        1781.053454, 0, 510.5751561,
        0, 1781.686873, 472.9735763,
        0, 0, 1);
    cv::Mat distCoeffs = (cv::Mat_<double>(1, 5) <<
        -0.04420740074, -0.1081537096,
        -0.0006506219039, 0.0002357380215, 0.466737247);  
	//1,接入视频
	cv::VideoCapture cap ("outpost/outpost.avi");
	if (!cap.isOpened()) {
		std::cerr << "error" << std::endl;
		return -1;
	}

    //变量区    
    std::vector<std::vector<cv::Point>> contours;
    std::vector<cv::RotatedRect>light_bars;
	cv::Mat frame;
    int frame_id = 0;
    cv::Mat rvec, tvec;
    std::vector<double>errs(4);
    double meanErr;
    double rx_rad, ry_rad, rz_rad;
    // fps 异常时兜底，否则 VideoWriter 会打开失败（fps<=0 或离谱值都写不进去）
    double fps = cap.get(cv::CAP_PROP_FPS);
    if (fps <= 1.0 || fps > 120.0) fps = 30.0;

    // 延迟打开：VideoWriter 必须用第一帧的【真实尺寸】创建，
    // 因为 CAP_PROP_FRAME_WIDTH/HEIGHT 在部分后端第一帧解码前返回 0，会导致打开失败
    cv::VideoWriter writer;
    bool writer_ready = false;   // 标记 writer 是否已成功打开，避免重复尝试

    int detect_valid = 0, pnp_success = 0;
    

	while (true) {
        //数值更新（很重要）
        int detect_valid = 0, pnp_success = 0;
		cap >> frame;
        frame_id++;
		if (frame.empty()) break;
       
        // 拿到第一帧真实尺寸后再打开 writer；open 失败必须报出来，不能静默继续
        if (!writer_ready) {
            writer_ready = writer.open("output.avi",
                cv::VideoWriter::fourcc('M', 'J', 'P', 'G'),
                fps, frame.size());
            if (!writer_ready) {
                std::cerr << "VideoWriter open failed" << std::endl;
                return -1;
            }
        }

        contours = Findcontours(frame);
        light_bars = detector(frame, contours);
        imagePoints = detectPoints(frame, light_bars);

       

        cv::Mat frame1 = frame.clone();

        //提取灯条
        for (const auto& light_bars1:light_bars) {
            cv::Point2f corner[4];light_bars1.points(corner);
            for (int i = 0;i < 4;i++) {
                cv::line(frame1, corner[i], corner[(i + 1) % 4],cv::Scalar(0, 0, 255),2);
            }
        }
       
        
        if (imagePoints.size() == 4) {

             detect_valid = 1;

             //提取装甲板
             for (int i = 0;i < 4;i++) {
                 cv::line(frame1, imagePoints[i], imagePoints[(i + 1) % 4], cv::Scalar(0, 255, 0), 2);
             }
             
             //画出中心点,角点编号
             double sumx = 0;
             double sumy = 0;
             cv::Point2f center;
             for (const auto& imagePoint : imagePoints) {
                 sumx += imagePoint.x;
                 sumy += imagePoint.y;
             }
             center.x = sumx / 4;
             center.y = sumy / 4;
             cv::circle(frame1, center, 4, cv::Scalar(0, 255, 0), -1);
             cv::putText(frame1, "0", imagePoints[0], cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(255, 255, 255), 2);
             cv::putText(frame1, "1", imagePoints[1], cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(255, 255, 255), 2);
             cv::putText(frame1, "2", imagePoints[2], cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(255, 255, 255), 2);
             cv::putText(frame1, "3", imagePoints[3], cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(255, 255, 255), 2);
             


             //求解
             //cv::Mat rvec, tvec;//是不是识别不到角点（当无配对的装甲板时，传不了角点）在imagePoints读取加一个成功检验，如有不成功
             bool success = cv::solvePnP(objectPoints, imagePoints, cameraMatrix,
                 distCoeffs, rvec, tvec, false,
                 cv::SOLVEPNP_IPPE);
             if (!success) { 
                 std::cout << "solvePnP error" << std::endl;
                 pnp_success = 0;   // 只标记失败，跳过本帧的误差计算与显示
                 detect_valid = 1;  // 检测到装甲板但求解失败
             }
             else {
                 pnp_success = 1;

                 std::vector<cv::Point2f> projectedPoints;
                 cv::projectPoints(objectPoints, rvec, tvec, cameraMatrix,
                     distCoeffs, projectedPoints);

                 cv::Mat R;
                 cv::Rodrigues(rvec, R);

                 cv::Mat mtxR, mtxQ;
                 cv::Vec3d euler_deg = cv::RQDecomp3x3(R, mtxR, mtxQ); // 返回角度制
                 rx_rad = euler_deg[0] * CV_PI / 180.0;
                 ry_rad = euler_deg[1] * CV_PI / 180.0;
                 rz_rad = euler_deg[2] * CV_PI / 180.0;






                 //重投影误差
                 double totalError = 0;
                 for (int i = 0; i < 4; i++) {
                     errs[i] = cv::norm(imagePoints[i] - projectedPoints[i]);
                     totalError += errs[i];
                 }
                 meanErr = totalError / 4;
                 std::cout << "tvec: " << tvec << "  " << "rvec:  " << rvec << std::endl;

                 //标注角点与重投影点
                 for (int i = 0;i < 4;i++) {
                     cv::circle(frame1, imagePoints[i], 4, cv::Scalar(255, 0, 0), -1);
                     cv::circle(frame1, projectedPoints[i], 4, cv::Scalar(0, 255, 0), -1);
                 }

                 
             }
         }
         //标注区
         if (pnp_success == 0) {
             putText2(frame1);
             put_err2(frame1);
             put_rad2(frame1);
         }
         if (pnp_success == 1) {
             putText1(frame1, rvec, tvec);        
             put_err1(frame1, errs, meanErr);
             put_rad1(frame1, rx_rad, ry_rad, rz_rad);
         }
         put_frame_id(frame1, fps, frame_id);
         put_valid(frame1,detect_valid, pnp_success);
         
         writer.write(frame1);//保存视频

         imshow("video", frame1);
		if (cv::waitKey(30) == 27)break;
	}
	cap.release();
    writer.release();
    cv::destroyAllWindows();
	return 0;
}
