
#include <limits>
#include <fstream>
#include <vector>


#include "base.h"
#include "FileLibrary.h"
#include "pclbase.h"


typedef PointTRGB PointRGBCloud;
typedef PointRGBCloud::Ptr PointRGBPtr;
typedef pcl::VoxelGrid<pcl::PointXYZRGB> VoxelGridT;

double  normal_radius, feature_radius, min_sample_distance, max_correspondence_distance, nr_iterations, voxel_grid_size1, voxel_grid_size2, euclidean, voxel_grid_size3;
float normalIter, neighborNum, neighborDistance;
bool parseArguments(const string &path){  

    normal_radius = feature_radius = min_sample_distance = max_correspondence_distance = nr_iterations = voxel_grid_size1 = voxel_grid_size2 = euclidean = 0;
    string configfile = path + "\\merge.cfg";
    if (!FileLibrary::getInstance()->isFileExists(configfile))
    {
        cout << COUT_PREFIX << "config file no find. file =" << configfile << endl;
        return false;
    }

    cout.precision(8);
    ifstream iff(configfile);
    string line;
    while (getline(iff, line))
    {
        if (line.empty())
        {
            continue;
        }

        int pos = line.find("=") + 1;
        if (line.find("normal_radius=") != string::npos)
        {
            string tmp = line.substr(pos, line.length() - pos);

            normal_radius = atof(line.substr(pos, line.length() - pos).c_str());
            cout << "normal_radius:" << normal_radius << endl;
        }
        else  if (line.find("feature_radius=") != string::npos)
        {
            string tmp = line.substr(pos, line.length() - pos);

            feature_radius = atof(line.substr(pos, line.length() - pos).c_str());
            cout << "feature_radius:" << feature_radius << endl;

        }
        else  if (line.find("min_sample_distance=") != string::npos)
        {
            string tmp = line.substr(pos, line.length() - pos);

            min_sample_distance = atof(line.substr(pos, line.length() - pos).c_str());
            cout << "normal_radius:" << normal_radius << endl;                  

        }
        else  if (line.find("max_correspondence_distance=") != string::npos)
        {
            string tmp = line.substr(pos, line.length() - pos);

            max_correspondence_distance = atof(line.substr(pos, line.length() - pos).c_str());
            cout << "max_correspondence_distance:" << max_correspondence_distance << endl;

        }
        else  if (line.find("nr_iterations=") != string::npos)
        {
            string tmp = line.substr(pos, line.length() - pos);

            nr_iterations = atof(line.substr(pos, line.length() - pos).c_str());
            cout << "nr_iterations:" << nr_iterations << endl;

        }
        else  if (line.find("voxel_grid_size1=") != string::npos)
        {
            string tmp = line.substr(pos, line.length() - pos);

            voxel_grid_size1 = atof(line.substr(pos, line.length() - pos).c_str());
            cout << "voxel_grid_size1:" << voxel_grid_size1 << endl;

        }
        else  if (line.find("voxel_grid_size2=") != string::npos)
        {
            string tmp = line.substr(pos, line.length() - pos);

            voxel_grid_size2 = atof(line.substr(pos, line.length() - pos).c_str());
            cout << "voxel_grid_size2:" << voxel_grid_size2 << endl;

        }
        else  if (line.find("voxel_grid_size3=") != string::npos)
        {
            string tmp = line.substr(pos, line.length() - pos);

            voxel_grid_size3 = atof(line.substr(pos, line.length() - pos).c_str());
            cout << "voxel_grid_size3:" << voxel_grid_size3 << endl;

        }
        else  if (line.find("euclidean=") != string::npos)
        {
            string tmp = line.substr(pos, line.length() - pos);

            euclidean = atof(line.substr(pos, line.length() - pos).c_str());
            cout << "euclidean:" << euclidean << endl;

        }
        else  if (line.find("normalIter=") != string::npos)
        {
            string tmp = line.substr(pos, line.length() - pos);

            normalIter = atof(line.substr(pos, line.length() - pos).c_str());
            cout << "normalIter:" << normalIter << endl;

        }
        else  if (line.find("neighborNum=") != string::npos)
        {
            string tmp = line.substr(pos, line.length() - pos);

            neighborNum = atof(line.substr(pos, line.length() - pos).c_str());
            cout << "neighborNum:" << neighborNum << endl;

        }
        else  if (line.find("neighborDistance=") != string::npos)
        {
            string tmp = line.substr(pos, line.length() - pos);

            neighborDistance = atof(line.substr(pos, line.length() - pos).c_str());
            cout << "neighborDistance:" << neighborDistance << endl;

        }
        
    }
    iff.close();
    
    return true;
}


