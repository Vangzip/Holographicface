#include <base.h>
#include <FileLibrary.h>
//#include <osgGA/TrackballManipulator>
#include "modelMoveHandler.h"
#include "crosswiseMoveHandler.h"
#if 1
//#include <osg/Camera>





template<typename Type> 
int parser(int argc, char *argv[], const string &name, Type &value){
    osg::ArgumentParser arguments(&argc, argv);

    int index = arguments.find(name) + 1;

    if (index > 0 && index < argc)
    {
        std::istringstream stream;
        stream.clear();
        stream.str(argv[index]);
        stream >> value;
    }

    return (index - 2 );
};

bool setMasterViewerGraphicsContext(osgViewer::Viewer *viewer, float x, float y, int width, int height){

    osg::ref_ptr<osg::GraphicsContext::Traits> traits = new osg::GraphicsContext::Traits();
    traits->x = x;
    traits->y = y;
    traits->width = width;
    traits->height = height;
    traits->windowDecoration = true;
	traits->doubleBuffer = true;
    traits->sharedContext = 0;
	//traits->supportsResize = false;
    //traits->samples = 8;
    traits->alpha = 1;
    //traits->stencil = 8;

    //创建图形环境特性
    osg::ref_ptr<osg::GraphicsContext> gc = osg::GraphicsContext::createGraphicsContext(traits.get());
    if (gc->valid())
    {
        //清除窗口颜色及清除颜色和深度缓冲
        //gc->setClearColor(osg::Vec4f(0,0,0,1.0f));
        //gc->setClearMask(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    }
    else
    {
        osg::notify(osg::NOTICE) << "  GraphicsWindow has not been created successfully." << std::endl;
    }

	//根据分辨率来确定合适的投影来保证显示的图形不变形
	double fovy, aspectRatio, zNear, zFar;
	viewer->getCamera()->getProjectionMatrixAsPerspective(fovy, aspectRatio, zNear, zFar);
	double newAspectRatio = double(traits->width) / double(traits->height);
	double aspectRatioChange = newAspectRatio / aspectRatio;
	if (aspectRatioChange != 1.0)
	{
		//设置投影矩阵
		viewer->getCamera()->getProjectionMatrix() *= osg::Matrix::scale(1.0 / aspectRatioChange, 1.0, 1.0);
	}
    viewer->getCamera()->setViewport(new osg::Viewport(0, 0, width, height));
	viewer->getCamera()->setGraphicsContext(gc);

    return true;
}

int main(int argc, char *argv[]){

    string dir, file, type, outdir,help;
    int angle, row, resolution;
    if (parser(argc, argv, "-h", help) >= 0)
    {
        cout << "-file: model path" << endl;;
        cout << "-dir: model dir" << endl;;
        cout << "-type:  model type(obj,ive,osgb)" << endl;;
        cout << "-outdir:  out dir  " << endl;;
        return 0;
    }

    if (parser(argc, argv,"-file", file) >= 0)
    {

    }
    if (parser(argc, argv, "-dir", dir) >= 0)
    {

    }
    if (parser(argc, argv, "-type", type) >= 0)
    {

    }
    if (parser(argc, argv, "-outdir", outdir) >= 0)
    {

    }
    if (parser(argc, argv, "-angle", angle) >= 0)
    {

    }
    if (parser(argc, argv, "-per", row) >= 0)
    {

    }
    if (parser(argc, argv, "-resolution", resolution) >= 0)
    {

    }

	if (!FileLibrary::getInstance()->isFileExists(file))
	{
		cout << "not find file = " << file << endl;
		return 0;
	}

    osgViewer::Viewer *viewer = new osgViewer::Viewer;


	viewer->addEventHandler(new osgGA::StateSetManipulator(viewer->getCamera()->getOrCreateStateSet()));
    // add the thread model handler
    viewer->addEventHandler(new osgViewer::ThreadingHandler);
    //add the window size toggle handler
    viewer->addEventHandler(new osgViewer::WindowSizeHandler);
    //add the stats handler
    viewer->addEventHandler(new osgViewer::StatsHandler);
    //add the record camera path handler
    viewer->addEventHandler(new osgViewer::RecordCameraPathHandler);
    // add the LOD Scale handler
    viewer->addEventHandler(new osgViewer::LODScaleHandler);
    // add the screen capture handler
    viewer->addEventHandler(new osgViewer::ScreenCaptureHandler);

    //注释掉解决上下拉伸,可以不用
    viewer->setCameraManipulator(NULL); //固定物体，调试远近距离，不用鼠标移动时使用
    //viewer->setUpViewOnSingleScreen();
    
    //灰色背景
    //viewer->getCamera()->setClearColor(osg::Vec4f(0.3f,0.3f,0.3f, 1.0f));
	//黑色背景
    viewer->getCamera()->setClearColor(osg::Vec4f(0.0f, 0.0f, 0.0f, 1.0f));
    setMasterViewerGraphicsContext(viewer, 100, 100, resolution, resolution);

#if 0	
    //设置光照
    osg::ref_ptr<osg::Light> light = new osg::Light;
    osg::Vec4 vec1(255 / 255.0f, 248 / 255.0f, 220 / 255.0f, 1.0f);
    osg::Vec4 vec2(156 / 255.0f, 156 / 255.0f, 156 / 255.0f, 1.0f);
    light->setDiffuse(vec2);
    light->setAmbient(vec1);
    light->setPosition(osg::Vec4(0, 0, 200, 1.0f));

    viewer->setLight(light);
    viewer->setLightingMode(osg::View::LightingMode::HEADLIGHT);
#endif


    
    osg::Image *pImage = new osg::Image;   


    viewer->getCamera()->setPostDrawCallback(new CaptureDrawCallback(pImage, resolution));

    //osg::StateSet* state = viewer->getCamera()->getOrCreateStateSet();
    //state->setMode(GL_BLEND, osg::StateAttribute::ON);
    //state->setMode(GL_LIGHTING, osg::StateAttribute::OFF | osg::StateAttribute::PROTECTED);


    list<string> listfile;
    list<string>::iterator it;
    osg::Group *pgroup = new osg::Group;
    if (type == "obj")        
    {   
        pgroup->addChild(osgDB::readNodeFile(file));
    }
    else if (type == "ive")
    {

        FileLibrary::getInstance()->getAllSubFiles(dir, listfile, false, true, false, ".ive");
        for (it = listfile.begin(); it != listfile.end();it++)
        {

            osg::Node *pnode = osgDB::readNodeFile(*it);
            if (pnode == NULL)
            {
                continue;
            }

            pgroup->addChild(pnode);

        }
    }
    else if (type == "osgb") 
    {
        FileLibrary::getInstance()->getAllSubFiles(dir, listfile, true, false, false);
        for (it = listfile.begin(); it != listfile.end(); it++){
            string tmpdir = *it;
            string dirname = FileLibrary::getInstance()->getFileNameFromPath(*it);
            string osdbfilepath = *it + "\\" + dirname + ".osgb";

            //cout << osdbfilepath << endl;
            if (!FileLibrary::getInstance()->isFileExists(osdbfilepath))
            {
                continue;
            }

            osg::Node *pnode = osgDB::readNodeFile(osdbfilepath);
            pgroup->addChild(pnode);

        }
    }
    //移动相机时注释掉, 物体旋转时打开；固定物体不使用鼠标移动时注释掉；
    //viewer->setCameraManipulator(new osgGA::TrackballManipulator());

    //模型旋转移动
    viewer->addEventHandler(new modelMoveHandler(viewer, pgroup, outdir, pImage, type, angle, row));

    //横向移动camera
    //viewer->addEventHandler(new crosswiseMoveHandler(viewer, pgroup, objdir, pImage, type));    

	//viewer->setSceneData(pgroup); //for test

    //viewer->realize();

    while (!viewer->done())
    {
        viewer->frame();
        //viewer->run();
    }





}
#endif

