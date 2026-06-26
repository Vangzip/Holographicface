#include "filter.h"

//************************************
// Method:    sor
// Access:    public 
// Returns:   void
// Describe:  统计滤波
// Parameter: PointTRGBNPtr src 输入点云
// Parameter: PointTRGBNPtr & tgt 输出点云
// Parameter: float neighbors 统计临近点数量
// Parameter: float threshold 离群点阈值
// Parameter: float itnum  迭代次数
//************************************
void point_base::sor(PointTRGBNPtr src, PointTRGBNPtr &tgt, float neighbors, float threshold, float itnum){

    cout << COUT_PREFIX << "src point: " << src->points.size() << endl;


    pcl::StatisticalOutlierRemoval<pcl::PointXYZRGBNormal> sor;  //创建滤波器对象
    for (size_t i = 0; i < itnum; i++)
    {

        sor.setInputCloud(src);		//设置待滤波的点云
        sor.setMeanK(neighbors);	//设置在进行统计时考虑查询点临近点数
        sor.setStddevMulThresh(threshold);	//设置判断是否为离群点的阀值
        sor.filter(*src);					//存储
    }

    pcl::copyPoint(src, tgt);
    
    cout << COUT_PREFIX << "sor tgt point: " << src->points.size() << endl;
    
}

//************************************
// Method:    ror
// Access:    public 
// Returns:   void
// Describe:  半径过滤
// Parameter: PointTRGBNPtr src 输入点云
// Parameter: PointTRGBNPtr & tgt 输出点云
// Parameter: float radius  半径
// Parameter: float threshold 阈值
// Parameter: float iternum 迭代次数
//************************************
void point_base::ror(PointTRGBNPtr src, PointTRGBNPtr &tgt, float radius, float threshold, float iternum){

    cout << COUT_PREFIX << "src point: " << src->points.size() << endl;

    pcl::RadiusOutlierRemoval<pcl::PointXYZRGBNormal> outrem;
    for (size_t i = 0; i < iternum; i++)
    {
        outrem.setInputCloud(src);
        outrem.setRadiusSearch(radius); // 0.00005 
        outrem.setMinNeighborsInRadius(threshold);        //3 
        outrem.filter(*src);

    }
    pcl::copyPoint(src, tgt);

    //outfile = outfile.substr(0, outfile.find_last_of(".")) + "_ror.ply";
    cout << COUT_PREFIX << "ror tgt point: " << tgt->points.size() << endl;
    //pcl::io::savePLYFile(outfile, *src_cloud);

}

//************************************
// Method:    vox
// Access:    public 
// Returns:   void
// Describe:  体素网格滤波
// Parameter: PointTRGBNPtr src 输入点云
// Parameter: PointTRGBNPtr & tgt 输出点云
// Parameter: float vox_size 网格半径大小
//************************************
void point_base::vox(PointTRGBNPtr src, PointTRGBNPtr &tgt, float vox_size)
{

    cout << COUT_PREFIX << "src point: " << src->points.size() << endl;

    //tgt->resize(src->points.size());
    pcl::VoxelGrid<pcl::PointXYZRGBNormal> vox_grid;
    vox_grid.setLeafSize(vox_size, vox_size, vox_size);
    vox_grid.setInputCloud(src);
    vox_grid.filter(*tgt);

    cout << COUT_PREFIX << "vox tgt point: " << tgt->points.size() << endl;
}