class FeatureCloud
{
public:
    // A bit of shorthand
    typedef PointRGBCloud PointCloud;
    typedef pcl::PointCloud<pcl::Normal> SurfaceNormals;
    typedef pcl::PointCloud<pcl::FPFHSignature33> LocalFeatures;
    typedef pcl::search::KdTree<pcl::PointXYZRGB> SearchMethod;

    FeatureCloud() :
        search_method_xyz_(new SearchMethod),
        normal_radius_(normal_radius),
        feature_radius_(feature_radius)
    {}

    ~FeatureCloud() {}

    // Process the given cloud
    void
        setInputCloud(PointCloud::Ptr xyz)
    {
            xyz_ = xyz;
            processInput();
        }

    // Load and process the cloud in the given PCD file
    void
        loadInputCloud(const std::string &pcd_file)
    {
            xyz_ = PointCloud::Ptr(new PointCloud);
            if (pcd_file.find(".ply") != string::npos)
            {
                pcl::io::loadPLYFile(pcd_file, *xyz_);

                if (voxel_grid_size1!=0)
                {
                    VoxelGridT vox_grid;
                    vox_grid.setInputCloud(xyz_);
                    vox_grid.setLeafSize(voxel_grid_size1, voxel_grid_size1, voxel_grid_size1);

                   PointRGBPtr tempCloud(new PointRGBCloud);
                    vox_grid.filter(*tempCloud);
                    xyz_ = tempCloud;

                }

            }
            else  if (pcd_file.find(".pcd") != string::npos)
            {
                pcl::io::loadPCDFile(pcd_file, *xyz_);
                VoxelGridT vox_grid;
                vox_grid.setInputCloud(xyz_);
                vox_grid.setLeafSize(voxel_grid_size1, voxel_grid_size1, voxel_grid_size1);

               PointRGBPtr tempCloud(new PointRGBCloud);
                vox_grid.filter(*tempCloud);
                xyz_ = tempCloud;

            }
            processInput();
        }

    // Get a pointer to the cloud 3D points
    PointCloud::Ptr
        getPointCloud() const
    {
            return (xyz_);
        }

    // Get a pointer to the cloud of 3D surface normals
    SurfaceNormals::Ptr
        getSurfaceNormals() const
    {
            return (normals_);
        }

    // Get a pointer to the cloud of feature descriptors
    LocalFeatures::Ptr
        getLocalFeatures() const
    {
            return (features_);
        }

protected:
    // Compute the surface normals and local features
    void
        processInput()
    {
            computeSurfaceNormals();
            computeLocalFeatures();
        }

    // Compute the surface normals
    void
        computeSurfaceNormals()
    {
            normals_ = SurfaceNormals::Ptr(new SurfaceNormals);
            

            pcl::NormalEstimation<pcl::PointXYZRGB, pcl::Normal> norm_est;
            //norm_est.setKSearch(30);
            norm_est.setInputCloud(xyz_);
            norm_est.setSearchMethod(search_method_xyz_);
            norm_est.setRadiusSearch(normal_radius_);
            norm_est.compute(*normals_);
            cout << COUT_PREFIX << "normals point size :" << normals_ ->points.size()<< endl;
        }

    // Compute the local feature descriptors
    void
        computeLocalFeatures()
    {
            features_ = LocalFeatures::Ptr(new LocalFeatures);

            pcl::FPFHEstimation<pcl::PointXYZRGB, pcl::Normal, pcl::FPFHSignature33> fpfh_est;
            fpfh_est.setInputCloud(xyz_);
            fpfh_est.setInputNormals(normals_);
            fpfh_est.setSearchMethod(search_method_xyz_);
            fpfh_est.setRadiusSearch(feature_radius_);
            fpfh_est.compute(*features_);
            cout << COUT_PREFIX << "features point size :" << features_->points.size() << endl;

        }

private:
    // Point cloud data
    PointCloud::Ptr xyz_;
    SurfaceNormals::Ptr normals_;
    LocalFeatures::Ptr features_;
    SearchMethod::Ptr search_method_xyz_;

    // Parameters
    float normal_radius_;
    float feature_radius_;
};

class TemplateAlignment
{
public:

    // A struct for storing alignment results
    struct Result
    {
        float fitness_score;
        Eigen::Matrix4f final_transformation;
        EIGEN_MAKE_ALIGNED_OPERATOR_NEW
    };

