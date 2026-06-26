#include "base.h"
#include "pointcloud_splice.h"
#include <pcl/common/transforms.h>
#include <pcl/registration/icp.h>
#include <pcl/registration/icp_nl.h>
#include <pcl/common/impl/centroid.hpp>

//typedef PointTRGB PointT;
typedef PointTRGB::Ptr PointTPtr;
typedef pcl::VoxelGrid<pcl::PointXYZRGB> VoxelGridT;

//double  normal_radius, feature_radius, euclidean, max_correspondence_distance, nr_iterations, voxel_grid_size1, voxel_grid_size2, min_sample_distance;
//float m_rotate,X, Y, Z;


#if 0
class FeatureCloud
{
public:
    // A bit of shorthand
    typedef PointT PointCloud;
    typedef pcl::PointCloud<pcl::Normal> SurfaceNormals;
    typedef pcl::PointCloud<pcl::FPFHSignature33> LocalFeatures;
    typedef pcl::search::KdTree<pcl::PointXYZRGB> SearchMethod;

    FeatureCloud(float normal_radius, float feature_radius, float voxelgrid) :
        search_method_xyz_(new SearchMethod),
        normal_radius_(normal_radius),
        feature_radius_(feature_radius),
        voxel_grid_size1(voxelgrid)
    {
        //voxel_grid_size1 = 0.0008;
        //normal_radius_ = 0.002;
    };

    ~FeatureCloud() {};

    // Process the given cloud
    void setInputCloud(PointCloud::Ptr xyz)
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

