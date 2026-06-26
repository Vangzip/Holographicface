
#include "crosswiseMoveHandler.h"

crosswiseMoveHandler::crosswiseMoveHandler(osgViewer::Viewer *viewer, osg::Group *pgroup, string &nodepath, osg::Image *pimage, const string & type){
    m_pImage = pimage;
    m_viewer = viewer;


    m_height = 1;
    num = m_frame = 0;
    m_leftMouse = m_rightMouse = m_status = false;

    //osg::ref_ptr<osg::MatrixTransform> mt = new osg::MatrixTransform;
    //设置转矩阵及位置信息
    //osg::ref_ptr<osg::CoordinateSystemNode> csn = new osg::CoordinateSystemNode();
    //csn->setEllipsoidModel(new osg::EllipsoidModel());
    //osg::EllipsoidModel* ellipsoid = csn->getEllipsoidModel();
    //osg::Matrix inheritedMatrix;
    //inheritedMatrix.makeIdentity();
    //osg::Matrixd matrix;
    ////osg::Matrixd matrix(inheritedMatrix);
    //matrix.set(inheritedMatrix);

    //ellipsoid->computeLocalToWorldTransformFromXYZ(0,-1,0, matrix);

    m_mt = new osg::MatrixTransform;
    
    m_mt->addChild(pgroup);          
   


    //m_mt->setMatrix(osg::Matrix::translate(-pgroup->getBound().center().x(), -pgroup->getBound().center().y(), -pgroup->getBound().center().z()));
    m_mt->setMatrix(m_mt->getMatrix()*osg::Matrixd::translate(-m_mt->getBound().center()));//模型中心放到世界坐标中心点上(0,0,0)

    m_modelcenter = m_mt->getBound().center();

    viewer->setSceneData(m_mt);

    double radius = m_mt->getBound().radius();
    osg::Vec3d eye(0, 0, 0), center(0, 0, 0), up(0, 0, 1);
    osg::Vec3d moveEve(0, -1, 0);

    double viewDistance; 
    if (type == "obj")
    {
        viewDistance = radius * 0.8;
                                  
    }
    else if (type == "ive")
    {
        viewDistance = radius * 0.8;
    }
    else if (type == "osgb")
    {
        initModelXLocation(90);
        m_modelcenter += osg::Vec3(70, -50, -70);
        //centermodel += osg::Vec3(100, 0, -50);
        viewDistance = radius *0.2;
    }



    //修改相机参数  

    osg::Vec3d eyepoint = m_modelcenter + moveEve * viewDistance;


    ////将参数设置给相机，并立即获取相机参数  
    viewer->getCamera()->setViewMatrixAsLookAt(eyepoint, m_modelcenter, up);

    printf("model center: %f, %f, %f\n", m_modelcenter.x(), m_modelcenter.y(), m_modelcenter.z());


    viewer->getCamera()->getViewMatrixAsLookAt(eyepoint, center, up);

    printf("1 init eye: %f,%f,%f\n", eyepoint._v[0], eyepoint._v[1], eyepoint._v[2]);
    printf("1 init center: %f,%f,%f\n\n", center._v[0], center._v[1], center._v[2]);

    //X轴一步移动的距离
    m_stepX = fabs((eyepoint.y() - m_modelcenter.y())) * 0.026186;// tan(1.5)
    m_stepZ = fabs((eyepoint.y() - m_modelcenter.y())) * 0.058243;// tan(3.3)
    //m_stepX = abs((eyepoint.y() - 0)) * 0.026186;// tan(1.5)
    //m_stepZ = abs((eyepoint.y() - 0)) * 0.058243;// tan(3.3)
    cout<<"step X: "<<m_stepX<<", step Z: "<<m_stepZ<<endl;

#if 0
    //m_stepZ = m_stepX / 2.5;
    //printf("moveX=%f, y distance=%f, tan=%f\n", moveX, eyepoint.y() - centermodel.y(), tan(1.5));
    osg::Vec3 neweyepint(eyepoint.x() + m_stepX, eyepoint.y(), eyepoint.z() - m_stepX);

    m_lefteyepoint = neweyepint;
    center = osg::Vec3(neweyepint.x(), center.y(), neweyepint.z());
    m_leftcenter = center;

    //将参数设置给相机，并立即获取相机参数  
    viewer->getCamera()->setViewMatrixAsLookAt(neweyepint, center, up);

    //重新获取
    viewer->getCamera()->getViewMatrixAsLookAt(eye, center, up);
    printf("2 init eye: %f,%f,%f,distance:%f， m_step: %f \n", eye._v[0], eye._v[1], eye._v[2], m_stepX, m_stepX / 30);
    printf("2 init center: %f,%f,%f\n\n", center._v[0], center._v[1], center._v[2]);
#endif
#if 1   // down center 

    double   fovy, aspectRatio, zNear, zFar;
    //viewer->getCamera()->getProjectionMatrixAsPerspective(fovy, aspectRatio, zNear, zFar);

    fovy = 60;
    viewer->getCamera()->setProjectionMatrixAsPerspective(fovy, 1, 1.0, 10000.0);
    viewer->getCamera()->getProjectionMatrixAsPerspective(fovy, aspectRatio, zNear, zFar);
    viewer->getCamera()->setNearFarRatio(0.000003f);

    printf("fov : %lf,%lf,%lf,%lf\n", fovy, aspectRatio, zNear, zFar);

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

#endif
    //viewer->getCamera()->setProjectionMatrixAsOrtho(-radius, radius, -radius, radius, neweyepint.y(), center.y());

    //double left, right, top, bottom, zfar, znear;
    //viewer->getCamera()->getProjectionMatrixAsOrtho(left, right, bottom, top, znear, zfar);
    //
    //printf("%f,%f,%f,%f,%f,%f\n", left, right, bottom, top, znear, zfar);

    init(type);
};


