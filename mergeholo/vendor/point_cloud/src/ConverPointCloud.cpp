

#include "ConverPointCloud.h"
#include "poissonmesh.hpp"
#include <pcl/surface/simplification_remove_unused_vertices.h>
#include <memory>


ConverPointCloud::ConverPointCloud(){

	 //m_bundleResizedTo = 80, m_textureWithSize = 0, m_textureResolution = 1024;
	 m_kSearch = 20, m_type = 1;
     m_focal_length = 105;

     m_mu = m_searchRadius = m_nearestNeighbors = m_maxSurfaceAngle = m_minAngle = m_maxAngle = m_leafsize =0;
     m_upsamplingType = m_upsamplingRadius = m_upsamplingStepSize =0;
}


ConverPointCloud::~ConverPointCloud(){}


string ConverPointCloud::buildMeshOutputPath(const string &srcfile, const string &suffix)
{
    if (m_strMeshOutputDir.empty())
    {
        return srcfile.substr(0, srcfile.find_last_of("_")) + suffix;
    }

    string filename = FileLibrary::getInstance()->getFileNameFromPath(srcfile);
    string baseName = filename;
    size_t rgbPos = baseName.find("_rgb.ply");
    if (rgbPos != string::npos)
    {
        baseName = baseName.substr(0, rgbPos);
    }
    else
    {
        size_t extPos = baseName.find_last_of('.');
        if (extPos != string::npos)
        {
            baseName = baseName.substr(0, extPos);
        }
    }

    return FileLibrary::getInstance()->combineFilePath(m_strMeshOutputDir, baseName + suffix);
}




