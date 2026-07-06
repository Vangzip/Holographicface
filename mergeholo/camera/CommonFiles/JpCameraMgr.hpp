 #ifndef JP_CAMERAMGR_HPP
#define JP_CAMERAMGR_HPP

#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <opencv2/opencv.hpp>            // C++
#include <opencv2/core/version.hpp>
#include "threadsafe_queue.hpp"
#include "JpCameraInterface.h"
#include "NewTec.hpp"
#include <QThread>
#include "AppConfig.hpp"
#include "DbLogic.h"
#include "DbMgr.hpp"
#include "DbEntity/DatabaseEntityHeader.h"
#include "JpCTime.hpp"
namespace JP {



//template <typename T >
//struct camera_op_traits;

//template<typename camera_op,  size_t CNT>
//class CJpCameraMgr;

//template <>
//struct camera_op_traits<>
//{

//};
//TEMPLATE_DECLARE_CHECK_MEMBER_FUNCTION(decode)

template <size_t CNT>
class CJpCameraMgrInterface
{
public:
    struct CameraFrame
    {
        cv::Mat data[CNT];
        int64_t time[CNT];
        int64_t timestamp;
        bool empty()
        {
            for(size_t i=0; i<CNT ; i++)
            {
                if( data[i].empty() )
                {
                    //std::cout<<" frame is empty" << std::endl;
                    return true;
                }
            }
            return false;
        }
        void release()

        {
            for(size_t i=0; i<CNT ; i++)
            {
                if( !data[i].empty() )
                {
                    data[i].release();
                }
            }
        }
    };
public:
    virtual void Init(int row, int col, bool display = true, float s = 5.0f, size_t szCameraGroupId =0, int iCard = 0) = 0;
    virtual void UnInit() = 0;
    virtual bool GetFrame(CameraFrame& frame) = 0;
    virtual void stop() = 0;
    virtual void ImgRelease(CameraFrame mm) = 0;
};

template<typename camera_op,  size_t CNT,
         typename std::enable_if_t<std::is_base_of_v<IJpCameraInterface,camera_op>>* = nullptr>
class CJpCameraMgr : public CJpCameraMgrInterface<CNT>
{
public:
    using CameraFrame = typename CJpCameraMgrInterface<CNT>::CameraFrame;

//    using camera_op_has_decode_v = has_member_function_decode<camera_op,
//        int,int,uint8_t*, int>;
public:
    CJpCameraMgr( bool bRgb =true)
        :width(0),height(0),m_id(0),
          m_vctQueueIdle(CNT),m_vctQueueBusy(CNT),m_bRgb(bRgb),m_vctArray(CNT)
    {
        for(size_t i=0; i<CNT; i++ )
        {

            //auto t = JpCCamera( m_vctQueueIdle[i], m_vctQueueBusy[i], bRgb);
            //m_vctCamera.emplace_back(camera_op_traits<camera_op>::Init(this));
            m_vctCamera.push_back( std::make_shared<camera_op>());
        }
    }
    ~CJpCameraMgr()
    {
        UnInit();
    }

public:
    virtual void Init(int row, int col, bool display = true, float s = 5.0f, size_t szCameraGroupId =0, int iCard = 0)
    {
        m_szCameraGroupId = szCameraGroupId;
        (void)display;
        // 初始化图像参数
        width = col;
        height = row;
        scale = s;



        // 为每一个相机初始化队列
        for(size_t i=0; i< CNT; i++ )
        {

            //std::cout<<" Camera Mgr Init " << row <<" " << col << std::endl;
            m_vctQueueBusy[i].setflag();
            m_vctQueueIdle[i].setflag();

            for (int j = 0; j < 3; ++j)
            {
                if( m_bRgb )
                {
                    m_vctArray[i].emplace_back(cv::Mat::zeros(height, width, CV_8UC3));
                }
                else
                {
                    m_vctArray[i].emplace_back( cv::Mat::zeros(height, width, CV_8UC1));
                }

                m_vctQueueIdle[i].push(m_vctArray[i][j]);
            }
        }


        auto& d = CDbMgr<CDbLogic>::GetDb();
        // 初始化相机
        for(size_t i=0; i<CNT; i++)
        {
//            if( szCameraGroupId > 1 )
//            {
//                szCameraGroupId = 1;
//            }
            auto pCamera = d.getRecord<CEntityCamera>(QC_T("CameraIndex", szCameraGroupId*CNT+i));

            // std::cout<<"Get camera Id " << pCamera->GetRecordId() << endl;
            //m_vctCamera[i]->setCa
            m_vctCamera[i]->Init(/*width, height,*/ const_cast<char*>(pCamera->m_strName.c_str()),
                                 szCameraGroupId*CNT+i);
        }


        // 启动抓图线程
        std::thread* pThread = new std::thread(captureData,this);
        m_pDataThread.reset(pThread);

//        std::thread* pControlThread = new std::thread(controlCamera,this);
//        m_pCtrlThread.reset(pThread);
    }
    virtual void UnInit()
    {
        stop();
    }
    virtual bool GetFrame(CameraFrame& frame)
    {
        // std::cout<<"GetFrame " << m_que_data.size()<< std::endl;
        return m_que_data.pop(frame);
    }
    virtual void ImgRelease(CameraFrame mm)
    {
        for(size_t i=0; i<CNT; i++ )
        {
            m_vctQueueIdle[i].push(mm.data[i]);
        }
    }

