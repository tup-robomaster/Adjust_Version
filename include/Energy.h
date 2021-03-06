//----------------------------------------------------------
//
// FileName: Energy.h
// Author: Liu Shang fyrwls@163.com
// Version: 1.0.20200924
// Date: 2020.09.24
// Description: Energy类的定义
// Function List:
//              1. void run(Mat &oriFrame)
//              2. void clearVectors()
//              3. void initFrames(Mat &frame)
//              4. bool getDirectionOfRotation()
//              5. bool predictTargetPoint(Mat &frame)
//              6. bool findArmors(Mat &src)
//              7. bool findTargetArmor(Mat &src_bin)
//              8. bool isValidArmor(vector<Point>&armor_contour)
//              9. bool predictRCenter(Mat &frame)
//              10. bool findFlowStripFan(Mat &src)
//              11. bool isValidFlowStripFan(Mat &src_bin, vector<Point> &flow_strip_fan_contour)
//              12. int getRectIntensity(Mat &src, Rect &roi_rect)
//
//----------------------------------------------------------
#pragma once

#include "./Params.h"
#include "./Debug.h"
#include "./Buff_Debug.h"

#include <iostream>
#include <string>   // 字符串
#include <vector>   // 向量容器
#include <queue>    // 队列
#include <mutex>    // 互斥量
#include <atomic>
#include <time.h>

#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/calib3d.hpp>
#include <opencv2/objdetect.hpp>

#include "./ShootGyro.h"
#include "../include/General.h"
#include "../include/serialport.h"
#include "../include/AngleSolver.hpp"

using namespace std;
using namespace cv;

//----------------------------------------------------------//
//                                                          //
//                      class Energy                        //
//                                                          //
//----------------------------------------------------------//

#ifndef DEF_CAMERA1
#define DEF_CAMERA1
    static Mat camera_Matrix1 = (cv::Mat_<double>(3, 3) <<  1200.9,  0.000000000000,  134.8634,  0.000000000000,  1196.2,  366.1528,  0.000000000000,  0.000000000000,  1.000000000000); 
    static Mat dis_Coeffs1 = (cv::Mat_<double>(1, 5) << -0.3524,  0.2160 , 0,0 ,0);
    static AngleSolver solve1(camera_Matrix1, dis_Coeffs1, 22.5, 5.5);
#endif

class Energy
{
public:
    Energy(SerialPort &port);        //能量机关构造函数
    Energy();                        //能量机关构造函数     
    ~Energy() {};

    bool run(Mat &oriFrame, ArmorPlate &predictArmor); // 识别总函数

private:
    bool getDirectionOfRotation();                    // 获得当前旋转方向
    bool predictTargetPoint(Mat &frame);              // 对打击点进行预测

    bool findArmors(Mat &src);                        // 寻找装甲板
    bool findTargetArmor(Mat &src_bin);               // 寻找最终的装甲板
    bool isValidArmor(vector<Point>& armor_contour);  // 检查装甲板是否符合要求

    bool predictRCenter(Mat &frame);                  // 预测字母R的中心点
    bool findCenterRROI();                            // 寻找圆心ROI
    bool findCenterR(Mat &src_bin);                   // 寻找中心R结构
    bool isValidCenterRContour(const vector<cv::Point> &center_R_contour);  //检查中心R是否符合要求
    
    void clearVectors();                              // 清空各vector
    void initFrame(Mat &frame);                       // 图像预处理
    void cutImageByROI(Mat &frame);                   // 根据ROI进行图像裁剪
    void ROIEnlargeByMissCnt(Mat &frame);             // 由丢失目标帧数进行ROI调整 
    void init();                                      // 参数初始化;
    void shootingAngleCompensate(double &distance,double &angle_x,double &angle_y); //射击补偿
    void finalHitCalc(ArmorPlate &predictArmor);                              //击打数据计算

    inline float spd_func(float time);                // spd函数  
    inline float theta_func(float time);              // 对spd的积分函数

    bool findFlowStripFan(Mat &src);                  // 寻找含流动条的扇叶
    bool isValidFlowStripFan(Mat &src_bin, vector<Point> &flow_strip_fan_contour);// 检查含流动条的扇叶是否合格

    int getRectIntensity(Mat &src, Rect &roi_rect);    // 获取roi区域的强度

    void Buff_ShootingAngleCompensate(ArmorPlate &predictArmor);

public:
    Params energyParams;                            //实例化参数对象

private:
    Kalman kalmanfilter{KALMAN_TYPE_ENERGY};        //卡尔曼滤波器

    queue<double> armor_angle_queue;                // 存储装甲板的角度,先存储3帧
    queue<Point2f> armor_center_in_centerR_cord;    //TODO::以中心R为左边原点的坐标系下装甲板中心点的集合,用于求delta theta
    queue<clock_t> armor_center_queue_time;         //记录上面集合的保存时间
    queue<double> armor_center_queue_omega;         //记录角速度增量

    vector<Point2f> armor_center_points;            // 装甲板中心点的集合,用于做拟合
    vector<RotatedRect> armors_rrect;               // 初步筛选到的各种装甲板
    vector<RotatedRect> flow_strip_fan_rrect;       // 筛选出来的有流动条的扇叶
    
    clock_t debug_cnt;                              // DEBUG时间戳

    Rect  image_ROI;                                //全图ROI
    RotatedRect center_ROI;                         // 中心ROI
    Point2i ROI_Offset;                             //ROI Offset（roi区域在传入图像中的位置）
    Point2f RCenter;                                // 中心点坐标
    Point2f predict_point;                          // 打击预测点
    
    RotatedRect target_armor;                       // 最终得到的装甲板
    RotatedRect prior_target_armor;                 // 上一次得到的最终装甲板
    RotatedRect target_flow_strip_fan;              // 最终得到的含有流动条的扇叶
    RotatedRect centerR;                            // 最终找到的R

    int rotation;                                    // 当前旋转方向
    int miss_cnt;                                    //丢失帧数

    bool is_first_predict;                           //是否为第一次预测

    double angle_confidence;                         // 角度置信度

public:
    AngleSolver solver_720 = solve1;  //角度解算类
    AngleSolverFactory angle_slover;

    double dist;
    double angle_x; //yaw
    double angle_y; //pitch
};