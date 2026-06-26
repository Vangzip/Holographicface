#include "poissonmesh.hpp"   

#include <pcl/surface/simplification_remove_unused_vertices.h>
#include <pcl\filters\crop_box.h>

texturemeshPoisson::texturemeshPoisson(const std::string &point_ply,const std::string &mesh_ply)
{
	 f_ = 0.1;
	 vector_field_ = Eigen::Vector3f (1, 0, 0); 
     m_strDirectoryPath = FileLibrary::getInstance()->getFileParentPath(point_ply);

     m_strPolygonMeshFile = mesh_ply;
     m_strPointcloudfile = point_ply;

}

texturemeshPoisson::~texturemeshPoisson()
{

}


int texturemeshPoisson::meshCropHull(){

    pcl::PointCloud<pcl::PointXYZRGB>::Ptr cloudSegmented(new pcl::PointCloud<pcl::PointXYZRGB>);

    
    if (m_strPointcloudfile.find(".pcd") != std::string::npos)
    {
        if (pcl::io::loadPCDFile<pcl::PointXYZRGB>(m_strPointcloudfile, *cloudSegmented) == -1)
        {
            PCL_ERROR("Couldn't read file mypointcloud.pcd\n");
            return -1;
        }
    }
    else
    {
        if (pcl::io::loadPLYFile<pcl::PointXYZRGB>(m_strPointcloudfile, *cloudSegmented) == -1)
        {
            PCL_ERROR("Couldn't read file mypointcloud.pcd\n");
            return -1;
        }
    }

   // meshCropHull(cloudSegmented);

    return 0;
}

