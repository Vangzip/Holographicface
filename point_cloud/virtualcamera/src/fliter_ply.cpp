#include "base.h"
#include "FileLibrary.h"
#include "ConverPointCloud.h"
#include "pclbase.h"

#if 0
void mesh2VRML(const pcl::PolygonMesh &inPutmesh, const string &file) {

    vtkSmartPointer<vtkPolyData> input;
    pcl::VTKUtils::mesh2vtk(inPutmesh, input);
    

    //obj->add

    vtkSmartPointer<vtkFillHolesFilter> fillHolesFilter = vtkSmartPointer<vtkFillHolesFilter>::New();

    fillHolesFilter->SetInputData(input);
    fillHolesFilter->SetHoleSize(0.02);//0.005
    fillHolesFilter->Update();

    //vtkSmartPointer<vtkPolyData> polyData = fillHolesFilter->GetOutput();


    // Make the triangle windong order consistent
    vtkSmartPointer<vtkPolyDataNormals> normals = vtkSmartPointer<vtkPolyDataNormals>::New();
    normals->SetInputConnection(fillHolesFilter->GetOutputPort());
    normals->ConsistencyOn();
    normals->SplittingOff();
    normals->Update();


    // Restore the original normals
    normals->GetOutput()->GetPointData()->SetNormals(input->GetPointData()->GetNormals());

    // Visualize
    // Define viewport ranges
    // (xmin, ymin, xmax, ymax)
    double leftViewport[4] = { 0.0, 0.0, 0.5, 1.0 };

    // Create a mapper and actor
    vtkSmartPointer<vtkPolyDataMapper> originalMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    if (originalMapper == NULL)
    {
        return;
    }
#if VTK_MAJOR_VERSION <= 5
    originalMapper->SetInputConnection(input->GetProducerPort());
#else
    originalMapper->SetInputData(input);
#endif

    vtkSmartPointer<vtkProperty> backfaceProp = vtkSmartPointer<vtkProperty>::New();
    backfaceProp->SetDiffuseColor(0.89, 0.81, 0.34);

    vtkSmartPointer<vtkActor> originalActor = vtkSmartPointer<vtkActor>::New();
    originalActor->SetMapper(originalMapper);
    originalActor->SetBackfaceProperty(backfaceProp);
    originalActor->GetProperty()->SetDiffuseColor(1.0, 0.3882, 0.2784);

    vtkSmartPointer<vtkPolyDataMapper> filledMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    filledMapper->SetInputConnection(normals->GetOutputPort());

    vtkSmartPointer<vtkActor> filledActor = vtkSmartPointer<vtkActor>::New();
    filledActor->SetMapper(filledMapper);
    filledActor->GetProperty()->SetDiffuseColor(1.0, 0.3882, 0.2784);

    // Create a renderer, render window, and interactor
    vtkSmartPointer<vtkRenderer> leftRenderer = vtkSmartPointer<vtkRenderer>::New();
    leftRenderer->SetViewport(leftViewport);


    vtkSmartPointer<vtkRenderWindow> renderWindow = vtkSmartPointer<vtkRenderWindow>::New();
    //renderWindow->SetSize(600, 600);

    renderWindow->AddRenderer(leftRenderer);

    vtkSmartPointer<vtkRenderWindowInteractor> renderWindowInteractor = vtkSmartPointer<vtkRenderWindowInteractor>::New();
    renderWindowInteractor->SetRenderWindow(renderWindow);

    // Add the actor to the scene
    leftRenderer->AddActor(originalActor);
    leftRenderer->SetBackground(.3, .6, .3); // Background color green

    //leftRenderer->GetActiveCamera()->SetPosition(0, -1, 0);
    //leftRenderer->GetActiveCamera()->SetFocalPoint(0, 0, 0);
    ////leftRenderer->GetActiveCamera()->SetViewUp(0, 0, 1);
    //leftRenderer->GetActiveCamera()->Azimuth(30);
    //leftRenderer->GetActiveCamera()->Elevation(30);

    //leftRenderer->ResetCamera();     

    // Render and interact
    //renderWindow->Render();

    //renderWindowInteractor->Start();

    vtkSmartPointer<vtkVRMLExporter> importer = vtkSmartPointer<vtkVRMLExporter>::New();
    importer->SetFileName(file.c_str());
    importer->SetRenderWindow(renderWindow);
    importer->Write();

#if 0
    // Restore the original normals
    normals->GetOutput()->GetPointData()->SetNormals(input->GetPointData()->GetNormals());

    // Visualize
    // Define viewport ranges
    // (xmin, ymin, xmax, ymax)

    vtkSmartPointer<vtkPolyDataMapper> filledMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    //filledMapper->SetInputData(input);
    filledMapper->SetInputConnection(normals->GetOutputPort());


    // Create a mapper and actor
    vtkSmartPointer<vtkPolyDataMapper> originalMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
#if VTK_MAJOR_VERSION <= 5
    originalMapper->SetInputConnection(input->GetProducerPort());
#else
    originalMapper->SetInputData(input);
#endif

    vtkSmartPointer<vtkProperty> backfaceProp = vtkSmartPointer<vtkProperty>::New();
    backfaceProp->SetDiffuseColor(0.89, 0.81, 0.34);

    vtkSmartPointer<vtkActor> originalActor = vtkSmartPointer<vtkActor>::New();
    originalActor->SetMapper(filledMapper);
    originalActor->SetBackfaceProperty(backfaceProp);
    originalActor->GetProperty()->SetDiffuseColor(1.0, 0.3882, 0.2784);

    double leftViewport[4] = { 0.0, 0.0, 0.0, 1.0 };
    // Create a renderer, render window, and interactor
    vtkSmartPointer<vtkRenderer> leftRenderer = vtkSmartPointer<vtkRenderer>::New();
    leftRenderer->SetViewport(leftViewport);
    leftRenderer->AddActor(originalActor);

    leftRenderer->SetBackground(.3, .6, .3); // Background color green

    leftRenderer->GetActiveCamera()->SetPosition(0, -1, 0);
    leftRenderer->GetActiveCamera()->SetFocalPoint(0, 0, 0);
    leftRenderer->GetActiveCamera()->SetViewUp(0, 0, 1);
    leftRenderer->GetActiveCamera()->Azimuth(30);
    leftRenderer->GetActiveCamera()->Elevation(30);

    vtkSmartPointer<vtkRenderWindow> renderWindow = vtkSmartPointer<vtkRenderWindow>::New();
    //renderWindow->SetSize(600, 300);

    renderWindow->AddRenderer(leftRenderer);

    vtkSmartPointer<vtkRenderWindowInteractor> renderWindowInteractor = vtkSmartPointer<vtkRenderWindowInteractor>::New();
    renderWindowInteractor->SetRenderWindow(renderWindow);

    // Render and interact
    renderWindow->Render();


    renderWindowInteractor->Start();

    //vtkSmartPointer<vtkVRMLExporter> importer = vtkSmartPointer<vtkVRMLExporter>::New();
    //importer->SetRenderWindow();

#endif
}

