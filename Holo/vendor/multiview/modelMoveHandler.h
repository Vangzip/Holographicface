
#ifndef MODELMOVEHANDLER_H
#define MODELMOVEHANDLER_H


#include "Base.h"
#include <FileLibrary.h>

#include <chrono>
#include <cstdint>
#include <mutex>


struct MultiviewTimingBucket
{
    std::int64_t count = 0;
    double totalSeconds = 0.0;
    double maxSeconds = 0.0;

    void add(double seconds)
    {
        ++count;
        totalSeconds += seconds;
        if (seconds > maxSeconds)
        {
            maxSeconds = seconds;
        }
    }

    double averageSeconds() const
    {
        return count > 0 ? totalSeconds / static_cast<double>(count) : 0.0;
    }
};

struct MultiviewTimingStats
{
    void addViewerFrame(double seconds) { add(viewerFrame, seconds); }
    void addReadPixels(double seconds) { add(readPixels, seconds); }
    void addRotate(double seconds) { add(rotate, seconds); }
    void addRowTransition(double seconds) { add(rowTransition, seconds); }
    void addImageCopy(double seconds) { add(imageCopy, seconds); }
    void addFlip(double seconds) { add(flipVertical, seconds); }
    void addImageWrite(double seconds) { add(imageWrite, seconds); }

    void addSkippedFrame()
    {
        std::lock_guard<std::mutex> lock(mutex);
        ++skippedFrames;
    }

    void addSavedImage()
    {
        std::lock_guard<std::mutex> lock(mutex);
        ++savedImages;
    }

    MultiviewTimingBucket viewerFrame;
    MultiviewTimingBucket readPixels;
    MultiviewTimingBucket rotate;
    MultiviewTimingBucket rowTransition;
    MultiviewTimingBucket imageCopy;
    MultiviewTimingBucket flipVertical;
    MultiviewTimingBucket imageWrite;
    std::int64_t skippedFrames = 0;
    std::int64_t savedImages = 0;

private:
    void add(MultiviewTimingBucket& bucket, double seconds)
    {
        std::lock_guard<std::mutex> lock(mutex);
        bucket.add(seconds);
    }

    std::mutex mutex;
};


inline double multiviewElapsedSeconds(std::chrono::steady_clock::time_point start)
{
    return std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
}



//得到抓图
struct CaptureDrawCallback : public osg::Camera::DrawCallback
{
    CaptureDrawCallback(osg::ref_ptr<osg::Image> image, float resolution, MultiviewTimingStats* timingStats = nullptr)
    {
        _image = image;
        m_resolution = resolution;
        m_timingStats = timingStats;
        _image->allocateImage(resolution, resolution, 1, GL_RGB, GL_UNSIGNED_BYTE);
    }

    ~CaptureDrawCallback(){}

    virtual void operator () (const osg::Camera& camera) const
    {
        //得到窗口系统接口
        //osg::ref_ptr<osg::GraphicsContext::WindowingSystemInterface> wsi = osg::GraphicsContext::getWindowingSystemInterface();

        //unsigned int width, height;
        //得到分辨率
        //wsi->getScreenResolution(osg::GraphicsContext::ScreenIdentifier(0), width, height);
#if 1 //jpg
        //分配一个image
        _image->setInternalTextureFormat(GL_RGB);

        //读取像素信息抓图
        const auto readStart = std::chrono::steady_clock::now();
        _image->readPixels(0, 0, m_resolution, m_resolution, GL_RGB, GL_UNSIGNED_BYTE);
        if (m_timingStats != nullptr)
        {
            m_timingStats->addReadPixels(multiviewElapsedSeconds(readStart));
        }
        //_image->readPixels(0,0, width, height, GL_RGB, GL_UNSIGNED_BYTE);
#endif

#if 0  //png
        //分配一个image
        _image->allocateImage(720, 720, 1, GL_RGBA, GL_UNSIGNED_BYTE);
        _image->setInternalTextureFormat(GL_RGBA);

        //读取像素信息抓图
        // _image->readPixels(width / 2 - 720 / 2, height / 2 - 720 / 2, 720, 720, GL_RGBA, GL_UNSIGNED_BYTE);
        _image->readPixels(width / 2 - 360, height / 2 - 360, 720, 720, GL_RGBA, GL_UNSIGNED_BYTE);
#endif

#if 0  //png
        //分配一个image
        _image->allocateImage(4096, 2160, 1, GL_RGB, GL_UNSIGNED_BYTE);
        _image->setInternalTextureFormat(GL_RGB);

        //读取像素信息抓图
        // _image->readPixels(width / 2 - 720 / 2, height / 2 - 720 / 2, 720, 720, GL_RGBA, GL_UNSIGNED_BYTE);
        _image->readPixels(0, 0, 4096, 2160, GL_RGB, GL_UNSIGNED_BYTE);
#endif
    }

    osg::ref_ptr<osg::Image> _image;
    float m_resolution;
    MultiviewTimingStats* m_timingStats;
};


using namespace osg;

struct ModelMoveCameraConfig
{
    double distanceScale = 2.0;
    osg::Vec3d centerOffset = osg::Vec3d(0.0, 0.0, 0.0);
    osg::Vec3d eyeDirection = osg::Vec3d(0.0, -1.0, 0.0);
    osg::Vec3d upDirection = osg::Vec3d(0.0, 0.0, 1.0);
    double fovyDeg = 0.0;
    double zNear = 0.0;
    double zFar = 0.0;
    bool hasInitialRotateXDeg = false;
    bool hasInitialRotateZDeg = false;
    double initialRotateXDeg = 0.0;
    double initialRotateZDeg = 0.0;
    bool captureFlipVertical = true;
};

class HUDAxis :public Camera
{
public:
    HUDAxis();
    HUDAxis(HUDAxis const& copy, CopyOp copyOp = CopyOp::SHALLOW_COPY);
    META_Node(osg, HUDAxis);
    inline  void setMainCamera(Camera* camera){ _mainCamera = camera; }
    virtual void traverse(NodeVisitor& nv);
protected:
    virtual ~HUDAxis();
    observer_ptr<Camera>       _mainCamera;
};
/////////////////////////////////////cpp///////////////////////////////////////////////////


//模型旋转
class  modelMoveHandler :public osgGA::GUIEventHandler
{
public:
    modelMoveHandler(osgViewer::Viewer *viewer, osg::Group *pgroup, string &nodepath, osg::Image *pimage, const  string &type, float, float, const ModelMoveCameraConfig&, MultiviewTimingStats* timingStats = nullptr);

    void init(const  string &type, float);

    ~modelMoveHandler(){};
    virtual bool handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa);

    bool isComplete() const { return m_complete; }

    bool rotateZ(float);

    bool rotateX(float);
 //
	//bool initModelZLocation(float angle);

	//bool initModelXLocation(float angle);


private:
    osg::Switch *m_pswitch;
    osg::MatrixTransform *m_mt;
    int num;
    osg::Vec3 m_lefteyepoint, m_leftcenter;
    osg::Image *m_pImage;
    float m_rotate;
    bool m_status;
    float m_stepX, m_stepZ;
    int m_width, m_height;
    int m_frame;
    bool m_complete;
    ModelMoveCameraConfig m_cameraConfig;
    MultiviewTimingStats* m_timingStats;

    osgViewer::Viewer *viewer;
	osg::Vec3d m_modelcenter;

    string m_strOutDir;//输出路径
    float m_angle, m_per;//旋转角度，每度几张
    float m_per_angle;
};



#endif
