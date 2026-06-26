

#include "ConverPointCloud.h"
#include "poissonmesh.hpp"
#include <pcl/surface/simplification_remove_unused_vertices.h>



void removeOutsideRadius(const pcl::PointCloud<PointXYZ>::Ptr &src_point, pcl::PolygonMesh &poisson_mesh, double radius)
{
    //vector for storing visible polygons 
    std::vector<pcl::Vertices> visibleFaces;
    pcl::PointCloud<pcl::PointXYZ> visiblePoints;
    pcl::PointCloud< pcl::PointXYZ> meshCloud;        
    pcl::fromPCLPointCloud2(poisson_mesh.cloud, meshCloud);

    pcl::search::KdTree<pcl::PointXYZ> search;
    search.setInputCloud(src_point);

    std::vector<int> indices(1);
    std::vector<float> distance(0.00008);

    //foreach face 
    std::vector<pcl::Vertices, std::allocator<pcl::Vertices>>::iterator face;
    for (face = poisson_mesh.polygons.begin(); face != poisson_mesh.polygons.end(); ++face)
    {
        bool isInside = true;
        //foreach vertex 
        unsigned int v1 = face->vertices[0];
        unsigned int v2 = face->vertices[1];
        unsigned int v3 = face->vertices[2];

        pcl::PointXYZ p1 = meshCloud.points.at(v1);
        pcl::PointXYZ p2 = meshCloud.points.at(v2);
        pcl::PointXYZ p3 = meshCloud.points.at(v3);

        for (size_t i = 0; i < 3; i++)
        {                                                                             

            for (int j = 0; j < src_point->points.size(); j++){
                if (src_point->points[j].x == meshCloud.points.at(face->vertices[i]).x && src_point->points[j].y == meshCloud.points.at(face->vertices[i]).y && src_point->points[j].z == meshCloud.points.at(face->vertices[i]).z)
                {
                    isInside = true;
                    break;
                }
                else
                {
                    isInside = false;
                }
            
            }
        }


        //int r1 = search.nearestKSearch(p1, 1, indices, distance);
        //int r2 = search.nearestKSearch(p2, 1, indices, distance);
        //int r3 = search.nearestKSearch(p3, 1, indices, distance);

        //if each vertex of face is inside sphere then keep it 
        if (isInside)
        {
            //save points 
            visiblePoints.points.push_back(p1);
            visiblePoints.points.push_back(p2);
            visiblePoints.points.push_back(p3);

            //save face 
            visibleFaces.push_back(*face);
        }
    }

    pcl::toPCLPointCloud2(visiblePoints, poisson_mesh.cloud);

    poisson_mesh.polygons.clear();
    poisson_mesh.polygons.insert(poisson_mesh.polygons.begin(), visibleFaces.begin(), visibleFaces.end());



}

#if 0
void removeOutsideRadius(pcl::PolygonMesh &mesh, pcl::PolygonMesh &outmesh, double radius)
{
    //vector for storing visible polygons 
    std::vector<pcl::Vertices> visibleFaces;
    pcl::PointCloud< pcl::PointXYZ> meshCloud;        
    pcl::fromPCLPointCloud2(mesh.cloud, meshCloud);
    //cloud for storing visible points 
    pcl::PointCloud<pcl::PointXYZ> visiblePoints;

    double rSquared = radius * radius;

    //foreach face 
    std::vector<pcl::Vertices, std::allocator<pcl::Vertices>>::iterator face;
    for (face = mesh.polygons.begin(); face != mesh.polygons.end(); ++face)
    {
        bool isInside = true;
        //foreach vertex 
        unsigned int v1 = face->vertices[0];
        unsigned int v2 = face->vertices[1];
        unsigned int v3 = face->vertices[2];

        pcl::PointXYZ p1 = meshCloud.points.at(v1);
        pcl::PointXYZ p2 = meshCloud.points.at(v2);
        pcl::PointXYZ p3 = meshCloud.points.at(v3);

        isInside = isInside && ((p1.x * p1.x + p1.y * p1.y + p1.z * p1.z) < rSquared);
        isInside = isInside && ((p2.x * p2.x + p2.y * p2.y + p2.z * p2.z) < rSquared);
        isInside = isInside && ((p3.x * p3.x + p3.y * p3.y + p3.z * p3.z) < rSquared);

        //if each vertex of face is inside sphere then keep it 
        if (isInside)
        {
            //save points 
            visiblePoints.points.push_back(p1);
            visiblePoints.points.push_back(p2);
            visiblePoints.points.push_back(p3);

            //save face 
            visibleFaces.push_back(*face);
        }
    }

    //save visible points to mesh 
    //meshCloud.clear(); 
    //meshCloud.insert(meshCloud.begin(), visiblePoints.begin(), visiblePoints.end()); 

    //pcl::toROSMsg(visiblePoints, mesh.cloud); 
    //pcl::toPCLPointCloud2(visiblePoints, mesh.cloud);

    //save visible faces to mesh 
    mesh.polygons.clear();
    mesh.polygons.insert(mesh.polygons.begin(), visibleFaces.begin(), visibleFaces.end());

    //this should remove unused vertices 
    //pcl::PolygonMesh visible(mesh);
    //pcl::surface::SimplificationRemoveUnusedVertices cleaner;
    //cleaner.simplify(visible, mesh);



}
#endif

ConverPointCloud::ConverPointCloud(){

	 //m_bundleResizedTo = 80, m_textureWithSize = 0, m_textureResolution = 1024;
	 m_kSearch = 20, m_type = 1;
     m_focal_length = 105;

     m_mu = m_searchRadius = m_nearestNeighbors = m_maxSurfaceAngle = m_minAngle = m_maxAngle = m_leafsize =0;
     m_upsamplingType = m_upsamplingRadius = m_upsamplingStepSize =0;
}


ConverPointCloud::~ConverPointCloud(){}




