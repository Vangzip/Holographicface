#include "OdmTexturing.hpp"
#include "FileLibrary.h"
//#include <cxcore.h>
//#include <cv.h>
//#include <highgui.h>
#include <math.h>

//#include "base.h"
//using namespace cv;


double get_distance(CvPoint  aA, CvPoint  aB)
{
    double distanceAB = 0.0;
    distanceAB = sqrt(double(aA.x - aB.x)*double(aA.x - aB.x) + double(aA.y - aB.y)*double(aA.y - aB.y));
    return distanceAB;

}

double get_triangleArea(CvPoint  aA, CvPoint  aB, CvPoint  aC)
{
    double distanceAB = get_distance(aA, aB);
    double distanceBC = get_distance(aB, aC);
    double distanceCA = get_distance(aC, aA);
    double distanceSum = (distanceAB + distanceBC + distanceCA) / 2;
    double area = 0.0;
    area = sqrt(distanceSum*(distanceSum - distanceAB)*(distanceSum - distanceBC)*(distanceSum - distanceCA));
    return area;

}


bool PointinTriangle(CvPoint  a, CvPoint  b, CvPoint  c, CvPoint  p)
{
    /*  double areaABC = get_triangleArea(aA, aB, aC);
    double areaABP = get_triangleArea(aA, aB, aP);
    double areaACP = get_triangleArea(aA, aC, aP);
    double areaBCP = get_triangleArea(aB, aC, aP);
    if (areaABC == areaABP + areaACP + areaBCP)
    {
    return true;
    }
    else
    {
    return false;
    }*/


    float signOfTrig = (b.x - a.x)*(c.y - a.y) - (b.y - a.y)*(c.x - a.x);
    float signOfAB = (b.x - a.x)*(p.y - a.y) - (b.y - a.y)*(p.x - a.x);
    float signOfCA = (a.x - c.x)*(p.y - c.y) - (a.y - c.y)*(p.x - c.x);
    float signOfBC = (c.x - b.x)*(p.y - c.y) - (c.y - b.y)*(p.x - c.x);

    bool d1 = (signOfAB * signOfTrig > 0);
    bool d2 = (signOfCA * signOfTrig > 0);
    bool d3 = (signOfBC * signOfTrig > 0);

    return d1&&d2&&d3;
}


OdmTexturing::OdmTexturing(const std::string &inputply) : log_(false)
{
    logFilePath_ = "odm_texturing_log.txt";

    bundleResizedTo_ = 1200.0;
    textureResolution_ = 4096; //输出纹理分辨率
    nrTextures_ = 0;
    padding_ = 15;

    mesh_ = pcl::TextureMeshPtr(new pcl::TextureMesh);
    patches_ = vector<Patch>(0);
    tTIA_ = vector<int>(0);
	inputModelPath_ = inputply;
}

OdmTexturing::~OdmTexturing()
{

}
int OdmTexturing::run(int argc, char **argv)
{
    if (argc <= 1)
    {
        return EXIT_SUCCESS;
    }

    try
    {
        parseArguments(argc, argv);
        loadMesh();
        loadCameras();
        triangleToImageAssignment();
        calculatePatches();
        sortPatches();
        createTextures();
        writeObjFile();
    }
    catch (const OdmTexturingException& e)
    {
        log_.setIsPrintingInCout(true);
        log_ << "Error in OdmTexturing:\n";
        log_ << e.what() << "\n";
        log_.printToFile(logFilePath_);
        log_ << "For more detailed information, see log file." << "\n";
        return EXIT_FAILURE;
    }
    catch (const exception& e)
    {
        log_.setIsPrintingInCout(true);
        log_ << "Error in OdmTexturing:\n";
        log_ << e.what() << "\n";
        log_.printToFile(logFilePath_);
        log_ << "For more detailed information, see log file." << "\n";
        return EXIT_FAILURE;
    }
    catch (...)
    {
        log_.setIsPrintingInCout(true);
        log_ << "Unknown error in OdmTexturing:\n";
        log_.printToFile(logFilePath_);
        log_ << "For more detailed information, see log file." << "\n";
        return EXIT_FAILURE;
    }

    log_.printToFile(logFilePath_);
    return EXIT_SUCCESS;
}

void OdmTexturing::parseArguments(int argc, char** argv)
{
    for (int argIndex = 1; argIndex < argc; ++argIndex)
    {
        // The argument to be parsed
        string argument = string(argv[argIndex]);
       if (argument == "-camera")
        {
            ++argIndex;
            if (argIndex >= argc)
            {
                throw OdmTexturingException("Missing argument for '" + argument + "'.");
            }
            bundlePath_ = string(argv[argIndex]);
            ifstream testFile(bundlePath_.c_str(), ios_base::binary);
            if (!testFile.is_open())
            {
                throw OdmTexturingException("Argument '" + argument + "' has a bad value (file not accessible).");
            }
        }
        else if (argument == "-image")
        {
            ++argIndex;
            if (argIndex >= argc)
            {
                throw OdmTexturingException("Missing argument for '" + argument + "'.");
            }
            imagesListPath_ = string(argv[argIndex]);
            ifstream testFile(imagesListPath_.c_str(), ios_base::binary);
            if (!testFile.is_open())
            {
                throw OdmTexturingException("Argument '" + argument + "' has a bad value (file not accessible).");
            }
        }
        else if (argument == "-model")
        {
            ++argIndex;
            if (argIndex >= argc)
            {
                throw OdmTexturingException("Missing argument for '" + argument + "'.");
            }
            inputModelPath_ = string(argv[argIndex]);
            ifstream testFile(inputModelPath_.c_str(), ios_base::binary);
            if (!testFile.is_open())
            {
                throw OdmTexturingException("Argument '" + argument + "' has a bad value (file not accessible).");

            }
        }
        else if (argument == "-out")
        {
            ++argIndex;
            if (argIndex >= argc)
            {
                throw OdmTexturingException("Missing argument for '" + argument + "'.");
            }
            stringstream ss(argv[argIndex]);
            ss >> outputFolder_;
            if (ss.bad())
            {
                throw OdmTexturingException("Argument '" + argument + "' has a bad value. (wrong type)");
            }
        }
        else if (argument == "-logFile")
        {
            ++argIndex;
            if (argIndex >= argc)
            {
                throw OdmTexturingException("Missing argument for '" + argument + "'.");
            }
            logFilePath_ = string(argv[argIndex]);
            ofstream testFile(logFilePath_.c_str());
            if (!testFile.is_open())
            {
                throw OdmTexturingException("Argument '" + argument + "' has a bad value.");
            }
        }
        else if (argument == "-textureResolution")
        {
            ++argIndex;
            if (argIndex >= argc)
            {
                throw OdmTexturingException("Missing argument for '" + argument + "'.");
            }
            stringstream ss(argv[argIndex]);
            ss >> textureResolution_;
            if (ss.bad())
            {
                throw OdmTexturingException("Argument '" + argument + "' has a bad value. (wrong type)");
            }
        }
    }


}