                if (voxel_grid_size1 != 0)
                {
                    VoxelGridT vox_grid;
                    vox_grid.setInputCloud(xyz_);
                    vox_grid.setLeafSize(voxel_grid_size1, voxel_grid_size1, voxel_grid_size1);

                    PointTPtr tempCloud(new PointT);
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

                PointTPtr tempCloud(new PointT);
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
    void  processInput()
    {
            computeSurfaceNormals();
            computeLocalFeatures();
        }

    // Compute the surface normals
    void  computeSurfaceNormals()
    {
        normals_ = SurfaceNormals::Ptr(new SurfaceNormals);


        pcl::NormalEstimation<pcl::PointXYZRGB, pcl::Normal> norm_est;
        //norm_est.setKSearch(30);
        norm_est.setInputCloud(xyz_);
        norm_est.setSearchMethod(search_method_xyz_);
        norm_est.setRadiusSearch(normal_radius_);
        norm_est.compute(*normals_);
        cout << COUT_PREFIX << "normals point size :" << normals_->points.size() << endl;
    }

    // Compute the local feature descriptors
    void  computeLocalFeatures()
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
    float voxel_grid_size1, voxel_grid_size2;
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
    

    TemplateAlignment(float min_sample_distance, float max_correspondence_distance, float nr_iterations)
    {   
        max_correspondence_distance_ = max_correspondence_distance;
        // Intialize the parameters in the Sample Consensus Intial Alignment (SAC-IA) algorithm
        sac_ia_.setMinSampleDistance(min_sample_distance);

        // Set the maximum distance threshold between two correspondent points in source <->target.
        // If the distance is larger than this threshold, the points will be ignored in the alignment process.
        sac_ia_.setMaxCorrespondenceDistance(max_correspondence_distance_);

        sac_ia_.setMaximumIterations(nr_iterations);
    };

    ~TemplateAlignment() {};

    // Set the given cloud as the target to which the templates will be aligned
    void setTargetCloud(FeatureCloud &target_cloud)
    {
        target_ = target_cloud;
        sac_ia_.setInputTarget(target_cloud.getPointCloud());
        sac_ia_.setTargetFeatures(target_cloud.getLocalFeatures());
    }

    // Add the given cloud to the list of template clouds
    void  addTemplateCloud(FeatureCloud &template_cloud)
    {
        templates_.push_back(template_cloud);
    }

    // Align the given template cloud to the target specified by setTargetCloud ()
    void align(FeatureCloud &template_cloud, TemplateAlignment::Result &result)
    {
        sac_ia_.setInputCloud(template_cloud.getPointCloud());
        sac_ia_.setSourceFeatures(template_cloud.getLocalFeatures());

        PointT registration_output;
        sac_ia_.align(registration_output);

        result.fitness_score = (float)sac_ia_.getFitnessScore(max_correspondence_distance_);
        result.final_transformation = sac_ia_.getFinalTransformation();
    }

    // Align all of template clouds set by addTemplateCloud to the target specified by setTargetCloud ()
    void alignAll(std::vector<TemplateAlignment::Result, Eigen::aligned_allocator<Result> > &results)
    {
        results.resize(templates_.size());
        for (size_t i = 0; i < templates_.size(); ++i)
        {
            align(templates_[i], results[i]);
        }
    }

    // Align all of template clouds to the target cloud to find the one with best alignment score
    int  findBestAlignment(TemplateAlignment::Result &result)
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
    float max_correspondence_distance_;

};
#endif


PointCloudSplice::PointCloudSplice(const string &infile1, const string &infile2){

    m_strInplyfile1 = infile1;
    m_strInplyfile2 = infile2;
    //m_poiontcloud1 = new pcl::PointCloud<pcl::PointXYZ>;
    //m_poiontcloud2 = new pcl::PointCloud<pcl::PointXYZ>; 
    //m_outpointcloud = new pcl::PointCloud<pcl::PointXYZ>;


    outdir = FileLibrary::getInstance()->getFileParentPath(infile1);
}
bool PointCloudSplice::parseArguments(const string &path){

    normal_radius = feature_radius = euclidean = max_correspondence_distance = nr_iterations = voxel_grid_size1 = voxel_grid_size2 = min_sample_distance = 0;
    string configfile = path + "\\icp.cfg";
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
        else  if (line.find("euclidean=") != string::npos)
        {
            string tmp = line.substr(pos, line.length() - pos);

            euclidean = atof(line.substr(pos, line.length() - pos).c_str());
            cout << "euclidean:" << euclidean << endl;

        }
        else  if (line.find("rotate=") != string::npos)
        {
            string tmp = line.substr(pos, line.length() - pos);

            m_rotate = atof(line.substr(pos, line.length() - pos).c_str());
            cout << "m_rotate:" << m_rotate << endl;

        }
        else  if (line.find("X=") != string::npos)
        {
            string tmp = line.substr(pos, line.length() - pos);

            X = atof(line.substr(pos, line.length() - pos).c_str());
            cout << "X:" << X << endl;

        }
        else  if (line.find("Y=") != string::npos)
        {
            string tmp = line.substr(pos, line.length() - pos);

            Y = atof(line.substr(pos, line.length() - pos).c_str());
            cout << "Y:" << Y << endl;

        }
        else  if (line.find("Z=") != string::npos)
        {
            string tmp = line.substr(pos, line.length() - pos);

            Z = atof(line.substr(pos, line.length() - pos).c_str());
            cout << "Z:" << Z << endl;

        }

    }
    iff.close();

    return true;
}

bool PointCloudSplice::downsample(pcl::PointCloud<pcl::PointXYZRGB>::Ptr inpointcloud, pcl::PointCloud<pcl::PointXYZRGB>::Ptr outpointcloud){
    pcl::VoxelGrid<pcl::PointXYZRGB> grid;
    grid.setLeafSize(voxel_grid_size1, voxel_grid_size1, voxel_grid_size1);      //设置滤波处理时采用的体素大小
    grid.setInputCloud(inpointcloud);
    grid.filter(*outpointcloud);
    cout << COUT_PREFIX << "voxel grid . point size : "<< outpointcloud->points.size() << endl;
    return true;
}