    TemplateAlignment() :
        min_sample_distance_(min_sample_distance),
        max_correspondence_distance_(max_correspondence_distance),
        nr_iterations_(nr_iterations)
    {
        // Intialize the parameters in the Sample Consensus Intial Alignment (SAC-IA) algorithm
        sac_ia_.setMinSampleDistance(min_sample_distance_);

       // Set the maximum distance threshold between two correspondent points in source <->target.
       // If the distance is larger than this threshold, the points will be ignored in the alignment process.
        sac_ia_.setMaxCorrespondenceDistance(max_correspondence_distance_);      
        
        sac_ia_.setMaximumIterations(nr_iterations_);
    }

    ~TemplateAlignment() {}

    // Set the given cloud as the target to which the templates will be aligned
    void
        setTargetCloud(FeatureCloud &target_cloud)
    {
            target_ = target_cloud;
            sac_ia_.setInputTarget(target_cloud.getPointCloud());
            sac_ia_.setTargetFeatures(target_cloud.getLocalFeatures());
        }

    // Add the given cloud to the list of template clouds
    void
        addTemplateCloud(FeatureCloud &template_cloud)
    {
            templates_.push_back(template_cloud);
        }

    // Align the given template cloud to the target specified by setTargetCloud ()
    void
        align(FeatureCloud &template_cloud, TemplateAlignment::Result &result)
    {
            sac_ia_.setInputSource(template_cloud.getPointCloud());
            sac_ia_.setSourceFeatures(template_cloud.getLocalFeatures());

            PointRGBCloud registration_output;
            sac_ia_.align(registration_output);

            result.fitness_score = (float)sac_ia_.getFitnessScore(max_correspondence_distance_);
            result.final_transformation = sac_ia_.getFinalTransformation();
        }

    // Align all of template clouds set by addTemplateCloud to the target specified by setTargetCloud ()
    void
        alignAll(std::vector<TemplateAlignment::Result, Eigen::aligned_allocator<Result> > &results)
    {
            results.resize(templates_.size());
            for (size_t i = 0; i < templates_.size(); ++i)
            {
                align(templates_[i], results[i]);
            }
        }

    // Align all of template clouds to the target cloud to find the one with best alignment score
    int
        findBestAlignment(TemplateAlignment::Result &result)
    {
            // Align all of the templates to the target cloud
            std::vector<Result, Eigen::aligned_allocator<Result> > results;
            alignAll(results);

            // Find the template with the best (lowest) fitness score
            float lowest_score = std::numeric_limits<float>::infinity();
            int best_template = 0;
            for (size_t i = 0; i < results.size(); ++i)
            {
                const Result &r = results[i];
                if (r.fitness_score < lowest_score)
                {
                    lowest_score = r.fitness_score;
                    best_template = (int)i;
                }
            }

            // Output the best alignment
            result = results[best_template];
            return (best_template);
        }

private:
    // A list of template clouds and the target to which they will be aligned
    std::vector<FeatureCloud> templates_;
    FeatureCloud target_;

    // The Sample Consensus Initial Alignment (SAC-IA) registration routine and its parameters
    pcl::SampleConsensusInitialAlignment<pcl::PointXYZRGB, pcl::PointXYZRGB, pcl::FPFHSignature33> sac_ia_;
    float min_sample_distance_;
    float max_correspondence_distance_;
    int nr_iterations_;
};