bool ConverPointCloud::parseArguments(const string &plyfile){

    string parentdir = FileLibrary::getInstance()->getFileParentPath(plyfile);
    string flyname = FileLibrary::getInstance()->getFileNameFromPath(plyfile);

    //m_strTexturepng = plyfile.substr(0, plyfile.find_last_of("_"))+".png";// parentdir + "\\" + flyname.substr(0, flyname.length() - 4) + ".png";

    string configfile = FileLibrary::getInstance()->getFileParentPath(parentdir) + "\\config.cfg";
    if (!FileLibrary::getInstance()->isFileExists(configfile))
    {
        cout << COUT_PREFIX << "config file no find. file =" << configfile << endl;
        return false;
    }


    ifstream iff(configfile);
    string line;
    while (getline(iff, line))
    {
        if (line.empty())
        {
            continue;
        }

        int pos = line.find("=") + 1;
        if (line.find("reconstruct=") != string::npos)
        {
            string tmp = line.substr(pos, line.length() - pos);

            m_type = atof(line.substr(pos, line.length() - pos).c_str());
            cout << m_type << " ";
        }else if (line.find("mlsSearchRadius=") != string::npos){
            m_mlsSearchRadius = atof(line.substr(pos, line.length() - pos).c_str());
        }
        else  if (line.find("kSearch=") != string::npos)
        {
            m_kSearch = atof(line.substr(pos, line.length() - pos).c_str());
            cout << m_kSearch << " ";
        }
        else if (line.find("searchradius=") != string::npos)
        {
            string tmp = line.substr(pos, line.length() - pos);

            m_searchRadius = atof(line.substr(pos, line.length() - pos).c_str());
            cout << m_searchRadius << " ";
        }
        else if (line.find("mu=") != string::npos)
        {
            m_mu = atof(line.substr(pos, line.length() - pos).c_str());
            cout << m_mu << " ";
        }
        else  if (line.find("maximumNearestNeighbors=") != string::npos)
        {
            m_nearestNeighbors = atof(line.substr(pos, line.length() - pos).c_str());
            cout << m_nearestNeighbors << " ";

        }
        else if (line.find("maximumSurfaceAngle=") != string::npos)
        {
            m_maxSurfaceAngle = atof(line.substr(pos, line.length() - pos).c_str());
            cout << m_maxSurfaceAngle << " ";
        }
        else  if (line.find("minimumAngle=") != string::npos)
        {
            m_minAngle = atof(line.substr(pos, line.length() - pos).c_str());
            cout << m_minAngle << " ";

        }
        else if (line.find("maximumAngle=") != string::npos)
        {
            m_maxAngle = atof(line.substr(pos, line.length() - pos).c_str());
            cout << m_maxAngle << " ";
        }
        else if (line.find("holesize=") != string::npos)
        {
            m_holesize = atof(line.substr(pos, line.length() - pos).c_str());
        }
        else if (line.find("nearest_distance=") != string::npos)
        {
            m_distance = atof(line.substr(pos, line.length() - pos).c_str());
        }
        else if (line.find("normalsFitIter1=") != string::npos)
        {
            m_normalsIter1 = atof(line.substr(pos, line.length() - pos).c_str());
        }
        else if (line.find("normalsFitIter2=") != string::npos)
        {
            m_normalsIter2 = atof(line.substr(pos, line.length() - pos).c_str());
        }
        else if (line.find("focus=") != string::npos)
        {
            m_focal_length = atof(line.substr(pos, line.length() - pos).c_str());
        }
        else if (line.find("neighbor_num=") != string::npos)
        {
            m_neighbor_num = atof(line.substr(pos, line.length() - pos).c_str());
        }
        else if (line.find("leafsize=") != string::npos)
        {
            m_leafsize = atof(line.substr(pos, line.length() - pos).c_str());
        }
        else if (line.find("upsamplingType=") != string::npos)        //mls upsampling param
        {
            m_upsamplingType = atoi(line.substr(pos, line.length() - pos).c_str());
        }
        else if (line.find("upsamplingStepSize=") != string::npos)
        {
            m_upsamplingStepSize = atof(line.substr(pos, line.length() - pos).c_str());
        }
        else if (line.find("upsamplingRadius=") != string::npos)
        {
            m_upsamplingRadius = atof(line.substr(pos, line.length() - pos).c_str());
        }
        else if (line.find("dilationVoxelSize=") != string::npos)
        {
            m_dilationVoxelSize = atof(line.substr(pos, line.length() - pos).c_str());
        }
        else if (line.find("dilationIterations=") != string::npos)
        {
            m_dilationIterations = atof(line.substr(pos, line.length() - pos).c_str());
        }
        else if (line.find("pointDensity=") != string::npos)
        {
            m_pointDensity = atof(line.substr(pos, line.length() - pos).c_str());
        }






    }
	iff.close();
    cout << endl;
	return true;
}

//bool ConverPointCloud::createPoissonMesh(const string &filepath){
//    return true;
//}
#if 1
//泊松算法生成mesh
bool ConverPointCloud::createPoissonMesh(const string &filepath){
    std::string srcfile = filepath;

    pcl::PointCloud<pcl::PointXYZRGBNormal>::Ptr cloud_filtered(new pcl::PointCloud<pcl::PointXYZRGBNormal>);
    pcl::PointCloud<pcl::PointXYZRGBNormal>::Ptr cloud(new pcl::PointCloud<pcl::PointXYZRGBNormal>);

    if (pcl::io::loadPLYFile(filepath, *cloud) != 0){
        cout << COUT_PREFIX << "load ply false. file = "<< filepath << endl;
        return false;
    }

    cout << COUT_PREFIX << "read ply file ok . point size = " << cloud->points.size()<< endl;

    if (m_leafsize != 0)
    {
        // 创建滤波器对象
        pcl::VoxelGrid<PointXYZRGBNormal> sor;//滤波处理对象
        sor.setInputCloud(cloud);
        sor.setLeafSize(m_leafsize, m_leafsize, m_leafsize);//设置滤波器处理时采用的体素大小的参数   0.00015 = 大约 9:1 
        sor.filter(*cloud_filtered);

        cout << COUT_PREFIX << "VoxelGrid file ok . point size = " << cloud_filtered->points.size() << endl;
        cloud = cloud_filtered;
        //string outfile = FileLibrary::getInstance()->getFileParentPath(filepath)+"\\vox_point.ply";
        //pcl::io::savePLYFile(outfile, *cloud);
                                                                                     
    }
#if 0


    pcl::PointCloud<pcl::PointXYZRGBNormal> mls_points;
    /*pcl::search::KdTree<pcl::PointXYZ>::Ptr tree(new pcl::search::KdTree<pcl::PointXYZ>);
    tree->setInputCloud(cloud);*/
    //pcl::KdTreeFLANN<pcl::PointXYZ> kdtree;

    //
    // Init object (second point type is for the normals, even if unused)
    //最小二乘法迭代拟合平滑点云
    for (size_t i = 0; i < m_normalsIter1; i++)
    {
        normalsMovingLeastSquares(cloud, mls_points);

        getPointFromPointNormal(cloud, mls_points);
    }
    if (mls_points.points.size() == 0)
    {
        cout << "mls false. point size = " << mls_points.points.size() << endl;
        return 0;
    }

    //法线拟合 compite normals from point // meshlab function
    fitNormal(mls_points);
    nearestKSearchNormal(mls_points);
    //pcl::io::savePLYFile(srcfile.substr(0, srcfile.find_last_of("_"))+"_mls_1.ply", mls_points);
    
    cout << COUT_PREFIX << "MLS normal ok . point size = " << mls_points.points.size() << endl;

    // 创建同时包含点和法向的数据结构的指针
    pcl::PointCloud<pcl::PointXYZRGBNormal>::Ptr cloud_with_normals(new pcl::PointCloud<pcl::PointXYZRGBNormal>(mls_points));
#endif
    pcl::search::KdTree<pcl::PointXYZRGBNormal>::Ptr tree(new pcl::search::KdTree<pcl::PointXYZRGBNormal>);
    pcl::PolygonMesh triangles;
    tree->setInputCloud(cloud);

    // 创建另一个kdtree用于重建
    //为kdtree输入点云数据,该点云数据类型为点和法向
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
    pn.setSearchMethod(tree);
    pn.setInputCloud(cloud);
    //执行重构
    pn.reconstruct(triangles);

    if (triangles.polygons.size() == 0)
    {
        cout << COUT_PREFIX << " polygons size 0." << endl;
        return 0;
    }

    //pcl::io::savePLYFile(srcfile.substr(0, srcfile.find_last_of("_")) + "_poisson.ply", triangles);

    cout << COUT_PREFIX << "poisson mesh reconstruct ok . point size = " << triangles.cloud.height*triangles.cloud.width<<", polygon size:"<< triangles.polygons.size() << endl;

#if 0
    cloud->clear();
    // get poisson point  
    pcl::PointCloud<pcl::PointXYZ>::Ptr poisson_point(new pcl::PointCloud<pcl::PointXYZ>);
    pcl::fromPCLPointCloud2(triangles.cloud, *cloud);
   
    //获取补洞后的点，重新拟合法线
    for (size_t i = 0; i < m_normalsIter2; i++)
    {
        //平滑点云
        normalsMovingLeastSquares(cloud, mls_points);

        getPointFromPointNormal(cloud, mls_points);

    }


    cout << COUT_PREFIX << "fit normal ok . " << endl;

    pcl::toPCLPointCloud2(mls_points, triangles.cloud);
#endif

    //save poisson mesh file
    m_strOutModelPath = srcfile.substr(0, srcfile.find_last_of("_")) + "_mesh.ply";
    /*if (pcl::io::savePLYFile(m_strOutModelPath, triangles) == 0){
        cout << COUT_PREFIX << "save mesh ply file ok. file= " << FileLibrary::getInstance()->getFileNameFromPath(m_strOutModelPath) << endl;
    }
    else
    {
        cout << "save mesh ply file false. file= " << FileLibrary::getInstance()->getFileNameFromPath(m_strOutModelPath) << endl;

    }*/

   
    pcl::PointCloud<pcl::PointXYZRGBNormal>::Ptr cloud_point(new pcl::PointCloud<pcl::PointXYZRGBNormal>);

    getPointFromPointNormal(cloud_point, *cloud);
    //getPointFromPointNormal(cloud, *cloud_with_normals);

    texturemeshPoisson poissoinmesh(srcfile,m_strOutModelPath);
    poissoinmesh.setPolygonMesh(triangles);
    poissoinmesh.meshCropHull(cloud_point);
    //test
    //poissoinmesh.getPolygonMesh(triangles);
    //mesh2VRML(triangles, m_strOutModelPath);

#if 0

    pcl::PolygonMesh mymesh;
    poissoinmesh.getPolygonMesh(mymesh);

    string meshfile = poissoinmesh.getPolygonMeshFile();

  /*  pcl::PolygonMesh meshPoly;
    if (pcl::io::loadPLYFile(meshfile, meshPoly) != 0){
        cout << COUT_PREFIX << "load ply false. file = "<< filepath << endl;
        return false;
    }*/
    //pcl::PolygonMesh outmeshPoly;
    //fillHole(meshPoly, outmeshPoly);

    cloud->clear();
    pcl::fromPCLPointCloud2(mymesh.cloud, *cloud);
    if (cloud->points.size() == 0)
    {
        cout << COUT_PREFIX << "cloud->points.size is null " << endl;
        return false;
    }

    //获取补洞后的点，重新拟合法线
    mls_points.clear();
    for (size_t i = 0; i < m_normalsIter2; i++)
    {
        //平滑点云
        normalsMovingLeastSquares(cloud, mls_points);

        getPointFromPointNormal(cloud, mls_points);

    }

    //法线拟合 compite normals from point // meshlab function
    //fitNormal(mls_points);

    //nearestKSearchNormal(mls_points);

    cout << COUT_PREFIX << "fit normal ok . " << endl;

    pcl::toPCLPointCloud2(mls_points, mymesh.cloud);

    pcl::io::savePLYFile(meshfile, mymesh);
#endif


    return true;
}

