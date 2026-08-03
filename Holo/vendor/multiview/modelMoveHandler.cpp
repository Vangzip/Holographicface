#include "modelMoveHandler.h"
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>

namespace {

osg::Vec3d normalizedOrDefault(const osg::Vec3d& value, const osg::Vec3d& fallback)
{
    const double length = value.length();
    if (length <= 0.000001) {
        return fallback;
    }
    return value / length;
}

} // namespace

modelMoveHandler::modelMoveHandler(osgViewer::Viewer *viewer, osg::Group *pgroup, string &out, osg::Image *pimage, const  string &type, float angle, float per, const ModelMoveCameraConfig& cameraConfig, MultiviewTimingStats* timingStats){
    m_pImage = pimage;
    m_mt = new osg::MatrixTransform;
    m_strOutDir = out;
    m_cameraConfig = cameraConfig;
    m_timingStats = timingStats;
    double viewDistance = 0;
    m_mt->addChild(pgroup);
    double radius = m_mt->getBound().radius();
    const double distanceScale = m_cameraConfig.distanceScale > 0.0 ? m_cameraConfig.distanceScale : 2.0;
    m_angle = angle;
    m_per = per;
    m_per_angle = 1.0 / per;

    if (type == "osgb")
    {

        m_mt->setMatrix(m_mt->getMatrix()*osg::Matrixd::translate(-m_mt->getBound().center()));//模型中心放到世界坐标中心点上(0,0,0)
        m_modelcenter = m_mt->getBound().center();// +osg::Vec3(0, 0, -50);

        printf("mt center: %f, %f, %f, radius:%f\n", m_mt->getBound().center().x(), m_mt->getBound().center().y(), m_mt->getBound().center().z(), m_mt->getBound().radius());


        init(type, angle);
       // m_modelcenter += osg::Vec3(70, -50, -70);
        m_modelcenter += osg::Vec3(100, 0, -50);
        viewDistance = radius * distanceScale;

    }
    else    if (type == "ive") {

        m_mt->setMatrix(m_mt->getMatrix()*osg::Matrixd::translate(-m_mt->getBound().center()));//模型中心放到世界坐标中心点上(0,0,0)

        m_modelcenter = m_mt->getBound().center();
        //m_modelcenter += osg::Vec3(0,0,50);
        init(type, angle);
        m_modelcenter += osg::Vec3(-5, 0, 0);
        viewDistance = radius * distanceScale;

    }
    else if(type == "obj")
    {
        m_mt->setMatrix(m_mt->getMatrix()*osg::Matrixd::translate(-m_mt->getBound().center()));//模型中心放到世界坐标中心点上(0,0,0)

        m_modelcenter = m_mt->getBound().center();
        init(type,angle);
        //m_modelcenter += osg::Vec3(0, 0, 0.1);

        viewDistance = radius * distanceScale;
    }

    //osg::Vec3d eye(2.655249, -600, -10), center(2.655249, 0, -10), up(0, 0, 1);
    osg::Vec3d eye(0,0,0), center(0,0,0);
    osg::Vec3d up = normalizedOrDefault(m_cameraConfig.upDirection, osg::Vec3d(0, 0, 1));

    //修改相机参数
    osg::Vec3d moveEve = normalizedOrDefault(m_cameraConfig.eyeDirection, osg::Vec3d(0, -1, 0));

    osg::Vec3d lookCenter = m_modelcenter + m_cameraConfig.centerOffset;
    osg::Vec3d eyepoint = lookCenter + moveEve * viewDistance;
    ////将参数设置给相机，并立即获取相机参数

	viewer->getCamera()->setViewMatrixAsLookAt(eyepoint, lookCenter, up);
    if (m_cameraConfig.fovyDeg > 0.0) {
        double fovy, aspectRatio, zNear, zFar;
        viewer->getCamera()->getProjectionMatrixAsPerspective(fovy, aspectRatio, zNear, zFar);
        if (aspectRatio <= 0.0) {
            aspectRatio = 1.0;
        }
        if (m_cameraConfig.zNear > 0.0) {
            zNear = m_cameraConfig.zNear;
        }
        if (m_cameraConfig.zFar > 0.0) {
            zFar = m_cameraConfig.zFar;
        }
        viewer->getCamera()->setProjectionMatrixAsPerspective(m_cameraConfig.fovyDeg, aspectRatio, zNear, zFar);
    }

    viewer->setSceneData(m_mt);
    printf("model center: %f, %f, %f\n", m_modelcenter.x(), m_modelcenter.y(), m_modelcenter.z());
    //printf("1 init eye: %f,%f,%f\n", eye._v[0], eye._v[1], eye._v[2]);
    //printf("1 init center: %f,%f,%f\n\n", center._v[0], center._v[1], center._v[2]);
    //osg::Vec3d eye, center, up;
    viewer->getCamera()->getViewMatrixAsLookAt(eye, center, up);
    printf("1 init eye: %f,%f,%f\n", eye._v[0], eye._v[1], eye._v[2]);
    printf("1 init center: %f,%f,%f\n\n", center._v[0], center._v[1], center._v[2]);
#if 0   // down center

    eyepoint = eye;

    m_stepX = abs((eyepoint.y() - centermodel.y())) * 0.0261 * 30;//sinf(1.5)/cosf(1.5)// ((1 - cosf(1.5)) / sinf(1.5));
    m_stepZ = m_stepX / 2.5;
    //printf("moveX=%f, y distance=%f, tan=%f\n", moveX, eyepoint.y() - centermodel.y(), tan(1.5));
    osg::Vec3 neweyepint(eyepoint.x() + m_stepX, eyepoint.y(), eyepoint.z() - m_stepX);
    m_lefteyepoint = neweyepint;
    center = osg::Vec3(neweyepint.x(), center.y(), neweyepint.z());
    m_leftcenter = center;

    //将参数设置给相机，并立即获取相机参数
    viewer->getCamera()->setViewMatrixAsLookAt(neweyepint, center, up);

    viewer->getCamera()->getViewMatrixAsLookAt(eye, center, up);
    printf("2 init eye: %f,%f,%f,distance:%f， m_step: %f \n", eye._v[0], eye._v[1], eye._v[2], m_stepX, m_stepX / 30);
    printf("2 init center: %f,%f,%f\n\n", center._v[0], center._v[1], center._v[2]);



#endif


#if 0 // left up point
    m_stepX = abs((eyepoint.y() - centermodel.y())) * 0.026186 * 30;
    m_stepZ = m_stepX / 2.5;
    //printf("moveX=%f, y distance=%f, tan=%f\n", moveX, eyepoint.y() - centermodel.y(), tan(1.5));
    osg::Vec3 neweyepint(eyepoint.x() + m_stepX, eyepoint.y(), eyepoint.z() - m_stepX);
    m_lefteyepoint = neweyepint;
    center = osg::Vec3(neweyepint.x(), center.y(), neweyepint.z());
    //center = osg::Vec3(0,0,0);
    m_leftcenter = center;

    //将参数设置给相机，并立即获取相机参数
    viewer->getCamera()->setViewMatrixAsLookAt(neweyepint, center, up);

    viewer->getCamera()->getViewMatrixAsLookAt(eye, center, up);
    printf("2 init eye: %f,%f,%f,distance:%f， m_step: %f \n", eye._v[0], eye._v[1], eye._v[2], m_stepX, m_stepX / 30);
    printf("2 init center: %f,%f,%f\n\n", center._v[0], center._v[1], center._v[2]);

    //viewer->getCamera()->setProjectionMatrixAsOrtho(-radius, radius, -radius, radius, neweyepint.y(), center.y());

	double   fovy, aspectRatio, zNear, zFar;
	viewer->getCamera()->getProjectionMatrixAsPerspective(fovy, aspectRatio, zNear, zFar);
	fovy = 20;
	viewer->getCamera()->setProjectionMatrixAsPerspective(fovy, aspectRatio, zNear, zFar);
	viewer->getCamera()->getProjectionMatrixAsPerspective(fovy, aspectRatio, zNear, zFar);

	printf("%lf,%lf,%lf,%lf\n", fovy, aspectRatio, zNear, zFar);

#endif

    //double left, right, top, bottom, zfar, znear;
    //viewer->getCamera()->getProjectionMatrixAsOrtho(left, right, bottom, top, znear, zfar);
    //
    //printf("%f,%f,%f,%f,%f,%f\n", left, right, bottom, top, znear, zfar);


    //initXYZ(viewer->getCamera(), m_modelcenter);
};