void OdmTexturing::loadMesh()
{
    // Read model from ply-file
    pcl::PolygonMeshPtr plyMeshPtr(new pcl::PolygonMesh);
    if (pcl::io::loadPLYFile(inputModelPath_, *plyMeshPtr.get()) == -1)
    {
        throw OdmTexturingException("Error when reading model from:\n" + inputModelPath_ + "\n");
    }

    // Transfer data from ply file to TextureMesh
    mesh_->cloud = plyMeshPtr->cloud;
    vector<pcl::Vertices> polygons;

    // Push faces from ply-mesh into TextureMesh
    polygons.resize(plyMeshPtr->polygons.size());
    for (size_t i = 0; i < plyMeshPtr->polygons.size(); ++i)
    {
        polygons[i] = plyMeshPtr->polygons[i];
    }
    mesh_->tex_polygons.push_back(polygons);

}

void OdmTexturing::loadCameras()
{
    ifstream bundleFile, imageListFile;
    int nrCameras;// = atoi(piclist.begin()->first.c_str());
    string parentPath = FileLibrary::getInstance()->getFileParentPath(imagesListPath_);
    imageListFile.open(imagesListPath_.c_str());
    string line;
    //map<string,int>piclist;
    vector<string> piclist;
    int i = 0;
    getline(imageListFile, line);
    list<string> listfile;
    if (line == "all")
    {

        FileLibrary::getInstance()->getAllSubFiles(parentPath, listfile, false, true, false, ".jpg");
        if (listfile.size() == 0)
        {
            FileLibrary::getInstance()->getAllSubFiles(parentPath, listfile, false, true, false, ".JPG");

        }

        if (listfile.size() == 0)
        {
            cout << "not find images ." << endl;
            return;
        }

        auto it = listfile.begin();
        for (int num = 1; it != listfile.end(); it++, num++)
        {
            string tmpfile = FileLibrary::getInstance()->getFileNameFromPath(*it);
            //piclist.insert(make_pair(tmpfile, num));
            piclist.push_back(tmpfile);
        }
        nrCameras = piclist.size();
    }
    else
    {
        nrCameras = atoi(line.c_str());

        while (getline(imageListFile, line) && i<nrCameras)
        {
            if (line.find("#") != string::npos || line.length() > 15 || line.empty())
                continue;

            //piclist.insert(make_pair(line, i));
            piclist.push_back(line);
            i++;
        }

        imageListFile.close();
    }

    if (piclist.size() == 0)
    {
        throw OdmTexturingException("image file false.");

    }



    // A temporary storage for a line from the file.
    string dummyLine = "";

    //getline(bundleFile, dummyLine);

    for (auto it = piclist.begin(); it != piclist.end(); it++)
    {

        // camera info
        bundleFile.open(bundlePath_.c_str());

        // Check if file is open
        if (!bundleFile.is_open())
        {
            throw OdmTexturingException("Error when reading the bundle file.");
        }
        else
        {
            //log_ << "Successfully read the bundle file.\n";
        }

        i = 0;
        while (getline(bundleFile, line) && i < nrCameras)
        {
            if (line != *it)
            {
                continue;
            }

            i++;
            double val;
            string tmp;
            pcl::TextureMapping<pcl::PointXYZ>::Camera cam;
            Eigen::Affine3f transform;
            //bundleFile >> cam.texture_file;
            cam.texture_file = parentPath + "\\" + line;

            //log_.printToFile(cam.texture_file);
            bundleFile >> tmp;
            bundleFile >> val; //Read focal length from bundle file
            cam.focal_length = val;
            cout << "texture file : " << cam.texture_file << ", focal: " << val << endl;
           
            bundleFile >> val; //Read k1 from bundle file
            cam.width = val * 2;
            bundleFile >> val; //Read k2 from bundle file
            cam.height = val * 2;


            bundleFile >> val; transform(0, 3) = val; // Read translation (0,3) from bundle file
            bundleFile >> val; transform(1, 3) = val; // Read translation (1,3) from bundle file
            bundleFile >> val; transform(2, 3) = val; // Read translation (2,3) from bundle file

            bundleFile >> val; bundleFile >> val; bundleFile >> val; bundleFile >> val; bundleFile >> val;
            bundleFile >> val; bundleFile >> val; bundleFile >> val; bundleFile >> val; bundleFile >> val;

            bundleFile >> val; transform(0, 0) = val; // Read rotation (0,0) from bundle file
            bundleFile >> val; transform(0, 1) = val; // Read rotation (0,1) from bundle file
            bundleFile >> val; transform(0, 2) = val; // Read rotation (0,2) from bundle file

            bundleFile >> val; transform(1, 0) = val; // Read rotation (1,0) from bundle file
            bundleFile >> val; transform(1, 1) = val; // Read rotation (1,1) from bundle file
            bundleFile >> val; transform(1, 2) = val; // Read rotation (1,2) from bundle file

            bundleFile >> val; transform(2, 0) = val; // Read rotation (2,0) from bundle file
            bundleFile >> val; transform(2, 1) = val; // Read rotation (2,1) from bundle file
            bundleFile >> val; transform(2, 2) = val; // Read rotation (2,2) from bundle file


            transform(3, 0) = 0.0;
            transform(3, 1) = 0.0;
            transform(3, 2) = 0.0;
            transform(3, 3) = 1.0;

            // Set values from bundle to current camera
            cam.pose = transform;

            // Read image to get full resolution size
            cv::Mat image = cv::imread(cam.texture_file);

            if (image.empty())
            {
                throw OdmTexturingException("Failed to read image:\n'" + cam.texture_file + "'\n");
            }

        
            // Add camera
            cameras_.push_back(cam);
        }
        bundleFile.close();
    }
    cout << "cameras_: " << cameras_.size() << endl;
}