#endif

int mysaveOBJFile(const std::string &file_name, const pcl::PolygonMesh &mesh, unsigned precision)
{
    if (mesh.cloud.data.empty())
    {
        PCL_ERROR("[pcl::io::saveOBJFile] Input point cloud has no data!\n");
        return (-1);
    }
    // Open file
    std::ofstream fs;
    fs.precision(precision);
    fs.open(file_name.c_str());

    /* Write 3D information */
    // number of points
    int nr_points = mesh.cloud.width * mesh.cloud.height;
    // point size
    unsigned point_size = static_cast<unsigned> (mesh.cloud.data.size() / nr_points);
    // number of faces for header
    unsigned nr_faces = static_cast<unsigned> (mesh.polygons.size());
    // Do we have vertices normals?
    int normal_index = getFieldIndex(mesh.cloud, "normal_x");
    int rgb_index = getFieldIndex(mesh.cloud, "rgb");

    // Write the header information
    fs << "####" << std::endl;
    fs << "# OBJ dataFile simple version. File name: " << file_name << std::endl;
    fs << "# Vertices: " << nr_points << std::endl;
    if (normal_index != -1)
        fs << "# Vertices normals : " << nr_points << std::endl;
    fs << "# Faces: " << nr_faces << std::endl;
    fs << "####" << std::endl;

    // Write vertex coordinates
    fs << "# List of Vertices, with (x,y,z) coordinates, w is optional." << std::endl;
    for (int i = 0; i < nr_points; ++i)
    {
        int xyz = 0;
        for (size_t d = 0; d < mesh.cloud.fields.size(); ++d)
        {
            int c = 0;
            // adding vertex
            if ((mesh.cloud.fields[d].datatype == pcl::PCLPointField::FLOAT32) && (
                mesh.cloud.fields[d].name == "x" ||
                mesh.cloud.fields[d].name == "y" ||
                mesh.cloud.fields[d].name == "z") )
            {
                if (mesh.cloud.fields[d].name == "x")
                    // write vertices beginning with v
                    fs << "v ";
                
                    float value;
                    memcpy(&value, &mesh.cloud.data[i * point_size + mesh.cloud.fields[d].offset + c * sizeof (float)], sizeof (float));
                    fs << value;

                

              /*  if (++xyz == 4)
                    break;*/
                    ++xyz;
                fs << " ";
            }
            else if ((mesh.cloud.fields[d].datatype == pcl::PCLPointField::FLOAT32) && (mesh.cloud.fields[d].name == "rgb"))
            {
                pcl::RGB color;
                memcpy(&color, &mesh.cloud.data[i * point_size + mesh.cloud.fields[rgb_index].offset + c * sizeof (float)], sizeof (RGB));
                int r, g, b;
                r = color.r;
                b = color.b;
                g = color.g;
                fs << r << " " << g << " " << b;
                //fs.write(reinterpret_cast<const char*> (&color.r), sizeof (unsigned char));
                ////fs << " ";
                //fs.write(reinterpret_cast<const char*> (&color.g), sizeof (unsigned char));
                ////fs << " ";
                //fs.write(reinterpret_cast<const char*> (&color.b), sizeof (unsigned char));

            }
          

        }
        if (xyz != 3)
        {
            PCL_ERROR("[pcl::io::saveOBJFile] Input point cloud has no XYZ data!\n");
            return (-2);
        }
        fs << std::endl;
    }

    fs << "# " << nr_points << " vertices" << std::endl;

    if (normal_index != -1)
    {
        fs << "# Normals in (x,y,z) form; normals might not be unit." << std::endl;
        // Write vertex normals
        for (int i = 0; i < nr_points; ++i)
        {
            int nxyz = 0;
            for (size_t d = 0; d < mesh.cloud.fields.size(); ++d)
            {
                int c = 0;
                // adding vertex
                if ((mesh.cloud.fields[d].datatype == pcl::PCLPointField::FLOAT32) && (
                    mesh.cloud.fields[d].name == "normal_x" ||
                    mesh.cloud.fields[d].name == "normal_y" ||
                    mesh.cloud.fields[d].name == "normal_z"))
                {
                    if (mesh.cloud.fields[d].name == "normal_x")
                        // write vertices beginning with vn
                        fs << "vn ";

                    float value;
                    memcpy(&value, &mesh.cloud.data[i * point_size + mesh.cloud.fields[d].offset + c * sizeof (float)], sizeof (float));
                    fs << value;
                    if (++nxyz == 3)
                        break;
                    fs << " ";
                }
            }
            if (nxyz != 3)
            {
                PCL_ERROR("[pcl::io::saveOBJFile] Input point cloud has no normals!\n");
                return (-2);
            }
            fs << std::endl;
        }

        fs << "# " << nr_points << " vertices normals" << std::endl;
    }

    fs << "# Face Definitions" << std::endl;
    // Write down faces
    if (normal_index == -1)
    {
        for (unsigned i = 0; i < nr_faces; i++)
        {
            fs << "f ";
            size_t j = 0;
            for (; j < mesh.polygons[i].vertices.size() - 1; ++j)
                fs << mesh.polygons[i].vertices[j] + 1 << " ";
            fs << mesh.polygons[i].vertices[j] + 1 << std::endl;
        }
    }
    else
    {
        for (unsigned i = 0; i < nr_faces; i++)
        {
            fs << "f ";
            size_t j = 0;
            for (; j < mesh.polygons[i].vertices.size() - 1; ++j)
                fs << mesh.polygons[i].vertices[j] + 1 << "//" << mesh.polygons[i].vertices[j] + 1 << " ";
            fs << mesh.polygons[i].vertices[j] + 1 << "//" << mesh.polygons[i].vertices[j] + 1 << std::endl;
        }
    }
    fs << "# End of File" << std::endl;

    // Close obj file
    fs.close();
    return 0;
}