#if 0
bool PointCloudSplice::begineReadPlyFile(){
    pcl::PointCloud<pcl::PointXYZRGB>::Ptr m_poiontcloud1(new pcl::PointCloud<pcl::PointXYZRGB>);
    pcl::PointCloud<pcl::PointXYZRGB>::Ptr m_poiontcloud2(new pcl::PointCloud<pcl::PointXYZRGB>);

    pcl::PointCloud<pcl::PointXYZRGB>::Ptr out_pointcloud1(new pcl::PointCloud<pcl::PointXYZRGB>);
    pcl::PointCloud<pcl::PointXYZRGB>::Ptr out_pointcloud2(new pcl::PointCloud<pcl::PointXYZRGB>);

    pcl::PointCloud<pcl::PointXYZRGB>::Ptr m_outpointcloud(new pcl::PointCloud<pcl::PointXYZRGB>);

    if (!FileLibrary::getInstance()->isFileExists(m_strInplyfile1) || !FileLibrary::getInstance()->isFileExists(m_strInplyfile1))
    {
        cout << COUT_PREFIX << "ply file error. file1:"<<m_strInplyfile1<<" file2:"<<m_strInplyfile2 << endl;
        return false;
    }

    if (pcl::io::loadPLYFile(m_strInplyfile1,*m_poiontcloud1) == -1)
    {
        cout << COUT_PREFIX<<" " << endl;
        return false;

    }
    cout << COUT_PREFIX << "point cloud 1. point size : " << m_poiontcloud1->points.size() << endl;

    //float theta = 45 * M_PI / 180;

    //Eigen::Affine3f transform_1 = Eigen::Affine3f::Identity();
    ////transform_1.translation() << -2, 0.0, 0.0;
    //transform_1.rotate(Eigen::AngleAxisf(-theta, Eigen::Vector3f::UnitY()));

    //pcl::transformPointCloud(*m_poiontcloud1, *out_pointcloud1, transform_1);


    if (pcl::io::loadPLYFile(m_strInplyfile2, *m_poiontcloud2) == -1)
    {
        cout << COUT_PREFIX << " " << endl;
        return false;

    }
    cout << COUT_PREFIX << "point cloud 2. point size : " << m_poiontcloud2->points.size() << endl;

    //Eigen::Affine3f transform_2 = Eigen::Affine3f::Identity();
    ////transform_1.translation() << -2, 0.0, 0.0;
    //transform_2.rotate(Eigen::AngleAxisf(theta, Eigen::Vector3f::UnitY()));

    //pcl::transformPointCloud(*m_poiontcloud2, *out_pointcloud2, transform_2);

    downsample(m_poiontcloud1, m_poiontcloud1);
    if (m_poiontcloud1->points.size() == 0)
    {
        cout << COUT_PREFIX << "1 point size 0." << endl;
        return false;
    }
    downsample(m_poiontcloud2, m_poiontcloud2);
    if (m_poiontcloud2->points.size() == 0)
    {
        cout << COUT_PREFIX << "2 point size 0." << endl;

        return false;
    }

    pcl::search::KdTree<pcl::PointXYZRGB>::Ptr tree(new pcl::search::KdTree<pcl::PointXYZRGB>());

    pcl::IterativeClosestPointNonLinear<pcl::PointXYZRGB, pcl::PointXYZRGB> icp;
    icp.setSearchMethodTarget(tree);
    icp.setInputSource(m_poiontcloud1);
    icp.setInputTarget(m_poiontcloud2);

    icp.setMaximumIterations(nr_iterations);     //迭代次数约束

    icp.setMaxCorrespondenceDistance(max_correspondence_distance); //忽略在此距离之外的点，如果两个点云距离较大，这个值要设的大一些（PCL默认距离单位是m）。
    icp.setTransformationEpsilon(1e-6); //这个值一般设为1e-6或者更小
    icp.setEuclideanFitnessEpsilon(euclidean);  //前后两次迭代误差的差值

    icp.align(*m_outpointcloud);
    string rotatefile = outdir + "\\out_pointcloud1.ply";
    pcl::io::savePLYFile(rotatefile, *m_outpointcloud);
    cout << COUT_PREFIX<< " out align file : " << rotatefile << endl;
    //Eigen::Matrix4f transformation = icp.getFinalTransformation();
    //m_outpointcloud->clear();
    //pcl::transformPointCloud(*m_poiontcloud1, *m_outpointcloud, transformation);

    //pcl::io::savePLYFile(outdir + "\\out_pointcloud2.ply", *m_outpointcloud);

    m_outpointcloud->resize(m_poiontcloud2->size() + m_outpointcloud->size());
    for (int i = 0; i<m_poiontcloud2->size(); i++)
    {  
        m_outpointcloud->push_back(m_poiontcloud2->points[i]);  
    } 
    string outJointfile = outdir + "\\out_pointcloud2.ply";
    pcl::io::savePLYFile(outJointfile, *m_outpointcloud);
    cout << COUT_PREFIX<< " out joint file : " << outJointfile << endl;


#if 0
    pcl::PointCloud<pcl::PointXYZ>::Ptr out_pointcloud1(new pcl::PointCloud<pcl::PointXYZ>), out_pointcloud2(new pcl::PointCloud<pcl::PointXYZ>);
    float theta =45 * M_PI / 180;

    //Eigen::Matrix4f transform_1 = Eigen::Matrix4f::Identity();
    Eigen::Affine3f transform_1 = Eigen::Affine3f::Identity();
    transform_1.translation() << -2, 0.0, 0.0;
    transform_1.rotate(Eigen::AngleAxisf(-theta, Eigen::Vector3f::UnitY()));

    pcl::transformPointCloud(*m_poiontcloud1, *out_pointcloud1, transform_1);

    pcl::io::savePLYFile(outdir + "\\out_pointcloud1.ply", *out_pointcloud1);


    Eigen::Affine3f transform_2 = Eigen::Affine3f::Identity();
    transform_2.translation() << 0, 0.0, -0.002;
    transform_2.rotate(Eigen::AngleAxisf(theta, Eigen::Vector3f::UnitY()));

    pcl::transformPointCloud(*m_poiontcloud2, *out_pointcloud2, transform_2);

    pcl::io::savePLYFile(outdir + "\\out_pointcloud2.ply", *out_pointcloud2);


    //m_outpointcloud = out_pointcloud1;
    //m_outpointcloud += out_pointcloud2;

    //pcl::io::savePLYFile(outdir + "\\out_pointcloud.ply", *m_outpointcloud);


    pcl::visualization::PCLVisualizer viewer("Matrix transformation example");

    viewer.addCoordinateSystem(1.0);
    viewer.setBackgroundColor(0.05, 0.05, 0.05, 0);
    pcl::visualization::PointCloudColorHandlerCustom<pcl::PointXYZRGB> transformed_cloud_color_handler(230, 20, 20); // Red  

    //pcl::PointCloud<pcl::PointXYZ>::Ptr out_pointcloud3(new pcl::PointCloud<pcl::PointXYZ>);
    //out_pointcloud3->resize(m_outpointcloud->points.size());
    //for (size_t i = 0; i < m_outpointcloud->points.size(); i++)
    //{
    //    out_pointcloud3->points[i].x = m_outpointcloud->points[i].x;
    //    out_pointcloud3->points[i].y = m_outpointcloud->points[i].y;
    //    out_pointcloud3->points[i].z = m_outpointcloud->points[i].z;
    //}
    
    //viewer.addPointCloud<pcl::PointXYZ>(out_pointcloud1, transformed_cloud_color_handler, "1");
    viewer.addPointCloud<pcl::PointXYZRGB>(m_outpointcloud, transformed_cloud_color_handler, "2");


    while (!viewer.wasStopped()) { // Display the visualiser until 'q' key is pressed  
        viewer.spinOnce();
    }
#endif
    return true;
}
#endif

