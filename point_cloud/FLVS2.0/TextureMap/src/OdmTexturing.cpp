#include "OdmTexturing.hpp"
#include<pcl\io\obj_io.h>

#include <math.h>


bool checkPointTriangle(const pcl::PointXY &p1, const pcl::PointXY &p2, const pcl::PointXY &p3, const pcl::PointXY &pt)
{
    // Compute vectors
    Eigen::Vector2d v0, v1, v2;
    v0(0) = p3.x - p1.x; v0(1) = p3.y - p1.y; // v0= C - A
    v1(0) = p2.x - p1.x; v1(1) = p2.y - p1.y; // v1= B - A
    v2(0) = pt.x - p1.x; v2(1) = pt.y - p1.y; // v2= P - A

    // Compute dot products
    double dot00 = v0.dot(v0); // dot00 = dot(v0, v0)
    double dot01 = v0.dot(v1); // dot01 = dot(v0, v1)
    double dot02 = v0.dot(v2); // dot02 = dot(v0, v2)
    double dot11 = v1.dot(v1); // dot11 = dot(v1, v1)
    double dot12 = v1.dot(v2); // dot12 = dot(v1, v2)


    float inverDeno = 1 / (dot00 * dot11 - dot01 * dot01);

    float u = (dot11 * dot02 - dot01 * dot12) * inverDeno;
    if (u < 0 || u > 1) // if u out of range, return directly
    {
        return false;
    }

    float v = (dot00 * dot12 - dot01 * dot02) * inverDeno;
    if (v < 0 || v > 1) // if v out of range, return directly
    {
        return false;
    }

    return u + v <= 1;

    // Compute barycentric coordinates
    //double invDenom = 1.0 / (dot00*dot11 - dot01*dot01);
    //double u = (dot11*dot02 - dot01*dot12) * invDenom;
    //double v = (dot00*dot12 - dot01*dot02) * invDenom;

    //// Check if point is in triangle
    //return ((u >= 0) && (v >= 0) && (u + v < 1));


    //float signOfTrig = (b.x - a.x)*(c.y - a.y) - (b.y - a.y)*(c.x - a.x);
    //float signOfAB = (b.x - a.x)*(p.y - a.y) - (b.y - a.y)*(p.x - a.x);
    //float signOfCA = (a.x - c.x)*(p.y - c.y) - (a.y - c.y)*(p.x - c.x);
    //float signOfBC = (c.x - b.x)*(p.y - c.y) - (c.y - b.y)*(p.x - c.x);
    //bool d1 = (signOfAB * signOfTrig > 0);
    //bool d2 = (signOfCA * signOfTrig > 0);
    //bool d3 = (signOfBC * signOfTrig > 0);

    //return d1&&d2&&d3;
}

OdmTexturing::OdmTexturing() 
{

    textureWithSize_ = 1024;
    textureResolution_ = 1024;
    nrTextures_ = 1;
    padding_ = 25;

    mesh_ = pcl::TextureMeshPtr(new pcl::TextureMesh);
	mesh_current_ = pcl::TextureMeshPtr(new pcl::TextureMesh);
    angle_ = 90;
}

OdmTexturing::~OdmTexturing()
{

}
int OdmTexturing::run(float){ return 0; }

//************************************
// Method:    run
// Access:    public 
// Returns:   int
// Describe:  
// Parameter: int argc
// Parameter: char * * argv
//************************************
int OdmTexturing::run(int argc, char **argv)
{
    if (argc <= 1)
    {
        return EXIT_SUCCESS;
    }

    parseArguments(argc, argv);
    loadMesh();
    loadCameras();
    triangleToImageAssignment();

    calculatePatches();
    
    //sortPatches();
    createTextures();
    //writeObjFile();

    return EXIT_SUCCESS;
}

