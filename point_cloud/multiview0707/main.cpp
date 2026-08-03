#include <base.h>
#include <FileLibrary.h>
//#include <osgGA/TrackballManipulator>
#include "memoryFrameSink.h"
#include "multiviewBatchRenderer.h"
#include "multiviewRenderPlan.h"
#include "multiviewMemoryDump.h"
#include "multiviewGraphicsConfig.h"
#include <chrono>
#include <exception>
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

bool setMasterViewerGraphicsContext(osgViewer::Viewer *viewer, float x, float y, int width, int height, const MultiviewGraphicsConfig& config){

    osg::ref_ptr<osg::GraphicsContext::Traits> traits = new osg::GraphicsContext::Traits();
    traits->x = x;
    traits->y = y;
    traits->width = width;
    traits->height = height;
    traits->windowDecoration = config.windowDecoration;
	traits->doubleBuffer = config.doubleBuffer;
    traits->vsync = config.vsync;
    traits->pbuffer = config.pbuffer;
    traits->sharedContext = 0;
	//traits->supportsResize = false;
    //traits->samples = 8;
    traits->alpha = 1;
    //traits->stencil = 8;

    //创建图形环境特性
    osg::ref_ptr<osg::GraphicsContext> gc = osg::GraphicsContext::createGraphicsContext(traits.get());
    if (gc.valid() && gc->valid())
    {
        //清除窗口颜色及清除颜色和深度缓冲
        //gc->setClearColor(osg::Vec4f(0,0,0,1.0f));
        //gc->setClearMask(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    }
    else
    {
        osg::notify(osg::NOTICE) << "  GraphicsWindow has not been created successfully." << std::endl;
            return false;
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
    viewer->getCamera()->setDrawBuffer(config.drawBuffer == MultiviewDrawBufferFront ? GL_FRONT : GL_BACK);
    viewer->getCamera()->setReadBuffer(config.readBuffer == MultiviewDrawBufferFront ? GL_FRONT : GL_BACK);
	viewer->getCamera()->setGraphicsContext(gc);

    return true;
}

int main(int argc, char *argv[]){
    const auto programStart = std::chrono::high_resolution_clock::now();

    string dir, file, type, outdir, help, outputMode;
    int angle = 0, row = 0, resolution = 0, dumpMemory = 1;
    if (parser(argc, argv, "-h", help) >= 0)
    {
        cout << "-file: model path" << endl;;
        cout << "-dir: model dir" << endl;;
        cout << "-type:  model type(obj,ive,osgb)" << endl;;
        cout << "-outdir:  out dir  " << endl;;
        cout << "-pre: samples per degree, alias of -per" << endl;;
        cout << "-output: memory or legacy-jpg" << endl;;
        cout << "-dump-memory: 1 writes memory frames to -outdir after memory render, 0 disables dump" << endl;;
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
    if (parser(argc, argv, "-pre", row) >= 0)
    {

    }
    if (parser(argc, argv, "-resolution", resolution) >= 0)
    {

    }
    if (parser(argc, argv, "-output", outputMode) < 0)
    {
        outputMode = "memory";
    }
    if (parser(argc, argv, "-dump-memory", dumpMemory) >= 0)
    {

    }
    if (outputMode != "memory" && outputMode != "legacy-jpg")
    {
        cout << "invalid output mode = " << outputMode << endl;
        return 1;
    }
    if (dumpMemory != 0 && dumpMemory != 1)
    {
        cout << "invalid dump-memory = " << dumpMemory << endl;
        return 1;
    }
    if (angle <= 0 || row <= 0 || resolution <= 0)
    {
        cout << "invalid multiview parameters: angle=" << angle << ", pre=" << row << ", resolution=" << resolution << endl;
        return 1;
    }
    if (type != "obj" && type != "ive" && type != "osgb")
    {
        cout << "invalid model type = " << type << endl;
        return 1;
    }
    if (type == "obj" && !FileLibrary::getInstance()->isFileExists(file))
    {
        cout << "not find file = " << file << endl;
        return 1;
    }
    if ((type == "ive" || type == "osgb") && dir.empty())
    {
        cout << "model dir is required for type = " << type << endl;
        return 1;
    }

    const auto validationEnd = std::chrono::high_resolution_clock::now();
    const auto processStart = programStart;

    const auto viewerSetupStart = std::chrono::high_resolution_clock::now();
    osgViewer::Viewer *viewer = new osgViewer::Viewer;
    if (outputMode == "memory")
    {
        viewer->setThreadingModel(osgViewer::Viewer::SingleThreaded);
    }
    if (outputMode == "legacy-jpg")
    {
        viewer->addEventHandler(new osgGA::StateSetManipulator(viewer->getCamera()->getOrCreateStateSet()));
        viewer->addEventHandler(new osgViewer::ThreadingHandler);
        viewer->addEventHandler(new osgViewer::WindowSizeHandler);
        viewer->addEventHandler(new osgViewer::StatsHandler);
        viewer->addEventHandler(new osgViewer::RecordCameraPathHandler);
        viewer->addEventHandler(new osgViewer::LODScaleHandler);
        viewer->addEventHandler(new osgViewer::ScreenCaptureHandler);
    }

    //注释掉解决上下拉伸,可以不用
    viewer->setCameraManipulator(NULL); //固定物体，调试远近距离，不用鼠标移动时使用
    //viewer->setUpViewOnSingleScreen();
    
    //灰色背景
    viewer->getCamera()->setClearColor(osg::Vec4f(0.3f,0.3f,0.3f, 1.0f));
	//黑色背景
    //viewer->getCamera()->setClearColor(osg::Vec4f(0.0f, 0.0f, 0.0f, 1.0f));
    const MultiviewGraphicsConfig graphicsConfig = makeMultiviewGraphicsConfig(outputMode == "memory");
    if (!setMasterViewerGraphicsContext(viewer, 100, 100, resolution, resolution, graphicsConfig))
    {
        cout << "failed to create graphics context" << endl;
        return 1;
    }

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


    
    osg::Image *pImage = NULL;
    if (outputMode == "legacy-jpg")
    {
        pImage = new osg::Image;
    }   


    if (outputMode == "legacy-jpg")
    {
        viewer->getCamera()->setPostDrawCallback(new CaptureDrawCallback(pImage, resolution));
    }

    osg::StateSet* state = viewer->getCamera()->getOrCreateStateSet();
    //state->setMode(GL_BLEND, osg::StateAttribute::ON);
    state->setMode(GL_LIGHTING, osg::StateAttribute::OFF | osg::StateAttribute::PROTECTED);
    const auto viewerSetupEnd = std::chrono::high_resolution_clock::now();

    const auto modelLoadStart = std::chrono::high_resolution_clock::now();


    list<string> listfile;
    list<string>::iterator it;
    osg::Group *pgroup = new osg::Group;
    if (type == "obj")        
    {   
        osg::Node *pnode = osgDB::readNodeFile(file);
        if (pnode == NULL)
        {
            cout << "failed to load model file = " << file << endl;
            return 1;
        }
        pgroup->addChild(pnode);
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
            if (pnode == NULL)
            {
                continue;
            }
            pgroup->addChild(pnode);

        }
    }
    //移动相机时注释掉, 物体旋转时打开；固定物体不使用鼠标移动时注释掉；
    if (pgroup->getNumChildren() == 0)
    {
        cout << "no model nodes loaded" << endl;
        return 1;
    }

    const auto modelLoadEnd = std::chrono::high_resolution_clock::now();

    //viewer->setCameraManipulator(new osgGA::TrackballManipulator());

    const auto handlerSetupStart = std::chrono::high_resolution_clock::now();
    //模型旋转移动
    modelMoveHandler *moveHandler = new modelMoveHandler(viewer, pgroup, outdir, pImage, type, angle, row);
    if (outputMode == "legacy-jpg")
    {
        viewer->addEventHandler(moveHandler);
    }

    const auto handlerSetupEnd = std::chrono::high_resolution_clock::now();

    //横向移动camera
    //viewer->addEventHandler(new crosswiseMoveHandler(viewer, pgroup, objdir, pImage, type));    

	//viewer->setSceneData(pgroup); //for test

    //viewer->realize();

    if (outputMode == "memory")
    {
        try
        {
            const auto endToEndStart = std::chrono::high_resolution_clock::now();
            const auto planStart = std::chrono::high_resolution_clock::now();
            MultiviewRenderPlan plan(angle, row, resolution);
            const auto planEnd = std::chrono::high_resolution_clock::now();
            const auto sinkStart = std::chrono::high_resolution_clock::now();
            MemoryFrameSink sink(plan, true);
            const auto sinkEnd = std::chrono::high_resolution_clock::now();
            const auto rendererSetupStart = std::chrono::high_resolution_clock::now();
            MultiviewBatchRenderer renderer(viewer, moveHandler->modelTransform(), plan, &sink);
            const auto rendererSetupEnd = std::chrono::high_resolution_clock::now();
            const auto renderCallStart = std::chrono::high_resolution_clock::now();
            MultiviewBatchStats stats = renderer.renderAll();
            const auto renderCallEnd = std::chrono::high_resolution_clock::now();
            const bool renderComplete = stats.framesRendered == plan.frameCount() &&
                                        stats.framesCaptured == plan.frameCount() &&
                                        stats.bytesCaptured == plan.totalBytes();
            MultiviewMemoryDumpStats dumpStats = {};
            if (dumpMemory != 0 && renderComplete)
            {
                dumpStats = dumpMultiviewMemoryFrames(plan, sink, outdir);
            }
            const auto endToEndEnd = std::chrono::high_resolution_clock::now();
        const std::chrono::duration<double> endToEndElapsed = endToEndEnd - endToEndStart;
        const std::chrono::duration<double> processElapsed = endToEndEnd - processStart;

        cout << "multiview_validation_seconds=" << std::chrono::duration<double>(validationEnd - programStart).count() << endl;
        cout << "multiview_viewer_setup_seconds=" << std::chrono::duration<double>(viewerSetupEnd - viewerSetupStart).count() << endl;
        cout << "multiview_model_load_seconds=" << std::chrono::duration<double>(modelLoadEnd - modelLoadStart).count() << endl;
        cout << "multiview_handler_setup_seconds=" << std::chrono::duration<double>(handlerSetupEnd - handlerSetupStart).count() << endl;
        cout << "multiview_plan_seconds=" << std::chrono::duration<double>(planEnd - planStart).count() << endl;
        cout << "multiview_memory_allocate_seconds=" << std::chrono::duration<double>(sinkEnd - sinkStart).count() << endl;
        cout << "multiview_renderer_setup_seconds=" << std::chrono::duration<double>(rendererSetupEnd - rendererSetupStart).count() << endl;
        cout << "multiview_render_call_seconds=" << std::chrono::duration<double>(renderCallEnd - renderCallStart).count() << endl;
        cout << "multiview_dump_enabled=" << dumpMemory << endl;
        cout << "multiview_dump_directory=" << outdir << endl;
        cout << "multiview_dump_frames_written=" << dumpStats.framesWritten << endl;
        cout << "multiview_dump_write_errors=" << dumpStats.writeErrors << endl;
        cout << "multiview_dump_seconds=" << dumpStats.seconds << endl;
        cout << "multiview_graphics_pbuffer=" << graphicsConfig.pbuffer << endl;
            cout << "multiview_graphics_double_buffer=" << graphicsConfig.doubleBuffer << endl;
            cout << "multiview_graphics_vsync=" << graphicsConfig.vsync << endl;
            cout << "multiview_graphics_window_decoration=" << graphicsConfig.windowDecoration << endl;
            cout << "multiview_memory_frames=" << stats.framesRendered << endl;
            cout << "multiview_memory_frames_captured=" << stats.framesCaptured << endl;
        cout << "multiview_memory_bytes=" << stats.bytesCaptured << endl;
        cout << "multiview_readback_errors=" << stats.readbackErrors << endl;
        cout << "multiview_render_seconds=" << stats.renderSeconds << endl;
        cout << "multiview_readback_seconds=" << stats.readbackSeconds << endl;
        cout << "multiview_total_seconds=" << stats.totalSeconds << endl;
        cout << "multiview_end_to_end_seconds=" << endToEndElapsed.count() << endl;
        cout << "multiview_process_seconds=" << processElapsed.count() << endl;

            if (!renderComplete ||
                (dumpMemory != 0 &&
                 (dumpStats.framesWritten != plan.frameCount() || dumpStats.writeErrors != 0)))
            {
                return 1;
            }

            return 0;
        }
        catch (const std::exception& ex)
        {
            cout << "multiview_error=" << ex.what() << endl;
            return 1;
        }
    }

    while (!viewer->done())
    {
        viewer->frame();
        //viewer->run();
    }





}
#endif

