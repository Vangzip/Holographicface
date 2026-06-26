// testreadpcd.cpp : 定义控制台应用程序的入口点。
//


#include "base.h"
//#include "scearthlibrary.h"

#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>
#include <osg/Node>
#include <osg/Geode>
#include <osg/Geometry>
#include <osgDB/ReaderWriter>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgGA/StateSetManipulator>
#include <osgUtil/Optimizer>
#include <osgUtil/DelaunayTriangulator>

int main(int argc, char *argv[]){

    if (argc < 2)
    {
        return 0;
    }

    string plyfile = argv[1];
    float size = atof(argv[2]);
    string outfile;

    PointTRGBPtr src_cloud(new PointTRGB);
    PointTRGBPtr tmp_cloud(new PointTRGB);
    if (pcl::io::loadPLYFile(plyfile, *src_cloud) < 0)
    {

        cout  << "load ply error. file :"<< plyfile << endl;
    }
    cout  << "point size:" << src_cloud->points.size() << endl;

    pcl::VoxelGrid<pcl::PointXYZRGB> vox_grid;
    vox_grid.setInputCloud(src_cloud);
    vox_grid.setLeafSize(size, size, size);
    vox_grid.filter(*tmp_cloud);

    src_cloud = tmp_cloud;
    cout  << "vox point size:" << src_cloud->points.size()<< endl;


    osg::ref_ptr<osgViewer::Viewer> viewer = new osgViewer::Viewer();
    viewer->addEventHandler(new osgGA::StateSetManipulator(viewer->getCamera()->getOrCreateStateSet()));
    
    // add the thread model handler
    viewer->addEventHandler(new osgViewer::ThreadingHandler);
    // add the window size toggle handler
    viewer->addEventHandler(new osgViewer::WindowSizeHandler);
    // add the stats handler
    viewer->addEventHandler(new osgViewer::StatsHandler);
    // add the record camera path handler
    viewer->addEventHandler(new osgViewer::RecordCameraPathHandler);
    // add the LOD Scale handler
    viewer->addEventHandler(new osgViewer::LODScaleHandler);
    // add the screen capture handler
    viewer->addEventHandler(new osgViewer::ScreenCaptureHandler);

    viewer->setCameraManipulator(NULL);

    osg::ref_ptr<osg::Group> root = new osg::Group();
    //准备点云数据数组
    osg::ref_ptr<osg::Vec3Array> coords = new osg::Vec3Array();
    osg::ref_ptr<osg::Vec4Array> color = new osg::Vec4Array();

    int nums = src_cloud->size();
    std::cout << "点云数据：" << nums << std::endl;
    int k = 0;
    for (int i = 0; i < nums; ++i)
    {
        coords->push_back(osg::Vec3(src_cloud->points[i].x, src_cloud->points[i].y, src_cloud->points[i].z));
        //color->push_back(osg::Vec4(src_cloud->points[i].r, src_cloud->points[i].g, src_cloud->points[i].b, src_cloud->points[i].a));
        float r = src_cloud->points[i].r;
        float g = src_cloud->points[i].g;
        float b = src_cloud->points[i].b;

        color->push_back(osg::Vec4(r, g, b , 1));
        //color->push_back(osg::Vec4(1,0,0, 1.0));
        k++;
    }

    osg::Geometry *geometry = new osg::Geometry;
    //设置点云几何数据
    geometry->setVertexArray(coords.get());
    geometry->setColorArray(color.get());
    geometry->setColorBinding(osg::Geometry::BIND_PER_VERTEX);

    osg::Vec3Array *normals = new osg::Vec3Array;
    normals->push_back(osg::Vec3(0.0f, -1.0f, 0.0f));

    geometry->setNormalArray(normals);
    geometry->setNormalBinding(osg::Geometry::BIND_OVERALL);
    geometry->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::POINTS, 0, k)); //设置点绘制方式
    
    //geometry->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::QUADS, 0, 4)); //设置四边形绘制方式
    //添加到叶子节点
    osg::ref_ptr<osg::Geode> geode = new osg::Geode();
    geode->addDrawable(geometry);
    root->addChild(geode.get());

    //outfile = SCEarthLibrary::getInstance()->getFileParentPath(plyfile) + "\\test.osg";
    //osgDB::writeNodeFile(*root, outfile);


    //优化场景数据
    osgUtil::Optimizer optimizer;
    optimizer.optimize(root.get());
    viewer->setSceneData(root.get());
    viewer->setUpViewOnSingleScreen();
    viewer->realize();
    viewer->run();
    
    
    while (!viewer->done())
    {
        viewer->frame();
    }


    return 0;
}