int texturemeshPoisson::meshCropHull(pcl::PointCloud<pcl::PointXYZRGBNormal>::Ptr &cloudSegmented)
{
    //pcl::PointCloud<pcl::PointXYZ>::Ptr cloudSegmented(new pcl::PointCloud<pcl::PointXYZ>);

    src_cloud = PointTRGBNPtr(new PointTRGBN);
    src_cloud = cloudSegmented;

    /*
    if (m_strPointcloudfile.find(".pcd") != std::string::npos)
    {
    if (pcl::io::loadPCDFile<pcl::PointXYZ>(m_strPointcloudfile, *cloudSegmented) == -1)
    {
    PCL_ERROR("Couldn't read file mypointcloud.pcd\n");
    return -1;
    }
    }
    else
    {
    if (pcl::io::loadPLYFile<pcl::PointXYZ>(m_strPointcloudfile, *cloudSegmented) == -1)
    {
    PCL_ERROR("Couldn't read file mypointcloud.pcd\n");
    return -1;
    }
    }*/



    //poisson point cloud
    pcl::PointCloud<pcl::PointXYZRGBNormal>::Ptr trianglesPoints(new pcl::PointCloud<pcl::PointXYZRGBNormal>);
    getPoissonPoint(*trianglesPoints);

    if (trianglesPoints->points.size() == 0)
    {
        cout << COUT_PREFIX << "get poisson pont size is 0 ." << endl;
        return false;

    }

    pcl::PointCloud<pcl::PointXYZRGBNormal>::Ptr outPutPoint(new pcl::PointCloud<pcl::PointXYZRGBNormal>);
    std::vector<pcl::Vertices> polygons;
    pcl::PointCloud<pcl::PointXYZRGBNormal>::Ptr surfaceHullPoint(new pcl::PointCloud<pcl::PointXYZRGBNormal>);

    pcl::ConvexHull<pcl::PointXYZRGBNormal> hull;
    hull.setDimension(3);
    hull.setInputCloud(cloudSegmented);
    hull.reconstruct(*surfaceHullPoint, polygons);

    //pcl::io::savePLYFile(m_strDirectoryPath + "\\ConvexHull.ply", *surfaceHullPoint);

    //boost::shared_ptr<pcl::visualization::PCLVisualizer> viewer(new pcl::visualization::PCLVisualizer("3D Viewer"));
    //viewer->setBackgroundColor(0, 0, 0.6);

    //pcl::visualization::PointCloudColorHandlerCustom<pcl::PointXYZ> transformed_cloud_color_handler1(230, 20, 20); //   
    //pcl::visualization::PointCloudColorHandlerCustom<pcl::PointXYZ> transformed_cloud_color_handler2(0, 255, 255); //   

    //viewer->addPolygon<PointXYZ>(cloudSegmented, 0, 255, 255, "vex_polygon");
    //viewer->addPolygonMesh<pcl::PointXYZ>(surfaceHullPoint, polygons, "vex_polygon"); 

    //viewer->addPointCloud<pcl::PointXYZ>(trianglesPoints, transformed_cloud_color_handler1,"1");
    //viewer->addPointCloud<pcl::PointXYZ>(cloudSegmented, transformed_cloud_color_handler2, "2");


    //viewer->setPointCloudRenderingProperties(pcl::visualization::PCL_VISUALIZER_POINT_SIZE, 3, "sample cloud");
    //viewer->addCoordinateSystem(1.0);

    pcl::CropHull<pcl::PointXYZRGBNormal> cropHull;
    //pcl::CropBox<pcl::PointXYZ> cropHull;

    cropHull.setKeepOrganized(true);
    cropHull.setCropOutside(true);
    cropHull.setDim(3);
    cropHull.setInputCloud(trianglesPoints);
    cropHull.setHullIndices(polygons);
    cropHull.setHullCloud(surfaceHullPoint);

    //std::vector<int> point_indices;    
    cropHull.filter(m_crophullpointindex);       //得到包围盒内部点索引
    cropHull.filter(*outPutPoint);

    //int size = cropHull.getRemovedIndices()->size();

    //pcl::io::savePLYFile(m_strDirectoryPath + "\\crophull.ply", *outPutPoint);


    std::vector<int>::iterator it;
    for (size_t i = 0; i < trianglesPoints->points.size(); i++)
    {
        bool isIndex = false;
        for (it = m_crophullpointindex.begin(); it != m_crophullpointindex.end(); it++)
        {
            int index = *it;
            if (i == index)
            {
                isIndex = true;
                break;
            }
        }
        if (!isIndex)
        {
            m_crophulloutpointindex.push_back(i);

        }
    }

    //合并原点云中的rgb信息到mesh中
    mergeMeshRGB(m_polygonmeshptr, src_cloud, m_crophullpointindex);



    pcl::PolygonMesh outPolygonMesh;
    getCropHullMesh(outPolygonMesh);

    cout << COUT_PREFIX << "point size :" << outPolygonMesh.cloud.width << "  polygons:" << outPolygonMesh.polygons.size() << endl;

    //string meshfile = m_strPolygonMeshFile.substr(0, m_strPolygonMeshFile.find_last_of("_"))+"_poisson_mesh.ply";
    //pcl::io::savePLYFile(m_strPolygonMeshFile, outPolygonMesh);
    m_polygonmesh = outPolygonMesh;


    //while (!viewer->wasStopped())
    //{
    //    viewer->spinOnce(100);
    //    boost::this_thread::sleep(boost::posix_time::microseconds(100000));
    //}


    return 0;


}