//贪婪算法生成mesh
bool ConverPointCloud::createGreedMesh(const string &filepath){
    std::string srcfile = filepath;
    //pcl::PointCloud<pcl::PointXYZRGB>::Ptr cloud_filtered(new pcl::PointCloud<pcl::PointXYZRGB>);
    pcl::PointCloud<pcl::PointXYZRGBNormal>::Ptr cloud_normal(new pcl::PointCloud<pcl::PointXYZRGBNormal>);
    pcl::PointCloud<pcl::PointXYZRGB>::Ptr cloud(new pcl::PointCloud<pcl::PointXYZRGB>);

    pcl::io::loadPLYFile(filepath, *cloud_normal);

    cout << COUT_PREFIX << "read ply file ok . point size=" << cloud_normal->points.size() << endl;

    pcl::PointCloud<pcl::PointXYZRGBNormal> mls_points;
#if 0
    //pcl::search::KdTree<pcl::PointXYZ>::Ptr tree(new pcl::search::KdTree<pcl::PointXYZ>);
    //tree->setInputCloud(cloud);
    //pcl::KdTreeFLANN<pcl::PointXYZ> kdtree;

    //pcl::PointCloud<pcl::PointXYZ>::Ptr cloud_filtered2(new pcl::PointCloud<pcl::PointXYZ>);

    //pcl::ApproximateVoxelGrid<pcl::PointXYZ> avg;
    //avg.setInputCloud(cloud);
    //avg.setLeafSize(0.0001, 0.0001, 0.0001);
    //avg.setDownsampleAllData(true);
    //avg.filter(*cloud_filtered);
    //
    //cout << "体素滤波器 size = " << cloud_filtered->points.size() << endl;
    if (m_leafsize != 0)
    {
        pcl::VoxelGrid<PointXYZRGB> sor;//滤波处理对象
        sor.setInputCloud(cloud);
        sor.setLeafSize(m_leafsize, m_leafsize, m_leafsize);//设置滤波器处理时采用的体素大小的参数   0.00015 = 大约 9:1 
        sor.filter(*cloud_filtered);
        cout << COUT_PREFIX << "VoxelGrid ok . point size=" << cloud_filtered->points.size() << endl;
        cloud = cloud_filtered;
    }
    m_strOutModelPath = srcfile.substr(0, srcfile.find_last_of("_")) + "_vox.ply";

    //pcl::io::savePLYFile(m_strOutModelPath, *cloud);

    //
    // Init object (second point type is for the normals, even if unused)
    //最小二乘法迭代拟合平滑点云    
    for (size_t i = 0; i < m_normalsIter1; i++)
    {
        normalsMovingLeastSquares(cloud, mls_points);

        getPointFromPointNormal(cloud, mls_points);
    }
    if (mls_points.points.size() == 0)
    {
        cout << "mls false. point size = " << mls_points.points.size() << endl;
        return 0;
    }

    m_strOutModelPath = srcfile.substr(0, srcfile.find_last_of("_")) + "_mls_1.ply";

    pcl::io::savePLYFile(m_strOutModelPath, mls_points);

    cout << COUT_PREFIX << "MLS normal ok . point size: " << mls_points.points.size() << endl;
#endif

    // 创建同时包含点和法向的数据结构的指针
    //pcl::PointCloud<pcl::PointXYZRGBNormal>::Ptr cloud_with_normals(new pcl::PointCloud<pcl::PointXYZRGBNormal>(mls_points));
    pcl::search::KdTree<pcl::PointXYZRGBNormal>::Ptr tree2(new pcl::search::KdTree<pcl::PointXYZRGBNormal>);

    pcl::PolygonMesh triangles;
    tree2->setInputCloud(cloud_normal);


    /*曲面重建模块*/
    // 创建贪婪三角形投影重建对象
    pcl::GreedyProjectionTriangulation<pcl::PointXYZRGBNormal> gp3;
    //创建多边形网格对象,用来存储重建结果
    gp3.setSearchRadius(m_searchRadius);  //设置连接点之间的最大距离，用于确定k近邻的球半径 （即是三角形最大边长）
    gp3.setMu(m_mu);  // 设置最近邻距离的乘子，已得到每个点的最终搜索半径（默认为0）
    gp3.setMaximumNearestNeighbors(m_nearestNeighbors);  //设置搜索的最近邻点的最大数量
    gp3.setMaximumSurfaceAngle(m_maxSurfaceAngle * (M_PI / 180)); // 90 degrees 最大平面角
    gp3.setMinimumAngle(m_minAngle * (M_PI / 180)); // 5 degrees 每个三角的最大角度
    gp3.setMaximumAngle(m_maxAngle * (M_PI / 180)); // 150 degrees
    gp3.setNormalConsistency(false);  //若法向量一致，设为true
    gp3.setConsistentVertexOrdering(true);
    // 设置点云数据和搜索方式
    gp3.setInputCloud(cloud_normal);
    gp3.setSearchMethod(tree2);
    //开始重建
    gp3.reconstruct(triangles);

    if (triangles.polygons.size() == 0){
        cout << " polygons size 0." << endl;
        return 0;
    }

    cout << COUT_PREFIX << "mesh reconstruct ok . point size:" << triangles.cloud.height* triangles.cloud.width << "	polygons:" << triangles.polygons.size() << endl;

    //补洞
    pcl::PolygonMesh meshPoly;
    fillHole(triangles, meshPoly);
    //m_strOutModelPath = srcfile.substr(0, srcfile.find_last_of("_")) + "_mesh.wrl";

    //mesh2VRML(triangles, m_strOutModelPath);
    m_strOutModelPath = srcfile.substr(0, srcfile.find_last_of("_")) + "_mesh.ply";
    pcl::io::savePLYFile(m_strOutModelPath, meshPoly);

#if 0
    cloud->clear();
    pcl::fromPCLPointCloud2(meshPoly.cloud, *cloud);
    if (cloud->points.size() == 0)
    {
        cout << COUT_PREFIX << "cloud->points.size is null " << endl;
        return false;
    }

    ////获取补洞后的点，重新拟合法线
    for (size_t i = 0; i < m_normalsIter2; i++)
    {
        //平滑点云
        mls_points.points.clear();
        normalsMovingLeastSquares(cloud, mls_points);

        getPointFromPointNormal(cloud, mls_points);

    }

    //法线拟合
    fitNormal(mls_points);

    nearestKSearchNormal(mls_points);

    m_strOutModelPath = srcfile.substr(0, srcfile.find_last_of("_")) + "_mls_2.ply";
    //pcl::io::savePLYFile(m_strOutModelPath, mls_points);


    pcl::toPCLPointCloud2(mls_points, meshPoly.cloud);

    cout << COUT_PREFIX << "fit normal ok . point: " << mls_points.points.size() << ", polygons: " << meshPoly.polygons.size() << endl;


    m_strOutModelPath = srcfile.substr(0, srcfile.find_last_of("_")) + "_mesh.ply";

    if (pcl::io::savePLYFile(m_strOutModelPath, meshPoly) == 0)
    //if (mysaveOBJFile(m_strOutModelPath, meshPoly,5) == 0)
    {
        cout << COUT_PREFIX << "save mesh ply file ok. file= " << FileLibrary::getInstance()->getFileNameFromPath(m_strOutModelPath) << endl;
    }
    else
    {
        cout << "save mesh ply file false. file= " << FileLibrary::getInstance()->getFileNameFromPath(m_strOutModelPath) << endl;

    }

#endif

    return true;
}