void transform(pcl::PointCloud<pcl::PointXYZRGB>::Ptr inPoint, pcl::PointCloud<pcl::PointXYZRGB>::Ptr &outPoint){


    Eigen::Matrix< float, 4, 1 > 	centroid;
    pcl::compute3DCentroid(*inPoint, centroid);

    cout.precision(6);
    //cout << COUT_PREFIX << "x:" << centroid.x() << "    Y:" << centroid.y() << "    Z:" << centroid.z() << endl;


    Eigen::Affine3f tMatrix = Eigen::Affine3f::Identity();


    //pcl::getTransformation(0, 0, 0, 0, 0, 0, tMatrix);

    tMatrix.translation() << 0,0, centroid.z() * -1;    

    pcl::transformPointCloud(*inPoint, *outPoint, tMatrix);

    
}

bool PointCloudSplice::begineReadPlyFile(){
    pcl::PointCloud<pcl::PointXYZRGB>::Ptr m_poiontcloud1(new pcl::PointCloud<pcl::PointXYZRGB>);
    pcl::PointCloud<pcl::PointXYZRGB>::Ptr m_poiontcloud2(new pcl::PointCloud<pcl::PointXYZRGB>);

    pcl::PointCloud<pcl::PointXYZRGB>::Ptr out_pointcloud1(new pcl::PointCloud<pcl::PointXYZRGB>);
    pcl::PointCloud<pcl::PointXYZRGB>::Ptr out_pointcloud2(new pcl::PointCloud<pcl::PointXYZRGB>);

    pcl::PointCloud<pcl::PointXYZRGB>::Ptr m_outpointcloud(new pcl::PointCloud<pcl::PointXYZRGB>);

    string outfile;

    if (!FileLibrary::getInstance()->isFileExists(m_strInplyfile1) || !FileLibrary::getInstance()->isFileExists(m_strInplyfile2))
    {
        cout << COUT_PREFIX << "ply file error. file1:" << m_strInplyfile1 << " file2:" << m_strInplyfile2 << endl;
        return false;
    }
    //1
	if (pcl::io::loadPLYFile(m_strInplyfile1, *out_pointcloud1) == -1)
    {
        cout << COUT_PREFIX << " " << endl;
        return false;

    }
    cout << COUT_PREFIX << "point cloud 1. point size : " << out_pointcloud1->points.size() << endl;
    transform(out_pointcloud1, out_pointcloud1);

    if (m_rotate == 1)
    {

        Eigen::Affine3f tMatrix = Eigen::Affine3f::Identity();   
        tMatrix.rotate(Eigen::AngleAxisf(Y * (M_PI / 180), Eigen::Vector3f::UnitY()));
        pcl::transformPointCloud(*out_pointcloud1, *m_poiontcloud1, tMatrix);
        pcl::copyPoint(out_pointcloud1, out_pointcloud1);
        //out_pointcloud1 = m_poiontcloud1;

		string stroutfile = FileLibrary::getInstance()->getFileParentPath(m_strInplyfile1) + "\\out_1.ply";
		pcl::io::savePLYFile(stroutfile, *out_pointcloud1);       
    }


    //2
    if (pcl::io::loadPLYFile(m_strInplyfile2, *out_pointcloud2) == -1)
    {
        cout << COUT_PREFIX << " " << endl;
        return false;

    }
    transform(out_pointcloud2, out_pointcloud2);
    if (m_rotate == 2)
    {

        Eigen::Affine3f tMatrix = Eigen::Affine3f::Identity();
        tMatrix.rotate(Eigen::AngleAxisf(Y * (M_PI / 180), Eigen::Vector3f::UnitY()));
        pcl::transformPointCloud(*out_pointcloud2, *m_poiontcloud2, tMatrix);
        out_pointcloud2 = m_poiontcloud2;
		string outfile = FileLibrary::getInstance()->getFileParentPath(m_strInplyfile1) + "\\out_2.ply";
		pcl::io::savePLYFile(outfile, *out_pointcloud2);

    }
    cout << COUT_PREFIX << "point cloud 2. point size : " << out_pointcloud2->points.size() << endl;




    //pcl::transformPointCloud(*m_poiontcloud1, *m_poiontcloud2, transform_1);
    //outfile = SCEarthLibrary::getInstance()->getFileParentPath(m_strInplyfile1) + "\\out_2.ply";
    //pcl::io::savePLYFile(outfile, *m_poiontcloud2);
    //cout << COUT_PREFIX << "point cloud 2. point size : " << m_poiontcloud2->points.size() << endl;




///////////////////////////////////////////////
    pcl::PointCloud<pcl::PointXYZRGB>::Ptr tempCloud1(new pcl::PointCloud<pcl::PointXYZRGB>);
    pcl::PointCloud<pcl::PointXYZRGB>::Ptr tempCloud2(new pcl::PointCloud<pcl::PointXYZRGB>);
    if (voxel_grid_size1 != 0)
    {
        //1
        pcl::VoxelGrid<pcl::PointXYZRGB> vox_grid1;
        vox_grid1.setInputCloud(out_pointcloud1);
        vox_grid1.setLeafSize(voxel_grid_size1, voxel_grid_size1, voxel_grid_size1);

        vox_grid1.filter(*tempCloud1);
        string outfile = FileLibrary::getInstance()->getFileParentPath(m_strInplyfile1) + "\\voxelGrid_1.ply";
        pcl::io::savePLYFile(outfile, *tempCloud1);

        //2
        pcl::VoxelGrid<pcl::PointXYZRGB> vox_grid2;
        vox_grid2.setLeafSize(voxel_grid_size1, voxel_grid_size1, voxel_grid_size1);       
        vox_grid2.setInputCloud(out_pointcloud2);
        vox_grid2.filter(*tempCloud2);
        outfile = FileLibrary::getInstance()->getFileParentPath(m_strInplyfile1) + "\\voxelGrid_2.ply";
        pcl::io::savePLYFile(outfile, *tempCloud2);

    }
    else
    {
        tempCloud1 = out_pointcloud1;
        tempCloud2 = out_pointcloud2;
    }

    pcl::PointCloud<pcl::PointXYZRGB>::Ptr RadiusOutlierRemoval_cloud_filtered(new pcl::PointCloud<pcl::PointXYZRGB>);
    pcl::RadiusOutlierRemoval<pcl::PointXYZRGB> outrem;

    outrem.setInputCloud(tempCloud1);
    outrem.setRadiusSearch(0.01); // 0.00005 
    outrem.setMinNeighborsInRadius(200);        //3 

    outrem.filter(*tempCloud1);

    outrem.setInputCloud(tempCloud2);
    outrem.filter(*tempCloud2);


    cout << COUT_PREFIX << "voxelGrid cloud 1:" << tempCloud1->points.size() << endl;
    cout << COUT_PREFIX << "voxelGrid cloud 2:" << tempCloud2->points.size() << endl;
    outfile = FileLibrary::getInstance()->getFileParentPath(m_strInplyfile1) + "\\radius_1.ply";
    pcl::io::savePLYFile(outfile, *tempCloud1);

    outfile = FileLibrary::getInstance()->getFileParentPath(m_strInplyfile1) + "\\radius_2.ply";
    pcl::io::savePLYFile(outfile, *tempCloud2);

    
     ////////////////////////////////
    pcl::search::KdTree<pcl::PointXYZRGB>::Ptr tree(new pcl::search::KdTree<pcl::PointXYZRGB>());
    pcl::PointCloud<pcl::PointXYZRGB>::Ptr align_pointcloud(new pcl::PointCloud<pcl::PointXYZRGB>);

    pcl::IterativeClosestPointNonLinear<pcl::PointXYZRGB, pcl::PointXYZRGB> icp;
    icp.setSearchMethodTarget(tree);
    icp.setInputSource(tempCloud1);
    icp.setInputTarget(tempCloud2);

    icp.setMaximumIterations(nr_iterations);                    //迭代次数约束

    icp.setMaxCorrespondenceDistance(max_correspondence_distance); //忽略在此距离之外的点，如果两个点云距离较大，这个值要设的大一些（PCL默认距离单位是m）。
    icp.setTransformationEpsilon(1e-8);                                 //这个值一般设为1e-6或者更小
    icp.setEuclideanFitnessEpsilon(euclidean);                      //前后两次迭代误差的差值

    icp.align(*align_pointcloud);

    Eigen::Matrix4f transformation = icp.getFinalTransformation();
    cout << transformation.matrix() << endl;
    //cloud->clear();
    //pcl::io::loadPLYFile(SCEarthLibrary::getInstance()->getFileParentPath(tag)+"\\1639_2.ply", *cloud);
    //out_cloud->clear();
    //pcl::transformPointCloud(*cloud, *out_cloud, transformation);

    outfile = FileLibrary::getInstance()->getFileParentPath(m_strInplyfile1) + "\\output_icp.ply";
    pcl::io::savePLYFile(outfile, *align_pointcloud);
    cout << COUT_PREFIX << "out_cloud :" << align_pointcloud->points.size() << endl;

	*tempCloud2 += *align_pointcloud;
	align_pointcloud->clear();
	pcl::VoxelGrid<pcl::PointXYZRGB> grid;
	grid.setInputCloud(tempCloud2);
	grid.setLeafSize(voxel_grid_size2, voxel_grid_size2, voxel_grid_size2);
	grid.filter(*align_pointcloud);

	outfile = FileLibrary::getInstance()->getFileParentPath(m_strInplyfile1) + "\\output_merger_rigid.ply";
	pcl::io::savePLYFile(outfile, *align_pointcloud);
    cout << COUT_PREFIX << "merger point :" << align_pointcloud->points.size() << endl;

    return true;
}
#if 0
void align(string file1, string file2){

    //ICP
    pcl::PointCloud<pcl::PointXYZRGB>::Ptr out_cloud(new pcl::PointCloud<pcl::PointXYZRGB>);
    pcl::PointCloud<pcl::PointXYZRGB>::Ptr srccloud(new pcl::PointCloud<pcl::PointXYZRGB>);

    pcl::PointCloud<pcl::PointXYZRGB>::Ptr tgtcloud(new pcl::PointCloud<pcl::PointXYZRGB>);

    pcl::PointCloud<pcl::PointXYZRGB>::Ptr tempCloud1(new pcl::PointCloud<pcl::PointXYZRGB>);
    pcl::PointCloud<pcl::PointXYZRGB>::Ptr tempCloud2(new pcl::PointCloud<pcl::PointXYZRGB>);

    
    pcl::io::loadPLYFile(file1, *srccloud);
    cout << COUT_PREFIX << "point cloud 1:" << srccloud->points.size() << endl;
    pcl::io::loadPLYFile(file2, *tgtcloud);
    cout << COUT_PREFIX << "point cloud 2:" << tgtcloud->points.size() << endl;
    //1
    pcl::VoxelGrid<pcl::PointXYZRGB> vox_grid;
    vox_grid.setInputCloud(srccloud);
    vox_grid.setLeafSize(voxel_grid_size1, voxel_grid_size1, voxel_grid_size1);
                                                                                    
    vox_grid.filter(*tempCloud1);
    cout << COUT_PREFIX << "voxelGrid cloud 1:" << tempCloud1->points.size() << endl;

    //2
    vox_grid.setInputCloud(tgtcloud);
    vox_grid.filter(*tempCloud2);
    cout << COUT_PREFIX << "voxelGrid cloud 2:" << tempCloud2->points.size() << endl;

    pcl::search::KdTree<pcl::PointXYZRGB>::Ptr tree(new pcl::search::KdTree<pcl::PointXYZRGB>());

    pcl::IterativeClosestPointNonLinear<pcl::PointXYZRGB, pcl::PointXYZRGB> icp;
    icp.setSearchMethodTarget(tree);
    icp.setInputSource(tempCloud1);
    icp.setInputTarget(tempCloud2);

    icp.setMaximumIterations(nr_iterations);                    //迭代次数约束

    icp.setMaxCorrespondenceDistance(max_correspondence_distance); //忽略在此距离之外的点，如果两个点云距离较大，这个值要设的大一些（PCL默认距离单位是m）。
    icp.setTransformationEpsilon(1e-8);                                 //这个值一般设为1e-6或者更小
    icp.setEuclideanFitnessEpsilon(euclidean);                      //前后两次迭代误差的差值

    icp.align(*out_cloud);

    Eigen::Matrix4f transformation = icp.getFinalTransformation();
    //cout << transformation.matrix() << endl;
    //cloud->clear();
    //pcl::io::loadPLYFile(SCEarthLibrary::getInstance()->getFileParentPath(tag)+"\\1639_2.ply", *cloud);
    //out_cloud->clear();
    //pcl::transformPointCloud(*cloud, *out_cloud, transformation);

    string outfile = FileLibrary::getInstance()->getFileParentPath(file1) + "\\output_icp.ply";
    pcl::io::savePLYFile(outfile, *out_cloud);
    cout << COUT_PREFIX << "out_cloud :" << out_cloud->points.size() << endl;



}

#endif
int main(int argv, char *argc[]){

    if (argv < 3)
    {
        return -1;
    }

    string infile1 = argc[1];
    string infile2 = argc[2];

    PointCloudSplice pcs(infile1, infile2);
    pcs.parseArguments(FileLibrary::getInstance()->getFileParentPath(infile1));

    pcs.begineReadPlyFile();


    //align(infile1,infile2);
    //Eigen::AngleAxisd rotation_vector(M_PI/4, Eigen::Vector3d(0,0,1)); //沿Z轴旋转45度
    //cout.precision(6);
    //cout << COUT_PREFIX << "rotation_vector matrix = \n" <<rotation_vector.matrix()<< endl;
    //Eigen::Quaterniond q = Eigen::Quaterniond(rotation_vector);

    //cout << COUT_PREFIX << "Quaterniond = \n" << q.coeffs() << endl;

    return 0;
}