#if 0
int texturemeshPoisson::meshCropHull(pcl::PointCloud<pcl::PointXYZRGB>::Ptr &cloudSegmented)
{
    //pcl::PointCloud<pcl::PointXYZ>::Ptr cloudSegmented(new pcl::PointCloud<pcl::PointXYZ>);

    src_cloud = PointTRGBPtr(new PointTRGB);
    src_cloud = cloudSegmented;

/*
    if (m_strPointcloudfile.find(".pcd") != std::string::npos)
    {
        if (pcl::io::loadPCDFile<pcl::PointXYZ>(m_strPointcloudfile, *cloudSegmented) == -1)
        {
            PCL_ERROR("Couldn't read file mypointcloud.pcd\n");
            return -1;
        }
    }
    else
    {
        if (pcl::io::loadPLYFile<pcl::PointXYZ>(m_strPointcloudfile, *cloudSegmented) == -1)
        {
            PCL_ERROR("Couldn't read file mypointcloud.pcd\n");
            return -1;
        }
    }*/

#if 0

    // Compute principal directions，  minimum oriented bounding box 
    Eigen::Vector4f pcaCentroid;
    pcl::compute3DCentroid(*cloudSegmented, pcaCentroid);
    Eigen::Matrix3f covariance;
    computeCovarianceMatrixNormalized(*cloudSegmented, pcaCentroid, covariance);

    Eigen::SelfAdjointEigenSolver<Eigen::Matrix3f> eigen_solver(covariance, Eigen::ComputeEigenvectors);
    Eigen::Matrix3f eigenVectorsPCA = eigen_solver.eigenvectors();
    eigenVectorsPCA.col(2) = eigenVectorsPCA.col(0).cross(eigenVectorsPCA.col(1));


    Eigen::Matrix4f projectionTransform = Eigen::Matrix4f::Identity();
    projectionTransform.block<3, 3>(0, 0) = eigenVectorsPCA.transpose();
    projectionTransform.block<3, 1>(0, 3) = -1.0f * (projectionTransform.block<3, 3>(0, 0) * pcaCentroid.head<3>());
    pcl::PointCloud<pcl::PointXYZ>::Ptr cloudPointsProjected(new pcl::PointCloud<pcl::PointXYZ>);

    pcl::transformPointCloud(*cloudSegmented, *cloudPointsProjected, projectionTransform);
    // Get the minimum and maximum points of the transformed cloud.
    pcl::PointXYZ minPoint, maxPoint;
    pcl::getMinMax3D(*cloudPointsProjected, minPoint, maxPoint);
    const Eigen::Vector3f meanDiagonal = 0.5f*(maxPoint.getVector3fMap() + minPoint.getVector3fMap());

    // Final transform
    const Eigen::Quaternionf bboxQuaternion(eigenVectorsPCA); 
    const Eigen::Vector3f bboxTransform = eigenVectorsPCA * meanDiagonal + pcaCentroid.head<3>();

    boost::shared_ptr<pcl::visualization::PCLVisualizer> viewer(new pcl::visualization::PCLVisualizer("3D Viewer"));
    viewer->setBackgroundColor(0, 0, 0.6);

    //viewer->addPointCloud<pcl::PointXYZ>(cloudSegmented);

    double width = maxPoint.x - minPoint.x;
    double height = maxPoint.y - minPoint.y;
    double depth = maxPoint.z - minPoint.z;
    //OBB
    //viewer->addCube(bboxTransform, bboxQuaternion, maxPoint.x - minPoint.x, maxPoint.y - minPoint.y, maxPoint.z - minPoint.z, "bbox");
    // axis aligned boounding box, AABB
    //viewer->addCube(minPoint.x, maxPoint.x, minPoint.y, maxPoint.y, minPoint.z, maxPoint.z);

    //viewer->setShapeRenderingProperties(pcl::visualization::PCL_VISUALIZER_REPRESENTATION, pcl::visualization::PCL_VISUALIZER_REPRESENTATION_WIREFRAME, "bbox");

/*
    pcl::PointCloud<pcl::PointXYZ>::Ptr bb(new pcl::PointCloud<pcl::PointXYZ>);
    pcl::PointXYZ dd[8];
    dd[0].x = maxPoint.x;
    dd[0].y = maxPoint.y;
    dd[0].z = maxPoint.z;

    dd[1].x = maxPoint.x-width;
    dd[1].y = maxPoint.y;
    dd[1].z = maxPoint.z;

    dd[2].x = maxPoint.x;
    dd[2].y = maxPoint.y-height;
    dd[2].z = maxPoint.z;

    dd[3].x = maxPoint.x-width;
    dd[3].y = maxPoint.y-height;
    dd[3].z = maxPoint.z;

    dd[4].x = minPoint.x+width;
    dd[4].y = minPoint.y;
    dd[4].z = minPoint.z;

    dd[5].x = minPoint.x;
    dd[5].y = minPoint.y+height;
    dd[5].z = minPoint.z;

    dd[6].x = minPoint.x+width;
    dd[6].y = minPoint.y+height;
    dd[6].z = minPoint.z;

    dd[7].x = minPoint.x;
    dd[7].y = minPoint.y;
    dd[7].z = minPoint.z;

    bb->push_back(dd[0]);
    bb->push_back(dd[1]);
    bb->push_back(dd[2]);
    bb->push_back(dd[3]);
    bb->push_back(dd[4]);
    bb->push_back(dd[5]);
    bb->push_back(dd[6]);
    bb->push_back(dd[7]);
*/

#endif
 
    //poisson point cloud
    pcl::PointCloud<pcl::PointXYZRGB>::Ptr trianglesPoints(new pcl::PointCloud<pcl::PointXYZRGB>);
    getPoissonPoint(*trianglesPoints);
    if (trianglesPoints->points.size() == 0)
    {
        cout << COUT_PREFIX << "get poisson pont size is 0 ." << endl;
        return false;

    }

    pcl::PointCloud<pcl::PointXYZRGB>::Ptr outPutPoint(new pcl::PointCloud<pcl::PointXYZRGB>);
    std::vector<pcl::Vertices> polygons;
    pcl::PointCloud<pcl::PointXYZRGB>::Ptr surfaceHullPoint(new pcl::PointCloud<pcl::PointXYZRGB>);

    pcl::ConvexHull<pcl::PointXYZRGB> hull;
    hull.setDimension(3);
    hull.setInputCloud(cloudSegmented);
    hull.reconstruct(*surfaceHullPoint, polygons);

    //pcl::io::savePLYFile(m_strDirectoryPath + "\\ConvexHull.ply", *surfaceHullPoint);

    //boost::shared_ptr<pcl::visualization::PCLVisualizer> viewer(new pcl::visualization::PCLVisualizer("3D Viewer"));
    //viewer->setBackgroundColor(0, 0, 0.6);

    //pcl::visualization::PointCloudColorHandlerCustom<pcl::PointXYZ> transformed_cloud_color_handler1(230, 20, 20); //   
    //pcl::visualization::PointCloudColorHandlerCustom<pcl::PointXYZ> transformed_cloud_color_handler2(0, 255, 255); //   

    //viewer->addPolygon<PointXYZ>(cloudSegmented, 0, 255, 255, "vex_polygon");
    //viewer->addPolygonMesh<pcl::PointXYZ>(surfaceHullPoint, polygons, "vex_polygon"); 
    
    //viewer->addPointCloud<pcl::PointXYZ>(trianglesPoints, transformed_cloud_color_handler1,"1");
    //viewer->addPointCloud<pcl::PointXYZ>(cloudSegmented, transformed_cloud_color_handler2, "2");


    //viewer->setPointCloudRenderingProperties(pcl::visualization::PCL_VISUALIZER_POINT_SIZE, 3, "sample cloud");
    //viewer->addCoordinateSystem(1.0);

    pcl::CropHull<pcl::PointXYZRGB> cropHull;
    //pcl::CropBox<pcl::PointXYZ> cropHull;
    
    cropHull.setKeepOrganized(true);
    cropHull.setCropOutside(true);
    cropHull.setDim(3);
    cropHull.setInputCloud(trianglesPoints);
    cropHull.setHullIndices(polygons);
    cropHull.setHullCloud(surfaceHullPoint);
    
    //std::vector<int> point_indices;    
    cropHull.filter(m_crophullpointindex);       //得到包围盒内部点索引
    cropHull.filter(*outPutPoint);

    //int size = cropHull.getRemovedIndices()->size();

    //pcl::io::savePLYFile(m_strDirectoryPath + "\\crophull.ply", *outPutPoint);


    std::vector<int>::iterator it;
    for (size_t i = 0; i < trianglesPoints->points.size(); i++)
    {
        bool isIndex = false;
        for (it = m_crophullpointindex.begin() ; it != m_crophullpointindex.end(); it++)
        {
            int index = *it;
            if (i == index)
            {
                isIndex = true;
                break;
            }
        }
        if (!isIndex)
        {
            m_crophulloutpointindex.push_back(i);

        }
    }

    //合并原点云中的rgb信息到mesh中
    mergeMeshRGB(m_polygonmeshptr, src_cloud, m_crophullpointindex);

#if 0
    PointIndices index;;
    int size = cropHull.getRemovedIndices()->size();
    cropHull.getRemovedIndices(index);

    pcl::io::savePLYFile(directoryPath + "crophull.ply", *outPutPoint);

    // Finish

    pcl::PolygonMesh poisson_mesh;
    pcl::io::loadPLYFile(directoryPath+"\\rotate_DSC_1190_00069.tif_pixel=2_patch6_7x7_reduction_mesh.ply", poisson_mesh);

    pcl::PointCloud<pcl::PointXYZ>::Ptr poisson_point(new pcl::PointCloud<pcl::PointXYZ>);
    pcl::fromPCLPointCloud2(m_polygonmeshptr.cloud, *poisson_point);
#endif

#if 0                         


    std::vector<pcl::Vertices> visibleFaces;

    std::vector<pcl::Vertices, std::allocator<pcl::Vertices>>::iterator face;
    for (face = m_polygonmeshptr.polygons.begin(); face != m_polygonmeshptr.polygons.end(); ++face)
    {
        //unsigned int v1 = face->vertices[0];
        //unsigned int v2 = face->vertices[1];
        //unsigned int v3 = face->vertices[2];

        bool isface[3] = { false };

        for (size_t i = 0; i < 3; i++)
        {
            for (size_t j = 0; j < m_crophullpointindex.size(); j++)
            {
                int index = m_crophullpointindex[j];
                if (face->vertices[i] == index)
                {
                    isface[i] = true;
                    break;
                }
            }
        }

        if (isface[0] && isface[1] && isface[2])
        {
            visibleFaces.push_back(*face);
        }


    }



    m_polygonmeshptr.polygons.clear();
    m_polygonmeshptr.polygons.insert(m_polygonmeshptr.polygons.begin(), visibleFaces.begin(), visibleFaces.end());

    pcl::PolygonMesh visible(m_polygonmeshptr);
    pcl::surface::SimplificationRemoveUnusedVertices cleaner;
    pcl::PolygonMesh aa;
    cleaner.simplify(visible, aa);

#endif

    pcl::PolygonMesh outPolygonMesh;
    getCropHullMesh(outPolygonMesh);

    cout << COUT_PREFIX << "point size :" << outPolygonMesh.cloud.width << "  polygons:" << outPolygonMesh.polygons.size() << endl;

    //string meshfile = m_strPolygonMeshFile.substr(0, m_strPolygonMeshFile.find_last_of("_"))+"_poisson_mesh.ply";
    pcl::io::savePLYFile(m_strPolygonMeshFile, outPolygonMesh);
    //m_polygonmesh = outPolygonMesh;


    //while (!viewer->wasStopped())
    //{
    //    viewer->spinOnce(100);
    //    boost::this_thread::sleep(boost::posix_time::microseconds(100000));
    //}


    return 0;


}
#endif
bool texturemeshPoisson::getCropHullMesh(pcl::PolygonMesh &outmeshfile){


    //pcl::PolygonMesh poisson_mesh;
    //pcl::io::loadPLYFile(poissonmeshfile, poisson_mesh);
    pcl::PointCloud<pcl::PointXYZRGB>::Ptr poisson_point(new pcl::PointCloud<pcl::PointXYZRGB>);
    pcl::fromPCLPointCloud2(m_polygonmeshptr.cloud, *poisson_point);


    std::vector<pcl::Vertices> visibleFaces;

    std::vector<pcl::Vertices, std::allocator<pcl::Vertices>>::iterator face;
    for (face = m_polygonmeshptr.polygons.begin(); face != m_polygonmeshptr.polygons.end(); ++face)
    {
        //unsigned int v1 = face->vertices[0];
        //unsigned int v2 = face->vertices[1];
        //unsigned int v3 = face->vertices[2];

        bool isface[3] = { false };

        for (size_t i = 0; i < 3; i++)
        {
            for (size_t j = 0; j < m_crophulloutpointindex.size(); j++)
            {
                int index = m_crophulloutpointindex[j];
                if (face->vertices[i] == index)
                {
                    isface[i] = true;
                    break;
                }
                
            }
        }

        if (!isface[0] && !isface[1] && !isface[2])
        {
            visibleFaces.push_back(*face);
        }


    }



    m_polygonmeshptr.polygons.clear();
    m_polygonmeshptr.polygons.insert(m_polygonmeshptr.polygons.begin(), visibleFaces.begin(), visibleFaces.end());


    //this should remove unused vertices 
    pcl::PolygonMesh visible(m_polygonmeshptr);
    pcl::surface::SimplificationRemoveUnusedVertices cleaner;
    cleaner.simplify(visible, outmeshfile);

    m_polygonmesh = outmeshfile;
    //cout << "polygons:" << poisson_mesh.polygons.size() << endl;

    //pcl::io::savePLYFile("e:\\poisson_mesh.ply", poisson_mesh);

    return true;
}