//************************************
// Method:    parseArguments
// Access:    private 
// Returns:   void
// Describe:  主要参数数据解析
// Parameter: int argc
// Parameter: char * * argv
//************************************
void OdmTexturing::parseArguments(int argc, char** argv)
{
    for (int argIndex = 1; argIndex < argc; ++argIndex)
    {
        // The argument to be parsed
        string argument = string(argv[argIndex]);
        if (argument == "-help")
        {
        }
        else if (argument == "-verbose")
        {
        }
        else if (argument == "-camera")
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
        else if (argument == "-texture")
        {
            ++argIndex;
            if (argIndex >= argc)
            {
                throw OdmTexturingException("Missing argument for '" + argument + "'.");
            }
            _allTexture = string(argv[argIndex]);
           
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
        else if (argument == "-angle")
        {
            ++argIndex;
            if (argIndex >= argc)
            {
                throw OdmTexturingException("Missing argument for '" + argument + "'.");
            }
            stringstream ss(argv[argIndex]);
            ss >> angle_;
            if (ss.bad())
            {
                throw OdmTexturingException("Argument '" + argument + "' has a bad value. (wrong type)");
            }
        }
    }

    if (textureWithSize_ > textureResolution_)
    {
        textureWithSize_ = textureResolution_;
    }

}

//************************************
// Method:    loadMesh
// Access:    private 
// Returns:   void
// Describe:  obj模型加载
//************************************
void OdmTexturing::loadMesh()
{
    pcl::PolygonMeshPtr plyMeshPtr(new pcl::PolygonMesh);

    //加载obj模型文件，获取面所有关系 bfzhao
    if (pcl::io::loadOBJFile(inputModelPath_, *mesh_.get()) == -1)
    {
        cout << "load model false ." << endl;
        return;
    }

    pcl::copyPointCloud(mesh_->cloud, mesh_current_->cloud);

}

//************************************
// Method:    loadCameras
// Access:    private 
// Returns:   void
// Describe:  根据pic.txt文件指定的加载相机参数
//************************************
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
    if (line == "all") //配置文件第一行"all"表示遍历当前目录下所有jpg文件
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
    //遍历获取相机参数
    for (auto it = piclist.begin(); it != piclist.end(); it++)
    {

        // camera info
        bundleFile.open(bundlePath_.c_str());

        // Check if file is open
        if (!bundleFile.is_open())
        {
            throw OdmTexturingException("Error when reading the bundle file.");
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


//************************************
// Method:    triangleToImageAssignment
// Access:    private 
// Returns:   void
// Describe:  每个三角面最优相机选择
//************************************
void OdmTexturing::triangleToImageAssignment()
{

    // Convert vertices to pcl::PointXYZ cloud
    pcl::PointCloud<pcl::PointXYZ>::Ptr meshCloud(new pcl::PointCloud<pcl::PointXYZ>);
    pcl::fromPCLPointCloud2(mesh_->cloud, *meshCloud);

    for (size_t cameraIndex = 0; cameraIndex < cameras_.size(); ++cameraIndex)
    {
        // Move vertices in mesh into the camera coordinate system 将网格中的顶点移动到摄像机坐标系统中
        pcl::PointCloud<pcl::PointXYZ>::Ptr cameraCloud(new pcl::PointCloud<pcl::PointXYZ>);
        pcl::transformPointCloud(*meshCloud, *cameraCloud, cameras_[cameraIndex].pose);

        std::vector<int> tTIA(mesh_->tex_polygons[0].size() , - 1);

        // Cloud to contain points projected into current camera
        pcl::PointCloud<pcl::PointXY>::Ptr projections(new pcl::PointCloud<pcl::PointXY>);

        // Vector containing information if the polygon is visible in current camera
        vector<bool> visibility;
        visibility.resize(mesh_->tex_polygons[0].size());

        // Vector for remembering the correspondence between uv-coordinates and faces 
        vector<pcl::texture_mapping::UvIndex> indexUvToPoints;

        // Count the number of vertices inside the camera frustum 
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


                // Update visibility vector
                visibility[faceIndex] = false;
            }


        }
        vector<double> local_tTIA_distances(mesh_->tex_polygons[0].size(), DBL_MAX);
        vector<double> local_tTIA_angles(mesh_->tex_polygons[0].size(), DBL_MAX);
        Patch cameraUV;
        bool fristUV = false;
        // If any faces are visible in the current camera perform occlusion culling  
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
                    // Vectors to store output from radiusSearch in acceleration structure        
                    vector<int> neighbors; vector<float> neighborsSquaredDistance;

                    // Variables for the vertices in face as projections in the camera plane
                    pcl::PointXY pixelPos0; pcl::PointXY pixelPos1; pcl::PointXY pixelPos2;

                    if (isFaceProjected(cameras_[cameraIndex],
                        cameraCloud->points[mesh_->tex_polygons[0][faceIndex].vertices[0]],
                        cameraCloud->points[mesh_->tex_polygons[0][faceIndex].vertices[1]],
                        cameraCloud->points[mesh_->tex_polygons[0][faceIndex].vertices[2]],
                        pixelPos0, pixelPos1, pixelPos2))
                    {
                        // Variables for a radius circumscribing the polygon in the camera plane and the center of the polygon
                       

                        double radius; pcl::PointXY center;

                        // Get values for radius and center
                        getTriangleCircumscribedCircleCentroid(pixelPos0, pixelPos1, pixelPos2, center, radius);

                        // Extract distances for all vertices for face to camera    
                        double d0 = cameraCloud->points[mesh_->tex_polygons[0][faceIndex].vertices[0]].z;
                        double d1 = cameraCloud->points[mesh_->tex_polygons[0][faceIndex].vertices[1]].z;
                        double d2 = cameraCloud->points[mesh_->tex_polygons[0][faceIndex].vertices[2]].z;

                        // Calculate largest distance and store in distance variable  
                        double distance = max(d0, max(d1, d2));
                        //在加速度结构中执行半径搜索
                        int radiusSearch = kdTree.radiusSearch(center, radius, neighbors, neighborsSquaredDistance);

#if 1
                        //Get points
                        pcl::PointXYZ p0 = cameraCloud->points[mesh_->tex_polygons[0][faceIndex].vertices[0]];
                        pcl::PointXYZ p1 = cameraCloud->points[mesh_->tex_polygons[0][faceIndex].vertices[1]];
                        pcl::PointXYZ p2 = cameraCloud->points[mesh_->tex_polygons[0][faceIndex].vertices[2]];
                        //Calculate face normal

                        pcl::PointXYZ diff0;
                        pcl::PointXYZ diff1;
                        diff0.x = p1.x - p0.x;
                        diff0.y = p1.y - p0.y;
                        diff0.z = p1.z - p0.z;
                        diff1.x = p2.x - p0.x;
                        diff1.y = p2.y - p0.y;
                        diff1.z = p2.z - p0.z;

                        pcl::PointXYZ normal;                      
                        normal.x = diff0.y*diff1.z - diff0.z*diff1.y;
                        normal.y = diff0.z*diff1.x - diff0.x*diff1.z;
                        normal.z = diff0.x*diff1.y - diff0.y*diff1.x;

                        double norm = sqrt(normal.x*normal.x + normal.y*normal.y + normal.z*normal.z);
                        //Angle of face to camera
                        double costmp = normal.z / norm;

                        double degree = (1.0 - costmp*costmp);
                        //Save distance of faceIndex to current camera
                        local_tTIA_distances[faceIndex] = distance;

                        //Save angle of faceIndex to current camera
                        if (costmp > cos((180 - angle_ * M_PI / 180)))
                        {
                            visibility[faceIndex] = false;
                        }
#endif                  


                        // Perform radius search in the acceleration structure
                        // If other projections are found inside the radius     
                        if (radiusSearch > 0)
                        {

                            // Compare distance to all neighbors inside radius  
                            for (size_t i = 0; i < neighbors.size(); ++i)
                            {
                                // Distance variable from neighbor to camera  
                                double neighborDistance = cameraCloud->points[indexUvToPoints[neighbors[i]].idx_cloud].z;

                                // If the neighbor has a greater distance to the camera and is inside face polygon set it as not visible
                                if (distance < neighborDistance)
                                {
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

        // Number of polygons that add current camera as the optimal camera 
        // Update optimal cameras for faces visible in current camera      
        for (size_t faceIndex = 0; faceIndex < visibility.size(); ++faceIndex)
        {
            if (visibility[faceIndex])
            {              
                if (tTIA[faceIndex] == -1)
                {
                    tTIA[faceIndex] = cameraIndex;
                }

            }
        }

        //camerasPathes_.push_back();
        tTIAVector_.push_back(tTIA);
        }// end cameras


    }

//************************************
// Method:    calculatePatches
// Access:    private 
// Returns:   void
// Describe:  每个三角面patch对应的像素坐标计算
//************************************
void OdmTexturing::calculatePatches()
{
    // Convert vertices to pcl::PointXYZ cloud
    pcl::PointCloud<pcl::PointXYZ>::Ptr meshCloud(new pcl::PointCloud<pcl::PointXYZ>);
    pcl::fromPCLPointCloud2(mesh_->cloud, *meshCloud);
    // Reserve size for patches_


    // Counter variables for visible and occluded faces
    int countOcc = 0;

    Patch nonVisibleFaces;
    nonVisibleFaces.optimalCameraIndex_ = -1;
    nonVisibleFaces.materialIndex_ = -1;
    nonVisibleFaces.placed_ = true;

  

    // Loop through all cameras
    for (size_t cameraIndex = 0; cameraIndex < cameras_.size(); ++cameraIndex)
    {
        int countVis = 0;

        vector<vector<int> > optFaceCameraVector = vector<vector<int> >(cameras_.size());
        std::vector<int> tTIA = tTIAVector_[cameraIndex];

        std::vector<Patch> patches;

        for (size_t i = 0; i < tTIA.size(); ++i)
        {
            if (tTIA[i] > -1)
            {
                countVis++;
                optFaceCameraVector[tTIA[i]].push_back(i);
            }
            else
            {
                nonVisibleFaces.faces_.push_back(i);
                countOcc++;
            }
        }

        cout << "visible Faces:" << countVis << endl;

        // Transform mesh into camera coordinate system
        pcl::PointCloud<pcl::PointXYZ>::Ptr cameraCloud(new pcl::PointCloud<pcl::PointXYZ>);
        pcl::transformPointCloud(*meshCloud, *cameraCloud, cameras_[cameraIndex].pose);

        // While faces visible in camera remains to be assigned to a patch 
        while (0 < optFaceCameraVector[cameraIndex].size())
        {
            Patch patch;
            vector<size_t> addedFaces = vector<size_t>(0);

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
                uvCoord1.x = round(uvCoord1.x);
                uvCoord1.y = round(uvCoord1.y);

                uvCoord2.x = round(uvCoord2.x);
                uvCoord2.y = round(uvCoord2.y);

                uvCoord3.x = round(uvCoord3.x);
                uvCoord3.y = round(uvCoord3.y);


                // Set minimum and maximum uv-coordinate value for patch
                patch.minu_ = min(uvCoord1.x, min(uvCoord2.x, uvCoord3.x));
                patch.minv_ = min(uvCoord1.y, min(uvCoord2.y, uvCoord3.y));
                patch.maxu_ = max(uvCoord1.x, max(uvCoord2.x, uvCoord3.x));
                patch.maxv_ = max(uvCoord1.y, max(uvCoord2.y, uvCoord3.y));

                patch.w = patch.maxu_ - patch.minu_;
                patch.h = patch.maxv_ - patch.minv_;

                //保存三角面到像素坐标 bfzhao
                patch.facesPointXY_.push_back(uvCoord1);
                patch.facesPointXY_.push_back(uvCoord2);
                patch.facesPointXY_.push_back(uvCoord3);
                patch.texturefile_ = cameras_[cameraIndex].texture_file;
#if 0
                while (0 < addedFaces.size())
                {
                    // Set face to check neighbors
                    size_t patchFaceIndex = addedFaces.back();

                    // Remove patchFaceIndex from addedFaces
                    addedFaces.pop_back();

                    // Check against all remaining faces with the same optimal camera   
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
#endif
                patch.optimalCameraIndex_ = cameraIndex;
                patches.push_back(patch);
            }

            // Add patch to patches_ vector
        }
        cout << "camera " << cameraIndex<<", patches: " << patches.size() << endl;
        camerasPathes_.push_back(patches);
    }
    //patches_.push_back(nonVisibleFaces);



}

//cv::Mat outtexture(2048, 2048, CV_8UC3, cv::Scalar(255, 255, 0));

//************************************
// Method:    filterRGB
// Access:    public 
// Returns:   bool
// Describe:  过滤黑色
// Parameter: float R
// Parameter: float G
// Parameter: float B
//************************************
bool filterRGB(float R, float G, float B){

    float filterRGB = 50;
    float filterGreen = 20;

    if (B <filterRGB && G<filterRGB && R < filterRGB)
    {
        return true;
    }

    //if (FileLibrary::getInstance()->Rgb2Hsv(R, G, B)){
    //    return true;
    //}

   /* if (FileLibrary::getInstance()->filterGreen(R, G, B, filterGreen)){
        return true;
    }*/

    return false;
}
//三角面纹理图片像素获取
//************************************
// Method:    getMatForFace
// Access:    private 
// Returns:   void
// Describe:  三角面反射变换，颜色过滤，替换
// Parameter: const Patch & srcpatch
// Parameter: const Patch & dstPatch
// Parameter: cv::Mat & srcout
// Parameter: cv::Mat & dstout
//************************************
void OdmTexturing::getMatForFace(const Patch & srcpatch, const Patch & dstPatch, cv::Mat &srcout, cv::Mat &dstout)
{

    cv::Mat image = _srcimage;
    //cv::Mat PixFlag(cv::Size(dstout.rows, dstout.cols), CV_8UC3);
    float offset = 5;
    int maxv = srcpatch.maxv_ + offset >= image.rows ? srcpatch.maxv_ : srcpatch.maxv_ + offset;
    int minv = srcpatch.minv_ - offset < 0 ? srcpatch.minv_ : srcpatch.minv_ - offset;

    int maxu = srcpatch.maxu_ + offset >= image.cols ? srcpatch.maxu_ : srcpatch.maxu_ + offset;;
    int minu = srcpatch.minu_ - offset < 0 ? srcpatch.minu_ : srcpatch.minu_ - offset;;


    //取face的最小矩形纹理
    for (size_t i = minv; i < maxv; i++)
    {
        uchar* p = image.ptr<uchar>(i);  //获取第i行的首地址

        uchar* t = srcout.ptr<uchar>(i - minv);  //获取第i行的首地址

        for (size_t j = minu; j < maxu; j++)
        {
            pcl::PointXY pint;
            pint.x = j;
            pint.y = i;

            //if (checkPointInsideTriangle(srcpatch.facesPointXY_[0], srcpatch.facesPointXY_[1], srcpatch.facesPointXY_[2], pint))
            {
                size_t tj = j - minu;
                t[tj * 3] = p[j * 3];
                t[tj * 3 + 1] = p[j * 3 + 1];
                t[tj * 3 + 2] = p[j * 3 + 2];

            }
        }
    }
    //cv::imwrite("e:\\test\\out\\src.jpg", srcout);

#if 1
    cv::Point2f srcTri[3];
    cv::Point2f dstTri[3];

    srcTri[0] = cv::Point2f(srcpatch.facesPointXY_[0].x - srcpatch.minu_, srcpatch.facesPointXY_[0].y - srcpatch.minv_);
    srcTri[1] = cv::Point2f(srcpatch.facesPointXY_[1].x - srcpatch.minu_, srcpatch.facesPointXY_[1].y - srcpatch.minv_);
    srcTri[2] = cv::Point2f(srcpatch.facesPointXY_[2].x - srcpatch.minu_, srcpatch.facesPointXY_[2].y - srcpatch.minv_);

    dstTri[0] = cv::Point2f(dstPatch.facesPointXY_[0].x - dstPatch.minu_, dstPatch.facesPointXY_[0].y - dstPatch.minv_);
    dstTri[1] = cv::Point2f(dstPatch.facesPointXY_[1].x - dstPatch.minu_, dstPatch.facesPointXY_[1].y - dstPatch.minv_);
    dstTri[2] = cv::Point2f(dstPatch.facesPointXY_[2].x - dstPatch.minu_, dstPatch.facesPointXY_[2].y - dstPatch.minv_);
    

    /// 求得仿射变换
    cv::Mat warp_mat = cv::getAffineTransform(srcTri, dstTri);
 
    /// 对源图像应用上面求得的仿射变换
    cv::warpAffine(srcout, dstout, warp_mat, dstout.size(), cv::InterpolationFlags::WARP_FILL_OUTLIERS /*| cv::InterpolationFlags::INTER_LINEAR*/, 0, cv::Scalar(0, 0, 0));
    
    //cv::imwrite("e:\\test\\out\\dst_1.jpg", dstout);


    //替换原三角面位置纹理
    for (size_t i = dstPatch.minv_; i < dstPatch.maxv_; i++)
    {
        size_t r = i - dstPatch.minv_;
        //uchar* p = dstout.ptr<uchar>(r);  //获取第i行的首地址

        //uchar* t = _outTxture.ptr<uchar>(i);  //获取第i行的首地址

        for (size_t j = dstPatch.minu_; j < dstPatch.maxu_; j++)
        {
            size_t tj = j - dstPatch.minu_;            
            pcl::PointXY point;
            point.x = j;
            point.y = i;

            if (!checkPointTriangle(dstPatch.facesPointXY_[0], dstPatch.facesPointXY_[1], dstPatch.facesPointXY_[2], point))
                continue;

            {
                float B = dstout.ptr<uchar>(r )[tj * 3];
                float G = dstout.ptr<uchar>(r)[tj * 3 + 1];
                float R = dstout.ptr<uchar>(r)[tj * 3 + 2];

                if (filterRGB(R,G,B))
                {
                    continue;
                }

                _outTxture.ptr<uchar>(i)[j * 3] = B;
                _outTxture.ptr<uchar>(i)[j * 3 + 1] = G;
                _outTxture.ptr<uchar>(i)[j * 3 + 2] = R;
            }
            
            //上像素
            if (i > 0 && r > 0)
            {
                float B = dstout.ptr<uchar>(r - 1)[tj * 3 ];
                float G = dstout.ptr<uchar>(r - 1)[tj * 3 + 1];
                float R = dstout.ptr<uchar>(r - 1)[tj * 3 + 2];
                if (filterRGB(R, G, B))
                {
                    continue;
                }

                _outTxture.ptr<uchar>(i - 1)[j * 3] = B;
                _outTxture.ptr<uchar>(i - 1)[j * 3 + 1] = G;
                _outTxture.ptr<uchar>(i - 1)[j * 3 + 2] = R;

            }
            //上像素 2
            if (i > 1 && r > 1)
            {
                float B = dstout.ptr<uchar>(r - 2)[tj * 3];
                float G = dstout.ptr<uchar>(r - 2)[tj * 3 + 1];
                float R = dstout.ptr<uchar>(r - 2)[tj * 3 + 2];

                if (filterRGB(R, G, B))
                {
                    continue;
                }

                _outTxture.ptr<uchar>(i - 2)[j * 3] = B;
                _outTxture.ptr<uchar>(i - 2)[j * 3 + 1] = G;
                _outTxture.ptr<uchar>(i - 2)[j * 3 + 2] = R;

            }
            //下像素
            if (i  < _outTxture.rows && r < dstout.rows)
            {
                float B = dstout.ptr<uchar>(r + 1)[tj * 3];
                float G = dstout.ptr<uchar>(r + 1)[tj * 3 + 1];
                float R = dstout.ptr<uchar>(r + 1)[tj * 3 + 2];

                if (filterRGB(R, G, B))
                {
                    continue;
                }
                _outTxture.ptr<uchar>(i + 1)[j * 3] = B;
                _outTxture.ptr<uchar>(i + 1)[j * 3 + 1] = G;
                _outTxture.ptr<uchar>(i + 1)[j * 3 + 2] = R;

            }
            
            //左像素
            if (j > 0 && tj > 0)
            {
                float B = dstout.ptr<uchar>(r)[(tj - 1) * 3];
                float G = dstout.ptr<uchar>(r)[(tj - 1) * 3 + 1];
                float R = dstout.ptr<uchar>(r)[(tj - 1) * 3 + 2];

                if (filterRGB(R, G, B))
                {
                    continue;
                }


                _outTxture.ptr<uchar>(i)[(j - 1) * 3] = B;
                _outTxture.ptr<uchar>(i)[(j - 1) * 3 + 1] = G;
                _outTxture.ptr<uchar>(i)[(j - 1) * 3 + 2] = R;

            }
            //左像素2
            if (j > 1 && tj > 1)
            {
                float B = dstout.ptr<uchar>(r)[(tj - 2) * 3 ];
                float G = dstout.ptr<uchar>(r)[(tj - 2) * 3 + 1];
                float R = dstout.ptr<uchar>(r)[(tj - 2) * 3 + 2];
                if (filterRGB(R, G, B))
                {
                    continue;
                }

                _outTxture.ptr<uchar>(i)[(j - 2) * 3] = B;
                _outTxture.ptr<uchar>(i)[(j - 2) * 3 + 1] = G;
                _outTxture.ptr<uchar>(i)[(j - 2) * 3 + 2] = R;

            }

            //右像素
            if (j  < _outTxture.cols && tj < dstout.cols)
            {
                float B = dstout.ptr<uchar>(r)[(tj + 1) * 3];
                float G = dstout.ptr<uchar>(r)[(tj + 1) * 3 + 1];
                float R = dstout.ptr<uchar>(r)[(tj + 1) * 3 + 2];

                if (filterRGB(R, G, B))
                {
                    continue;
                }

                _outTxture.ptr<uchar>(i)[(j + 1) * 3] = B;
                _outTxture.ptr<uchar>(i)[(j + 1) * 3 + 1] = G;
                _outTxture.ptr<uchar>(i)[(j + 1) * 3 + 2] = R;
            }


        }

    }

#endif
    //cv::imwrite("e:\\test\\out\\dst_2.jpg", _outTxture);
}


//************************************
// Method:    replaceFace
// Access:    private 
// Returns:   void
// Describe:  三角面像素替换
// Parameter: const Patch & src 原三角面patch
// Parameter: Patch dst  目标三角面patch
//************************************
void OdmTexturing::replaceFace(const Patch &src, Patch dst){

    float offset = 10; //偏移范围

    cv::Mat  warp_mat = cv::Mat::zeros(max(dst.h, src.h) + offset, max(dst.w, src.w) + offset, CV_8UC3);
    cv::Mat  obj_mat = cv::Mat::zeros(max(dst.h, src.h) + offset, max(dst.w, src.w) + offset, CV_8UC3);

    getMatForFace(src, dst, warp_mat, obj_mat); //获取原三角面的纹理

}

 //************************************
 // Method:    createTextures
 // Access:    private 
 // Returns:   void
 // Describe:  计算UV到纹理中的像素坐标，根据三角面索引判断是否一致，调用替换
 //************************************
 void OdmTexturing::createTextures()
{

    float offset = 1.0;        

    for (int cameraIndex = 0; cameraIndex < cameras_.size(); ++cameraIndex)
    {
        //vector<vector<pcl::Vertices> > faceVector = vector<vector<pcl::Vertices> >(1);
        std::vector<Patch> patches = camerasPathes_[cameraIndex];
        std::vector<Patch> objPatches;
        for (size_t patchIndex = 0; patchIndex < patches.size(); ++patchIndex)
        {              
            // If patch is placed in current mesh add all containing faces to that submesh 
            //if (patches_[patchIndex].materialIndex_ == textureIndex)
            {                   

                // Loop through all faces in patch

                for (size_t faceIndex = 0; faceIndex < patches[patchIndex].faces_.size(); ++faceIndex)
                {
                    // Setup global face index in mesh_
                    float globalFaceIndex = patches[patchIndex].faces_[faceIndex];

                    //faceVector[0].push_back(mesh_->tex_polygons[0][globalFaceIndex]);

                    { //bfzhao add
                        Patch objtmppath;
                        Eigen::Vector2f uv1, uv2, uv3;
                        pcl::PointXY p1, p2, p3;
                        //保存当前三角面
                        objtmppath.faces_.push_back(globalFaceIndex);

                        //推算原模型中对应的三角面纹理像素点位置
                        float srcIndex = faceIndex;
                        globalFaceIndex = globalFaceIndex * 3;

                        uv1 = mesh_->tex_coordinates[0][globalFaceIndex];
                        p1.x = (uv1(0)) * textureResolution_;
                        p1.y = (1.0 - uv1(1)) * textureResolution_;

                        uv2 = mesh_->tex_coordinates[0][globalFaceIndex + 1];
                        p2.x = (uv2(0)) * textureResolution_;
                        p2.y = (1.0 - uv2(1)) * textureResolution_;

                        uv3 = mesh_->tex_coordinates[0][globalFaceIndex + 2];
                        p3.x = (uv3(0)) * textureResolution_;
                        p3.y = (1.0 - uv3(1)) * textureResolution_;

                        //outpx << p1 << "    " << p2 << "    " << p3<< endl;
                        p1.x = lround(p1.x);
                        p1.y = lround(p1.y);

                        p2.x = lround(p2.x);
                        p2.y = lround(p2.y);

                        p3.x = lround(p3.x);
                        p3.y = lround(p3.y);
#if 0
                        {
                            //最小x, -1

                            float x = min(p1.x, min(p2.x, p3.x));

                            if (p1.x == x)
                            {
                                p1.x -= offset;
                                   
                            }
                            if (p2.x == x )
                            {
                                p2.x -= offset;
                                   
                            }
                            if (p3.x == x)
                            {
                                p3.x -= offset;
                                   
                            }
                        }
                        {
                            //最小y, -1
                            float y = min(p1.y, min(p2.y, p3.y));

                            if ( p1.y == y)
                            {
                                p1.y -= offset;
                                   
                            }
                            if (p2.y == y)
                            {
                                  
                                p2.y -= offset;
                            } 
                            if (p3.y == y)
                            {
                                   
                                p3.y -= offset;
                            }
                        }


                        {
                            //最大x+1
                            float x = max(p1.x, max(p2.x, p3.x));
                            if (p1.x == x)
                            {
                                p1.x += offset;
                            }
                            if (p2.x == x)
                            {
                                p2.x += offset;
                            }
                            if (p3.x == x)
                            {
                                p3.x += offset;
                            }
                        }

                        {
                            //最大y+1
                            float y = max(p1.y, max(p2.y, p3.y));
                            if (p1.y == y)
                            {
                                p1.y += offset;
                            }
                            if (p2.y == y)
                            {
                                p2.y += offset;
                            }
                            if (p3.y == y)
                            {
                                p3.y += offset;
                            }
                        }
#endif
                        if (p1.x<0 || p1.x>textureResolution_ || p1.y < 0 || p1.y > textureResolution_)
                        {
                            cout << p1 << endl;
                        }

                        if (p2.x<0 || p2.x>textureResolution_ || p2.y < 0 || p2.y > textureResolution_)
                        {
                            cout << p2 << endl;
                        }

                          

                        if (p3.x<0 || p3.x>textureResolution_ || p3.y < 0 || p3.y > textureResolution_)
                        {
                            cout << p3 << endl;
                        }
                          


                        objtmppath.facesPointXY_.push_back(p1);
                        objtmppath.facesPointXY_.push_back(p2);
                        objtmppath.facesPointXY_.push_back(p3);

                        objtmppath.minu_ = min(p1.x, min(p2.x, p3.x))- 1;
                        objtmppath.minv_ = min(p1.y, min(p2.y, p3.y) - 1);

                        objtmppath.maxu_ = max(p1.x, max(p2.x, p3.x));
                        objtmppath.maxv_ = max(p1.y, max(p2.y, p3.y));

                        objtmppath.w = objtmppath.maxu_ - objtmppath.minu_;
                        objtmppath.h = objtmppath.maxv_ - objtmppath.minv_;


                        objtmppath.optimalCameraIndex_ = patches[patchIndex].optimalCameraIndex_;
                        //cv::Mat out;
                        //getMatForFace(objtmppath, out);
                        // Add patch to patches_ vector
                        objPatches.push_back(objtmppath);
                    }

                }
            }
                
        }
        objAllPathes_.push_back(objPatches);
    }


//保存纹理

    //--------------------------------------------------------------------------------------------------
    //for (int textureIndex = 0; textureIndex < nrTextures_; ++textureIndex)
    {
        for (int cameraIndex = 0; cameraIndex < static_cast<int>(cameras_.size()); ++cameraIndex)
        {
            // Load image for current camera
            _outTxture = cv::imread(_allTexture, 1);

            cout << "cameras index : " << cameraIndex << endl;
            _srcimage = cv::imread(cameras_[cameraIndex].texture_file, 1);                
            
            std::vector<Patch> patches = camerasPathes_[cameraIndex];

            std::vector<Patch> objPatches = objAllPathes_[cameraIndex];

            for (size_t patchIndex = 0; patchIndex < patches.size(); ++patchIndex)
            {
                if (patches[patchIndex].optimalCameraIndex_ == cameraIndex)
                {

                    //三角面纹理替换
                    for (size_t objfaceindex = 0; objfaceindex < objPatches.size(); objfaceindex++)
                    {
                        if (objPatches[objfaceindex].faces_[0] == patches[patchIndex].faces_[0]) //相同三角面
                        {
                            replaceFace(patches[patchIndex], objPatches[objfaceindex]);

                            objPatches.erase(objPatches.begin(), objPatches.begin() + objfaceindex);

                            break;
                        }

                    }

                }
            }//end for

            string outfile = FileLibrary::getInstance()->getFileNameFromPath(cameras_[cameraIndex].texture_file);

            cv::imwrite(outputFolder_ +"\\"+ outfile, _outTxture);

        }//end cameras


    }

}
