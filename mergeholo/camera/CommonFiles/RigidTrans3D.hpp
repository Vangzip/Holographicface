#ifndef RIGID_TRANS_3D_HPP
#define RIGID_TRANS_3D_HPP

#include <opencv2/opencv.hpp>

namespace testCv
{

        /**
         * @struct TRigidTrans3D
         * @brief 存储三维刚 性变换的结构体，包括旋转矩阵和平移向量。
         */
        struct TRigidTrans3D
        {
                float matR[9]; ///< 旋转矩阵 (3x3)

                float X; ///< 平移向量的X分量
                float Y; ///< 平移向量的Y分量
                float Z; ///< 平移向量的Z分量
        };

        /**
         * @brief 计算三维刚性变换
         *
         * @param srcPoints 源点数组
         * @param dstPoints 目标点数组
         * @param pointsNum 点的数量
         * @param transform 输出变换结果
         * @return true 如果计算成功
         * @return false 如果计算失败
         */
        bool GetRigidTrans3D(cv::Point3f *srcPoints, cv::Point3f *dstPoints, int pointsNum, TRigidTrans3D &transform)
        {
                float srcSumX = 0.0f; ///< 源点X坐标的累计和
                float srcSumY = 0.0f; ///< 源点Y坐标的累计和
                float srcSumZ = 0.0f; ///< 源点Z坐标的累计和

                float dstSumX = 0.0f; ///< 目标点X坐标的累计和
                float dstSumY = 0.0f; ///< 目标点Y坐标的累计和
                float dstSumZ = 0.0f; ///< 目标点Z坐标的累计和

                // 累计源点和目标点坐标
                for (int i = 0; i < pointsNum; ++i)
                {
                        srcSumX += srcPoints[i].x;
                        srcSumY += srcPoints[i].y;
                        srcSumZ += srcPoints[i].z;

                        dstSumX += dstPoints[i].x;
                        dstSumY += dstPoints[i].y;
                        dstSumZ += dstPoints[i].z;
                }

                cv::Point3f centerSrc, centerDst;

                // 计算源点和目标点的质心
                centerSrc.x = float(srcSumX / pointsNum);
                centerSrc.y = float(srcSumY / pointsNum);
                centerSrc.z = float(srcSumZ / pointsNum);

                centerDst.x = float(dstSumX / pointsNum);
                centerDst.y = float(dstSumY / pointsNum);
                centerDst.z = float(dstSumZ / pointsNum);

                // 创建存储去质心后的点的矩阵
                cv::Mat srcMat(3, pointsNum, CV_32FC1);
                cv::Mat dstMat(3, pointsNum, CV_32FC1);

                float *srcDat = (float *)(srcMat.data); ///< 指向源点数据的指针
                float *dstDat = (float *)(dstMat.data); ///< 指向目标点数据的指针

                // 减去质心，得到新的点坐标
                for (int i = 0; i < pointsNum; ++i)
                {
                        srcDat[i] = srcPoints[i].x - centerSrc.x;
                        srcDat[pointsNum + i] = srcPoints[i].y - centerSrc.y;
                        srcDat[pointsNum * 2 + i] = srcPoints[i].z - centerSrc.z;

                        dstDat[i] = dstPoints[i].x - centerDst.x;
                        dstDat[pointsNum + i] = dstPoints[i].y - centerDst.y;
                        dstDat[pointsNum * 2 + i] = dstPoints[i].z - centerDst.z;
                }

                // 计算协方差矩阵
                cv::Mat matS = srcMat * dstMat.t();

                cv::Mat matU, matW, matV;

                // 进行奇异值分解
                cv::SVDecomp(matS, matW, matU, matV);

                // 计算旋转矩阵
                cv::Mat matTemp = matU * matV;
                float det = cv::determinant(matTemp);

                float datM[] = {1, 0, 0, 0, 1, 0, 0, 0, det};
                cv::Mat matM(3, 3, CV_32FC1, datM);

                cv::Mat matR = matV.t() * matM * matU.t();

                // 将旋转矩阵赋值给变换结构体
                memcpy(transform.matR, matR.data, sizeof(float) * 9);

                // 计算平移向量
                float *datR = (float *)(matR.data);
                transform.X = centerDst.x - (centerSrc.x * datR[0] + centerSrc.y * datR[1] + centerSrc.z * datR[2]);
                transform.Y = centerDst.y - (centerSrc.x * datR[3] + centerSrc.y * datR[4] + centerSrc.z * datR[5]);
                transform.Z = centerDst.z - (centerSrc.x * datR[6] + centerSrc.y * datR[7] + centerSrc.z * datR[8]);

                return true;
        }

}

#endif