#if 0
bool ConverPointCloud::createGreedMesh(const string &filepath){
    std::string srcfile = filepath;
    pcl::PointCloud<pcl::PointXYZ>::Ptr cloud_filtered(new pcl::PointCloud<pcl::PointXYZ>);
    pcl::PointCloud<pcl::PointXYZ>::Ptr cloud(new pcl::PointCloud<pcl::PointXYZ>);

    pcl::io::loadPLYFile(filepath, *cloud);

    cout << COUT_PREFIX << "read ply file ok . point size=" << cloud ->points.size()<< endl;

    pcl::PointCloud<pcl::PointNormal> mls_points;
    //pcl::search::KdTree<pcl::PointXYZ>::Ptr tree(new pcl::search::KdTree<pcl::PointXYZ>);
    //tree->setInputCloud(cloud);
    //pcl::KdTreeFLANN<pcl::PointXYZ> kdtree;

    //pcl::PointCloud<pcl::PointXYZ>::Ptr cloud_filtered2(new pcl::PointCloud<pcl::PointXYZ>);

    //pcl::ApproximateVoxelGrid<pcl::PointXYZ> avg;
    //avg.setInputCloud(cloud);
    //avg.setLeafSize(0.0001, 0.0001, 0.0001);
    //avg.setDownsampleAllData(true);
    //avg.filter(*cloud_filtered);
    //
    //cout << "体素滤波器 size = " << cloud_filtered->points.size() << endl;

    //
    // Init object (second point type is for the normals, even if unused)
    //最小二乘法迭代拟合平滑点云    
	for (size_t i = 0; i < m_normalsIter1; i++)
    {
        normalsMovingLeastSquares(cloud, mls_points);
        
        getPointFromPointNormal(cloud, mls_points);
    }
    if (mls_points.points.size() == 0)
    {
        cout << "mls false. point size = " << mls_points.points.size() << endl;
        return 0;
    }

    //pcl::io::savePLYFile("e:\\mls.ply", mls_points);


	cout << COUT_PREFIX << "MLS normal ok . point size: " << mls_points.points.size() << endl;

    // 创建同时包含点和法向的数据结构的指针
    pcl::PointCloud<pcl::PointNormal>::Ptr cloud_with_normals(new pcl::PointCloud<pcl::PointNormal>(mls_points));
    pcl::search::KdTree<pcl::PointNormal>::Ptr tree2(new pcl::search::KdTree<pcl::PointNormal>);
    pcl::PolygonMesh triangles;
    tree2->setInputCloud(cloud_with_normals);

 
    /*曲面重建模块*/
    // 创建贪婪三角形投影重建对象
    pcl::GreedyProjectionTriangulation<pcl::PointNormal> gp3;
    //创建多边形网格对象,用来存储重建结果
    gp3.setSearchRadius(m_searchRadius);  //设置连接点之间的最大距离，用于确定k近邻的球半径 （即是三角形最大边长）
    gp3.setMu(m_mu);  // 设置最近邻距离的乘子，已得到每个点的最终搜索半径（默认为0）
    gp3.setMaximumNearestNeighbors(m_nearestNeighbors);  //设置搜索的最近邻点的最大数量
    gp3.setMaximumSurfaceAngle(m_maxSurfaceAngle * (M_PI / 180)); // 90 degrees 最大平面角
    gp3.setMinimumAngle(m_minAngle * (M_PI / 180)); // 5 degrees 每个三角的最大角度
    gp3.setMaximumAngle(m_maxAngle * (M_PI / 180)); // 150 degrees
    gp3.setNormalConsistency(false);  //若法向量一致，设为true
    gp3.setConsistentVertexOrdering(true);
    // 设置点云数据和搜索方式
    gp3.setInputCloud(cloud_with_normals);
    gp3.setSearchMethod(tree2);
    //开始重建
    gp3.reconstruct(triangles);

    if (triangles.polygons.size() == 0){
        cout << " polygons size 0." << endl;
        return 0;
    }

    //pcl::io::savePLYFile("e:\\triangles.ply", triangles);
	cout << COUT_PREFIX << "mesh reconstruct ok . point size:" << triangles.cloud.height* triangles.cloud.width<<"	polygons:"<< triangles.polygons.size() << endl;

	//补洞
    pcl::PolygonMesh meshPoly;
    //fillHole(triangles, meshPoly);

	vtkSmartPointer<vtkPolyData> input;
	pcl::VTKUtils::mesh2vtk(triangles, input);

	vtkSmartPointer<vtkFillHolesFilter> fillHolesFilter = vtkSmartPointer<vtkFillHolesFilter>::New();

	fillHolesFilter->SetInputData(input);
	fillHolesFilter->SetHoleSize(m_holesize);//0.005
	fillHolesFilter->Update();

	vtkSmartPointer<vtkPolyData> polyData = fillHolesFilter->GetOutput();

	pcl::VTKUtils::vtk2mesh(polyData, meshPoly);
	cout << COUT_PREFIX << "fill hole ok, size=" << m_holesize << endl;

#if 1
    cloud->clear();
    pcl::fromPCLPointCloud2(meshPoly.cloud, *cloud);
    if (cloud->points.size() == 0)
    {
        cout << COUT_PREFIX << "cloud->points.size is null " << endl;
        return false;
    }
    
    //获取补洞后的点，重新拟合法线
	for (size_t i = 0; i < m_normalsIter2; i++)
    {
        //平滑点云
        mls_points.points.clear();
        normalsMovingLeastSquares(cloud, mls_points);

        getPointFromPointNormal(cloud, mls_points);

    }

    //法线拟合
    fitNormal(mls_points);

    nearestKSearchNormal(mls_points);

	cout << COUT_PREFIX << "fit normal ok . point size:" << mls_points.points.size() << endl;

#endif


    pcl::toPCLPointCloud2(mls_points, meshPoly.cloud);

    m_strOutModelPath = srcfile.substr(0, srcfile.find_last_of("_")) + "_mesh.ply";

    if (pcl::io::savePLYFile(m_strOutModelPath, meshPoly) == 0){
        cout << COUT_PREFIX << "save mesh ply file ok. file= " << FileLibrary::getInstance()->getFileNameFromPath(m_strOutModelPath) << endl;
    }
    else
    {
        cout << "save mesh ply file false. file= " << FileLibrary::getInstance()->getFileNameFromPath(m_strOutModelPath) << endl;

    }


    return true;
}