void crosswiseMoveHandler::init(const string & type){


    osg::Vec3 neweyepint, newcenter;
    //初始化开始位置
    osg::Vec3 eye, center, up;
    m_viewer->getCamera()->getViewMatrixAsLookAt(eye, center, up);

    if (type =="obj")
    {
        //摄像机位置在右下方
        osg::Vec3 neweyepint(eye.x() + m_stepX*29, eye.y(), eye.z() - m_stepZ*13.5);

        //摄像机位置在中心一行
        //osg::Vec3 neweyepint(eye.x() + m_stepX * 30, eye.y(), eye.z());

        //摄像机位置在中心一列
        //osg::Vec3 neweyepint(eye.x(), eye.y(), eye.z() - m_stepX * 29);

        osg::Vec3 newcenter(neweyepint.x(), center.y(), neweyepint.z());
        m_viewer->getCamera()->setViewMatrixAsLookAt(neweyepint, newcenter, up);  
    }
    else if (type == "ive")
    {

    }
    else if (type == "osgb")
    {
        downMove(-12);
        moveLeftBegin(30);
        rightMove();
    }

};



bool crosswiseMoveHandler::handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa){

    osgViewer::Viewer * viewer = dynamic_cast<osgViewer::Viewer*>(&aa);
    if (viewer == NULL)
        return false;

    switch (ea.getEventType())
    {
    case osgGA::GUIEventAdapter::FRAME:{

#if 1

                                           /* if (m_height>21)
                                           {
                                           return false;
                                           }*/

                                           //右键开始移动
                                           if (!m_status)
                                           {
                                               return false;
                                           }
                                           
                                           //三帧移动一次，为了缓存到当前帧图片
                                           m_frame++;
                                           if (m_frame <= 3)
                                           {

                                               return false;
                                           }

                                           m_frame = 0;

                                           if (m_height > 27)
                                           {
                                               return false;
                                           }
                                           //一行移动60次
                                           if (num >= 60)
                                           {   
                                               moveLeftBegin(60);                                              
                                               downMove(1);
                                               //moveLeftBegin(30);
                                               //XLevel(60); 
                                               //ZLevel(1);
                                               //rightMove();
                                               num = 0;
                                               m_height++;
                                               
                                               return false;
                                           }


                                           //移动
                                           rightMove();
                                           //downMove();

                                           std::stringstream ss;
                                           stringstream line, rows;
                                           num++;

                                           if (m_height >= 10)
                                           {
                                               line << m_height;
                                           }
                                           else
                                           {
                                               line << "0" << m_height;
                                           }

                                           if (num >= 10)
                                           {
                                               rows << num;
                                           }
                                           else
                                           {
                                               rows << "0" << num;
                                           }

                                           
                                           ss << "E:\\work\\data\\PCL_data_test\\mutliview_test_data\\" << line.str() << rows.str() << ".jpg";
                                           string filedir;

                                           
                                           ss >> filedir;


                                           //保存图片

                                                                    
                                           //osgDB::writeImageFile(*m_pImage, filedir);//图片写入到当前程序目录下
                                           cout << num << " " << filedir << endl;
#endif
#if 0
                                           /* if (m_height>21)
                                           {
                                           return false;
                                           }*/
                                           //三帧移动一次，为了缓存到当前帧图片
                                           m_frame++;
                                           if (m_frame <= 3)
                                           {

                                               return false;
                                           }
                                           m_frame = 0;

                                           //一行移动60次
                                           if (num >= 61)
                                           {
                                               /* if (m_height++>21)
                                               {
                                               return false;
                                               }*/
                                               //moveLeftBegin();                                              

                                               //num = 0;
                                               return false;
                                           }
                                           //右键开始移动
                                           if (!m_status)
                                           {
                                               return false;
                                           }


                                           std::stringstream ss;
                                           ss << "E:\\20170913\\";
                                           string filedir;
                                           //移动
                                           //rightMove();
                                           //downMove();
                                           if (m_leftMouse)
                                           {
                                               ss << "X";
                                               ss << "_" << num << ".jpg";
                                               ss >> filedir;
                                               //osgDB::writeImageFile(*m_pImage, filedir);//图片写入到当前程序目录下
                                               //rotateXModel();
                                           }
                                           if (m_rightMouse)
                                           {
                                               ss << "Z";
                                               ss << "_" << num << ".jpg";
                                               ss >> filedir;
                                               //osgDB::writeImageFile(*m_pImage, filedir);//图片写入到当前程序目录下
                                               //rotateZModel();
                                           }
                                           //保存图片


                                           //osgDB::writeImageFile(*m_pImage, filedir);//图片写入到当前程序目录下
                                           num++;
                                           cout << num << " " << filedir << endl;
#endif

    }break;

    case osgGA::GUIEventAdapter::PUSH:{
                                          if (osgGA::GUIEventAdapter::MouseButtonMask::RIGHT_MOUSE_BUTTON == ea.getButtonMask())
                                          {

                                              
                                              m_status = true;
                                              num = 0;

                                          }
#if 0
                                          if (osgGA::GUIEventAdapter::MouseButtonMask::RIGHT_MOUSE_BUTTON == ea.getButtonMask())
                                          {
                                              if (m_rotate < 45)
                                              {
                                                  m_rotate += 1.5;
                                              }
                                              else
                                              {
                                                  m_rotate = -45;
                                              }

                                              m_status = true;
                                              num = 0;


                                              osg::Vec3d centermodel = m_mt->getBound().center();
                                              if (m_leftMouse)
                                              {
                                                  m_mt->setMatrix(m_mt->getMatrix()*osg::Matrixd::translate(-centermodel) //先将物体中心平移到世界坐标的原点 
                                                      //*osg::Matrixd::scale(0.5, 0.5, 0.5) //缩放 
                                                      *osg::Matrixd::rotate(osg::DegreesToRadians(45.0f), 1, 0, 0)//旋转
                                                      //*osg::Matrixd::translate(1, 0, 0)//平移
                                                      *osg::Matrixd::translate(centermodel));//变换后再将物体移回
                                              }
                                              m_mt->setMatrix(m_mt->getMatrix()*osg::Matrixd::translate(-centermodel) //先将物体中心平移到世界坐标的原点 
                                                  //*osg::Matrixd::scale(0.5, 0.5, 0.5) //缩放 
                                                  *osg::Matrixd::rotate(osg::DegreesToRadians(-45.0f), 0, 0, 1)//旋转
                                                  //*osg::Matrixd::translate(1, 0, 0)//平移
                                                  *osg::Matrixd::translate(centermodel));//变换后再将物体移回

                                              m_rightMouse = true; m_leftMouse = false;

                                          }

                                          if (osgGA::GUIEventAdapter::MouseButtonMask::LEFT_MOUSE_BUTTON == ea.getButtonMask())
                                          {
                                              m_status = true;
                                              osg::Vec3d centermodel = m_mt->getBound().center();
                                              if (m_rightMouse)
                                              {
                                                  m_mt->setMatrix(m_mt->getMatrix()*osg::Matrixd::translate(-centermodel) //先将物体中心平移到世界坐标的原点 
                                                      //*osg::Matrixd::scale(0.5, 0.5, 0.5) //缩放 
                                                      *osg::Matrixd::rotate(osg::DegreesToRadians(-45.0f), 0, 0, 1)//旋转
                                                      //*osg::Matrixd::translate(1, 0, 0)//平移
                                                      *osg::Matrixd::translate(centermodel));//变换后再将物体移回
                                              }
                                              m_mt->setMatrix(m_mt->getMatrix()*osg::Matrixd::translate(-centermodel) //先将物体中心平移到世界坐标的原点 
                                                  //*osg::Matrixd::scale(0.5, 0.5, 0.5) //缩放 
                                                  //*osg::Matrixd::rotate(osg::DegreesToRadians(45.0f), 1, 0, 0)//旋转
                                                  *osg::Matrixd::translate(-0.015, 0, 0)//平移
                                                  *osg::Matrixd::translate(centermodel));//变换后再将物体移回
                                              num = 0;
                                              m_leftMouse = true; m_rightMouse = false;
                                          }
#endif
    }break;

    }

    return false;
}