int main(int argc, char *argv[]){
    string type = argv[1];
    string plyfile = argv[2];
    string outfile;

    if (!FileLibrary::getInstance()->isFileExists(plyfile))
    {

        cout << COUT_PREFIX << "not find : "<< plyfile << endl;
        return -1;
    }

#if 0
    PointTRGBPtr src_cloud(new PointTRGB);
    PointTRGBPtr tgt_cloud(new PointTRGB);
    

    pcl::io::loadPLYFile(plyfile, *tgt_cloud);
    cout << COUT_PREFIX << "target point: " << tgt_cloud->points.size() << endl;

    float rgb = atof(argv[3]);
    for (size_t i = 0; i < src_cloud->points.size(); i++)
    {
        pcl::PointXYZRGB Point = src_cloud->points[i];
        //if (Point.g - Point.r < rgb && Point.g - Point.b < rgb)
        
        
        if (type == "1" && Point.r < rgb && Point.g < rgb && Point.b < rgb)
        {
            continue;

        }
        else  if (type == "2" && Point.g - Point.r > rgb && Point.g - Point.b >rgb )
        {
            continue;

        }

        


        tgt_cloud->points.push_back(Point);
        
    }



    pcl::StatisticalOutlierRemoval<pcl::PointXYZRGB> sor;  //创建滤波器对象
    sor.setInputCloud(tgt_cloud);                     //设置待滤波的点云
    sor.setMeanK(atof(argv[4]));                                //设置在进行统计时考虑查询点临近点数
    sor.setStddevMulThresh(3);            //设置判断是否为离群点的阀值
    sor.filter(*tgt_cloud);                        //存储

    PointTRGBPtr tmp_src_cloud(new PointTRGB);
    pcl::VoxelGrid<pcl::PointXYZRGB> vox_grid;
    vox_grid.setLeafSize(0.002, 0.002, 0.002);

    vox_grid.setInputCloud(tgt_cloud);
    vox_grid.filter(*tmp_src_cloud);
    tgt_cloud = tmp_src_cloud;
    cout << COUT_PREFIX << "vox point: " << tgt_cloud->points.size() << endl;


    //string outfile = FileLibrary::getInstance()->getFileParentPath(plyfile) + "\\fliter_point.ply";
    string outfile = plyfile.substr(0,plyfile.find(".ply"))+"_1.ply";
    pcl::io::savePLYFile(outfile, *tgt_cloud);

    //cout << COUT_PREFIX << "target point: "<<tgt_cloud->points.size() << endl;

    PointTRGBNPtr n(new PointTRGBN);

    PCLBASE::getInstance()->normalsMovingLeastSquares(tgt_cloud, *n, 0.002);

    pcl::PolygonMesh triangles;
    /*曲面重建模块*/
    pcl::search::KdTree<pcl::PointXYZRGBNormal>::Ptr tree2(new pcl::search::KdTree<pcl::PointXYZRGBNormal>);
    pcl::Poisson<pcl::PointXYZRGBNormal> pn;
    pn.setConfidence(true);    //是否使用法向量的大小作为置信信息。如果false，所有法向量均归一化。
    pn.setDegree(2);            //设置参数degree[1,5],值越大越精细，耗时越久。
    pn.setDepth(8);             //树的最大深度，求解2^d x 2^d x 2^d立方体元。由于八叉树自适应采样密度，指定值仅为最大深度。
    pn.setIsoDivide(8);         //用于提取ISO等值面的算法的深度
    pn.setManifold(true);      //是否添加多边形的重心，当多边形三角化时。 设置流行标志，如果设置为true，则对多边形进行细分三角话时添加重心，设置false则不添加
    pn.setOutputPolygons(false); //是否输出多边形网格（而不是三角化移动立方体的结果）
    pn.setSamplesPerNode(3);  //设置落入一个八叉树结点中的样本点的最小数量。无噪声，[1.0-5.0],有噪声[15.-20.]平滑
    pn.setScale(1.1); //设置用于重构的立方体直径和样本边界立方体直径的比率。
    pn.setSolverDivide(8); //设置求解线性方程组的Gauss-Seidel迭代方法的深度
    pn.setPointWeight(4.0);

    //pn.setIndices();

    //设置搜索方法和输入点云
    pn.setSearchMethod(tree2);
    pn.setInputCloud(n);
    //执行重构
    pn.reconstruct(triangles);

    cout << COUT_PREFIX << "poisson mesh reconstruct ok . point size = " << triangles.cloud.height*triangles.cloud.width << ", polygon size:" << triangles.polygons.size() << endl;

    outfile = plyfile.substr(0, plyfile.find(".ply")) + "_mesh.ply";
    pcl::io::savePLYFile(outfile, triangles);

    
#endif

    pcl::PolygonMesh triangles;
    pcl::io::loadPLYFile(plyfile, triangles);
    outfile = plyfile.substr(0, plyfile.find(".ply")) + "_mesh_vrml.wrl";
    mesh2VRML(triangles, outfile);


    return 0;
}


#endif