#endif
bool ConverPointCloud::createModel(const string &plyfile){

    OdmTexturing createmodel(plyfile, m_strTexturepng);

    createmodel.run(m_focal_length);


	return true;
}



bool ConverPointCloud::meshAPI(const string &flypath){

    if (!FileLibrary::getInstance()->isFileExists(flypath))
    {
        cout << "fly file no find. file =" << flypath << endl;
        return false;
    }
    if (parseArguments(flypath) == 0){
        return false;
    }
    if (m_type == 1)
        return createPoissonMesh(flypath);
    if (m_type ==2 )
    {
        return createGreedMesh(flypath);
    }
    return true;
}
bool ConverPointCloud::modelAPI(const string &flypath){

    if (!FileLibrary::getInstance()->isFileExists(flypath))
    {
        cout << "fly file no find. file =" << flypath << endl;
        return false;
    }

    if (flypath.find("disparity") != string::npos)
    {
        m_strTexturepng = flypath.substr(0, flypath.find_last_of("_"));// +"_rgb.png";
        m_strTexturepng = m_strTexturepng.substr(0, m_strTexturepng.find_last_of("_")) + "_rgb.png";

    }
    else
    {
        m_strTexturepng = flypath.substr(0, flypath.find_last_of("_")) +"_rgb.png";

    }

    if (!FileLibrary::getInstance()->isFileExists(m_strTexturepng))
    {
        cout << "texture file no find. file =" << m_strTexturepng << endl;
        return false;
    }

    if (parseArguments(flypath) == 0){
        return false;
    }

    return createModel(flypath);
}

//法线标准化
bool ConverPointCloud::calculateVertexNormal(pcl::PointCloud<pcl::PointXYZRGBNormal> &mls_points){

    for (size_t i = 0; i < mls_points.points.size(); i++)
    {
        float vp_x = 0, vp_y = 0, vp_z = 0;
        float nx = mls_points.points[i].normal_x, ny = mls_points.points[i].normal_y, nz = mls_points.points[i].normal_z;


        vp_x -= mls_points.points[i].x;
        vp_y -= mls_points.points[i].y;
        vp_z -= mls_points.points[i].z;

        // Dot product between the (viewpoint - point) and the plane normal
        float cos_theta = (vp_x * nx + vp_y * ny + vp_z * nz);
        // Flip the plane normal
        if (cos_theta < 0)
        {
            mls_points.points[i].normal_x *= -1;
            mls_points.points[i].normal_y *= -1;
            mls_points.points[i].normal_z *= -1;
        }
        float L = sqrtf(mls_points.points[i].normal_x*mls_points.points[i].normal_x + mls_points.points[i].normal_y * mls_points.points[i].normal_y + mls_points.points[i].normal_z * mls_points.points[i].normal_z);

        mls_points.points[i].normal_x /= L;
        mls_points.points[i].normal_y /= L;
        mls_points.points[i].normal_z /= L;

    }
    return true;
}
bool ConverPointCloud::searchKNeightbor(const pcl::PointXYZRGB &current_point, std::vector<int> &indices, float distance){


    return true;
}

bool ConverPointCloud::getPointFromPointNormal(pcl::PointCloud<pcl::PointXYZRGBNormal>::Ptr cloud, pcl::PointCloud<PointXYZRGBNormal> &mls_points){

    cloud->clear();
    cloud->resize(mls_points.points.size());

    
    for (size_t i = 0; i < mls_points.points.size(); i++)
    {
        cloud->points[i].x = mls_points.points[i].x;
        cloud->points[i].y = mls_points.points[i].y;
        cloud->points[i].z = mls_points.points[i].z;
        cloud->points[i].r = mls_points.points[i].r;
        cloud->points[i].g = mls_points.points[i].g;
        cloud->points[i].b = mls_points.points[i].b;
        

        cloud->points[i].normal_x = mls_points.points[i].normal_x;
        cloud->points[i].normal_y = mls_points.points[i].normal_y;
        cloud->points[i].normal_z = mls_points.points[i].normal_z;
    }

    return true;

}

#if 1
bool ConverPointCloud::getPointFromPointNormal(pcl::PointCloud<pcl::PointXYZRGB>::Ptr cloud, pcl::PointCloud<PointXYZRGBNormal> &mls_points){

    cloud->clear();
    cloud->resize(mls_points.points.size());


    for (size_t i = 0; i < mls_points.points.size(); i++)
    {
        cloud->points[i].x = mls_points.points[i].x;
        cloud->points[i].y = mls_points.points[i].y;
        cloud->points[i].z = mls_points.points[i].z;
        cloud->points[i].r = mls_points.points[i].r;            
        cloud->points[i].g = mls_points.points[i].g;
        cloud->points[i].b = mls_points.points[i].b;

    }

    return true;

}
#endif
bool ConverPointCloud::normalsMovingLeastSquares(pcl::PointCloud<pcl::PointXYZRGB>::Ptr cloud, pcl::PointCloud<PointXYZRGBNormal> &mls_points){

    pcl::MovingLeastSquares<pcl::PointXYZRGB, pcl::PointXYZRGBNormal> mls;
    pcl::search::KdTree<pcl::PointXYZRGB>::Ptr tree(new pcl::search::KdTree<pcl::PointXYZRGB>);
    tree->setInputCloud(cloud);

    // Set parameters
    mls.setInputCloud(cloud);
    //mls.setPolynomialFit(true); //对于法线的估计是有多项式还是仅仅依靠切线
    mls.setComputeNormals(true);
    mls.setSearchMethod(tree);
    mls.setPointDensity(30);
    mls.setPolynomialOrder(4);
    mls.setSearchRadius(m_mlsSearchRadius); // 0.001jingnan //确定搜索的半径,在这个半径里进行表面映射和曲面拟合。从实验结果可知：半径越小拟合后曲面的失真度越小，反之有可能出现过拟合的现象
    if (m_upsamplingType == 0)
    {
        mls.setUpsamplingMethod(mls.NONE);

    }
    else if (m_upsamplingType == 2)
    {
        mls.setUpsamplingMethod(mls.SAMPLE_LOCAL_PLANE);// 这个方法就是参考论文中采用的方法，当然此方法所需的计算强度也相当庞大。若使用此方法，将需要调用两个函数：
        mls.setUpsamplingRadius(m_upsamplingRadius);//此函数规定了点云增长的区域。可以这样理解：把整个点云按照此半径划分成若干个子点云，然后一一索引进行点云增长。           0.1
        mls.setUpsamplingStepSize(m_upsamplingStepSize);//对于每个子点云处理时迭代的步长。

    }
    else if (m_upsamplingType == 3)
    {
        mls.setUpsamplingMethod(mls.RANDOM_UNIFORM_DENSITY);   //也是使用上面子点云的原理，只不过它使得稀疏区域的密度增加，从而使得整个点云的密度均匀
        mls.setPointDensity(m_pointDensity);  //注意此函数输入整型变量，意为半径内点的个数。（这个半径应该是search的半径，不需要重新设置）。

    }
    else if (m_upsamplingType == 4)
    {

        mls.setUpsamplingMethod(mls.VOXEL_GRID_DILATION); //这个方法有两个步骤：首先将点云以voxels分割，然后进行迭代使得voxels的数目增加。它的结果是：填充空洞和平均化点云的密度。它需要调用的函数为：
        mls.setDilationVoxelSize(m_dilationVoxelSize);   //设定voxel的大小。
        mls.setDilationIterations(m_dilationIterations); //设置迭代的次数
    }


    // Reconstruct
    mls.process(mls_points);


    return true;
}