bool crosswiseMoveHandler::downMove(int step){

    //osg::Vec3d centermodel = m_mt->getBound().center();

    //printf("model radius : %f,center:  %f,%f,%f\n", radius, centermodel.x(), centermodel.y(), centermodel.z());

    //得到相机默认的参数设置  
    osg::Vec3d eye, center, up;

    m_viewer->getCamera()->getViewMatrixAsLookAt(eye, center, up);
    //printf("1 ye: %f,%f,%f new\n", eye._v[0], eye._v[1], eye._v[2]);
    //printf("1 center: %f,%f,%f new\n\n", center._v[0], center._v[1], center._v[2]);

    osg::Vec3 neweyepint(eye.x(), eye.y(), eye.z() - m_stepZ*step);

    osg::Vec3 newcenter = osg::Vec3(neweyepint.x(), center.y(), neweyepint.z());

    //将参数设置给相机，并立即获取相机参数  
    m_viewer->getCamera()->setViewMatrixAsLookAt(neweyepint, m_modelcenter, up);


    //viewer->getCamera()->getViewMatrixAsLookAt(eye, center, up);

    ////将参数打印出来  
    //printf("2 eye: %f,%f,%f new\n", neweyepint._v[0], neweyepint._v[1], neweyepint._v[2]);
    //printf("2 center: %f,%f,%f new\n\n", m_modelcenter._v[0], m_modelcenter._v[1], m_modelcenter._v[2]);

    return true;
};