bool texturemeshPoisson::mergeMeshRGB(pcl::PolygonMesh &mesh, PointTRGBPtr &cloud, vector<int> index_vector){
    return false;
    PointTRGBPtr poisson_point(new PointTRGB);
    pcl::fromPCLPointCloud2(mesh.cloud, *poisson_point);

    for (size_t i = 0; i < index_vector.size(); i++)
    {
        int index = index_vector[i];
        pcl::PointXYZRGB point = cloud->points[index];
        poisson_point->points[index].r = point.r;
        poisson_point->points[index].g = point.g;
        poisson_point->points[index].b = point.b;
        poisson_point->points[index].a = point.a;
        
    }
    pcl::toPCLPointCloud2(*poisson_point,mesh.cloud);

    return true;
}

bool texturemeshPoisson::mergeMeshRGB(pcl::PolygonMesh &mesh, PointTRGBNPtr &cloud, vector<int> index_vector){
    return false;
    PointTRGBNPtr poisson_point(new PointTRGBN);
    pcl::fromPCLPointCloud2(mesh.cloud, *poisson_point);

    for (size_t i = 0; i < index_vector.size(); i++)
    {
        int index = index_vector[i];
        pcl::PointXYZRGBNormal point = cloud->points[index];
        poisson_point->points[index].r = point.r;
        poisson_point->points[index].g = point.g;
        poisson_point->points[index].b = point.b;
        poisson_point->points[index].a = point.a;

        poisson_point->points[i].normal_x = point.normal_x;
        poisson_point->points[i].normal_y = point.normal_y;
        poisson_point->points[i].normal_z = point.normal_z;


    }
    pcl::toPCLPointCloud2(*poisson_point, mesh.cloud);

    return true;
}