bool ConverPointCloud::fitNormal(pcl::PointCloud<pcl::PointXYZRGBNormal> &mls_points)
{

    //法线重新计算
    pcl::PointCloud<pcl::PointXYZRGB>::Ptr allpoint(new pcl::PointCloud<pcl::PointXYZRGB>);
    allpoint->resize(mls_points.points.size());
    for (size_t i = 0; i < mls_points.points.size(); i++)
    {
        allpoint->points[i].x = mls_points.points[i].x;
        allpoint->points[i].y = mls_points.points[i].y;
        allpoint->points[i].z = mls_points.points[i].z;
        allpoint->points[i].r = mls_points.points[i].r;
        allpoint->points[i].g = mls_points.points[i].g;
        allpoint->points[i].b = mls_points.points[i].b;

    }
    
    pcl::search::KdTree<pcl::PointXYZRGB> search;
    search.setInputCloud(allpoint);
    pcl::PointCloud<PointXYZRGB>::Ptr neight(new PointCloud<pcl::PointXYZRGB>);

    for (size_t i = 0; i < mls_points.points.size(); i++)
    {

        std::vector<int> indices(m_neighbor_num);
		std::vector<float> distance(m_distance);
        //由于VTK/OpenGL并没有存储NaN格式
        pcl::PointXYZRGB current_point;
        current_point.x = mls_points.points[i].x;
        current_point.y = mls_points.points[i].y;
        current_point.z = mls_points.points[i].z;

        search.nearestKSearch(current_point, m_neighbor_num, indices, distance);//其中current_point为选中的点 
        Eigen::Matrix3f covariance_matrix;
        Eigen::Matrix<float, 4, 1> xyz_centroid;


        neight->resize(indices.size());
        for (size_t Knum = 0; Knum < m_neighbor_num; Knum++)
        {
            int neightId = indices[Knum];
            neight->points[Knum].x = mls_points.points[neightId].x;
            neight->points[Knum].y = mls_points.points[neightId].y;
            neight->points[Knum].z = mls_points.points[neightId].z;
        }
        //根据中心点、协方差计算法线
        pcl::compute3DCentroid(*neight, xyz_centroid);
        pcl::computeCovarianceMatrix(*neight, xyz_centroid, covariance_matrix);

        Eigen::SelfAdjointEigenSolver<Eigen::Matrix<float, 3, 3> > eig(covariance_matrix);
        Eigen::Matrix<float, 3, 1> eval = eig.eigenvalues();
        Eigen::Matrix<float, 3, 3> evec = eig.eigenvectors();
        eval = eval.cwiseAbs();
        int minInd;
        eval.minCoeff(&minInd);

        mls_points.points[i].normal_x = evec(0, minInd);
        mls_points.points[i].normal_y = evec(1, minInd);
        mls_points.points[i].normal_z = evec(2, minInd);

    }
    
    return true;

}

bool ConverPointCloud::nearestKSearchNormal(pcl::PointCloud<PointXYZRGBNormal> &mls_points){

    pcl::PointCloud<pcl::PointXYZRGB>::Ptr allpoint(new pcl::PointCloud<pcl::PointXYZRGB>);
    allpoint->resize(mls_points.points.size());
    for (size_t i = 0; i < mls_points.points.size(); i++)
    {
        allpoint->points[i].x = mls_points.points[i].x;
        allpoint->points[i].y = mls_points.points[i].y;
        allpoint->points[i].z = mls_points.points[i].z;
    }


    pcl::PointCloud<pcl::PointXYZRGBNormal> tmpNormalPoint;
    pcl::search::KdTree<pcl::PointXYZRGB> search;
    search.setInputCloud(allpoint);

    //拟合邻居K计算法线
    for (size_t iterNum = 0; iterNum < m_normalsIter2; iterNum++)
    {
        tmpNormalPoint.clear();
        tmpNormalPoint.points.resize(mls_points.points.size());


        for (size_t i = 0; i < mls_points.points.size(); i++)
        {

            std::vector<int> indices(m_neighbor_num);
			std::vector<float> distance(m_distance); //0.01
            //由于VTK/OpenGL并没有存储NaN格式
            pcl::PointXYZRGB current_point;
            current_point.x = mls_points.points[i].x;
            current_point.y = mls_points.points[i].y;
            current_point.z = mls_points.points[i].z;
            search.nearestKSearch(current_point, m_neighbor_num, indices, distance);//其中current_point为选中的点 

            for (size_t Knum = 0; Knum < m_neighbor_num; Knum++)
            {
                int neightId = indices[Knum];
                pcl::PointXYZRGBNormal neightborpoint = mls_points.points[neightId];
                //邻居法线 * 当前点法线
                float LN = neightborpoint.normal_x*mls_points.points[i].normal_x + neightborpoint.normal_y*mls_points.points[i].normal_y + neightborpoint.normal_z * mls_points.points[i].normal_z;

                tmpNormalPoint.points[i].x = mls_points.points[i].x;
                tmpNormalPoint.points[i].y = mls_points.points[i].y;
                tmpNormalPoint.points[i].z = mls_points.points[i].z;
                tmpNormalPoint.points[i].r = mls_points.points[i].r;
                tmpNormalPoint.points[i].g = mls_points.points[i].g;
                tmpNormalPoint.points[i].b = mls_points.points[i].b;


                if (LN > 0)
                {
                    tmpNormalPoint.points[i].normal_x += neightborpoint.normal_x;
                    tmpNormalPoint.points[i].normal_y += neightborpoint.normal_y;
                    tmpNormalPoint.points[i].normal_z += neightborpoint.normal_z;
                }
                else{
                    tmpNormalPoint.points[i].normal_x -= neightborpoint.normal_x;
                    tmpNormalPoint.points[i].normal_y -= neightborpoint.normal_y;
                    tmpNormalPoint.points[i].normal_z -= neightborpoint.normal_z;

                }
            }// end neighbours 
        }// end every poiont

        mls_points.points = tmpNormalPoint.points;


        //法线标准化
        calculateVertexNormal(mls_points);
        //cout << "end iter : "<< iterNum << endl;
    }

    return true;
}