    virtual void stop()
    {
        m_bstop = true;
        if( m_pDataThread.get() )
        {
            m_pDataThread->join();
        }

        if( m_pCtrlThread.get() )
        {
            m_pCtrlThread->join();
        }

        for(size_t i=0; i<CNT; i++)
        {
            m_vctCamera[i]->UnInit();
        }
    }

    void setCameraFps( int fps )
    {
        for(auto pCamera : m_vctCamera )
        {
            JP::camera_op_traits<camera_op>::setFps(pCamera, fps);
        }
    }
//    static void controlCamera(CJpCameraMgr* p)
//    {
//        while (!p->m_bstop)
//        {

//            for(auto pCamera : p->m_vctCamera )
//            {
//                if( true )
//                {
//                    JP::camera_op_traits<camera_op>::setFps(pCamera, 1);
//                }
//                else
//                {
//                    JP::camera_op_traits<camera_op>::setFps(pCamera, 30);
//                }

//            }


//            QThread::msleep(10);
//        }
//    }
    static void captureData(CJpCameraMgr* p)
    {
        //std::cout<<"capture_data "<< p->m_bstop << std::endl;
        JpCtime oTimer("capture_data");
        while (!p->m_bstop)
        {

//            if( !p->m_que_data.empty() )
//            {
//                // std::cout<<" queue is not empty" << std::endl;
//                QThread::msleep(5);
//                continue;
//            }
            //std::cout<<"----------------------------------------" << std::endl<< std::endl<< std::endl<< std::endl<< std::endl;
            //printf("queue size is :%lu\n",p->m_que_data.size());
            CameraFrame myframe;
            int64_t timestamp = 0;
            bool bRs = true;

            int64_t itimeMax =-1;;

            for(size_t i=0; i<CNT; i++ )
            {
                auto pCamera = p->m_vctCamera[i];

                uint64_t fid = 0;

//                myframe.data[i] = cv::Mat(p->height,p->width,CV_8UC3);
//                int iLen = pCamera->GetImage(myframe.data[i].ptr(),
//                                             p->height*p->width*sizeof(uchar)*3,
//                                             fid,
//                                             myframe.timestamp);

                while(0 ==  pCamera->GetImage(myframe.data[i],fid, myframe.time[i]))
                {
                    QThread::msleep(1);
                }
                itimeMax = std::max(itimeMax, myframe.time[i]);
                // oTimer.printAlltime("get a group of image");
//                uint8_t* pData = pCamera->GetImage(fid, timestamp, iLen);
//                if( !pData )
//                {
//                    break;
//                }
                //printf("captrue %d %d\n", p->height,p->width);

//                memcpy(myframe.data[i].ptr(),pData, iLen );
                // = timestamp;
                //delete[] pData;
//                cv::Mat

//                while(!p->m_vctQueueBusy[i].pop(myframe.data[i]))
//                {
//                    //no op
//                }
            }

            if( itimeMax <=0 )
            {
                QThread::msleep(1);
                continue;
            }
            for( int i=0; i<CNT && bRs;i++  )
            {
                auto pCamera = p->m_vctCamera[i];
                uint64_t fid = 0;
                while( true )
                {
                    int diff = itimeMax - myframe.time[i];
//                    printf("Camera %lu cnt %d sync time is %lu diff is %ld\n",
//                           p->m_szCameraGroupId,i,myframe.time[i],diff);
                    if( diff > 40000)
                    {
                        //pCamera->releaseImage(myframe.data[i]);
                        while(0 ==  pCamera->GetImage(myframe.data[i],fid, myframe.time[i]))
                        {
                            QThread::msleep(1);
                        }

                    }
                    else if ( diff <-40000)
                    {
                        bRs = false;
                        break;
                    }
                    else
                    {
                        break;
                    }
                    QThread::msleep(1);
                }

            }

            /*
            while( true )
            {
                uint64_t fid = 0;
                int diff = myframe.time[0] - myframe.time[1];
                // printf("[MGr %lu] diff is %d\n", p->m_szCameraGroupId,diff);
                if( std::abs(diff)> 100000000)
                {
                    bRs = false;
                    break;
                }
                else if( std::abs(diff) > 100000 )
                {
                    if( diff > 0 )
                    {
                        while(0 ==  p->m_vctCamera[1]->GetImage(myframe.data[1],fid, myframe.time[1]))
                        {
                            QThread::msleep(5);
                        }
                    }
                    else
                    {
                        while(0 ==  p->m_vctCamera[0]->GetImage(myframe.data[0],fid, myframe.time[0]))
                        {
                            QThread::msleep(5);
                        }
                    }
                }

                else
                {
                    break;
                }
            }
*/
            if( bRs )
            {
                myframe.timestamp = myframe.time[0];

//                printf("[Camera %lu] sync ",p->m_szCameraGroupId);
//                for(int i=0;i < CNT; i++ )
//                {
//                    printf("  ,%ld",myframe.time[i]);
//                }
//                printf("\n");
                p->m_que_data.push(myframe);
            }




           // std::cout<<" data_queue.size " <<p->m_que_data.size()<< std::endl;
        }
    }
private:
    int width;
    int height;
    int  m_id;
    float scale;
    bool m_bRgb = false;
    size_t m_szCameraGroupId;
    std::atomic<bool> m_bstop =false;
private:
    threadsafe_queue<CameraFrame> m_que_data;

    std::vector< threadsafe_queue<cv::Mat> > m_vctQueueIdle;
    std::vector< threadsafe_queue<cv::Mat> > m_vctQueueBusy;
    std::vector<std::vector<cv::Mat>> m_vctArray;

    std::vector<JP::IJpCameraInterface::Ptr> m_vctCamera;
    std::shared_ptr<std::thread> m_pDataThread;
    std::shared_ptr<std::thread> m_pCtrlThread;
};

//template <typename camera_op >class CJpCameraMgr<camera_op,1>;
//template <typename camera_op >class CJpCameraMgr<camera_op,2>;
//template <typename camera_op >class CJpCameraMgr<camera_op,4>;
//template class CJpCameraMgr<2>;
//template class CJpCameraMgr<4>;


}
#endif