void OdmTexturing::triangleToImageAssignment()
{
    // Resize the triangleToImageAssigmnent vector to match the number of faces in the mesh
    tTIA_.resize(mesh_->tex_polygons[0].size());

    // Set all values in the triangleToImageAssignment vector to a default value (-1) if there are no optimal camera
    for (size_t i = 0; i < tTIA_.size(); ++i)
    {
        tTIA_[i] = -1;
    }

    // Vector containing information if the face has been given an optimal camera or not
    vector<bool> hasOptimalCamera = vector<bool>(mesh_->tex_polygons[0].size());

    vector<double> tTIA_distances(mesh_->tex_polygons[0].size(), DBL_MAX);
    //Vector containing minimal angles of face to cameraplane normals
    vector<double> tTIA_angles(mesh_->tex_polygons[0].size(), DBL_MAX);


    // Set default value that no face has an optimal camera
    for (size_t faceIndex = 0; faceIndex < hasOptimalCamera.size(); ++faceIndex)
    {
        hasOptimalCamera[faceIndex] = false;
    }

    // Convert vertices to pcl::PointXYZ cloud
    pcl::PointCloud<pcl::PointXYZ>::Ptr meshCloud(new pcl::PointCloud<pcl::PointXYZ>);
    pcl::fromPCLPointCloud2(mesh_->cloud, *meshCloud);

    // Create dummy point and UV-index for vertices not visible in any cameras
    pcl::PointXY nanPoint;
    nanPoint.x = numeric_limits<float>::quiet_NaN();
    nanPoint.y = numeric_limits<float>::quiet_NaN();
    pcl::texture_mapping::UvIndex uvNull;
    uvNull.idx_cloud = -1;
    uvNull.idx_face = -1;

    for (size_t cameraIndex = 0; cameraIndex < cameras_.size(); ++cameraIndex)
    {
        // Move vertices in mesh into the camera coordinate system 将网格中的顶点移动到摄像机坐标系统中
        pcl::PointCloud<pcl::PointXYZ>::Ptr cameraCloud(new pcl::PointCloud<pcl::PointXYZ>);
        pcl::transformPointCloud(*meshCloud, *cameraCloud, cameras_[cameraIndex].pose);

        // Cloud to contain points projected into current camera
        pcl::PointCloud<pcl::PointXY>::Ptr projections(new pcl::PointCloud<pcl::PointXY>);

        // Vector containing information if the polygon is visible in current camera
        vector<bool> visibility;
        visibility.resize(mesh_->tex_polygons[0].size());

        // Vector for remembering the correspondence between uv-coordinates and faces  用来记住uv坐标系和面相对应的矢量
        vector<pcl::texture_mapping::UvIndex> indexUvToPoints;

        // Count the number of vertices inside the camera frustum  计算当前相机内的顶点数的个数
        int countInsideFrustum = 0;

        // Frustum culling for all faces
        for (size_t faceIndex = 0; faceIndex < mesh_->tex_polygons[0].size(); ++faceIndex)
        {
            // Variables for the face vertices as projections in the camera plane
            pcl::PointXY pixelPos0; pcl::PointXY pixelPos1; pcl::PointXY pixelPos2;

            // If the face is inside the camera frustum
            if (isFaceProjected(cameras_[cameraIndex],
                cameraCloud->points[mesh_->tex_polygons[0][faceIndex].vertices[0]],
                cameraCloud->points[mesh_->tex_polygons[0][faceIndex].vertices[1]],
                cameraCloud->points[mesh_->tex_polygons[0][faceIndex].vertices[2]],
                pixelPos0, pixelPos1, pixelPos2))
            {
                // Add pixel positions in camera to projections
                projections->points.push_back((pixelPos0));
                projections->points.push_back((pixelPos1));
                projections->points.push_back((pixelPos2));

                // Remember corresponding face
                pcl::texture_mapping::UvIndex u1, u2, u3;
                u1.idx_cloud = mesh_->tex_polygons[0][faceIndex].vertices[0];
                u2.idx_cloud = mesh_->tex_polygons[0][faceIndex].vertices[1];
                u3.idx_cloud = mesh_->tex_polygons[0][faceIndex].vertices[2];
                u1.idx_face = faceIndex; u2.idx_face = faceIndex; u3.idx_face = faceIndex;
                indexUvToPoints.push_back(u1);
                indexUvToPoints.push_back(u2);
                indexUvToPoints.push_back(u3);

                // Update visibility vector
                visibility[faceIndex] = true;

                // Update count
                ++countInsideFrustum;

            }
            else
            {
                // If not visible set nanPoint and uvNull
                projections->points.push_back(nanPoint);
                projections->points.push_back(nanPoint);
                projections->points.push_back(nanPoint);
                indexUvToPoints.push_back(uvNull);
                indexUvToPoints.push_back(uvNull);
                indexUvToPoints.push_back(uvNull);

                // Update visibility vector
                visibility[faceIndex] = false;
            }


        }
        vector<double> local_tTIA_distances(mesh_->tex_polygons[0].size(), DBL_MAX);
        vector<double> local_tTIA_angles(mesh_->tex_polygons[0].size(), DBL_MAX);
        Patch cameraUV;
        bool fristUV = false;
        // If any faces are visible in the current camera perform occlusion culling   如果在当前的摄像机中有任何面孔可以进行遮挡筛选
        if (countInsideFrustum > 0)
        {
            // Set up acceleration structure
            pcl::KdTreeFLANN<pcl::PointXY> kdTree;
            kdTree.setInputCloud(projections);

            // Loop through all faces and perform occlusion culling for faces inside frustum
            for (size_t faceIndex = 0; faceIndex < mesh_->tex_polygons[0].size(); ++faceIndex)
            {
                if (visibility[faceIndex])
                {
                    // Vectors to store output from radiusSearch in acceleration structure         矢量在加速度结构中储存辐射的输出
                    vector<int> neighbors; vector<float> neighborsSquaredDistance;

                    // Variables for the vertices in face as projections in the camera plane 平面上的顶点的变量作为摄像机平面的投影
                    pcl::PointXY pixelPos0; pcl::PointXY pixelPos1; pcl::PointXY pixelPos2;

                    if (isFaceProjected(cameras_[cameraIndex],
                        cameraCloud->points[mesh_->tex_polygons[0][faceIndex].vertices[0]],
                        cameraCloud->points[mesh_->tex_polygons[0][faceIndex].vertices[1]],
                        cameraCloud->points[mesh_->tex_polygons[0][faceIndex].vertices[2]],
                        pixelPos0, pixelPos1, pixelPos2))
                    {
                        // Variables for a radius circumscribing the polygon in the camera plane and the center of the polygon
                       

                        //在摄像机平面内的多边形和多边形的中心，半径的变化
                        double radius; pcl::PointXY center;

                        // Get values for radius and center
                        getTriangleCircumscribedCircleCentroid(pixelPos0, pixelPos1, pixelPos2, center, radius);

                        // Extract distances for all vertices for face to camera     将所有顶点的距离提取到相机的所有顶点
                        double d0 = cameraCloud->points[mesh_->tex_polygons[0][faceIndex].vertices[0]].z;
                        double d1 = cameraCloud->points[mesh_->tex_polygons[0][faceIndex].vertices[1]].z;
                        double d2 = cameraCloud->points[mesh_->tex_polygons[0][faceIndex].vertices[2]].z;

                        // Calculate largest distance and store in distance variable   计算最大距离和距离变量的存储
                        double distance = max(d0, max(d1, d2));
                        //在加速度结构中执行半径搜索
                        int radiusSearch = kdTree.radiusSearch(center, radius, neighbors, neighborsSquaredDistance);

#if 1
                        //Get points
                        pcl::PointXYZ p0 = cameraCloud->points[mesh_->tex_polygons[0][faceIndex].vertices[0]];
                        pcl::PointXYZ p1 = cameraCloud->points[mesh_->tex_polygons[0][faceIndex].vertices[1]];
                        pcl::PointXYZ p2 = cameraCloud->points[mesh_->tex_polygons[0][faceIndex].vertices[2]];
                        //Calculate face normal
						//面法线计算
                        pcl::PointXYZ diff0;
                        pcl::PointXYZ diff1;
                        diff0.x = p1.x - p0.x;
                        diff0.y = p1.y - p0.y;
                        diff0.z = p1.z - p0.z;
                        diff1.x = p2.x - p0.x;
                        diff1.y = p2.y - p0.y;
                        diff1.z = p2.z - p0.z;
                        pcl::PointXYZ normal;
                        //normal.x = diff0.y*diff1.z - diff0.z*diff1.y;
                        //normal.y = -(diff0.x*diff1.z - diff0.z*diff1.x);
                        //normal.z = diff0.x*diff1.y - diff0.y*diff1.x;

                        normal.x = diff0.y*diff1.z - diff0.z*diff1.y;
                        normal.y = diff0.z*diff1.x - diff0.x*diff1.z;
                        normal.z = diff0.x*diff1.y - diff0.y*diff1.x;

                        double norm = sqrt(normal.x*normal.x + normal.y*normal.y + normal.z*normal.z);
                        //Angle of face to camera
                        double cos = normal.z / norm;

                        double degree = (1.0 - cos*cos);
                        //Save distance of faceIndex to current camera
                        local_tTIA_distances[faceIndex] = distance;

                        //Save angle of faceIndex to current camera
                        if (degree < 0.75 && normal.z < 0 /*&& cos < 0*/)    //基本约束，夹角不能超过60度      
                        {
                            local_tTIA_angles[faceIndex] = degree;//sqrt(1.0 - cos*cos);
                        }
                       
#endif                  


                        // Perform radius search in the acceleration structure
                        // If other projections are found inside the radius     
                        if (radiusSearch > 0)
                        {

                            // Compare distance to all neighbors inside radius   
                            for (size_t i = 0; i < neighbors.size(); ++i)
                            {
                                // Distance variable from neighbor to camera   从邻到相机的距离变量
                                double neighborDistance = cameraCloud->points[indexUvToPoints[neighbors[i]].idx_cloud].z;

                                // If the neighbor has a greater distance to the camera and is inside face polygon set it as not visible
                                //如果邻居有更大的距离和摄像机并且在面多边形中，将其设置为不可见
                                if (distance < neighborDistance)
                                {
                                    //检查邻居点是否在三角形中
                                    if (checkPointInsideTriangle(pixelPos0, pixelPos1, pixelPos2, projections->points[neighbors[i]]))
                                    {
                                        // Update visibility for neighbors 
                                        visibility[indexUvToPoints[neighbors[i]].idx_face] = false;

                                    }
                                }
                            }
                        }
                    }
                }
            }//end  mesh_->tex_polygons[0].size()
        }

        // Number of polygons that add current camera as the optimal camera 将当前相机作为最优相机的多边形数
        // Update optimal cameras for faces visible in current camera       更新当前相机中可见的人脸的最优相机
        for (size_t faceIndex = 0; faceIndex < visibility.size(); ++faceIndex)
        {
            if (visibility[faceIndex])
            {              
                if (tTIA_[faceIndex] == -1) // 三角面对应唯一相机判断                                                 
                {
                    hasOptimalCamera[faceIndex] = true;
                    tTIA_angles[faceIndex] = local_tTIA_angles[faceIndex];
                    tTIA_distances[faceIndex] = local_tTIA_distances[faceIndex];
                    tTIA_[faceIndex] = cameraIndex;
                }

            }
        }


        }// end cameras


    }

    void OdmTexturing::calculatePatches()
    {
        // Convert vertices to pcl::PointXYZ cloud
        pcl::PointCloud<pcl::PointXYZ>::Ptr meshCloud(new pcl::PointCloud<pcl::PointXYZ>);
        pcl::fromPCLPointCloud2(mesh_->cloud, *meshCloud);
        // Reserve size for patches_
        patches_.reserve(tTIA_.size());

        // Vector containing vector with indicies to faces visible in corresponding camera index
        vector<vector<int> > optFaceCameraVector = vector<vector<int> >(cameras_.size());

        // Counter variables for visible and occluded faces
        int countVis = 0;
        int countOcc = 0;

        Patch nonVisibleFaces;
        nonVisibleFaces.optimalCameraIndex_ = -1;
        nonVisibleFaces.materialIndex_ = -1;
        nonVisibleFaces.placed_ = true;

        // Setup vector containing vectors with all faces correspondning to camera according to triangleToImageAssignment vector
        for (size_t i = 0; i < tTIA_.size(); ++i)
        {
            if (tTIA_[i] > -1)
            {
                // If face has an optimal camera add to optFaceCameraVector and update counter for visible faces
                //如果面有一个最优的摄像头，可以添加到optfacecameravopecamera，并为可见的面更新计数器
                countVis++;
                optFaceCameraVector[tTIA_[i]].push_back(i);
            }
            else
            {
                // Add non visible face to patch nonVisibleFaces
                nonVisibleFaces.faces_.push_back(i);
                //log_ << "non Visible Faces faces: " << i << "\n";
                //tTIA_[i] = 0;
                //optFaceCameraVector[tTIA_[i]].push_back(0);

                // Update counter for occluded faces
                countOcc++;
            }
        }
        cout << "non visible Faces:" << countOcc << endl;

        // Loop through all cameras
        for (size_t cameraIndex = 0; cameraIndex < cameras_.size(); ++cameraIndex)
        {
            // Transform mesh into camera coordinate system
            pcl::PointCloud<pcl::PointXYZ>::Ptr cameraCloud(new pcl::PointCloud<pcl::PointXYZ>);
            pcl::transformPointCloud(*meshCloud, *cameraCloud, cameras_[cameraIndex].pose);

            // While faces visible in camera remains to be assigned to a patch 在相机中可以看到的人脸仍然被分配到一个补丁中
            while (0 < optFaceCameraVector[cameraIndex].size())
            {
                // Create current patch
                Patch patch;
                // Vector containing faces to check connectivity with current patch
                vector<size_t> addedFaces = vector<size_t>(0);

                // Add last face in optFaceCameraVector to faces to check connectivity and add it to the current patch
                //在optfacecameravopecamera中添加最后一个面来检查连接并将其添加到当前补丁中
                addedFaces.push_back(optFaceCameraVector[cameraIndex].back());

                // Add first face to patch
                patch.faces_.push_back(optFaceCameraVector[cameraIndex].back());

                // Remove face from optFaceCameraVector
                optFaceCameraVector[cameraIndex].pop_back();

                // Declare uv-coordinates for face
                pcl::PointXY uvCoord1; pcl::PointXY uvCoord2; pcl::PointXY uvCoord3;

                // Calculate uv-coordinates for face in camera
                if (isFaceProjected(cameras_[cameraIndex],
                    cameraCloud->points[mesh_->tex_polygons[0][addedFaces.back()].vertices[0]],
                    cameraCloud->points[mesh_->tex_polygons[0][addedFaces.back()].vertices[1]],
                    cameraCloud->points[mesh_->tex_polygons[0][addedFaces.back()].vertices[2]],
                    uvCoord1, uvCoord2, uvCoord3))
                {
                    // Set minimum and maximum uv-coordinate value for patch
                    patch.minu_ = min(uvCoord1.x, min(uvCoord2.x, uvCoord3.x));
                    patch.minv_ = min(uvCoord1.y, min(uvCoord2.y, uvCoord3.y));
                    patch.maxu_ = max(uvCoord1.x, max(uvCoord2.x, uvCoord3.x));
                    patch.maxv_ = max(uvCoord1.y, max(uvCoord2.y, uvCoord3.y));

                    while (0 < addedFaces.size())
                    {
                        // Set face to check neighbors
                        size_t patchFaceIndex = addedFaces.back();

                        // Remove patchFaceIndex from addedFaces
                        addedFaces.pop_back();

                        // Check against all remaining faces with the same optimal camera   用同样的最优相机检查所有剩余的面
                        for (size_t i = 0; i < optFaceCameraVector[cameraIndex].size(); ++i)
                        {
                            size_t modelFaceIndex = optFaceCameraVector[cameraIndex][i];

                            // Don't check against self
                            if (modelFaceIndex != patchFaceIndex)
                            {
                                // Store indices for vertices of both faces
                                size_t face0v0 = mesh_->tex_polygons[0][modelFaceIndex].vertices[0];
                                size_t face0v1 = mesh_->tex_polygons[0][modelFaceIndex].vertices[1];
                                size_t face0v2 = mesh_->tex_polygons[0][modelFaceIndex].vertices[2];
                                size_t face1v0 = mesh_->tex_polygons[0][patchFaceIndex].vertices[0];
                                size_t face1v1 = mesh_->tex_polygons[0][patchFaceIndex].vertices[1];
                                size_t face1v2 = mesh_->tex_polygons[0][patchFaceIndex].vertices[2];

                                // Count the number of shared vertices
                                size_t nShared = 0;
                                nShared += (face0v0 == face1v0 ? 1 : 0) + (face0v0 == face1v1 ? 1 : 0) + (face0v0 == face1v2 ? 1 : 0);
                                nShared += (face0v1 == face1v0 ? 1 : 0) + (face0v1 == face1v1 ? 1 : 0) + (face0v1 == face1v2 ? 1 : 0);
                                nShared += (face0v2 == face1v0 ? 1 : 0) + (face0v2 == face1v1 ? 1 : 0) + (face0v2 == face1v2 ? 1 : 0);

                                // If sharing a vertex
                                if (nShared > 0)
                                {
                                    // Declare uv-coordinates for face
                                    pcl::PointXY uv1; pcl::PointXY uv2; pcl::PointXY uv3;

                                    // Calculate uv-coordinates for face in camera
                                    isFaceProjected(cameras_[cameraIndex],
                                        cameraCloud->points[mesh_->tex_polygons[0][modelFaceIndex].vertices[0]],
                                        cameraCloud->points[mesh_->tex_polygons[0][modelFaceIndex].vertices[1]],
                                        cameraCloud->points[mesh_->tex_polygons[0][modelFaceIndex].vertices[2]],
                                        uv1, uv2, uv3);

                                    // Update minimum and maximum uv-coordinate value for patch
                                    patch.minu_ = min(patch.minu_, min(uv1.x, min(uv2.x, uv3.x)));
                                    patch.minv_ = min(patch.minv_, min(uv1.y, min(uv2.y, uv3.y)));
                                    patch.maxu_ = max(patch.maxu_, max(uv1.x, max(uv2.x, uv3.x)));
                                    patch.maxv_ = max(patch.maxv_, max(uv1.y, max(uv2.y, uv3.y)));

                                    // Add modelFaceIndex to patch
                                    patch.faces_.push_back(modelFaceIndex);

                                    // Add modelFaceIndex from faces to check for neighbors with same optimal camera
                                    addedFaces.push_back(modelFaceIndex);

                                    // Remove modelFaceIndex from optFaceCameraVector to exclude it from comming iterations
                                    optFaceCameraVector[cameraIndex].erase(optFaceCameraVector[cameraIndex].begin() + i);
                                }

                            }
                        }
                    }
                }

                // Set optimal camera for patch
                patch.optimalCameraIndex_ = static_cast<int>(cameraIndex);

                // Add patch to patches_ vector
                patches_.push_back(patch);
            }
        }
        cout << "patches_: " << patches_.size() << endl;
        patches_.push_back(nonVisibleFaces);



    }

    Coords OdmTexturing::recursiveFindCoords(Node &n, float w, float h)
    {
        // Coordinates to return and place patch
        Coords c;

        if (NULL != n.lft_)
        {
            c = recursiveFindCoords(*(n.lft_), w, h);
            if (c.success_)
            {
                return c;
            }
            else
            {
                return recursiveFindCoords(*(n.rgt_), w, h);
            }
        }
        else
        {
            // If the patch is to large or occupied return success false for coord
            if (n.used_ || w > n.width_ || h > n.height_)
            {
                c.success_ = false;
                return c;
            }

            // If the patch matches perfectly, store it
            if (w == n.width_ && h == n.height_)
            {
                n.used_ = true;
                c.r_ = n.r_;
                c.c_ = n.c_;
                c.success_ = true;

                return c;
            }

            // Initialize children for node
            n.lft_ = new Node(n);
            n.rgt_ = new Node(n);

            n.rgt_->used_ = false;
            n.lft_->used_ = false;
            n.rgt_->rgt_ = NULL;
            n.rgt_->lft_ = NULL;
            n.lft_->rgt_ = NULL;
            n.lft_->lft_ = NULL;

            // Check how to adjust free space  检查如何调整空闲空间
            if (n.width_ - w > n.height_ - h)
            {
                n.lft_->width_ = w;
                n.rgt_->c_ = n.c_ + w;
                n.rgt_->width_ = n.width_ - w;
            }
            else
            {
                n.lft_->height_ = h;
                n.rgt_->r_ = n.r_ + h;
                n.rgt_->height_ = n.height_ - h;
            }

            return recursiveFindCoords(*(n.lft_), w, h);
        }
    }

    void OdmTexturing::sortPatches()
    {
        // Bool to set true when done
        bool done = false;

        // Material index
        int materialIndex = 0;

        // Number of patches left from last loop
        size_t countLeftLastIteration = 0;

        while (!done)
        {
            // Create container for current material
            Node root;
            root.width_ = textureResolution_;
            root.height_ = textureResolution_;

            // Set done to true
            done = true;

            // Number of patches that did not fit in current material
            size_t countNotPlacedPatches = 0;

            // Number of patches placed in current material
            size_t placed = 0;

            for (size_t patchIndex = 0; patchIndex < patches_.size(); ++patchIndex)
            {
                if (!patches_[patchIndex].placed_)
                {
                    // Calculate dimensions of the patch  计算patch的尺寸
                    float w = patches_[patchIndex].maxu_ - patches_[patchIndex].minu_ + 2 * padding_;
                    float h = patches_[patchIndex].maxv_ - patches_[patchIndex].minv_ + 2 * padding_;

                    // Try to place patch in root container for this material 尝试在根容器中放置补丁
                    if (w > 0.0 && h > 0.0)
                    {
                        patches_[patchIndex].c_ = recursiveFindCoords(root, w, h);
                    }

                    if (!patches_[patchIndex].c_.success_)
                    {
                        ++countNotPlacedPatches;
                        done = false;
                    }
                    else
                    {
                        // Set patch material as current material
                        patches_[patchIndex].materialIndex_ = materialIndex;

                        // Set patch as placed
                        patches_[patchIndex].placed_ = true;

                        // Update number of patches placed in current material
                        placed++;

                        // Update patch with padding_   更新补丁和填充
                        //patches_[patchIndex].c_.c_ += padding_;
                        //patches_[patchIndex].c_.r_ += padding_;
                        patches_[patchIndex].minu_ -= padding_;
                        patches_[patchIndex].minv_ -= padding_;
                        patches_[patchIndex].maxu_ = min((patches_[patchIndex].maxu_ + padding_), textureResolution_);
                        patches_[patchIndex].maxv_ = min((patches_[patchIndex].maxv_ + padding_), textureResolution_);
                    }
                }
            }
            ++materialIndex;

            // Update material index

            if (countLeftLastIteration == countNotPlacedPatches && countNotPlacedPatches != 0)
            {
                done = true;
            }
            countLeftLastIteration = countNotPlacedPatches;
        }

        // Set number of textures
        nrTextures_ = materialIndex;
        cout << "textures num: " << nrTextures_ << endl;
        log_ << "Faces sorted into " << nrTextures_ << " textures.\n";
    }

    void OdmTexturing::createTextures()
    {

        // Convert vertices to pcl::PointXYZ cloud
        pcl::PointCloud<pcl::PointXYZ>::Ptr meshCloud(new pcl::PointCloud<pcl::PointXYZ>);
        pcl::fromPCLPointCloud2(mesh_->cloud, *meshCloud);

        // Container for faces according to submesh. Used to replace faces in mesh_.
        vector<vector<pcl::Vertices> > faceVector = vector<vector<pcl::Vertices> >(nrTextures_ + 1);

        // Container for texture coordinates according to submesh. Used to replace texture coordinates in mesh_.
        vector<vector<Eigen::Vector2f, Eigen::aligned_allocator<Eigen::Vector2f>> > textureCoordinatesVector = vector<vector<Eigen::Vector2f, Eigen::aligned_allocator<Eigen::Vector2f>> >(nrTextures_ + 1);

        // Container for materials according to submesh. Used to replace materials in mesh_.
        vector<pcl::TexMaterial> materialVector = vector<pcl::TexMaterial>(nrTextures_ + 1);

        // Setup model according to patches placement
        for (int textureIndex = 0; textureIndex < nrTextures_; ++textureIndex)
        {
            for (size_t patchIndex = 0; patchIndex < patches_.size(); ++patchIndex)
            {
                // If patch is placed in current mesh add all containing faces to that submesh  如果将补丁放在当前的网格中，将所有包含的面添加到该子网格中
                if (patches_[patchIndex].materialIndex_ == textureIndex)
                {
                    // Transform mesh into camera
                    pcl::PointCloud<pcl::PointXYZ>::Ptr cameraCloud(new pcl::PointCloud<pcl::PointXYZ>);
                    pcl::transformPointCloud(*meshCloud, *cameraCloud, cameras_[patches_[patchIndex].optimalCameraIndex_].pose);

                    // Loop through all faces in patch

                    for (size_t faceIndex = 0; faceIndex < patches_[patchIndex].faces_.size(); ++faceIndex)
                    {
                        // Setup global face index in mesh_
                        size_t globalFaceIndex = patches_[patchIndex].faces_[faceIndex];

                        // Add current face to current submesh
                        faceVector[textureIndex].push_back(mesh_->tex_polygons[0][globalFaceIndex]);

                        // Pixel positions
                        pcl::PointXY pixelPos0; pcl::PointXY pixelPos1; pcl::PointXY pixelPos2;

                        // Get pixel positions in corresponding camera for the vertices of the face
                        getPixelCoordinates(cameraCloud->points[mesh_->tex_polygons[0][globalFaceIndex].vertices[0]], cameras_[patches_[patchIndex].optimalCameraIndex_], pixelPos0);
                        getPixelCoordinates(cameraCloud->points[mesh_->tex_polygons[0][globalFaceIndex].vertices[1]], cameras_[patches_[patchIndex].optimalCameraIndex_], pixelPos1);
                        getPixelCoordinates(cameraCloud->points[mesh_->tex_polygons[0][globalFaceIndex].vertices[2]], cameras_[patches_[patchIndex].optimalCameraIndex_], pixelPos2);
                        
                        // Shorthands for patch variables
                        float c = patches_[patchIndex].c_.c_ + padding_;
                        float r = patches_[patchIndex].c_.r_ + padding_;
                        float minu = patches_[patchIndex].minu_ + padding_;
                        float minv = patches_[patchIndex].minv_ + padding_;

                        // Declare uv coordinates
                        Eigen::Vector2f uv1, uv2, uv3;

                        // Set uv coordinates according to patch  根据补丁设置uv坐标 ,顶点uv是在所属path的位置
                        uv1(0) = (pixelPos0.x - minu + c) / textureResolution_;
                        uv1(1) = 1.0f - (pixelPos0.y - minv + r) / textureResolution_;

                        uv2(0) = (pixelPos1.x - minu + c) / textureResolution_;
                        uv2(1) = 1.0f - (pixelPos1.y - minv + r) / textureResolution_;

                        uv3(0) = (pixelPos2.x - minu + c) / textureResolution_;
                        uv3(1) = 1.0f - (pixelPos2.y - minv + r) / textureResolution_;

                        // Add uv coordinates to submesh
                        textureCoordinatesVector[textureIndex].push_back(uv1);
                        textureCoordinatesVector[textureIndex].push_back(uv2);
                        textureCoordinatesVector[textureIndex].push_back(uv3);


                    }
                }
            }

            // Declare material and setup default values
            pcl::TexMaterial meshMaterial;
            meshMaterial.tex_Ka.r = 0.0f; meshMaterial.tex_Ka.g = 0.0f; meshMaterial.tex_Ka.b = 0.0f;
            meshMaterial.tex_Kd.r = 0.0f; meshMaterial.tex_Kd.g = 0.0f; meshMaterial.tex_Kd.b = 0.0f;
            meshMaterial.tex_Ks.r = 0.0f; meshMaterial.tex_Ks.g = 0.0f; meshMaterial.tex_Ks.b = 0.0f;
            meshMaterial.tex_d = 1.0f; meshMaterial.tex_Ns = 200.0f; meshMaterial.tex_illum = 2;
            stringstream tex_name;
            tex_name << "texture_" << textureIndex;
            tex_name >> meshMaterial.tex_name;
            meshMaterial.tex_file = meshMaterial.tex_name + ".jpg";
            materialVector[textureIndex] = meshMaterial;
        }

        // Add non visible patches to submesh
        for (size_t patchIndex = 0; patchIndex < patches_.size(); ++patchIndex)
        {
            // If the patch does not have an optimal camera
            if (patches_[patchIndex].optimalCameraIndex_ == -1)
            {
                // Add all faces and set uv coordinates
                for (size_t faceIndex = 0; faceIndex < patches_[patchIndex].faces_.size(); ++faceIndex)
                {
                    // Setup global face index in mesh_
                    size_t globalFaceIndex = patches_[patchIndex].faces_[faceIndex];

                    // Add current face to current submesh
                    faceVector[nrTextures_].push_back(mesh_->tex_polygons[0][globalFaceIndex]);

                    // Declare uv coordinates
                    Eigen::Vector2f uv1, uv2, uv3;

                    // Set uv coordinates according to patch
                    uv1(0) = 0.25f;//(pixelPos0.x - minu + c)/textureResolution_;
                    uv1(1) = 0.25f;//1.0f - (pixelPos0.y - minv + r)/textureResolution_;

                    uv2(0) = 0.25f;//(pixelPos1.x - minu + c)/textureResolution_;
                    uv2(1) = 0.75f;//1.0f - (pixelPos1.y - minv + r)/textureResolution_;

                    uv3(0) = 0.75f;//(pixelPos2.x - minu + c)/textureResolution_;
                    uv3(1) = 0.75f;//1.0f - (pixelPos2.y - minv + r)/textureResolution_;

                    // Add uv coordinates to submesh
                    textureCoordinatesVector[nrTextures_].push_back(uv1);
                    textureCoordinatesVector[nrTextures_].push_back(uv2);
                    textureCoordinatesVector[nrTextures_].push_back(uv3);
                }
            }
        }

        // Declare material and setup default values for nonVisibileFaces submesh
        pcl::TexMaterial meshMaterial;
        meshMaterial.tex_Ka.r = 0.0f; meshMaterial.tex_Ka.g = 0.0f; meshMaterial.tex_Ka.b = 0.0f;
        meshMaterial.tex_Kd.r = 0.0f; meshMaterial.tex_Kd.g = 0.0f; meshMaterial.tex_Kd.b = 0.0f;
        meshMaterial.tex_Ks.r = 0.0f; meshMaterial.tex_Ks.g = 0.0f; meshMaterial.tex_Ks.b = 0.0f;
        meshMaterial.tex_d = 1.0f; meshMaterial.tex_Ns = 200.0f; meshMaterial.tex_illum = 2;
        stringstream tex_name;
        tex_name << "non_visible_faces_texture";
        tex_name >> meshMaterial.tex_name;
        meshMaterial.tex_file = meshMaterial.tex_name + ".jpg";
        materialVector[nrTextures_] = meshMaterial;

        // Replace polygons, texture coordinates and materials in mesh_
        mesh_->tex_polygons = faceVector;
        mesh_->tex_coordinates = textureCoordinatesVector;
        mesh_->tex_materials = materialVector;

        // Containers for image and the resized image used for texturing
        cv::Mat image;
        cv::Mat resizedImage;
        for (int textureIndex = 0; textureIndex < nrTextures_; ++textureIndex)
        {
            // Current texture for corresponding material
            cv::Mat texture = cv::Mat::zeros(textureResolution_, textureResolution_, CV_8UC3);

            for (int cameraIndex = 0; cameraIndex < static_cast<int>(cameras_.size()); ++cameraIndex)
            {
                // Load image for current camera
                cout << "cameras index : " << cameraIndex << endl;
                image = cv::imread(cameras_[cameraIndex].texture_file, 1);

                // Calculate the resize factor to texturize with textureWithSize_
                double resizeFactor = 1;

                // Resize image to the resolution used to texture with
                cv::resize(image, resizedImage, cv::Size(), resizeFactor, resizeFactor, CV_INTER_AREA);

                // Loop through all patches
                for (size_t patchIndex = 0; patchIndex < patches_.size(); ++patchIndex)
                {
                    // If the patch has the current camera as optimal camera  如果这个补丁有当前的相机作为最优相机
                    if (patches_[patchIndex].materialIndex_ == textureIndex && patches_[patchIndex].optimalCameraIndex_ == cameraIndex)
                    {
                        // Pixel coordinates to extract image information from   像素坐标提取图像信息
                        int extractX = static_cast<int>(floor(patches_[patchIndex].minu_));// +padding_);
                        int extractY = static_cast<int>(floor(patches_[patchIndex].minv_));// +padding_);

                        // Pixel coordinates to insert the image information to    像素坐标将图像信息插入
                        int insertX = static_cast<int>(floor(patches_[patchIndex].c_.c_));
                        int insertY = static_cast<int>(floor(patches_[patchIndex].c_.r_));

                        // The size of the image information to use 使用的图像信息的大小
                        int width = static_cast<int>(floor(patches_[patchIndex].maxu_)) - extractX - 1;
                        int height = static_cast<int>(floor(patches_[patchIndex].maxv_)) - extractY - 1;


                        // Get image information and add to texture 获取图像信息并添加到纹理中
                        cv::Mat src = resizedImage(cv::Rect(extractX, extractY, width, height));
                        cv::Mat dst = texture(cv::Rect(insertX, insertY, width, height));
                        src.copyTo(dst);

                    }
                }//end for
            }//end cameras

            cv::imwrite(outputFolder_ + mesh_->tex_materials[textureIndex].tex_file, texture);
        }
        // Create nonVisibleFaces texture and save to file
        cv::Mat nonVisibleFacesTexture = cv::Mat::zeros(50, 50, CV_8UC3) + cv::Scalar(255, 255, 255);
        cv::imwrite(outputFolder_ + mesh_->tex_materials[nrTextures_].tex_file, nonVisibleFacesTexture);
    }

    void OdmTexturing::writeObjFile()
    {
        string outfile = FileLibrary::getInstance()->combineFilePath(outputFolder_,"model.obj");
        if (testsaveOBJFile(outfile, *mesh_.get(), 7) == 0)
        {
            cout << "save obj: " << outfile << endl;
        }
    }