//************************************
// Method:    parseArguments
// Access:    private
// Returns:   bool
// Describe:  鐢熸垚obj妯″瀷鏄彧闇€瑕乫ocus鍙傛暟
// Parameter: const string & config 閰嶇疆鏂囦欢璺緞
//************************************
bool ConverPointCloud::parseArguments(const string &config){

    //string parentdir = FileLibrary::getInstance()->getFileParentPath(plyfile);
    //string flyname = FileLibrary::getInstance()->getFileNameFromPath(plyfile);

    //m_strTexturepng = plyfile.substr(0, plyfile.find_last_of("_"))+".png";// parentdir + "\\" + flyname.substr(0, flyname.length() - 4) + ".png";

	string configfile = config;// FileLibrary::getInstance()->getFileParentPath(parentdir) + "\\config.cfg";
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


#if 1
//娉婃澗绠楁硶鐢熸垚mesh
bool ConverPointCloud::createPoissonMesh(const string &filepath){
    std::string srcfile = filepath;

    pcl::PointCloud<pcl::PointXYZRGB>::Ptr cloud_rgb(new pcl::PointCloud<pcl::PointXYZRGB>);
    pcl::PointCloud<pcl::PointXYZRGB>::Ptr cloud_rgb_filtered(new pcl::PointCloud<pcl::PointXYZRGB>);

    if (pcl::io::loadPLYFile(filepath, *cloud_rgb) != 0){
        cout << COUT_PREFIX << "load ply false. file = " << filepath << endl;
        return false;
    }

    cout << COUT_PREFIX << "read ply file ok . point size = " << cloud_rgb->points.size() << endl;

    if (m_leafsize != 0)
    {
        pcl::VoxelGrid<pcl::PointXYZRGB> sor;
        sor.setInputCloud(cloud_rgb);
        sor.setLeafSize(m_leafsize, m_leafsize, m_leafsize);
        sor.filter(*cloud_rgb_filtered);

        cout << COUT_PREFIX << "VoxelGrid file ok . point size = " << cloud_rgb_filtered->points.size() << endl;
        cloud_rgb = cloud_rgb_filtered;
    }

    cout << COUT_PREFIX << "Calculating normals..." << endl;

    pcl::PointCloud<pcl::Normal>::Ptr normals(new pcl::PointCloud<pcl::Normal>);
    std::unique_ptr<pcl::NormalEstimation<pcl::PointXYZRGB, pcl::Normal>> ne(
        new pcl::NormalEstimation<pcl::PointXYZRGB, pcl::Normal>);
    pcl::search::KdTree<pcl::PointXYZRGB>::Ptr normal_tree(new pcl::search::KdTree<pcl::PointXYZRGB>);

    normal_tree->setInputCloud(cloud_rgb);
    ne->setInputCloud(cloud_rgb);
    ne->setSearchMethod(normal_tree);
    ne->setRadiusSearch(0.01);
    ne->compute(*normals);

    if (normals->points.size() != cloud_rgb->points.size())
    {
        cout << COUT_PREFIX << "Warning: Normal calculation failed, size mismatch." << endl;
        return false;
    }

    pcl::PointCloud<pcl::PointXYZRGBNormal>::Ptr cloud(new pcl::PointCloud<pcl::PointXYZRGBNormal>);
    cloud->points.resize(cloud_rgb->points.size());
    cloud->width = cloud_rgb->width;
    cloud->height = cloud_rgb->height;
    cloud->is_dense = cloud_rgb->is_dense;

    for (size_t i = 0; i < cloud_rgb->points.size(); ++i)
    {
        cloud->points[i].x = cloud_rgb->points[i].x;
        cloud->points[i].y = cloud_rgb->points[i].y;
        cloud->points[i].z = cloud_rgb->points[i].z;
        cloud->points[i].r = cloud_rgb->points[i].r;
        cloud->points[i].g = cloud_rgb->points[i].g;
        cloud->points[i].b = cloud_rgb->points[i].b;

        cloud->points[i].normal_x = normals->points[i].normal_x;
        cloud->points[i].normal_y = normals->points[i].normal_y;
        cloud->points[i].normal_z = normals->points[i].normal_z;
    }

    cout << COUT_PREFIX << "Normals calculated successfully." << endl;

#if 0
    pcl::PointCloud<pcl::PointXYZRGBNormal>::Ptr cloud_filtered(new pcl::PointCloud<pcl::PointXYZRGBNormal>);
    pcl::PointCloud<pcl::PointXYZRGBNormal>::Ptr cloud(new pcl::PointCloud<pcl::PointXYZRGBNormal>);

    if (pcl::io::loadPLYFile(filepath, *cloud) != 0){
        cout << COUT_PREFIX << "load ply false. file = "<< filepath << endl;
        return false;
    }

    cout << COUT_PREFIX << "read ply file ok . point size = " << cloud->points.size()<< endl;

    if (m_leafsize != 0)
    {
        // 鍒涘缓婊ゆ尝鍣ㄥ璞?        pcl::VoxelGrid<PointXYZRGBNormal> sor;//婊ゆ尝澶勭悊瀵硅薄
        sor.setInputCloud(cloud);
        sor.setLeafSize(m_leafsize, m_leafsize, m_leafsize);//璁剧疆婊ゆ尝鍣ㄥ鐞嗘椂閲囩敤鐨勪綋绱犲ぇ灏忕殑鍙傛暟   0.00015 = 澶х害 9:1
        sor.filter(*cloud_filtered);

        cout << COUT_PREFIX << "VoxelGrid file ok . point size = " << cloud_filtered->points.size() << endl;
        cloud = cloud_filtered;
        //string outfile = FileLibrary::getInstance()->getFileParentPath(filepath)+"\\vox_point.ply";
        //pcl::io::savePLYFile(outfile, *cloud);

    }

    // Check if normals exist, if not, calculate them
    // Check if normals are valid (non-zero length)
    bool hasNormals = false;
    if (cloud->points.size() > 0) {
        // Check first few points to see if normals are valid
        int checkCount = (10 < (int)cloud->points.size()) ? 10 : (int)cloud->points.size();
        int validCount = 0;
        for (int i = 0; i < checkCount; i++) {
            float nx = cloud->points[i].normal_x;
            float ny = cloud->points[i].normal_y;
            float nz = cloud->points[i].normal_z;
            float len = sqrt(nx*nx + ny*ny + nz*nz);
            if (len > 0.1) { // Normalized normals should have length ~1.0
                validCount++;
            }
        }
        hasNormals = (validCount > checkCount / 2); // If more than half have valid normals
    }

    if (!hasNormals) {
        cout << COUT_PREFIX << "Point cloud has no normals, calculating normals..." << endl;

        // Convert to PointXYZRGB for normal estimation
        pcl::PointCloud<pcl::PointXYZRGB>::Ptr cloud_rgb(new pcl::PointCloud<pcl::PointXYZRGB>);
        cloud_rgb->points.resize(cloud->points.size());
        for (size_t i = 0; i < cloud->points.size(); i++) {
            cloud_rgb->points[i].x = cloud->points[i].x;
            cloud_rgb->points[i].y = cloud->points[i].y;
            cloud_rgb->points[i].z = cloud->points[i].z;
            cloud_rgb->points[i].r = cloud->points[i].r;
            cloud_rgb->points[i].g = cloud->points[i].g;
            cloud_rgb->points[i].b = cloud->points[i].b;
        }
        cloud_rgb->width = cloud->width;
        cloud_rgb->height = cloud->height;
        cloud_rgb->is_dense = cloud->is_dense;

        // Calculate normals using NormalEstimation
        pcl::PointCloud<pcl::Normal>::Ptr normals(new pcl::PointCloud<pcl::Normal>);
        pcl::NormalEstimation<pcl::PointXYZRGB, pcl::Normal> ne;
        pcl::search::KdTree<pcl::PointXYZRGB>::Ptr tree(new pcl::search::KdTree<pcl::PointXYZRGB>);
        tree->setInputCloud(cloud_rgb);
        ne.setInputCloud(cloud_rgb);
        ne.setSearchMethod(tree);
        ne.setRadiusSearch(0.01); // Use radius search for normal estimation
        ne.compute(*normals);

        // Copy normals back to cloud
        if (normals->points.size() == cloud->points.size()) {
            for (size_t i = 0; i < cloud->points.size(); i++) {
                cloud->points[i].normal_x = normals->points[i].normal_x;
                cloud->points[i].normal_y = normals->points[i].normal_y;
                cloud->points[i].normal_z = normals->points[i].normal_z;
            }
            cout << COUT_PREFIX << "Normals calculated successfully." << endl;
        }
        else {
            cout << COUT_PREFIX << "Warning: Normal calculation failed, size mismatch." << endl;
            cout << COUT_PREFIX << "Attempting mesh reconstruction without normals..." << endl;
        }
    }
    else {
        cout << COUT_PREFIX << "Point cloud already has normals." << endl;
    }
#endif

#if 0


    pcl::PointCloud<pcl::PointXYZRGBNormal> mls_points;
    /*pcl::search::KdTree<pcl::PointXYZ>::Ptr tree(new pcl::search::KdTree<pcl::PointXYZ>);
    tree->setInputCloud(cloud);*/
    //pcl::KdTreeFLANN<pcl::PointXYZ> kdtree;

    //
    // Init object (second point type is for the normals, even if unused)
    //鏈€灏忎簩涔樻硶杩唬鎷熷悎骞虫粦鐐逛簯
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

    //娉曠嚎鎷熷悎 compite normals from point // meshlab function
    fitNormal(mls_points);
    nearestKSearchNormal(mls_points);
    //pcl::io::savePLYFile(srcfile.substr(0, srcfile.find_last_of("_"))+"_mls_1.ply", mls_points);

    cout << COUT_PREFIX << "MLS normal ok . point size = " << mls_points.points.size() << endl;

    // 鍒涘缓鍚屾椂鍖呭惈鐐瑰拰娉曞悜鐨勬暟鎹粨鏋勭殑鎸囬拡
    pcl::PointCloud<pcl::PointXYZRGBNormal>::Ptr cloud_with_normals(new pcl::PointCloud<pcl::PointXYZRGBNormal>(mls_points));
#endif
    pcl::search::KdTree<pcl::PointXYZRGBNormal>::Ptr tree(new pcl::search::KdTree<pcl::PointXYZRGBNormal>);
    pcl::PolygonMesh triangles;
    tree->setInputCloud(cloud);

    // 鍒涘缓鍙︿竴涓猭dtree鐢ㄤ簬閲嶅缓
    //涓簁dtree杈撳叆鐐逛簯鏁版嵁,璇ョ偣浜戞暟鎹被鍨嬩负鐐瑰拰娉曞悜
    pcl::Poisson<pcl::PointXYZRGBNormal> pn;
    pn.setConfidence(true);    //鏄惁浣跨敤娉曞悜閲忕殑澶у皬浣滀负缃俊淇℃伅銆傚鏋渇alse锛屾墍鏈夋硶鍚戦噺鍧囧綊涓€鍖栥€?    pn.setDegree(2);            //璁剧疆鍙傛暟degree[1,5],鍊艰秺澶ц秺绮剧粏锛岃€楁椂瓒婁箙銆?    pn.setDepth(8);             //鏍戠殑鏈€澶ф繁搴︼紝姹傝В2^d x 2^d x 2^d绔嬫柟浣撳厓銆傜敱浜庡叓鍙夋爲鑷€傚簲閲囨牱瀵嗗害锛屾寚瀹氬€间粎涓烘渶澶ф繁搴︺€?    pn.setIsoDivide(8);         //鐢ㄤ簬鎻愬彇ISO绛夊€奸潰鐨勭畻娉曠殑娣卞害
    pn.setManifold(true);      //鏄惁娣诲姞澶氳竟褰㈢殑閲嶅績锛屽綋澶氳竟褰笁瑙掑寲鏃躲€?璁剧疆娴佽鏍囧織锛屽鏋滆缃负true锛屽垯瀵瑰杈瑰舰杩涜缁嗗垎涓夎璇濇椂娣诲姞閲嶅績锛岃缃甪alse鍒欎笉娣诲姞
    pn.setOutputPolygons(false); //鏄惁杈撳嚭澶氳竟褰㈢綉鏍硷紙鑰屼笉鏄笁瑙掑寲绉诲姩绔嬫柟浣撶殑缁撴灉锛?    pn.setSamplesPerNode(3);  //璁剧疆钀藉叆涓€涓叓鍙夋爲缁撶偣涓殑鏍锋湰鐐圭殑鏈€灏忔暟閲忋€傛棤鍣０锛孾1.0-5.0],鏈夊櫔澹癧15.-20.]骞虫粦
    pn.setScale(1.1); //璁剧疆鐢ㄤ簬閲嶆瀯鐨勭珛鏂逛綋鐩村緞鍜屾牱鏈竟鐣岀珛鏂逛綋鐩村緞鐨勬瘮鐜囥€?    pn.setSolverDivide(8); //璁剧疆姹傝В绾挎€ф柟绋嬬粍鐨凣auss-Seidel杩唬鏂规硶鐨勬繁搴?    pn.setPointWeight(4.0);

    //pn.setIndices();

    //璁剧疆鎼滅储鏂规硶鍜岃緭鍏ョ偣浜?    pn.setSearchMethod(tree);
    pn.setInputCloud(cloud);
    //鎵ц閲嶆瀯
    pn.reconstruct(triangles);

    if (triangles.polygons.size() == 0)
    {
        cout << COUT_PREFIX << "Error: Poisson reconstruction failed - polygons size 0." << endl;
        cout << COUT_PREFIX << "Possible reasons:" << endl;
        cout << COUT_PREFIX << "  1. Point cloud has no valid normals" << endl;
        cout << COUT_PREFIX << "  2. Point cloud is too sparse or has insufficient points" << endl;
        cout << COUT_PREFIX << "  3. Point cloud scale is incorrect" << endl;
        cout << COUT_PREFIX << "Suggestion: Try using GreedyProjectionTriangulation (reconstruct=2) instead." << endl;
        return false;
    }

    //pcl::io::savePLYFile(srcfile.substr(0, srcfile.find_last_of("_")) + "_poisson.ply", triangles);

    cout << COUT_PREFIX << "poisson mesh reconstruct ok . point size = " << triangles.cloud.height*triangles.cloud.width<<", polygon size:"<< triangles.polygons.size() << endl;

#if 0
    cloud->clear();
    // get poisson point
    pcl::PointCloud<pcl::PointXYZ>::Ptr poisson_point(new pcl::PointCloud<pcl::PointXYZ>);
    pcl::fromPCLPointCloud2(triangles->cloud, *cloud);

    //鑾峰彇琛ユ礊鍚庣殑鐐癸紝閲嶆柊鎷熷悎娉曠嚎
    for (size_t i = 0; i < m_normalsIter2; i++)
    {
        //骞虫粦鐐逛簯
        normalsMovingLeastSquares(cloud, mls_points);

        getPointFromPointNormal(cloud, mls_points);

    }


    cout << COUT_PREFIX << "fit normal ok . " << endl;

    pcl::toPCLPointCloud2(mls_points, triangles->cloud);
#endif

    //save poisson mesh file
    m_strOutModelPath = buildMeshOutputPath(srcfile, "_mesh.ply");
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

    texturemeshPoisson* poissoinmesh = new texturemeshPoisson(srcfile,m_strOutModelPath);
    poissoinmesh->setPolygonMesh(triangles);
    poissoinmesh->meshCropHull(cloud_point);
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

    //鑾峰彇琛ユ礊鍚庣殑鐐癸紝閲嶆柊鎷熷悎娉曠嚎
    mls_points.clear();
    for (size_t i = 0; i < m_normalsIter2; i++)
    {
        //骞虫粦鐐逛簯
        normalsMovingLeastSquares(cloud, mls_points);

        getPointFromPointNormal(cloud, mls_points);

    }

    //娉曠嚎鎷熷悎 compite normals from point // meshlab function
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

bool ConverPointCloud::createGreedMeshFromCloud(
    const pcl::PointCloud<pcl::PointXYZRGB>::ConstPtr &inputCloud,
    const string &srcfile,
    pcl::PolygonMesh *meshOut,
    bool writeMeshFile)
{
    if (!inputCloud || inputCloud->points.empty())
    {
        cout << COUT_PREFIX << "input point cloud is empty." << endl;
        return false;
    }

    pcl::PointCloud<pcl::PointXYZRGB>::ConstPtr cloud_rgb = inputCloud;
    pcl::PointCloud<pcl::PointXYZRGB>::Ptr cloud_rgb_filtered(new pcl::PointCloud<pcl::PointXYZRGB>);

    cout << COUT_PREFIX << "read memory point cloud ok . point size=" << cloud_rgb->points.size() << endl;

    if (m_leafsize != 0)
    {
        pcl::VoxelGrid<pcl::PointXYZRGB> sor;
        sor.setInputCloud(cloud_rgb);
        sor.setLeafSize(m_leafsize, m_leafsize, m_leafsize);
        sor.filter(*cloud_rgb_filtered);

        cout << COUT_PREFIX << "VoxelGrid memory ok . point size = " << cloud_rgb_filtered->points.size() << endl;
        cloud_rgb = cloud_rgb_filtered;
    }

    cout << COUT_PREFIX << "Calculating normals..." << endl;

    pcl::PointCloud<pcl::Normal>::Ptr normals(new pcl::PointCloud<pcl::Normal>);
    std::unique_ptr<pcl::NormalEstimation<pcl::PointXYZRGB, pcl::Normal>> ne(
        new pcl::NormalEstimation<pcl::PointXYZRGB, pcl::Normal>);
    pcl::search::KdTree<pcl::PointXYZRGB>::Ptr normal_tree(new pcl::search::KdTree<pcl::PointXYZRGB>);

    normal_tree->setInputCloud(cloud_rgb);
    ne->setInputCloud(cloud_rgb);
    ne->setSearchMethod(normal_tree);
    ne->setRadiusSearch(0.01);
    ne->compute(*normals);

    if (normals->points.size() != cloud_rgb->points.size())
    {
        cout << COUT_PREFIX << "Warning: Normal calculation failed, size mismatch." << endl;
        ne.reset();
        return false;
    }

    pcl::PointCloud<pcl::PointXYZRGBNormal>::Ptr cloud_normal(new pcl::PointCloud<pcl::PointXYZRGBNormal>);
    cloud_normal->points.resize(cloud_rgb->points.size());
    cloud_normal->width = cloud_rgb->width;
    cloud_normal->height = cloud_rgb->height;
    cloud_normal->is_dense = cloud_rgb->is_dense;

    for (size_t i = 0; i < cloud_rgb->points.size(); ++i)
    {
        cloud_normal->points[i].x = cloud_rgb->points[i].x;
        cloud_normal->points[i].y = cloud_rgb->points[i].y;
        cloud_normal->points[i].z = cloud_rgb->points[i].z;
        cloud_normal->points[i].r = cloud_rgb->points[i].r;
        cloud_normal->points[i].g = cloud_rgb->points[i].g;
        cloud_normal->points[i].b = cloud_rgb->points[i].b;

        cloud_normal->points[i].normal_x = normals->points[i].normal_x;
        cloud_normal->points[i].normal_y = normals->points[i].normal_y;
        cloud_normal->points[i].normal_z = normals->points[i].normal_z;
    }

    cout << COUT_PREFIX << "Normals calculated successfully." << endl;

    pcl::search::KdTree<pcl::PointXYZRGBNormal>::Ptr tree2(new pcl::search::KdTree<pcl::PointXYZRGBNormal>);
    std::unique_ptr<pcl::PolygonMesh> triangles(new pcl::PolygonMesh);
    tree2->setInputCloud(cloud_normal);

    std::unique_ptr<pcl::GreedyProjectionTriangulation<pcl::PointXYZRGBNormal>> gp3(
        new pcl::GreedyProjectionTriangulation<pcl::PointXYZRGBNormal>);
    auto releaseGreedyProjection = [&gp3]() {
        if (gp3)
        {
            gp3->setInputCloud(pcl::PointCloud<pcl::PointXYZRGBNormal>::ConstPtr());
            gp3->setSearchMethod(pcl::search::KdTree<pcl::PointXYZRGBNormal>::Ptr());
            gp3.reset();
        }
    };
    gp3->setSearchRadius(m_searchRadius);
    gp3->setMu(m_mu);
    gp3->setMaximumNearestNeighbors(m_nearestNeighbors);
    gp3->setMaximumSurfaceAngle(m_maxSurfaceAngle * (M_PI / 180));
    gp3->setMinimumAngle(m_minAngle * (M_PI / 180));
    gp3->setMaximumAngle(m_maxAngle * (M_PI / 180));
    gp3->setNormalConsistency(false);
    gp3->setConsistentVertexOrdering(true);
    gp3->setInputCloud(cloud_normal);
    gp3->setSearchMethod(tree2);
    gp3->reconstruct(*triangles);

    if (triangles->polygons.size() == 0){
        cout << " polygons size 0." << endl;
        releaseGreedyProjection();
        ne.reset();
        return false;
    }

    cout << COUT_PREFIX << "mesh reconstruct ok . point size:" << triangles->cloud.height * triangles->cloud.width << "\tpolygons:" << triangles->polygons.size() << endl;

    std::unique_ptr<pcl::PolygonMesh> meshPoly(new pcl::PolygonMesh);
    fillHole(*triangles, *meshPoly);

    m_strOutModelPath = buildMeshOutputPath(srcfile, "_mesh.ply");
    if (writeMeshFile && pcl::io::savePLYFile(m_strOutModelPath, *meshPoly) != 0)
    {
        cout << "save mesh ply file false. file= " << FileLibrary::getInstance()->getFileNameFromPath(m_strOutModelPath) << endl;
        releaseGreedyProjection();
        ne.reset();
        return false;
    }

    if (meshOut)
    {
        *meshOut = *meshPoly;
    }

    meshPoly.reset();
    triangles.reset();
    releaseGreedyProjection();
    ne.reset();

    return true;
}

//璐┆绠楁硶鐢熸垚mesh
bool ConverPointCloud::createGreedMesh(const string &filepath){
    std::string srcfile = filepath;
    pcl::PointCloud<pcl::PointXYZRGB>::Ptr cloud_rgb(new pcl::PointCloud<pcl::PointXYZRGB>);
    pcl::PointCloud<pcl::PointXYZRGB>::Ptr cloud_rgb_filtered(new pcl::PointCloud<pcl::PointXYZRGB>);

    if (pcl::io::loadPLYFile(filepath, *cloud_rgb) != 0){
        cout << COUT_PREFIX << "load ply false. file = " << filepath << endl;
        return false;
    }

    cout << COUT_PREFIX << "read ply file ok . point size=" << cloud_rgb->points.size() << endl;

    if (m_leafsize != 0)
    {
        pcl::VoxelGrid<pcl::PointXYZRGB> sor;
        sor.setInputCloud(cloud_rgb);
        sor.setLeafSize(m_leafsize, m_leafsize, m_leafsize);
        sor.filter(*cloud_rgb_filtered);

        cout << COUT_PREFIX << "VoxelGrid file ok . point size = " << cloud_rgb_filtered->points.size() << endl;
        cloud_rgb = cloud_rgb_filtered;
    }

    cout << COUT_PREFIX << "Calculating normals..." << endl;

    pcl::PointCloud<pcl::Normal>::Ptr normals(new pcl::PointCloud<pcl::Normal>);
    std::unique_ptr<pcl::NormalEstimation<pcl::PointXYZRGB, pcl::Normal>> ne(
        new pcl::NormalEstimation<pcl::PointXYZRGB, pcl::Normal>);
    pcl::search::KdTree<pcl::PointXYZRGB>::Ptr normal_tree(new pcl::search::KdTree<pcl::PointXYZRGB>);

    normal_tree->setInputCloud(cloud_rgb);
    ne->setInputCloud(cloud_rgb);
    ne->setSearchMethod(normal_tree);
    ne->setRadiusSearch(0.01);
    ne->compute(*normals);

    if (normals->points.size() != cloud_rgb->points.size())
    {
        cout << COUT_PREFIX << "Warning: Normal calculation failed, size mismatch." << endl;
        return false;
    }

    pcl::PointCloud<pcl::PointXYZRGBNormal>::Ptr cloud_normal(new pcl::PointCloud<pcl::PointXYZRGBNormal>);
    cloud_normal->points.resize(cloud_rgb->points.size());
    cloud_normal->width = cloud_rgb->width;
    cloud_normal->height = cloud_rgb->height;
    cloud_normal->is_dense = cloud_rgb->is_dense;

    for (size_t i = 0; i < cloud_rgb->points.size(); ++i)
    {
        cloud_normal->points[i].x = cloud_rgb->points[i].x;
        cloud_normal->points[i].y = cloud_rgb->points[i].y;
        cloud_normal->points[i].z = cloud_rgb->points[i].z;
        cloud_normal->points[i].r = cloud_rgb->points[i].r;
        cloud_normal->points[i].g = cloud_rgb->points[i].g;
        cloud_normal->points[i].b = cloud_rgb->points[i].b;

        cloud_normal->points[i].normal_x = normals->points[i].normal_x;
        cloud_normal->points[i].normal_y = normals->points[i].normal_y;
        cloud_normal->points[i].normal_z = normals->points[i].normal_z;
    }

    cout << COUT_PREFIX << "Normals calculated successfully." << endl;

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
    //cout << "浣撶礌婊ゆ尝鍣?size = " << cloud_filtered->points.size() << endl;
    if (m_leafsize != 0)
    {
        pcl::VoxelGrid<PointXYZRGB> sor;//婊ゆ尝澶勭悊瀵硅薄
        sor.setInputCloud(cloud);
        sor.setLeafSize(m_leafsize, m_leafsize, m_leafsize);//璁剧疆婊ゆ尝鍣ㄥ鐞嗘椂閲囩敤鐨勪綋绱犲ぇ灏忕殑鍙傛暟   0.00015 = 澶х害 9:1
        sor.filter(*cloud_filtered);
        cout << COUT_PREFIX << "VoxelGrid ok . point size=" << cloud_filtered->points.size() << endl;
        cloud = cloud_filtered;
    }
    m_strOutModelPath = buildMeshOutputPath(srcfile, "_vox.ply");

    //pcl::io::savePLYFile(m_strOutModelPath, *cloud);

    //
    // Init object (second point type is for the normals, even if unused)
    //鏈€灏忎簩涔樻硶杩唬鎷熷悎骞虫粦鐐逛簯
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

    m_strOutModelPath = buildMeshOutputPath(srcfile, "_mls_1.ply");

    pcl::io::savePLYFile(m_strOutModelPath, mls_points);

    cout << COUT_PREFIX << "MLS normal ok . point size: " << mls_points.points.size() << endl;
#endif

    // 鍒涘缓鍚屾椂鍖呭惈鐐瑰拰娉曞悜鐨勬暟鎹粨鏋勭殑鎸囬拡
    //pcl::PointCloud<pcl::PointXYZRGBNormal>::Ptr cloud_with_normals(new pcl::PointCloud<pcl::PointXYZRGBNormal>(mls_points));
    pcl::search::KdTree<pcl::PointXYZRGBNormal>::Ptr tree2(new pcl::search::KdTree<pcl::PointXYZRGBNormal>);

    std::unique_ptr<pcl::PolygonMesh> triangles(new pcl::PolygonMesh);
    tree2->setInputCloud(cloud_normal);

    std::unique_ptr<pcl::GreedyProjectionTriangulation<pcl::PointXYZRGBNormal>> gp3(
        new pcl::GreedyProjectionTriangulation<pcl::PointXYZRGBNormal>);
    auto releaseGreedyProjection = [&gp3]() {
        if (gp3)
        {
            gp3->setInputCloud(pcl::PointCloud<pcl::PointXYZRGBNormal>::ConstPtr());
            gp3->setSearchMethod(pcl::search::KdTree<pcl::PointXYZRGBNormal>::Ptr());
            gp3.reset();
        }
    };
    gp3->setSearchRadius(m_searchRadius);
    gp3->setMu(m_mu);
    gp3->setMaximumNearestNeighbors(m_nearestNeighbors);
    gp3->setMaximumSurfaceAngle(m_maxSurfaceAngle * (M_PI / 180));
    gp3->setMinimumAngle(m_minAngle * (M_PI / 180));
    gp3->setMaximumAngle(m_maxAngle * (M_PI / 180));
    gp3->setNormalConsistency(false);
    gp3->setConsistentVertexOrdering(true);
    gp3->setInputCloud(cloud_normal);
    gp3->setSearchMethod(tree2);
    gp3->reconstruct(*triangles);

    if (triangles->polygons.size() == 0){
        cout << " polygons size 0." << endl;
        releaseGreedyProjection();
        ne.reset();
        return 0;
    }

    cout << COUT_PREFIX << "mesh reconstruct ok . point size:" << triangles->cloud.height * triangles->cloud.width << "\tpolygons:" << triangles->polygons.size() << endl;

    std::unique_ptr<pcl::PolygonMesh> meshPoly(new pcl::PolygonMesh);
    fillHole(*triangles, *meshPoly);

    m_strOutModelPath = buildMeshOutputPath(srcfile, "_mesh.ply");
    pcl::io::savePLYFile(m_strOutModelPath, *meshPoly);
    meshPoly.reset();
    triangles.reset();
    releaseGreedyProjection();
    ne.reset();

#if 0
    cloud->clear();
    pcl::fromPCLPointCloud2(meshPoly.cloud, *cloud);
    if (cloud->points.size() == 0)
    {
        cout << COUT_PREFIX << "cloud->points.size is null " << endl;
        return false;
    }

    ////鑾峰彇琛ユ礊鍚庣殑鐐癸紝閲嶆柊鎷熷悎娉曠嚎
    for (size_t i = 0; i < m_normalsIter2; i++)
    {
        //骞虫粦鐐逛簯
        mls_points.points.clear();
        normalsMovingLeastSquares(cloud, mls_points);

        getPointFromPointNormal(cloud, mls_points);

    }

    //娉曠嚎鎷熷悎
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
    //cout << "浣撶礌婊ゆ尝鍣?size = " << cloud_filtered->points.size() << endl;

    //
    // Init object (second point type is for the normals, even if unused)
    //鏈€灏忎簩涔樻硶杩唬鎷熷悎骞虫粦鐐逛簯
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

    // 鍒涘缓鍚屾椂鍖呭惈鐐瑰拰娉曞悜鐨勬暟鎹粨鏋勭殑鎸囬拡
    pcl::PointCloud<pcl::PointNormal>::Ptr cloud_with_normals(new pcl::PointCloud<pcl::PointNormal>(mls_points));
    pcl::search::KdTree<pcl::PointNormal>::Ptr tree2(new pcl::search::KdTree<pcl::PointNormal>);
    pcl::PolygonMesh triangles;
    tree2->setInputCloud(cloud_with_normals);


    /*鏇查潰閲嶅缓妯″潡*/
    // 鍒涘缓璐┆涓夎褰㈡姇褰遍噸寤哄璞?    pcl::GreedyProjectionTriangulation<pcl::PointNormal> gp3;
    //鍒涘缓澶氳竟褰㈢綉鏍煎璞?鐢ㄦ潵瀛樺偍閲嶅缓缁撴灉
    gp3.setSearchRadius(m_searchRadius);  //璁剧疆杩炴帴鐐逛箣闂寸殑鏈€澶ц窛绂伙紝鐢ㄤ簬纭畾k杩戦偦鐨勭悆鍗婂緞 锛堝嵆鏄笁瑙掑舰鏈€澶ц竟闀匡級
    gp3.setMu(m_mu);  // 璁剧疆鏈€杩戦偦璺濈鐨勪箻瀛愶紝宸插緱鍒版瘡涓偣鐨勬渶缁堟悳绱㈠崐寰勶紙榛樿涓?锛?    gp3.setMaximumNearestNeighbors(m_nearestNeighbors);  //璁剧疆鎼滅储鐨勬渶杩戦偦鐐圭殑鏈€澶ф暟閲?    gp3.setMaximumSurfaceAngle(m_maxSurfaceAngle * (M_PI / 180)); // 90 degrees 鏈€澶у钩闈㈣
    gp3.setMinimumAngle(m_minAngle * (M_PI / 180)); // 5 degrees 姣忎釜涓夎鐨勬渶澶ц搴?    gp3.setMaximumAngle(m_maxAngle * (M_PI / 180)); // 150 degrees
    gp3.setNormalConsistency(false);  //鑻ユ硶鍚戦噺涓€鑷达紝璁句负true
    gp3.setConsistentVertexOrdering(true);
    // 璁剧疆鐐逛簯鏁版嵁鍜屾悳绱㈡柟寮?    gp3.setInputCloud(cloud_with_normals);
    gp3.setSearchMethod(tree2);
    //寮€濮嬮噸寤?    gp3.reconstruct(triangles);

    if (triangles.polygons.size() == 0){
        cout << " polygons size 0." << endl;
        return 0;
    }

    //pcl::io::savePLYFile("e:\\triangles.ply", triangles);
	cout << COUT_PREFIX << "mesh reconstruct ok . point size:" << triangles.cloud.height* triangles.cloud.width<<"	polygons:"<< triangles.polygons.size() << endl;

	//琛ユ礊
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

    //鑾峰彇琛ユ礊鍚庣殑鐐癸紝閲嶆柊鎷熷悎娉曠嚎
	for (size_t i = 0; i < m_normalsIter2; i++)
    {
        //骞虫粦鐐逛簯
        mls_points.points.clear();
        normalsMovingLeastSquares(cloud, mls_points);

        getPointFromPointNormal(cloud, mls_points);

    }

    //娉曠嚎鎷熷悎
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

//************************************
// Method:    createModel
// Access:    private
// Returns:   bool
// Describe:  鏋勫缓璐村浘瀵硅薄锛屼紶鍏esh锛宑onfig閰嶇疆鏂囦欢
// Parameter: const string & plyfile :mesh璺緞
//************************************
bool ConverPointCloud::createModel(const string &plyfile){

    OdmTexturing createmodel(plyfile, m_strTexturepng);

    createmodel.run(m_focal_length);


	return true;
}

bool ConverPointCloud::createModelFromMesh(const pcl::PolygonMesh &mesh, const string &logicalMeshPath){

    OdmTexturing createmodel(mesh, logicalMeshPath, m_strTexturepng);

    return createmodel.run(m_focal_length) == 0;
}



bool ConverPointCloud::meshAPI(const string &flypath, const string &config, const string &outputDir){

    if (!FileLibrary::getInstance()->isFileExists(flypath))
    {
        cout << COUT_PREFIX << "Error: Point cloud file not found. file = " << flypath << endl;
        return false;
    }

    if (parseArguments(config) == false){
        cout << COUT_PREFIX << "Error: Failed to parse configuration file." << endl;
        return false;
    }
    m_strMeshOutputDir = outputDir;

    cout << COUT_PREFIX << "Reconstruction type: " << (m_type == 1 ? "Poisson" : "GreedyProjectionTriangulation") << endl;

    bool result = false;
    if (m_type == 1) {
        result = createPoissonMesh(flypath);
        if (!result) {
            cout << COUT_PREFIX << "Poisson reconstruction failed. You may want to try GreedyProjectionTriangulation (set reconstruct=2 in config)." << endl;
        }
    }
    else if (m_type == 2) {
        result = createGreedMesh(flypath);
        if (!result) {
            cout << COUT_PREFIX << "GreedyProjectionTriangulation failed." << endl;
        }
    }
    else {
        cout << COUT_PREFIX << "Error: Unknown reconstruction type: " << m_type << endl;
        cout << COUT_PREFIX << "Valid values: 1=Poisson, 2=GreedyProjectionTriangulation" << endl;
        return false;
    }

    return result;
}

bool ConverPointCloud::meshAPIFromCloud(
    const pcl::PointCloud<pcl::PointXYZRGB>::ConstPtr &cloud,
    const string &logicalFlypath,
    const string &config,
    const string &outputDir,
    pcl::PolygonMesh *meshOut,
    bool writeMeshFile)
{
    if (!cloud || cloud->points.empty())
    {
        cout << COUT_PREFIX << "Error: Point cloud memory is empty." << endl;
        return false;
    }

    if (parseArguments(config) == false){
        cout << COUT_PREFIX << "Error: Failed to parse configuration file." << endl;
        return false;
    }
    m_strMeshOutputDir = outputDir;

    cout << COUT_PREFIX << "Reconstruction type: " << (m_type == 1 ? "Poisson" : "GreedyProjectionTriangulation") << endl;

    if (m_type != 2) {
        cout << COUT_PREFIX << "Error: Memory mesh path currently supports GreedyProjectionTriangulation only." << endl;
        return false;
    }

    const bool result = createGreedMeshFromCloud(cloud, logicalFlypath, meshOut, writeMeshFile);
    if (!result) {
        cout << COUT_PREFIX << "GreedyProjectionTriangulation failed." << endl;
    }
    return result;
}

//************************************
// Method:    modelAPI
// Access:    public
// Returns:   bool
// Describe:  閰嶇疆鏂囦欢鍙傛暟瑙ｆ瀽锛屾煡鎵惧尮閰嶇汗鐞嗗浘鐗?// Parameter: const string & flypath
// Parameter: const string & config
//************************************
bool ConverPointCloud::modelAPI(const string &flypath, const string &config){

    if (!FileLibrary::getInstance()->isFileExists(flypath))
    {
        cout << COUT_PREFIX << "Error: Mesh file not found. file = " << flypath << endl;
        return false;
    }

    // Extract base name from mesh file path
    // Example: "0_mesh.ply" -> "0", "face_mesh.ply" -> "face"
    string filename = FileLibrary::getInstance()->getFileNameFromPath(flypath);
    string baseName = filename;

    // Remove "_mesh.ply" or ".ply" extension
    size_t pos = baseName.find("_mesh.ply");
    if (pos != string::npos) {
        baseName = baseName.substr(0, pos);
    }
    else {
        pos = baseName.find(".ply");
        if (pos != string::npos) {
            baseName = baseName.substr(0, pos);
        }
    }

    // Build texture file path: parent directory + base name + .jpg
    // Example: "0_mesh.ply" -> "0.jpg", "face_mesh.ply" -> "face.jpg"
    string parentPath = FileLibrary::getInstance()->getFileParentPath(flypath);
    m_strTexturepng = parentPath + "\\" + baseName + ".jpg";

    cout << COUT_PREFIX << "Looking for texture file: " << m_strTexturepng << endl;

    // Try .jpg first, then .png as fallback
    if (!FileLibrary::getInstance()->isFileExists(m_strTexturepng))
    {
        // Try .png as fallback
        string texturePng = parentPath + "\\" + baseName + ".png";
        if (FileLibrary::getInstance()->isFileExists(texturePng)) {
            m_strTexturepng = texturePng;
            cout << COUT_PREFIX << "Found texture file (PNG): " << m_strTexturepng << endl;
        }
        else {
            // Try _rgb.jpg or _rgb.png
            string textureRgbJpg = parentPath + "\\" + baseName + "_rgb.jpg";
            string textureRgbPng = parentPath + "\\" + baseName + "_rgb.png";
            if (FileLibrary::getInstance()->isFileExists(textureRgbJpg)) {
                m_strTexturepng = textureRgbJpg;
                cout << COUT_PREFIX << "Found texture file (_rgb.jpg): " << m_strTexturepng << endl;
            }
            else if (FileLibrary::getInstance()->isFileExists(textureRgbPng)) {
                m_strTexturepng = textureRgbPng;
                cout << COUT_PREFIX << "Found texture file (_rgb.png): " << m_strTexturepng << endl;
            }
            else {
                cout << COUT_PREFIX << "Error: Texture file not found. Tried:" << endl;
                cout << COUT_PREFIX << "  - " << m_strTexturepng << endl;
                cout << COUT_PREFIX << "  - " << texturePng << endl;
                cout << COUT_PREFIX << "  - " << textureRgbJpg << endl;
                cout << COUT_PREFIX << "  - " << textureRgbPng << endl;
                return false;
            }
        }
    }
    else {
        cout << COUT_PREFIX << "Found texture file (JPG): " << m_strTexturepng << endl;
    }

    if (parseArguments(config) == false){
        cout << COUT_PREFIX << "Error: Failed to parse configuration file." << endl;
        return false;
    }

    return createModel(flypath);
}

bool ConverPointCloud::modelAPIFromMesh(
    const pcl::PolygonMesh &mesh,
    const string &logicalMeshPath,
    const string &texturePath,
    const string &config)
{
    if (mesh.cloud.data.empty() || mesh.polygons.empty())
    {
        cout << COUT_PREFIX << "Error: Mesh memory is empty." << endl;
        return false;
    }

    m_strTexturepng = texturePath;
    cout << COUT_PREFIX << "Looking for texture file: " << m_strTexturepng << endl;
    if (!FileLibrary::getInstance()->isFileExists(m_strTexturepng))
    {
        cout << COUT_PREFIX << "Error: Texture file not found. file = " << m_strTexturepng << endl;
        return false;
    }
    cout << COUT_PREFIX << "Found texture file (memory path): " << m_strTexturepng << endl;

    if (parseArguments(config) == false){
        cout << COUT_PREFIX << "Error: Failed to parse configuration file." << endl;
        return false;
    }

    return createModelFromMesh(mesh, logicalMeshPath);
}

//娉曠嚎鏍囧噯鍖?
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
    //mls.setPolynomialFit(true); //瀵逛簬娉曠嚎鐨勪及璁℃槸鏈夊椤瑰紡杩樻槸浠呬粎渚濋潬鍒囩嚎
    mls.setComputeNormals(true);
    mls.setSearchMethod(tree);
    mls.setPointDensity(30);
    mls.setPolynomialOrder(4);
    mls.setSearchRadius(m_mlsSearchRadius); // 0.001jingnan //纭畾鎼滅储鐨勫崐寰?鍦ㄨ繖涓崐寰勯噷杩涜琛ㄩ潰鏄犲皠鍜屾洸闈㈡嫙鍚堛€備粠瀹為獙缁撴灉鍙煡锛氬崐寰勮秺灏忔嫙鍚堝悗鏇查潰鐨勫け鐪熷害瓒婂皬锛屽弽涔嬫湁鍙兘鍑虹幇杩囨嫙鍚堢殑鐜拌薄
    if (m_upsamplingType == 0)
    {
        mls.setUpsamplingMethod(mls.NONE);

    }
    else if (m_upsamplingType == 2)
    {
        mls.setUpsamplingMethod(mls.SAMPLE_LOCAL_PLANE);// 杩欎釜鏂规硶灏辨槸鍙傝€冭鏂囦腑閲囩敤鐨勬柟娉曪紝褰撶劧姝ゆ柟娉曟墍闇€鐨勮绠楀己搴︿篃鐩稿綋搴炲ぇ銆傝嫢浣跨敤姝ゆ柟娉曪紝灏嗛渶瑕佽皟鐢ㄤ袱涓嚱鏁帮細
        mls.setUpsamplingRadius(m_upsamplingRadius);//姝ゅ嚱鏁拌瀹氫簡鐐逛簯澧為暱鐨勫尯鍩熴€傚彲浠ヨ繖鏍风悊瑙ｏ細鎶婃暣涓偣浜戞寜鐓ф鍗婂緞鍒掑垎鎴愯嫢骞蹭釜瀛愮偣浜戯紝鐒跺悗涓€涓€绱㈠紩杩涜鐐逛簯澧為暱銆?          0.1
        mls.setUpsamplingStepSize(m_upsamplingStepSize);//瀵逛簬姣忎釜瀛愮偣浜戝鐞嗘椂杩唬鐨勬闀裤€?
    }
    else if (m_upsamplingType == 3)
    {
        mls.setUpsamplingMethod(mls.RANDOM_UNIFORM_DENSITY);   //涔熸槸浣跨敤涓婇潰瀛愮偣浜戠殑鍘熺悊锛屽彧涓嶈繃瀹冧娇寰楃█鐤忓尯鍩熺殑瀵嗗害澧炲姞锛屼粠鑰屼娇寰楁暣涓偣浜戠殑瀵嗗害鍧囧寑
        mls.setPointDensity(m_pointDensity);  //娉ㄦ剰姝ゅ嚱鏁拌緭鍏ユ暣鍨嬪彉閲忥紝鎰忎负鍗婂緞鍐呯偣鐨勪釜鏁般€傦紙杩欎釜鍗婂緞搴旇鏄痵earch鐨勫崐寰勶紝涓嶉渶瑕侀噸鏂拌缃級銆?
    }
    else if (m_upsamplingType == 4)
    {

        mls.setUpsamplingMethod(mls.VOXEL_GRID_DILATION); //杩欎釜鏂规硶鏈変袱涓楠わ細棣栧厛灏嗙偣浜戜互voxels鍒嗗壊锛岀劧鍚庤繘琛岃凯浠ｄ娇寰梫oxels鐨勬暟鐩鍔犮€傚畠鐨勭粨鏋滄槸锛氬～鍏呯┖娲炲拰骞冲潎鍖栫偣浜戠殑瀵嗗害銆傚畠闇€瑕佽皟鐢ㄧ殑鍑芥暟涓猴細
        mls.setDilationVoxelSize(m_dilationVoxelSize);   //璁惧畾voxel鐨勫ぇ灏忋€?
        mls.setDilationIterations(m_dilationIterations); //璁剧疆杩唬鐨勬鏁?
    }


    // Reconstruct
    mls.process(mls_points);


    return true;
}

bool ConverPointCloud::fitNormal(pcl::PointCloud<pcl::PointXYZRGBNormal> &mls_points)
{

    //娉曠嚎閲嶆柊璁＄畻
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
        //鐢变簬VTK/OpenGL骞舵病鏈夊瓨鍌∟aN鏍煎紡
        pcl::PointXYZRGB current_point;
        current_point.x = mls_points.points[i].x;
        current_point.y = mls_points.points[i].y;
        current_point.z = mls_points.points[i].z;

        search.nearestKSearch(current_point, m_neighbor_num, indices, distance);//鍏朵腑current_point涓洪€変腑鐨勭偣
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
        //鏍规嵁涓績鐐广€佸崗鏂瑰樊璁＄畻娉曠嚎
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

    //鎷熷悎閭诲眳K璁＄畻娉曠嚎
    for (size_t iterNum = 0; iterNum < m_normalsIter2; iterNum++)
    {
        tmpNormalPoint.clear();
        tmpNormalPoint.points.resize(mls_points.points.size());


        for (size_t i = 0; i < mls_points.points.size(); i++)
        {

            std::vector<int> indices(m_neighbor_num);
			std::vector<float> distance(m_distance); //0.01
            //鐢变簬VTK/OpenGL骞舵病鏈夊瓨鍌∟aN鏍煎紡
            pcl::PointXYZRGB current_point;
            current_point.x = mls_points.points[i].x;
            current_point.y = mls_points.points[i].y;
            current_point.z = mls_points.points[i].z;
            search.nearestKSearch(current_point, m_neighbor_num, indices, distance);//鍏朵腑current_point涓洪€変腑鐨勭偣

            for (size_t Knum = 0; Knum < m_neighbor_num; Knum++)
            {
                int neightId = indices[Knum];
                pcl::PointXYZRGBNormal neightborpoint = mls_points.points[neightId];
                //閭诲眳娉曠嚎 * 褰撳墠鐐规硶绾?
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


        //娉曠嚎鏍囧噯鍖?        calculateVertexNormal(mls_points);
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
    ////淇ˉ绌烘礊
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