bool texturemeshPoisson::getPolygonMesh(pcl::PolygonMesh &mesh){
    mesh = m_polygonmesh;
    return true;
}
int texturemeshPoisson::MLSPointCloud(const std::string &pcdFilenName)
{

    typedef pcl::PointXYZRGB Point;
    typedef pcl::PointCloud<Point> PointCloud;
    typedef pcl::Normal PointNormal;
    typedef pcl::PointCloud<PointNormal> PointCloudNormal;
    typedef pcl::PointXYZRGBNormal PointRGBNormal;
    typedef pcl::PointCloud<PointRGBNormal> PointCloudRGBNormal;

    std::string filePath = m_strDirectoryPath + pcdFilenName;
    std::string sampleOutPutPoint = m_strDirectoryPath + pcdFilenName.substr(0, pcdFilenName.find_first_of('.')) + "_downsample.ply";

    std::string removeOutPutPoint = m_strDirectoryPath + pcdFilenName.substr(0, pcdFilenName.find_first_of('.')) + "_remove.ply";

    std::string plyFileMLSoutPutPoint = m_strDirectoryPath + pcdFilenName.substr(0, pcdFilenName.find_first_of('.')) + "_MLS.ply";

    pcl::PointCloud<Point>::Ptr cloud_src(new pcl::PointCloud<Point>);
    pcl::PointCloud<Point>::Ptr cloud_remove(new pcl::PointCloud<Point>);
    pcl::PointCloud<Point>::Ptr cloud_tgt(new pcl::PointCloud<Point>);



    pcl::io::loadPLYFile(filePath, *cloud_src);



    pcl::PointCloud<Point>::Ptr cloud_filtered(new pcl::PointCloud<Point>);


    // 创建滤波器对象
    pcl::VoxelGrid<Point> sor;//滤波处理对象
    sor.setInputCloud(cloud_src);
    sor.setLeafSize(0.0003f, 0.0003f, 0.0003f);//设置滤波器处理时采用的体素大小的参数   0.00015 = 大约 9:1 
    sor.filter(*cloud_filtered);

    std::cerr << "PointCloud after filtering: " << cloud_filtered->width * cloud_filtered->height
        << " data points (" << pcl::getFieldsList(*cloud_filtered) << ").";


    pcl::io::savePLYFile(sampleOutPutPoint, *cloud_filtered);



    pcl::StatisticalOutlierRemoval<Point> removeFilter;// 创建滤波器对象
    removeFilter.setInputCloud(cloud_filtered);                        //设置呆滤波的点云
    removeFilter.setMeanK(300);                                //设置在进行统计时考虑查询点邻近点数
    removeFilter.setStddevMulThresh(1.0);                    //设置判断是否为离群点的阈值
    removeFilter.filter(*cloud_remove);                    //执行滤波处理保存内点到cloud_filtered

    pcl::io::savePLYFile(removeOutPutPoint, *cloud_remove);

    pcl::search::KdTree<Point>::Ptr tree(new pcl::search::KdTree<Point>);
    PointCloudRGBNormal mls_points;
    pcl::MovingLeastSquares<Point, PointRGBNormal> mls;

    //mls.setComputeNormals(true);
    //mls.setInputCloud(cloud_src);
    //mls.setPolynomialFit(true); //对于法线的估计是有多项式还是仅仅依靠切线。
    //mls.setSearchMethod(tree); // 使用kdTree加速搜索
    //mls.setPolynomialOrder(2);
    //
    //mls.setSearchRadius(0.03); //数值越大，输出的点越多，确定搜索的半径。也就是说在这个半径里进行表面映射和曲面拟合。从实验结果可知：半径越小拟合后曲面的失真度越小，反之有可能出现过拟合的现象。


    //setUpsamplingMethod这个函数比较特殊，他会调用不同的枚举变量， 每个枚举变量有对应的几个不同的函数，因此这里我将一一解释。经过试验证明：这个upsampling函数只能增加密度较小区域的密度对于holes的填补却无能为力（本来想着用之填补点云缺失的部分，却发现此函数并没有那么强大）。接下来将会一一介绍四个不同的方法。

    //mls.setUpsamplingMethod(mls.VOXEL_GRID_DILATION); //这个方法有两个步骤：首先将点云以voxels分割，然后进行迭代使得voxels的数目增加。它的结果是：填充空洞和平均化点云的密度。它需要调用的函数为：
    //mls.setDilationVoxelSize(1);   //设定voxel的大小。
    //mls.setDilationIterations(2); //设置迭代的次数

    //mls.setUpsamplingMethod(mls.SAMPLE_LOCAL_PLANE);// 这个方法就是参考论文中采用的方法，当然此方法所需的计算强度也相当庞大。若使用此方法，将需要调用两个函数：
    //mls.setUpsamplingStepSize(0.05);//对于每个子点云处理时迭代的步长。
    //mls.setUpsamplingRadius(0.025);//此函数规定了点云增长的区域。可以这样理解：把整个点云按照此半径划分成若干个子点云，然后一一索引进行点云增长。

    //mls.setUpsamplingMethod(mls.RANDOM_UNIFORM_DENSITY);   //也是使用上面子点云的原理，只不过它使得稀疏区域的密度增加，从而使得整个点云的密度均匀
    //mls.setPointDensity(20);  //注意此函数输入整型变量，意为半径内点的个数。（这个半径应该是search的半径，不需要重新设置）。

    mls.setInputCloud(cloud_filtered);
    mls.setSearchMethod(tree);
    mls.setPolynomialFit(true);
    mls.setPolynomialOrder(4);
    mls.setPointDensity(20);
    mls.setComputeNormals(true);
    mls.setSearchRadius(0.001);
    mls.setUpsamplingMethod(mls.NONE);
    //mls.setUpsamplingRadius(0.1);
    //mls.setUpsamplingStepSize(4.5);

    clock_t start = clock();
    mls.process(mls_points);
    clock_t end = clock();
    std::cout << (end - start) / CLOCKS_PER_SEC << "s used!" << std::endl;



    // 将mls后的结果xyzrgbNormal转换成为xyzrgb
    cloud_tgt->points.resize(mls_points.size());
    cout << "Point cloud count: ." << mls_points.size() << endl;

    for (size_t i = 0; i < mls_points.size(); ++i)
    {
        cloud_tgt->points[i].x = mls_points.points[i].x;
        cloud_tgt->points[i].y = mls_points.points[i].y;
        cloud_tgt->points[i].z = mls_points.points[i].z;
        cloud_tgt->points[i].r = mls_points.points[i].r;
        cloud_tgt->points[i].g = mls_points.points[i].g;
        cloud_tgt->points[i].b = mls_points.points[i].b;
    }
    cloud_tgt->width = mls_points.size();
    cloud_tgt->height = 1;

    // Save outPutPoint
    //pcl::io::savePCDFile(pcdFileMLSoutPutPoint, *cloud_tgt);
    pcl::io::savePLYFile(plyFileMLSoutPutPoint, *cloud_tgt);

    return 0;

}

//设置泊松后的mesh文件
bool texturemeshPoisson::setPlygonMeshFile(const string &meshfile){
    m_strPolygonMeshFile = meshfile;
    return true;
}
//设置泊松后的polygonsmesh对象
bool texturemeshPoisson::setPolygonMesh(const pcl::PolygonMesh &mesh){
    m_polygonmeshptr = mesh;
    return true;
}



bool texturemeshPoisson::getPoissonPoint(pcl::PointCloud<pcl::PointXYZRGBNormal> & mesh_point){

    if (m_polygonmeshptr.polygons.size() == 0)
    {
        
        if (!FileLibrary::getInstance()->isFileExists(m_strPolygonMeshFile))
        {
            cout << COUT_PREFIX << "no file . file = " << m_strPolygonMeshFile << endl;
            return false;
        }

        if (pcl::io::loadPLYFile(m_strPolygonMeshFile, m_polygonmeshptr) != 0){
            cout << COUT_PREFIX << "load file false. file = "<< m_strPolygonMeshFile << endl;
            return false;
    
        }

    }

    pcl::fromPCLPointCloud2(m_polygonmeshptr.cloud, mesh_point);
    
    return true;
}