int testvtk2mesh(const vtkSmartPointer<vtkPolyData>& poly_data, pcl::PolygonMesh& mesh)
{
    mesh.polygons.clear();
    mesh.cloud.data.clear();
    mesh.cloud.width = mesh.cloud.height = 0;
    mesh.cloud.is_dense = true;

    vtkSmartPointer<vtkPoints> mesh_points = poly_data->GetPoints();
    vtkIdType nr_points = mesh_points->GetNumberOfPoints();
    vtkIdType nr_polygons = poly_data->GetNumberOfPolys();

    if (nr_points == 0)
        return 0;

    vtkUnsignedCharArray* poly_colors = NULL;
    if (poly_data->GetPointData() != NULL)
        poly_colors = vtkUnsignedCharArray::SafeDownCast(poly_data->GetPointData()->GetScalars("Colors"));

    // Some applications do not save the name of scalars (including PCL's native vtk_io)
    if (!poly_colors)
        poly_colors = vtkUnsignedCharArray::SafeDownCast(poly_data->GetPointData()->GetScalars("scalars"));

    // TODO: currently only handles rgb values with 3 components
    if (poly_colors && (poly_colors->GetNumberOfComponents() == 3))
    {
        pcl::PointCloud<pcl::PointXYZRGB>::Ptr cloud_temp(new pcl::PointCloud<pcl::PointXYZRGB>());
        cloud_temp->points.resize(nr_points);
        double point_xyz[3];
        unsigned char point_color[3];
        for (vtkIdType i = 0; i < mesh_points->GetNumberOfPoints(); ++i)
        {
            mesh_points->GetPoint(i, &point_xyz[0]);
            cloud_temp->points[i].x = static_cast<float> (point_xyz[0]);
            cloud_temp->points[i].y = static_cast<float> (point_xyz[1]);
            cloud_temp->points[i].z = static_cast<float> (point_xyz[2]);

            poly_colors->GetTypedTuple(i, &point_color[0]);
            cloud_temp->points[i].r = point_color[0];
            cloud_temp->points[i].g = point_color[1];
            cloud_temp->points[i].b = point_color[2];
        }
        cloud_temp->width = static_cast<uint32_t> (cloud_temp->points.size());
        cloud_temp->height = 1;
        cloud_temp->is_dense = true;

        pcl::toPCLPointCloud2(*cloud_temp, mesh.cloud);
    }
    else // in case points do not have color information:
    {
        pcl::PointCloud<pcl::PointXYZ>::Ptr cloud_temp(new pcl::PointCloud<pcl::PointXYZ>());
        cloud_temp->points.resize(nr_points);
        double point_xyz[3];
        for (vtkIdType i = 0; i < mesh_points->GetNumberOfPoints(); ++i)
        {
            mesh_points->GetPoint(i, &point_xyz[0]);
            cloud_temp->points[i].x = static_cast<float> (point_xyz[0]);
            cloud_temp->points[i].y = static_cast<float> (point_xyz[1]);
            cloud_temp->points[i].z = static_cast<float> (point_xyz[2]);
        }
        cloud_temp->width = static_cast<uint32_t> (cloud_temp->points.size());
        cloud_temp->height = 1;
        cloud_temp->is_dense = true;

        pcl::toPCLPointCloud2(*cloud_temp, mesh.cloud);
    }

    mesh.polygons.resize(nr_polygons);
    const vtkIdType* cell_points;
    vtkIdType nr_cell_points;
    vtkCellArray * mesh_polygons = poly_data->GetPolys();
    mesh_polygons->InitTraversal();
    int id_poly = 0;
    while (mesh_polygons->GetNextCell(nr_cell_points, cell_points))
    {
        mesh.polygons[id_poly].vertices.resize(nr_cell_points);
        for (int i = 0; i < nr_cell_points; ++i)
            mesh.polygons[id_poly].vertices[i] = static_cast<unsigned int> (cell_points[i]);
        ++id_poly;
    }

    return (static_cast<int> (nr_points));
}
bool ConverPointCloud::fillHole(const pcl::PolygonMesh &inPutmesh, pcl::PolygonMesh &outMesh){
    if (m_holesize == 0.0 )
    {
        outMesh = inPutmesh;
        return false;
    }
    ////修补空洞
    /*pcl::PolygonMesh meshPoly = inPutmesh;*/
    vtkSmartPointer<vtkPolyData> input;
    pcl::VTKUtils::mesh2vtk(inPutmesh, input);
       

    vtkSmartPointer<vtkFillHolesFilter> fillHolesFilter = vtkSmartPointer<vtkFillHolesFilter>::New();
#if VTK_MAJOR_VERSION <= 5
    fillHolesFilter->SetInputConnection(input->GetProducerPort());
#else
    fillHolesFilter->SetInputData(input);
#endif
    //fillHolesFilter->SetInputData(input);
    fillHolesFilter->SetHoleSize(m_holesize);//0.005
    fillHolesFilter->Update();

    vtkSmartPointer<vtkPolyData> polyData = fillHolesFilter->GetOutput();

   /* vtkUnsignedCharArray* poly_colors = NULL;
    if (polyData->GetPointData() != NULL)
        poly_colors = vtkUnsignedCharArray::SafeDownCast(polyData->GetPointData()->GetScalars("Colors"));*/

    pcl::VTKUtils::vtk2mesh(polyData, outMesh);
    //testvtk2mesh(polyData, outMesh);

    cout << COUT_PREFIX << "fill hole ok, size=" << m_holesize << endl;

    return true;
}


void ConverPointCloud::mesh2VRML(const pcl::PolygonMesh &inPutmesh, const string &file) {

    vtkSmartPointer<vtkPolyData> input;
    pcl::VTKUtils::mesh2vtk(inPutmesh, input);


    //obj->add

    vtkSmartPointer<vtkFillHolesFilter> fillHolesFilter = vtkSmartPointer<vtkFillHolesFilter>::New();

    fillHolesFilter->SetInputData(input);
    fillHolesFilter->SetHoleSize(m_holesize);//0.005
    fillHolesFilter->Update();

    //vtkSmartPointer<vtkPolyData> polyData = fillHolesFilter->GetOutput();

    
    // Make the triangle windong order consistent
    vtkSmartPointer<vtkPolyDataNormals> normals = vtkSmartPointer<vtkPolyDataNormals>::New();
    normals->SetInputConnection(fillHolesFilter->GetOutputPort());
    normals->ConsistencyOn();
    normals->SplittingOff();
    normals->Update();


    // Restore the original normals
    normals->GetOutput()->GetPointData()-> SetNormals(input->GetPointData()->GetNormals());

    // Visualize
    // Define viewport ranges
    // (xmin, ymin, xmax, ymax)
    double leftViewport[4] = { 0.0, 0.0, 0.5, 1.0 };

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
    originalActor->SetMapper(originalMapper);
    originalActor->SetBackfaceProperty(backfaceProp);
    originalActor->GetProperty()->SetDiffuseColor(1.0, 0.3882, 0.2784);

    vtkSmartPointer<vtkPolyDataMapper> filledMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    filledMapper->SetInputConnection(normals->GetOutputPort());

    vtkSmartPointer<vtkActor> filledActor = vtkSmartPointer<vtkActor>::New();
    filledActor->SetMapper(filledMapper);
    filledActor->GetProperty()->SetDiffuseColor(1.0, 0.3882, 0.2784);

    // Create a renderer, render window, and interactor
    vtkSmartPointer<vtkRenderer> leftRenderer =  vtkSmartPointer<vtkRenderer>::New();
    leftRenderer->SetViewport(leftViewport);


    vtkSmartPointer<vtkRenderWindow> renderWindow = vtkSmartPointer<vtkRenderWindow>::New();
    //renderWindow->SetSize(600, 600);

    renderWindow->AddRenderer(leftRenderer);

    vtkSmartPointer<vtkRenderWindowInteractor> renderWindowInteractor =    vtkSmartPointer<vtkRenderWindowInteractor>::New();
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
    vtkSmartPointer<vtkPolyDataMapper> originalMapper =    vtkSmartPointer<vtkPolyDataMapper>::New();
#if VTK_MAJOR_VERSION <= 5
    originalMapper->SetInputConnection(input->GetProducerPort());
#else
    originalMapper->SetInputData(input);
#endif

    vtkSmartPointer<vtkProperty> backfaceProp =  vtkSmartPointer<vtkProperty>::New();
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