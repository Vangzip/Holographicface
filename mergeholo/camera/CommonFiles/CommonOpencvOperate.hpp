#ifndef	COMMON_OPENCV_OPERATE_HPP
#define COMMON_OPENCV_OPERATE_HPP
#include "opencv2/opencv.hpp"
namespace CvOpt
{
	// 找到匹配的特征点
void find_feature_matches(const cv::Mat& img_1, const cv::Mat& img_2,
                                        std::vector<cv::KeyPoint>& keypoints_1,
                                        std::vector<cv::KeyPoint>& keypoints_2,
                                        std::vector<cv::DMatch>& matches){
    // 初始化
    cv::Mat descriptors_1, descriptors_2;
    cv::Ptr<cv::FeatureDetector>detector = cv::ORB::create();
    cv::Ptr<cv::DescriptorExtractor>descriptor = cv::ORB::create();
    cv::Ptr<cv::DescriptorMatcher> matcher = cv::DescriptorMatcher::create("BruteForce-Hamming");   // 暴力匹配

    // 第一步：检测Oriented FAST角点位置
    detector->detect(img_1, keypoints_1);
    detector->detect(img_2, keypoints_2);

    // 第二步： 根据角点位置计算BRIEF描述子
    descriptor->compute(img_1, keypoints_1, descriptors_1);
    descriptor->compute(img_2, keypoints_2, descriptors_2);

    // 第三步：对两幅图像中的BRIEF描述子进行匹配，使用汉明距离
    std::vector<cv::DMatch> match;
    matcher->match(descriptors_1, descriptors_2, match);

    // 第四步：对匹配点对进行筛选
    double min_dist = 10000, max_dist = 0;

    // 找出所有匹配之间的最小距离和最大距离
    // 即是最相似和最不相似的两组点之间的距离
    for(int i=0; i<descriptors_1.rows; i++){
        double dist = match[i].distance;
        min_dist = min_dist<dist?min_dist:dist;
        max_dist = max_dist>dist?max_dist:dist;
    }
//    printf("--Max dist : %f\n", max_dist);
//    printf("--Min dist : %f \n", min_dist);

    // 当描述子之间的距离大于两倍的最小距离时，即认为匹配有误
    // 设置30为阈值
    for(int i=0; i<descriptors_1.rows; i++){
        if(match[i].distance <= std::max(2*min_dist, 30.0)){
            matches.push_back(match[i]);
        }
    }
}

void pose_estimation_2d2d(std::vector<cv::KeyPoint>keypoints_1,
                                                std::vector<cv::KeyPoint>keypoints_2,
                                                std::vector<cv::DMatch>matches,
                                                cv::Mat&R ,cv::Mat& t, cv::Mat K){
    // 相机内参,TUM Feriburg2
    //Mat K = (Mat_<double>(3,3) << 520.9, 0, 325.1, 0, 521.0, 249.7, 0, 0, 1);

    // 把匹配点转化为vector<Point2f>的形式
    std::vector<cv::Point2f>points1;
    std::vector<cv::Point2f>points2;

    for(int i=0; i<(int)matches.size(); i++){
        points1.push_back(keypoints_1[matches[i].queryIdx].pt);
        points2.push_back(keypoints_2[matches[i].trainIdx].pt);
    }

    // 计算基础矩阵:使用的8点法，但是书上说8点法是用来计算本质矩阵的呀，这两个有什么关系吗
    // 答：对于计算来说没有什么区别，本质矩阵就是基础矩阵乘以一个相机内参
    // 多于8个点就用最小二乘去解
    cv::Mat fundamental_matrix;
    fundamental_matrix = findFundamentalMat(points1, points2, cv::FM_8POINT);    // Eigen库计算会更快一些
    //cout<<"fundamental_matrix is "<<endl<<fundamental_matrix<<endl;

    // 计算本质矩阵：是由对极约束定义的：对极约束是等式为零的约束
    cv::Point2d principal_point(K.at<double>(0,2), K.at<double>(1,2));  // 相机光心,TUM dataset标定值
    double focal_length = K.at<double>(0,0);      // 相机焦距，TUM dataset标定值
    cv::Mat essential_matrix;
    essential_matrix = findEssentialMat(points1, points2, focal_length, principal_point);
    //cout<<"essential_matrix is "<<endl<<essential_matrix<<endl;

    // 计算单应矩阵:通常描述处于共同平面上的一些点在两张图像之间的变换关系
    cv::Mat homography_matrix;
    homography_matrix = findHomography(points1, points2, cv::RANSAC, 3);
    //cout<<"homography_matrix is "<<endl<<homography_matrix<<endl;

    // 从不本质矩阵中恢复旋转和平移信息
    // 这里的R,t组成的变换矩阵，满足的对极约束是:x2 = R * x1 + t,是第一个图到第二个图的坐标变换矩阵x2 = T21 * x1
    recoverPose(essential_matrix, points1, points2, R, t, focal_length, principal_point);
    //cout<<"R is "<<endl<<R<<endl;
    //cout<<"t is "<<endl<<t<<endl;

//    for(int i=0; i< R.rows; i++ )
//    {
//        for( int j=0; j< R.cols; j++ )
//        {
//            cout<<R.at<double>(i,j)<<",";
//        }
//        cout << t.at<double>(i,0)<<",";
//        cout<< endl;
//    }

}

	
};

#endif
