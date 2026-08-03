
#ifndef MODELMOVEHANDLER_H
#define MODELMOVEHANDLER_H


#include "Base.h"
#include "ModelMoveCameraConfig.h"
#include <FileLibrary.h>




//得到抓图
struct CaptureDrawCallback : public osg::Camera::DrawCallback
{
    CaptureDrawCallback(osg::ref_ptr<osg::Image> image, float resolution)
    {
        _image = image;
        m_resolution = resolution;
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
        _image->readPixels(0, 0, m_resolution, m_resolution, GL_RGB, GL_UNSIGNED_BYTE);
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
};


using namespace osg;

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
    modelMoveHandler(osgViewer::Viewer *viewer, osg::Group *pgroup, string &nodepath, osg::Image *pimage, const  string &type, float, float, const ModelMoveCameraConfig&);

    void init(const  string &type, float);

    ~modelMoveHandler(){};
    virtual bool handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa);

    bool isComplete() const { return m_complete; }

    bool rotateZ(float);

    bool rotateX(float);

    bool rotateY(float);

    osg::MatrixTransform* modelTransform() const;
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

    osgViewer::Viewer *viewer;
	osg::Vec3d m_modelcenter;

    string m_strOutDir;//输出路径
    float m_angle, m_per;//旋转角度，每度几张
    float m_per_angle;
};



#endif