#if 0
// Align a collection of object templates to a sample point cloud
int
main(int argc, char **argv)
{


    if (argc < 3)
    {
        printf("No target PCD file given!\n");
        return (-1);
    }

    // Load the object templates specified in the object_templates.txt file
    std::vector<FeatureCloud> object_templates;
    std::ifstream input_stream(argv[1]);
    object_templates.resize(0);
    std::string pcd_filename;
    string outdir;
   PointRGBPtr cloud2(new PointRGBCloud);

    while (input_stream.good())
    {
        std::getline(input_stream, pcd_filename);
        if (pcd_filename.empty() || pcd_filename.at(0) == '#') // Skip blank lines or comments
            continue;

        cloud2->clear();
        pcl::io::loadPCDFile<pcl::PointXYZRGB>(pcd_filename, *cloud2);


        outdir = pcd_filename.substr(0, pcd_filename.find(".pcd"))+".ply";// FileLibrary::getInstance()->getFileParentPath(pcd_filename);
        pcl::io::savePLYFile<pcl::PointXYZRGB>(outdir, *cloud2);

        FeatureCloud template_cloud;
        template_cloud.loadInputCloud(pcd_filename);
        object_templates.push_back(template_cloud);
    }
    input_stream.close();

    // Load the target cloud PCD file
   PointRGBPtr cloud(new PointRGBCloud);
    pcl::io::loadPCDFile(argv[2], *cloud);

    // Preprocess the cloud by...
    // ...removing distant points
    const float depth_limit = 1.0;
    pcl::PassThrough<pcl::PointXYZRGB> pass;
    pass.setInputCloud(cloud);
    pass.setFilterFieldName("z");
    pass.setFilterLimits(0, depth_limit);
    pass.filter(*cloud);

    // ... and downsampling the point cloud
    const float voxel_grid_size = 0.005f;
    VoxelGridT vox_grid;
    vox_grid.setInputCloud(cloud);
    vox_grid.setLeafSize(voxel_grid_size, voxel_grid_size, voxel_grid_size);
    //vox_grid.filter (*cloud); // Please see this http://www.pcl-developers.org/Possible-problem-in-new-VoxelGrid-implementation-from-PCL-1-5-0-td5490361.html
   PointRGBPtr tempCloud(new PointRGBCloud);
    vox_grid.filter(*tempCloud);
    cloud = tempCloud;

    // Assign to the target FeatureCloud
    FeatureCloud target_cloud;
    target_cloud.setInputCloud(cloud);

    // Set the TemplateAlignment inputs
    TemplateAlignment template_align;
    for (size_t i = 0; i < object_templates.size(); ++i)
    {
        template_align.addTemplateCloud(object_templates[i]);
    }
    template_align.setTargetCloud(target_cloud);

    // Find the best template alignment
    TemplateAlignment::Result best_alignment;
    int best_index = template_align.findBestAlignment(best_alignment);
    const FeatureCloud &best_template = object_templates[best_index];

    // Print the alignment fitness score (values less than 0.00002 are good)
    printf("Best fitness score: %f\n", best_alignment.fitness_score);

    // Print the rotation matrix and translation vector
    Eigen::Matrix3f rotation = best_alignment.final_transformation.block<3, 3>(0, 0);
    Eigen::Vector3f translation = best_alignment.final_transformation.block<3, 1>(0, 3);

    printf("\n");
    printf("    | %6.3f %6.3f %6.3f | \n", rotation(0, 0), rotation(0, 1), rotation(0, 2));
    printf("R = | %6.3f %6.3f %6.3f | \n", rotation(1, 0), rotation(1, 1), rotation(1, 2));
    printf("    | %6.3f %6.3f %6.3f | \n", rotation(2, 0), rotation(2, 1), rotation(2, 2));
    printf("\n");
    printf("t = < %0.3f, %0.3f, %0.3f >\n", translation(0), translation(1), translation(2));

    // Save the aligned template for visualization
    PointRGBCloud transformed_cloud;
    pcl::transformPointCloud(*best_template.getPointCloud(), transformed_cloud, best_alignment.final_transformation);
    //pcl::io::savePCDFileBinary("output.pcd", transformed_cloud);
    //string outdir = FileLibrary::getInstance()->getFileParentPath(pcd_filename);
    cout << COUT_PREFIX << "" << outdir << endl;
    //pcl::io::savePLYFile(outdir+"\\output.ply", transformed_cloud);


    //PointRGBCloud::Ptr cloud1(new PointRGBCloud);
    //PointRGBCloud::Ptr cloud2(new PointRGBCloud);

    //pcl::io::loadPCDFile<pcl::PointXYZRGB>(argv[2], *cloud1);
    
    //pcl::io::savePLYFileBinary<pcl::PointXYZRGB>(outdir + "\\person.ply", *cloud1);
    

    return (0);

    }
#endif