bool crosswiseMoveHandler::rightMove(){


    //得到相机默认的参数设置  
    osg::Vec3d eye, center, up;
    
    m_viewer->getCamera()->getViewMatrixAsLookAt(eye, center, up);

    osg::Vec3 neweyepint(eye.x() + m_stepX, eye.y(), eye.z());

    //osg::Vec3 newcenter = osg::Vec3(neweyepint.x(), center.y(), neweyepint.z());

    //将参数设置给相机，并立即获取相机参数  
    m_viewer->getCamera()->setViewMatrixAsLookAt(neweyepint, m_modelcenter, up);

    //m_viewer->getCamera()->getViewMatrixAsLookAt(eye, center, up);

    ////将参数打印出来  
    //printf("eye: %f,%f,%f new\n", eye._v[0], eye._v[1], eye._v[2]);
    //printf("center: %f,%f,%f new\n", center._v[0], center._v[1], center._v[2]);

    return true;

};

bool crosswiseMoveHandler::moveLeftBegin(int step){

    //得到相机默认的参数设置  
    osg::Vec3d eye, center, up;

    m_viewer->getCamera()->getViewMatrixAsLookAt(eye, center, up);

    osg::Vec3 neweyepint(eye.x() - m_stepX*step, eye.y(), eye.z());

    osg::Vec3 newcenter = osg::Vec3(neweyepint.x(), center.y(), neweyepint.z());

    //将参数设置给相机，并立即获取相机参数  
    m_viewer->getCamera()->setViewMatrixAsLookAt(neweyepint, m_modelcenter, up);

    return true;
}

bool crosswiseMoveHandler::initModelXLocation(float angle){

    //osg::Vec3d centermodel = m_mt->getBound().center();
    m_mt->setMatrix(m_mt->getMatrix()*osg::Matrixd::translate(-m_modelcenter) //先将物体中心平移到世界坐标的原点 
        //*osg::Matrixd::scale(0.5, 0.5, 0.5) //缩放 
        *osg::Matrixd::rotate(osg::DegreesToRadians(angle), 1, 0, 0)//旋转
        //*osg::Matrixd::translate(-0.015, 0, 0)//平移
        *osg::Matrixd::translate(m_modelcenter));//变换后再将物体移回

    return true;

}

bool crosswiseMoveHandler::XLevel(int step){

    //得到相机默认的参数设置  
    osg::Vec3d eye, center, up;

    m_viewer->getCamera()->getViewMatrixAsLookAt(eye, center, up);

    osg::Vec3 neweyepint(eye.x() - m_stepX*step, eye.y(), eye.z());

    osg::Vec3 newcenter = osg::Vec3(neweyepint.x(), center.y(), neweyepint.z());

    //将参数设置给相机，并立即获取相机参数  
    m_viewer->getCamera()->setViewMatrixAsLookAt(neweyepint, newcenter, up);

    return true;
}

bool crosswiseMoveHandler::ZLevel(int step){


    //得到相机默认的参数设置  
    osg::Vec3d eye, center, up;

    m_viewer->getCamera()->getViewMatrixAsLookAt(eye, center, up);

    osg::Vec3 neweyepint(eye.x(), eye.y(), eye.z() - m_stepZ*step);

    osg::Vec3 newcenter = osg::Vec3(neweyepint.x(), center.y(), neweyepint.z());

    //将参数设置给相机，并立即获取相机参数  
    m_viewer->getCamera()->setViewMatrixAsLookAt(neweyepint, newcenter, up);

    return true;
}