void modelMoveHandler::init(const  string &type, float angle){


    num = m_frame = m_rotate = 0;
    m_status = true;
    m_complete = false;
    m_height = 1;//行控制

    //osdb
    //if (type == "osgb")//初始化模型位置
    //{
    //    rotateZ(90);

    //    rotateX(13.5);
    //    rotateZ(30);
    //

    //}
    //else if (type == "ive")//初始化模型位置
    //{
    //    //单体全息模型角度初始化

    //    rotateZ(-180);
    //    rotateX(50); //60
    //    rotateZ(30);   //-183

    //}
    //else
    if (type == "obj") //初始化模型位置
    {
        //initModelXLocation(45);
        //initModelXLocation(-40);

        rotateX(m_cameraConfig.hasInitialRotateXDeg ? static_cast<float>(m_cameraConfig.initialRotateXDeg) : angle / 2);
        //rotateX(20);
        rotateZ(m_cameraConfig.hasInitialRotateZDeg ? static_cast<float>(m_cameraConfig.initialRotateZDeg) : -angle / 2);

    }


};


bool modelMoveHandler::handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa){

    viewer = dynamic_cast<osgViewer::Viewer*>(&aa);
	if (viewer == NULL )
        return false;

    switch (ea.getEventType())
    {
    case osgGA::GUIEventAdapter::FRAME:{

										   if (!m_status)
										   {
											   return false;
										   }
                                           // Every two frames, write one image.
                                           const int captureFrameInterval = 2;
                                           m_frame++;
                                           if (m_frame < captureFrameInterval)
                                           {
                                               if (m_timingStats != nullptr)
                                               {
                                                   m_timingStats->addSkippedFrame();
                                               }
                                               return false;
                                           }
                                           m_frame = 0;
                                           //结束退出
                                           if (m_height > m_angle*m_per)
										   {
                                               m_complete = true;
                                               if (viewer != NULL)
                                               {
                                                   viewer->setDone(true);
                                               }
											   return false;
										   }


                                           //一行结束，换行
                                           if (m_rotate == m_angle*m_per)
                                           {

                                               //return 0;

                                               const auto rowStart = std::chrono::steady_clock::now();
                                               rotateZ(-m_angle/2);
                                               rotateX(-m_per_angle);
                                               rotateZ(-m_angle/2);
                                               if (m_timingStats != nullptr)
                                               {
                                                   m_timingStats->addRowTransition(multiviewElapsedSeconds(rowStart));
                                               }
                                               m_rotate = 0;
                                               cout << "m_height:" << m_height << endl;
                                               m_height++;
                                               return false;
                                           }

                                           //rotateXModel();
                                           const auto rotateStart = std::chrono::steady_clock::now();
                                           rotateZ(m_per_angle);
                                           if (m_timingStats != nullptr)
                                           {
                                               m_timingStats->addRotate(multiviewElapsedSeconds(rotateStart));
                                           }
                                           m_rotate += 1.0;

                                           //保存图片
                                           string filedir;
                                           std::stringstream ss;
                                           stringstream line, rows;

                                           if (m_height < 10 )
                                           {
                                               line<<"00"<<m_height;
                                           }
                                           else    if (m_height < 100)
                                           {
                                               line << "0" << m_height;
                                           }else
                                           {
                                               line <<  m_height;
                                           }

                                           if (m_rotate < 10 )
                                           {
                                               rows <<"00"<< m_rotate;
                                           }
                                           else if (m_rotate < 100)
                                           {
                                               rows << "0" << m_rotate;
                                           }else {
                                               rows << m_rotate;
                                           }

                                           string name = line.str() + rows.str() + ".jpg";
                                           //ss << m_strOutDir<< line.str() << rows.str() << ".jpg";
                                           filedir = FileLibrary::getInstance()->combineFilePath(m_strOutDir, name);
                                           //ss >> filedir;

										   //cout << filedir << endl;
                                           const auto copyStart = std::chrono::steady_clock::now();
                                           osg::ref_ptr<osg::Image> outputImage = new osg::Image(*m_pImage, osg::CopyOp::DEEP_COPY_ALL);
                                           if (m_timingStats != nullptr)
                                           {
                                               m_timingStats->addImageCopy(multiviewElapsedSeconds(copyStart));
                                           }
                                           if (m_cameraConfig.captureFlipVertical)
                                           {
                                               const auto flipStart = std::chrono::steady_clock::now();
                                               outputImage->flipVertical();
                                               if (m_timingStats != nullptr)
                                               {
                                                   m_timingStats->addFlip(multiviewElapsedSeconds(flipStart));
                                               }
                                           }
                                           const auto writeStart = std::chrono::steady_clock::now();
                                           const bool wroteImage = osgDB::writeImageFile(*outputImage, filedir);//图片写入到当前程序目录下
                                           if (m_timingStats != nullptr)
                                           {
                                               m_timingStats->addImageWrite(multiviewElapsedSeconds(writeStart));
                                               if (wroteImage)
                                               {
                                                   m_timingStats->addSavedImage();
                                               }
                                           }
                                           if (!wroteImage)
                                           {
                                               cout << "[multiview] failed to write image: " << filedir << endl;
                                           }
                                           num++;

    }break;

    case osgGA::GUIEventAdapter::PUSH:{//鼠标右键开始转动模型
                                          if (osgGA::GUIEventAdapter::MouseButtonMask::RIGHT_MOUSE_BUTTON == ea.getButtonMask())
                                          {

                                              m_status = true;
                                              num = 0;
                                              osg::Vec3d eye(0, 0, 0), center(0, 0, 0), up(0, 0, 1);

                                              viewer->getCamera()->getViewMatrixAsLookAt(eye, center, up);
                                              printf("1 init eye: %f,%f,%f\n", eye._v[0], eye._v[1], eye._v[2]);
                                              printf("1 init center: %f,%f,%f\n\n", center._v[0], center._v[1], center._v[2]);

                                          }
    }break;
    }

    return false;
}
//Z轴旋转  angle：旋转角度
bool modelMoveHandler::rotateZ(float angle){
    osg::Vec3d centermodel = m_mt->getBound().center();
    //以物体为中心旋转
	m_mt->setMatrix(m_mt->getMatrix()*osg::Matrixd::translate(-centermodel) //先将物体中心平移到世界坐标的原点
        *osg::Matrixd::rotate(-1*osg::DegreesToRadians(angle), 0, 0, 1)//旋转
        *osg::Matrixd::translate(centermodel));//变换后再将物体移回

    //以当前视点为中心旋转
    //m_mt->setMatrix(m_mt->getMatrix() *osg::Matrixd::rotate(-1 * osg::DegreesToRadians(angle), 0, 0, 1));//旋转

    return true;
};
//X轴旋转  angle：旋转角度
bool modelMoveHandler::rotateX(float angle){
    osg::Vec3d centermodel = m_mt->getBound().center();

    //以物体为中心旋转
	m_mt->setMatrix(m_mt->getMatrix()*osg::Matrixd::translate(-centermodel) //先将物体中心平移到世界坐标的原点
        *osg::Matrixd::rotate(osg::DegreesToRadians(angle), 1, 0, 0)//旋转
        *osg::Matrixd::translate(centermodel)
        );//变换后再将物体移回

    //以当前视点为中心旋转
    //m_mt->setMatrix(m_mt->getMatrix() *osg::Matrixd::rotate(osg::DegreesToRadians(angle), 1, 0, 0));//旋转

    return true;
};