#if 0
int  main(int argc, char **argv)
{

    if (argc < 3)
    {
        printf("No target PCD file given!\n");
        return (-1);
    }

    if (!parseArguments(FileLibrary::getInstance()->getFileParentPath(argv[2]) ) ){
        return false;
    }


    // Load the object templates specified in the object_templates.txt file
    std::vector<FeatureCloud> object_templates;
    object_templates.resize(0);
    PointRGBPtr src_cloud(new PointRGBCloud);
    std::string pcd_filename;


    string src_point_cloud_path = argv[1];      //test
    //pcl::io::loadPLYFile(src_point_cloud_path, *src_cloud);

    //Eigen::Matrix< float, 4, 1 > centroid;
    //pcl::compute3DCentroid(*src_cloud, centroid);
    //Eigen::Affine3f tMatrix = Eigen::Affine3f::Identity();     
    //tMatrix.translation() << centroid.x() * -1, centroid.y() * -1, centroid.z() * -1;

    //PointRGBPtr tmp_src_cloud(new PointRGBCloud);
    //pcl::transformPointCloud(*src_cloud, *tmp_src_cloud, tMatrix);
    //string outfile = FileLibrary::getInstance()->getFileParentPath(src_point_cloud_path) + "\\out_center_1.ply";
    //pcl::io::savePLYFile(outfile, *tmp_src_cloud);

    //src_cloud = tmp_src_cloud;


    FeatureCloud template_cloud;
    template_cloud.loadInputCloud(src_point_cloud_path);
    //template_cloud.setInputCloud(src_cloud);
    object_templates.push_back(template_cloud);   

   

    //std::ifstream input_stream(argv[1]);
    //while (input_stream.good())
    //{
    //    std::getline(input_stream, pcd_filename);
    //    if (pcd_filename.empty() || pcd_filename.at(0) == '#') // Skip blank lines or comments
    //        continue;                        

    //    FeatureCloud template_cloud;
    //    template_cloud.loadInputCloud(pcd_filename);
    //    object_templates.push_back(template_cloud);
    //    src_point_cloud = pcd_filename;
    //}
    //input_stream.close();

    // Load the target cloud PCD file
    PointRGBPtr tgt_cloud(new PointRGBCloud);
    string tat_path = argv[2];
    if (tat_path.find(".pcd") != string::npos)
    {
        pcl::io::loadPCDFile(argv[2], *tgt_cloud);

    }
    else if (tat_path.find(".ply") != string::npos)
    {
        pcl::io::loadPLYFile(argv[2], *tgt_cloud);

    }


    //tmp_src_cloud->clear();
    //pcl::compute3DCentroid(*src_cloud, centroid);    
    //tMatrix.translation() << centroid.x() * -1, centroid.y() * -1, centroid.z() * -1;    
    //pcl::transformPointCloud(*tgt_cloud, *tmp_src_cloud, tMatrix);
    //outfile = FileLibrary::getInstance()->getFileParentPath(src_point_cloud_path) + "\\out_center_2.ply";
    //pcl::io::savePLYFile(outfile, *tmp_src_cloud);

    //return 0;

    // Preprocess the cloud by...
    // ...removing distant points
    //const float depth_limit = 1.0;
    //pcl::PassThrough<pcl::PointXYZRGB> pass;
    //pass.setInputCloud(cloud);
    //pass.setFilterFieldName("z");
    //pass.setFilterLimits(0, depth_limit);
    //pass.filter(*cloud);

    // ... and downsampling the point cloud
    if (voxel_grid_size2 != 0)
    {
        VoxelGridT vox_grid;
        vox_grid.setInputCloud(tgt_cloud);
        vox_grid.setLeafSize(voxel_grid_size2, voxel_grid_size2, voxel_grid_size2);
        
        PointRGBPtr tempCloud(new PointRGBCloud);
        vox_grid.filter(*tempCloud);
        cout << COUT_PREFIX << "VoxelGrid point size :" << tempCloud->points.size() << endl;
        tgt_cloud = tempCloud;

    }

    // Assign to the target FeatureCloud
    FeatureCloud target_cloud;
    target_cloud.setInputCloud(tgt_cloud);

    // Set the TemplateAlignment inputs
    TemplateAlignment template_align;
    for (size_t i = 0; i < object_templates.size(); ++i)
    {
        template_align.addTemplateCloud(object_templates[i]);
    }
    template_align.setTargetCloud(target_cloud);

    // Find the best template alignment
    TemplateAlignment::Result best_alignment;
    int best_index = template_align.findBestAlignment(best_alignment);
    const FeatureCloud &best_template = object_templates[best_index];

    // Print the alignment fitness score (values less than 0.00002 are good)
    printf("Best fitness score: %f\n", best_alignment.fitness_score);

    // Print the rotation matrix and translation vector
    Eigen::Matrix3f rotation = best_alignment.final_transformation.block<3, 3>(0, 0);
    Eigen::Vector3f translation = best_alignment.final_transformation.block<3, 1>(0, 3);

    //printf("\n");
    //printf("    | %6.3f %6.3f %6.3f | \n", rotation(0, 0), rotation(0, 1), rotation(0, 2));
    //printf("R = | %6.3f %6.3f %6.3f | \n", rotation(1, 0), rotation(1, 1), rotation(1, 2));
    //printf("    | %6.3f %6.3f %6.3f | \n", rotation(2, 0), rotation(2, 1), rotation(2, 2));
    //printf("\n");
    //printf("t = < %0.3f, %0.3f, %0.3f >\n", translation(0), translation(1), translation(2));

    // Save the aligned template for visualization
   PointRGBPtr transformed_cloud(new PointRGBCloud);
   
    //加载原点云,并通过对齐矩阵转换,保存
    pcl::io::loadPLYFile(src_point_cloud_path, *src_cloud);
    pcl::transformPointCloud(*src_cloud/**best_template.getPointCloud()*/, *transformed_cloud, best_alignment.final_transformation);
    
    string outfile = FileLibrary::getInstance()->getFileParentPath(tat_path) + "\\output_align.ply";
    pcl::io::savePLYFile(outfile, *transformed_cloud);    
    
    cout << COUT_PREFIX << " align point: " << transformed_cloud->points.size() << endl;
    if (transformed_cloud->points.size() ==0)
    {
        return false;
    }
    //ICP ------------------------------------------------------------------------
   PointRGBPtr icp_out_cloud(new PointRGBCloud);    
    //读取目标点云
    pcl::io::loadPLYFile(argv[2], *tgt_cloud); 

    VoxelGridT vox_grid;
    vox_grid.setInputCloud(tgt_cloud);
    vox_grid.setLeafSize(voxel_grid_size1, voxel_grid_size1, voxel_grid_size1);
    //下采样目标点云
   PointRGBPtr vox_tgt_cloud(new PointRGBCloud);
    vox_grid.filter(*vox_tgt_cloud);
    

    pcl::search::KdTree<pcl::PointXYZRGB>::Ptr tree(new pcl::search::KdTree<pcl::PointXYZRGB>());
    //
    pcl::IterativeClosestPointNonLinear<pcl::PointXYZRGB, pcl::PointXYZRGB> icp;
    icp.setSearchMethodTarget(tree);
    icp.setInputSource(best_template.getPointCloud());     //特征粗配后的点云
    icp.setInputTarget(vox_tgt_cloud);

    icp.setMaximumIterations(nr_iterations);                    //迭代次数约束

    icp.setMaxCorrespondenceDistance(max_correspondence_distance); //忽略在此距离之外的点，如果两个点云距离较大，这个值要设的大一些（PCL默认距离单位是m）。
    icp.setTransformationEpsilon(1e-8);                                 //这个值一般设为1e-6或者更小
    icp.setEuclideanFitnessEpsilon(euclidean);                      //前后两次迭代误差的差值(两次迭代矩阵之间的距离)

    icp.align(*icp_out_cloud);

    cout << COUT_PREFIX << " icp align point: " << icp_out_cloud->points.size() << endl;
    if (icp_out_cloud->points.size() == 0)
    {
        return false;
    }
    //保存把原点云转换递进目标的点云(采样后)
    outfile = FileLibrary::getInstance()->getFileParentPath(tat_path) + "\\output_icp_1.ply";
    pcl::io::savePLYFile(outfile, *icp_out_cloud);

    //获取icp后的矩阵,通过矩阵转换原始点云
    Eigen::Matrix4f transformation = icp.getFinalTransformation();
    cout << transformation.matrix() << endl;
    //cloud->clear();
    
    icp_out_cloud->clear();
    pcl::transformPointCloud(*src_cloud, *icp_out_cloud, transformation);

    outfile = FileLibrary::getInstance()->getFileParentPath(tat_path) + "\\output_icp_2.ply";
    pcl::io::savePLYFile(outfile, *icp_out_cloud);

    // joint
    //合并原,目标点云
    *tgt_cloud += *icp_out_cloud;

    //栅格采样点云
    vox_grid.setInputCloud(tgt_cloud);
    vox_grid.setLeafSize(voxel_grid_size3, voxel_grid_size3, voxel_grid_size3);  
    vox_tgt_cloud->clear();
    vox_grid.filter(*vox_tgt_cloud);

    //保存融合的点云
    outfile = FileLibrary::getInstance()->getFileParentPath(tat_path) + "\\output_merge.ply";
    pcl::io::savePLYFile(outfile, *vox_tgt_cloud);
    //end joint
    cout << COUT_PREFIX << " icp merge vox point: " << vox_tgt_cloud->points.size() << endl;

    return (0);
}
#endif



