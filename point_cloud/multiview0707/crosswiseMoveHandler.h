
#ifndef CROSSWISEMOVEHANDLER_H
#define CROSSWISEMOVEHANDLER_H

#include "base.h"
#include <FileLibrary.h>


//∫·œÚ“∆∂Ø…„œÒª˙
class crosswiseMoveHandler :public osgGA::GUIEventHandler{
public:
    crosswiseMoveHandler(osgViewer::Viewer *viewer, osg::Group *pgroup, string &nodepath,osg::Image *pimage, const string &);

    void init(const string &);

    ~crosswiseMoveHandler(){};
    
    virtual bool handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa);

    bool downMove(int step);

    bool rightMove();

    bool moveLeftBegin(int step);

    bool initModelXLocation(float angle);

    bool XLevel(int step);

    bool ZLevel(int step);


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
    bool m_leftMouse, m_rightMouse;
    osgViewer::Viewer *m_viewer;
	osg::Vec3d m_modelcenter;
};


#endif