// testreadpcd.cpp : 定义控制台应用程序的入口点。
//



#include "base.h"  
#include "FileLibrary.h"    
#include "depth_io.h"
#include "ConverPointCloud.h"

//深度图生成点云
void pointcloudFunc(const string &dir, int label){
    depthImage depth(label);

    list<string>listfile;
    FileLibrary::getInstance()->getAllSubFiles(dir, listfile, false, true, false, ".png");
    list<string>::iterator it = listfile.begin();

    for (it; it != listfile.end(); it++){
        string filepath = *it;
        string filename = FileLibrary::getInstance()->getFileNameFromPath(filepath);
        if (filename.find("_rgb") != string::npos)
        {
            continue;
        }


        if (!FileLibrary::getInstance()->isFileExists(filepath))
        {
            cout << "fly no find. file =" << filepath << endl;
            return ;
        }

        string depthimage = filepath;
        string rgb_png = depthimage.substr(0, depthimage.find_last_of("_")) + "_rgb.png";
        if (!FileLibrary::getInstance()->isFileExists(rgb_png))
        {
            cout << COUT_PREFIX << "not find file = " << rgb_png << endl;
            continue;
        }

        depth.depthToPlyColor(depthimage, rgb_png);
    }

};

//点云生成mesh
void meshFunc(const string &dir){
    ConverPointCloud *convertomesh = new ConverPointCloud();

    list<string>listfile;
    int num = 1;
    string plyfile1, plyfile2;
    FileLibrary::getInstance()->getAllSubFiles(dir, listfile, false, true, false, "_rgb.ply");
    list<string>::iterator it = listfile.begin();
    for (it; it != listfile.end(); it++, num++){
        string filepath = *it;

        cout << "num = " << num << ", file = " << FileLibrary::getInstance()->getFileNameFromPath(filepath) << endl;

        convertomesh->meshAPI(filepath);
    }
}


//mesh生成模型obj
void modelFunc(const string &dir){
    ConverPointCloud *convertomesh = new ConverPointCloud();
    list<string>listfile;
    FileLibrary::getInstance()->getAllSubFiles(dir, listfile, false, true, false, "_mesh.ply");
    list<string>::iterator it = listfile.begin();
    int num = 1;
    for (it; it != listfile.end(); it++, num++){
        string filepath = *it;

        cout << "num = " << num << ", file = " << FileLibrary::getInstance()->getFileNameFromPath(filepath) << endl;
        if (!FileLibrary::getInstance()->isFileExists(filepath))
        {
            cout << "fly no find. file =" << filepath << endl;
            return;
        }

        convertomesh->modelAPI(filepath);
    }

    delete convertomesh;

}



#if  1
int main(int argc, char *argv[]){


  /*  list<string>listfile;
    FileLibrary::getInstance()->getAllSubFiles("C:\\Program Files\\PCL 1.8.0\\lib",listfile, false, true, false, "release.lib");
    list<string>::iterator it = listfile.begin();

    for (it; it != listfile.end(); it++){
        string filepath = *it;
        string filename = FileLibrary::getInstance()->getFileNameFromPath(filepath);

        cout << filename << endl;

    }

    return 0;*/

    string point, mesh, model, dir;

    if (argc >= 5)
    {
        point = argv[1];
        mesh = argv[2];
        model = argv[3];
        dir = argv[4];

    }
    else
    {
        cout << "error." << endl;
        return -1;
    }
    cout << "pointcloud status: " << point << endl;
    cout << "mesh status: " << mesh << endl;
    cout << "model status: " << model << endl;

    if (point == "1")
    {
        pointcloudFunc(dir, 0);
    }

    if (mesh == "1")
    {

        meshFunc(dir);

    }

    if (model == "1"){

        modelFunc(dir);
    }


    return 0;
}
#endif


#if 0               //一致性采样
//SAC 
double epsilon_sac = 0.1; // 10cm 
int iter_sac = 10000;
pcl::registration::CorrespondenceRejectorSampleConsensus<pcl::PointXYZ> sac;
//pcl::registration::corres 
sac.setInputCloud(cloud_src);
sac.setTargetCloud(cloud_tgt);
sac.setInlierThreshold(epsilon_sac);
sac.setMaxIterations(iter_sac);
sac.setInputCorrespondences(cor_all_ptr);

//配准
pcl::SampleConsensusPrerejective<PointNT,PointNT,FeatureT> align;
align.setInputSource (object);
align.setSourceFeatures (object_features);
align.setInputTarget(scene);
align.setTargetFeatures(scene_features);
align.setMaximumIterations(50000); // Number of RANSAC iterations
align.setNumberOfSamples(3); // Number of points to sample for generating/prerejecting a pose
align.setCorrespondenceRandomness(5); // Number of nearest features to use
align.setSimilarityThreshold(0.9f); // Polygonal edge length similarity threshold
align.setMaxCorrespondenceDistance(2.5f * leaf); // Inlier threshold
align.setInlierFraction(0.25f); // Required inlier fraction for accepting a pose hypothesis
{
    pcl::ScopeTime t("Alignment");
    align.align(*object_aligned);
}



//vtkIterativeClosestPointTransform
https://www.vtk.org/Wiki/VTK/Examples/Cxx/Filtering/IterativeClosestPointsTransform

//随机一致性采样
pcl::PointCloud<pcl::PointXYZ>::Ptr cloud(new pcl::PointCloud<pcl::PointXYZ>);
pcl::PointCloud<pcl::PointXYZ>::Ptr final(new pcl::PointCloud<pcl::PointXYZ>);

pcl::SampleConsensusModelSphere<pcl::PointXYZ>::Ptr
model_s(new pcl::SampleConsensusModelSphere<pcl::PointXYZ>(cloud));

pcl::RandomSampleConsensus<pcl::PointXYZ> ransac(model_s);
ransac.setDistanceThreshold(.01);
ransac.computeModel();
ransac.getInliers(inliers);

pcl::copyPointCloud<pcl::PointXYZ>(*cloud, inliers, *final);

#endif