#if 1

int  main(int argc, char **argv)
{

    if (argc < 3)
    {
        printf("No target PCD file given!\n");
        return (-1);
    }

    if (!parseArguments(FileLibrary::getInstance()->getFileParentPath(argv[2]))){
        return false;
    }
    string outfile;

    // Load the object templates specified in the object_templates.txt file
    std::vector<FeatureCloud> object_templates;
    object_templates.resize(0);
    PointRGBPtr src_cloud(new PointRGBCloud);
    std::string pcd_filename;


    string src_point_cloud_path = argv[1];      //test      
    if (src_point_cloud_path.find(".ply") != string::npos)
    {
        pcl::io::loadPLYFile(src_point_cloud_path, *src_cloud);
        PointRGBPtr tmp_cloud(new PointRGBCloud);
        Eigen::Affine3f tMatrix1 = Eigen::Affine3f::Identity();
        tMatrix1.rotate(Eigen::AngleAxisf(-30 * (M_PI / 180), Eigen::Vector3f::UnitX()));
        pcl::transformPointCloud(*src_cloud, *tmp_cloud, tMatrix1);
        outfile = FileLibrary::getInstance()->getFileParentPath(src_point_cloud_path) + "\\input_rotate_1.ply";
        pcl::io::savePLYFile(outfile, *tmp_cloud);

        Eigen::Affine3f tMatrix2 = Eigen::Affine3f::Identity();
        tMatrix2.rotate(Eigen::AngleAxisf(-90 * (M_PI / 180), Eigen::Vector3f::UnitY()));
        pcl::transformPointCloud(*tmp_cloud, *src_cloud, tMatrix2);
        outfile = FileLibrary::getInstance()->getFileParentPath(src_point_cloud_path) + "\\input_rotate_2.ply";
        pcl::io::savePLYFile(outfile, *src_cloud);

        Eigen::Affine3f tMatrix3 = Eigen::Affine3f::Identity();
        tMatrix3.rotate(Eigen::AngleAxisf(30 * (M_PI / 180), Eigen::Vector3f::UnitX()));
        pcl::transformPointCloud(*src_cloud, *tmp_cloud, tMatrix3);
        src_cloud = tmp_cloud;
        outfile = FileLibrary::getInstance()->getFileParentPath(src_point_cloud_path) + "\\input_rotate_3.ply";

        pcl::io::savePLYFile(outfile, *src_cloud);
    }

    // Load the target cloud PCD file
    PointRGBPtr tgt_cloud(new PointRGBCloud);
    string tat_path = argv[2];
    if (tat_path.find(".ply") != string::npos)
    {
        pcl::io::loadPLYFile(tat_path, *tgt_cloud);

    }

    // ... and downsampling the point cloud
    if (voxel_grid_size2 != 0)
    {
        //VoxelGridT vox_grid;
        //vox_grid.setInputCloud(tgt_cloud);
        //vox_grid.setLeafSize(voxel_grid_size2, voxel_grid_size2, voxel_grid_size2);

        //PointRGBPtr tempCloud(new PointRGBCloud);
        //vox_grid.filter(*tempCloud);
        //cout << COUT_PREFIX << "VoxelGrid point size :" << tempCloud->points.size() << endl;
        //tgt_cloud = tempCloud;

    }

    //ICP ------------------------------------------------------------------------

    //读取目标点云
    pcl::io::loadPLYFile(argv[2], *tgt_cloud);

    VoxelGridT vox_grid;
    vox_grid.setLeafSize(voxel_grid_size1, voxel_grid_size1, voxel_grid_size1);

    PointRGBPtr tmp_src_cloud(new PointRGBCloud);
    vox_grid.setInputCloud(src_cloud);
    vox_grid.filter(*tmp_src_cloud);
    //src_cloud = tmp_src_cloud;
    cout << COUT_PREFIX << " vox src point: " << tmp_src_cloud->points.size() << endl;

    vox_grid.setLeafSize(voxel_grid_size2, voxel_grid_size2, voxel_grid_size2);

    vox_grid.setInputCloud(tgt_cloud);
    //下采样目标点云
    PointRGBPtr vox_tgt_cloud(new PointRGBCloud);
    vox_grid.filter(*vox_tgt_cloud);
    cout << COUT_PREFIX << " vox tgt point: " << vox_tgt_cloud->points.size() << endl;



    PointTRGBNPtr src_n(new PointTRGBN);
    PointTRGBNPtr tat_n(new PointTRGBN);
    for (size_t i = 0; i < normalIter; i++)
    {
        PCLBASE::getInstance()->normalsMovingLeastSquares(tmp_src_cloud, *src_n, neighborDistance);
        PCLBASE::getInstance()->getPointFromPointNormal(tmp_src_cloud, *src_n);

    }
    PCLBASE::getInstance()->nearestKSearchNormal(*src_n, normalIter, neighborNum, neighborDistance);

    outfile = FileLibrary::getInstance()->getFileParentPath(src_point_cloud_path) + "\\out_normals_1.ply";
    pcl::io::savePLYFile(outfile, *src_n);
    cout << COUT_PREFIX << " tgt normal point: " << src_n->points.size() << endl;

    for (size_t i = 0; i < normalIter; i++){
        PCLBASE::getInstance()->normalsMovingLeastSquares(vox_tgt_cloud, *tat_n, neighborDistance);
        PCLBASE::getInstance()->getPointFromPointNormal(vox_tgt_cloud, *tat_n);

    }

    PCLBASE::getInstance()->nearestKSearchNormal(*tat_n, normalIter, neighborNum, neighborDistance);
    outfile = FileLibrary::getInstance()->getFileParentPath(src_point_cloud_path) + "\\out_normals_2.ply";
    pcl::io::savePLYFile(outfile, *tat_n);
    cout << COUT_PREFIX << " tgt normal point: " << tat_n->points.size() << endl;
     //----------------------------feature
    FeatureCloud src_feature;
    src_feature.setInputCloud(tmp_src_cloud);
    FeatureCloud tgt_feature;
    tgt_feature.setInputCloud(vox_tgt_cloud);

    TemplateAlignment template_align;
    template_align.addTemplateCloud(src_feature);
    template_align.setTargetCloud(tgt_feature);

    TemplateAlignment::Result best_alignment;
    int best_index = template_align.findBestAlignment(best_alignment);
    Eigen::Matrix4f rotation = best_alignment.final_transformation;

    PointRGBPtr transformed_cloud;
    pcl::transformPointCloud(*src_feature.getPointCloud(), *transformed_cloud, best_alignment.final_transformation);

    *transformed_cloud += *vox_tgt_cloud;

    outfile = FileLibrary::getInstance()->getFileParentPath(tat_path) + "\\transformed.ply";
    pcl::io::savePLYFile(outfile, *transformed_cloud);

    return 0;

   //-----------------------------------------

    pcl::search::KdTree<pcl::PointXYZRGBNormal>::Ptr tree(new pcl::search::KdTree<pcl::PointXYZRGBNormal>());
    //
    //pcl::IterativeClosestPointNonLinear<pcl::PointXYZRGB, pcl::PointXYZRGB> icp;
    pcl::IterativeClosestPointWithNormals<pcl::PointXYZRGBNormal, pcl::PointXYZRGBNormal> icp;
    icp.setSearchMethodTarget(tree);
    icp.setInputSource(src_n);     //特征粗配后的点云
    icp.setInputTarget(tat_n);

    icp.setMaximumIterations(nr_iterations);                    //迭代次数约束

    icp.setMaxCorrespondenceDistance(max_correspondence_distance); //忽略在此距离之外的点，如果两个点云距离较大，这个值要设的大一些（PCL默认距离单位是m）。
    icp.setTransformationEpsilon(1e-8);                                 //这个值一般设为1e-6或者更小
    icp.setEuclideanFitnessEpsilon(euclidean);                      //前后两次迭代误差的差值(两次迭代矩阵之间的距离)

    PointTRGBNPtr icp_out_cloud_n(new PointTRGBN);


    icp.align(*icp_out_cloud_n);

    PointRGBPtr icp_out_cloud(new PointRGBCloud);
    PCLBASE::getInstance()->getPointFromPointNormal(icp_out_cloud, *icp_out_cloud_n);

    cout << COUT_PREFIX << " icp align point: " << icp_out_cloud->points.size() << endl;
    if (icp_out_cloud->points.size() == 0)
    {
        return false;
    }
    //保存把原点云转换递进目标的点云(采样后)
    outfile = FileLibrary::getInstance()->getFileParentPath(tat_path) + "\\output_icp_1.ply";
    pcl::io::savePLYFile(outfile, *tat_n);

    //获取icp后的矩阵,通过矩阵转换原始点云
    Eigen::Matrix4f transformation = icp.getFinalTransformation();
    cout << transformation.matrix() << endl;
    //cloud->clear();

    icp_out_cloud->clear();

    pcl::io::loadPLYFile(FileLibrary::getInstance()->getFileParentPath(tat_path)+"\\out_center_1.ply", *src_cloud);
    //pcl::io::loadPLYFile(FileLibrary::getInstance()->getFileParentPath(tat_path) + "\\1855.ply", *tgt_cloud);

    pcl::transformPointCloud(*src_cloud, *icp_out_cloud, transformation);

    outfile = FileLibrary::getInstance()->getFileParentPath(tat_path) + "\\output_icp_2.ply";
    pcl::io::savePLYFile(outfile, *icp_out_cloud);


    // joint
    //合并原,目标点云

    *tgt_cloud += *icp_out_cloud;

    //栅格采样点云
    vox_grid.setInputCloud(tgt_cloud);
    vox_grid.setLeafSize(voxel_grid_size3, voxel_grid_size3, voxel_grid_size3);
    vox_tgt_cloud->clear();
    vox_grid.filter(*vox_tgt_cloud);

    //保存融合的点云
    outfile = FileLibrary::getInstance()->getFileParentPath(tat_path) + "\\output_merge.ply";
    pcl::io::savePLYFile(outfile, *vox_tgt_cloud);
    //end joint
    cout << COUT_PREFIX << " icp merge vox point: " << vox_tgt_cloud->points.size() << endl;

    return (0);
}
#endif