//************************************
// Method:    mls
// Access:    public 
// Returns:   void
// Describe:  最小二乘，平滑点云
// Parameter: PointTRGBNPtr src 输入点云
// Parameter: PointTRGBNPtr & tgt 输出点云
// Parameter: float radius  采样半径
// Parameter: float iterNum 迭代次数
// Parameter: float mlsType 采样类型
//************************************
void point_base::mls(PointTRGBNPtr src, PointTRGBNPtr &tgt, float radius, float iterNum, float mlsType)
{
    cout << COUT_PREFIX << "src point: " << src->points.size() << endl;

    for (size_t i = 0; i < iterNum; i++)
    {

        pcl::search::KdTree<pcl::PointXYZRGBNormal>::Ptr tree(new pcl::search::KdTree<pcl::PointXYZRGBNormal>);
        tree->setInputCloud(src);


        pcl::MovingLeastSquares<pcl::PointXYZRGBNormal, pcl::PointXYZRGBNormal> mls;
        // Set parameters
        mls.setInputCloud(src);
        //mls.setPolynomialFit(true); //对于法线的估计是有多项式还是仅仅依靠切线
        mls.setComputeNormals(false);
        mls.setSearchMethod(tree);
        mls.setPointDensity(30);
        mls.setPolynomialOrder(4);
        mls.setSearchRadius(radius); // 确定搜索的半径,在这个半径里进行表面映射和曲面拟合。从实验结果可知：半径越小拟合后曲面的失真度越小，反之有可能出现过拟合的现象
        mls.setUpsamplingMethod(mls.NONE);
        //mlsType = 4;
        if (mlsType == 0)
        {
            mls.setUpsamplingMethod(mls.NONE);

        }
        else if (mlsType == 2)
        {
            mls.setUpsamplingMethod(mls.SAMPLE_LOCAL_PLANE);// 这个方法就是参考论文中采用的方法，当然此方法所需的计算强度也相当庞大。若使用此方法，将需要调用两个函数：
            mls.setUpsamplingRadius(0.01);//此函数规定了点云增长的区域。可以这样理解：把整个点云按照此半径划分成若干个子点云，然后一一索引进行点云增长。           0.1
            mls.setUpsamplingStepSize(0.1);//对于每个子点云处理时迭代的步长。

        }
        else if (mlsType == 3)
        {
            mls.setUpsamplingMethod(mls.RANDOM_UNIFORM_DENSITY);   //也是使用上面子点云的原理，只不过它使得稀疏区域的密度增加，从而使得整个点云的密度均匀
            mls.setPointDensity(100);  //注意此函数输入整型变量，意为半径内点的个数。（这个半径应该是search的半径，不需要重新设置）。

        }
        else if (mlsType == 4)
        {

            mls.setUpsamplingMethod(mls.VOXEL_GRID_DILATION); //这个方法有两个步骤：首先将点云以voxels分割，然后进行迭代使得voxels的数目增加。它的结果是：填充空洞和平均化点云的密度。
            mls.setDilationVoxelSize(0.01);   //设定voxel的大小。
            mls.setDilationIterations(10); //设置迭代的次数
        }

        mls.process(*tgt);
                
        src->resize(tgt->points.size());        
        pcl::copyPointCloud(*tgt, *src);

    }

    //outfile = outfile.substr(0, outfile.find_last_of(".")) + "_mls.ply";
    cout << COUT_PREFIX << "mls tgt point: " << tgt->points.size() << endl;

}

//************************************
// Method:    rg
// Access:    public 
// Returns:   void
// Describe:  区域增长算法
// Parameter: PointTRGBNPtr src 输入点云
// Parameter: PointTRGBNPtr & tgt 输出点云
//************************************
void point_base::rg(PointTRGBPtr src, PointTRGBPtr &tgt){


    cout << COUT_PREFIX << "src point: " << src->points.size() << endl;

    pcl::search::Search<pcl::PointXYZRGB>::Ptr normaltree(new pcl::search::KdTree<pcl::PointXYZRGB>);
    pcl::PointCloud <pcl::Normal>::Ptr normals(new pcl::PointCloud <pcl::Normal>);
    pcl::NormalEstimation<pcl::PointXYZRGB, pcl::Normal> normal_estimator;
    normal_estimator.setSearchMethod(normaltree);
    normal_estimator.setInputCloud(src);
    normal_estimator.setKSearch(50);
    normal_estimator.compute(*normals);

    pcl::RegionGrowing<pcl::PointXYZRGB, pcl::Normal> reg;
    reg.setMinClusterSize(10);  //最小聚类点
    reg.setMaxClusterSize(src->points.size()); //最大聚类点
    reg.setSearchMethod(normaltree); //搜索方式
    reg.setNumberOfNeighbours(30); //设置搜索临近点个数
    reg.setInputCloud(src);
    reg.setInputNormals(normals); //法线
    reg.setSmoothnessThreshold(3.0 / 180.0 * M_PI);//设置平滑度
    reg.setCurvatureThreshold(1.0); //设置曲率阈值
    std::vector<pcl::PointIndices> cluster_indices;
    reg.extract(cluster_indices); //聚类索引几何

    std::cout << "clusternum = " << cluster_indices.size() << std::endl;

    pcl::PointCloud<pcl::PointXYZRGB>::Ptr cloud_cluster(new pcl::PointCloud<pcl::PointXYZRGB>);
    cloud_cluster = reg.getColoredCloud();

#if 0
    pcl::PointCloud<pcl::PointXYZRGB>::Ptr cloud_cluster(new pcl::PointCloud<pcl::PointXYZRGB>);
    int j = 0;
    for (std::vector<pcl::PointIndices>::const_iterator it = cluster_indices.begin(); it != cluster_indices.end(); ++it)
    {
        //if (j == 0)
        {
            for (std::vector<int>::const_iterator pit = it->indices.begin(); pit != it->indices.end(); ++pit)
            {
                cloud_cluster->points.push_back(cloud->points[*pit]);
            }
        }
        j++;
    }
    cloud_cluster->width = cloud_cluster->points.size();
    cloud_cluster->height = 1;
    cloud_cluster->is_dense = true;

#endif

    cout << COUT_PREFIX << "tgt point: " << cloud_cluster->points.size() << endl;
    //outfile = outfile.substr(0, outfile.find_last_of(".")) + "_rg.ply";
    tgt = cloud_cluster;

}