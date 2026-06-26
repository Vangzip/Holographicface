#include "depth_io.h"


depthImage::depthImage(int movelabel){
    m_movelLabel = movelabel; 
    m_passThrough = 0;
    
};
          
          
depthImage::~depthImage(){}
#if 0
bool depthImage::depthToPlyColor(const std::string &depthImageSrc, const std::string &colorImage,
    double disp,    //用于算基线baseline 1
    double step,    //算视差 0.03
    double label, //标签数量
    double focus,   //焦点 50mm
    double fdis)    //
{

    std::string directoryPath = FileLibrary::getInstance()->getFileParentPath(depthImageSrc)+"\\";

    std::string pcdFileOutPut =  depthImageSrc.substr(0, depthImageSrc.find_last_of('.')) + "_rgb.pcd";
    m_strFlyFileOut = depthImageSrc.substr(0, depthImageSrc.find_last_of('.')) + "_rgb.ply";
    string m_strFlyFileOutNORGB = depthImageSrc.substr(0, depthImageSrc.find_last_of('.')) + "_norgb.ply";
    m_strDepthFileOut = depthImageSrc.substr(0, depthImageSrc.find_last_of('.')) + "_depth.png";

    std::string depthPath = depthImageSrc;
    std::string rgbPath = colorImage;
    // 定义点云类型
    typedef pcl::PointXYZRGB Point;
    typedef pcl::PointCloud<Point> PointCloud;

    // 相机内参
    const double camera_factor = 1000; //单位
    double camera_cx = 0.0;
    double camera_cy = 0.0;

    double f = focus;   //焦距 单位毫米
    double F = fdis; //两点之间视差的像素偏移差值 单位像素
    double u = 4.8 / 1000.0; //像素点尺寸 单位米
    double baseline = disp * (4.8 / 1000.0);  //视差基线  单位米

    // 图像矩阵
    cv::Mat rgb, depth;
    rgb = cv::imread(rgbPath);
    depth = cv::imread(depthPath, -1);

    camera_cx = depth.cols / 2.0;  //中心点
    camera_cy = depth.rows / 2.0;  //中心点

    PointCloud::Ptr cloud(new PointCloud);

    //f = 100.0;
    //F = 4.25;
    //baseline = 25.0;
    //u = 35 / 512.0;

    //f = 0.79;
    //F = 0.005;
    //baseline = 0.198;
    //u = 4.8 / 1000.0;
    //int uvsize = depth.rows*depth.cols * 5;
    //float* uv = new float[depth.rows*depth.cols * 5];
    //memset(uv, 0, uvsize);

    // 遍历深度图
    for (int m = 0; m < depth.rows; m++)
    {
        for (int n = 0; n <depth.cols; n++)
        {
            // 获取深度图中(m,n)处的值
            ushort d = depth.ptr<ushort>(m)[n];

            double l = d / 100.0;
            //double v = (label * step) * u; //
            double v = ((l - label / 2) * step) * u;
            if (fabs(v) < 0.00001)
            {
#if 0
                //Point p;
                //p.z = 0.0;
                //p.x = 0.0;
                //p.y = 0.0;
                //p.b = rgb.ptr<uchar>(m)[n * 3];
                //p.g = rgb.ptr<uchar>(m)[n * 3 + 1];
                //p.r = rgb.ptr<uchar>(m)[n * 3 + 2];

                //cloud->points.push_back(p);

                ////uv坐标
                //int index = m * depth.cols + n;
                //uv[index++] = p.x;
                //uv[index++] = p.y;
                //uv[index++] = p.z;
                //uv[index++] = m;
                //uv[index++] = n;

#endif
                continue;
            }


            /*
            
            The underlying equation that performs this conversion is:
            Z = fB/d

            where
            Z = distance along the camera Z axis
            f = focal length (in pixels)
            B = baseline (in metres)
            d = disparity (in pixels)

            X = u*Z/f
            Y = v*Z/f

            */

            Point p;
            // z = baseline * f / v;
            p.z = baseline * f * F * 1000;
            p.z = p.z / (v * F * 1000 * u + baseline * f);
            p.z = p.z / camera_factor; //转成单位米

            p.x = ((n - camera_cx) * p.z) * u / f;
            p.y = ((m - camera_cy) * p.z) * u / f;

            //p.x *= -1;
            p.y *= -1;
            p.z *= -1;
            /*Point p;
            p.z = d;

            p.x = ((n - camera_cx) * p.z) / 500;
            p.y = ((m - camera_cy) * p.z) / 500;*/


            p.b = rgb.ptr<uchar>(m)[n * 3];
            p.g = rgb.ptr<uchar>(m)[n * 3 + 1];
            p.r = rgb.ptr<uchar>(m)[n * 3 + 2];
            //过滤绿色
            //if (p.g - p.r>30 && p.g - p.b>30)
            //if (p.g - p.r>5 && p.g - p.b>5)
            if (p.g - p.r>10 && p.g - p.b>10)

            { 
                continue;
            }
            cloud->points.push_back(p);

            ////uv坐标
            //int index = m * depth.cols + n;
            //uv[index++] = p.x;
            //uv[index++] = p.y;
            //uv[index++] = p.z;
            //uv[index++] = m;
            //uv[index++] = n;
        }
    }

    // 设置并保存点云
    cloud->height = 1;
    cloud->width = cloud->points.size();
    //cloud->height = depth.rows;
    //cloud->width = depth.cols;

    cout << "point cloud size = " << cloud->points.size() << endl;
    cloud->is_dense = false;
    //pcl::io::savePCDFile(pcdFileOutPut, *cloud);
    pcl::io::savePLYFile(m_strFlyFileOut, *cloud);    
#if 1
    //点云滤波处理.
    string config = FileLibrary::getInstance()->getFileParentPath(FileLibrary::getInstance()->getFileParentPath(m_strFlyFileOut)) + "\\config.cfg";
    if (!FileLibrary::getInstance()->isFileExists(config))
    {
        cout <<COUT_PREFIX<<"no find config . file = "<<config << endl;
        return 0;
    }
    ifstream iff(config);
    std::string line;



    while (getline(iff,line))
    {
        if (line.empty())
        {
            continue;
        }
        int pos = line.find("=") + 1;
        if (line.find("meanK=") != std::string::npos)
        {
            meank = atoi(line.substr(pos, line.length() - pos).c_str());
        }
        else if (line.find("stddevMulThresh=") != std::string::npos)
        {
            stddevmulthresh = atof(line.substr(pos, line.length() - pos).c_str());
        }if (line.find("radiussearch=") != std::string::npos)
        {
            radius = atof(line.substr(pos, line.length() - pos).c_str());
        }if (line.find("minNeighborInRadius=") != std::string::npos)
        {
            minNeighborsInRadius = atoi(line.substr(pos, line.length() - pos).c_str());
        }
    }

    iff.close();

    pcl::PointCloud<pcl::PointXYZ>::Ptr cloud_filtered(new pcl::PointCloud<pcl::PointXYZ>);
    pcl::PointCloud<pcl::PointXYZ>::Ptr cloud_piont(new pcl::PointCloud<pcl::PointXYZ>);
    cloud_piont->resize(cloud->points.size());

    for (size_t i = 0; i < cloud->points.size(); i++)
    {
        cloud_piont->points[i].x = cloud->points[i].x;
        cloud_piont->points[i].y = cloud->points[i].y;
        cloud_piont->points[i].z = cloud->points[i].z;
    }

    //统计滤波器
    pcl::StatisticalOutlierRemoval<pcl::PointXYZ> sor;  //创建滤波器对象
    sor.setInputCloud(cloud_piont);                     //设置待滤波的点云
    sor.setMeanK(meank);                                //设置在进行统计时考虑查询点临近点数
    sor.setStddevMulThresh(stddevmulthresh);            //设置判断是否为离群点的阀值
    sor.filter(*cloud_filtered);                        //存储
    
    cout << "统计滤波器 size = " << cloud_filtered->points.size() << endl;
    //直通滤波器
    //// Create the filtering object  
    //pcl::PassThrough<pcl::PointXYZ> pass;
    //pass.setInputCloud(cloud);
    //pass.setFilterFieldName("z");
    //pass.setFilterLimits(-0.5, 0);
    ////pass.setFilterLimitsNegative (true);  
    //pass.filter(*cloud_filtered);

    //体素滤波器
    //pcl::PointCloud<pcl::PointXYZ>::Ptr cloud_filtered2(new pcl::PointCloud<pcl::PointXYZ>);

    //pcl::ApproximateVoxelGrid<pcl::PointXYZ> avg;
    //avg.setInputCloud(cloud_filtered);
    //avg.setLeafSize(0.01, 0.01, 0.01);
    //avg.setDownsampleAllData(true);
    //avg.filter(*cloud_filtered2);
    //
    //cout << "体素滤波器 size = " << cloud_filtered2->points.size() << endl;


    // 半径过滤器
    pcl::PointCloud<pcl::PointXYZ>::Ptr RadiusOutlierRemoval_cloud_filtered(new pcl::PointCloud<pcl::PointXYZ>);
    pcl::RadiusOutlierRemoval<pcl::PointXYZ> outrem;    
    outrem.setInputCloud(cloud_filtered);
    outrem.setRadiusSearch(radius); // 0.00005 
    outrem.setMinNeighborsInRadius(minNeighborsInRadius);        //3 
    
    outrem.filter(*RadiusOutlierRemoval_cloud_filtered);
    
    cout << "半径过滤器 size = " << RadiusOutlierRemoval_cloud_filtered->points.size() << endl;

    //string filter_ply = m_strFlyFileOut.substr(0,m_strFlyFileOut.length() - 4)+ "_filter.ply";
    pcl::io::savePLYFile(m_strFlyFileOutNORGB, *RadiusOutlierRemoval_cloud_filtered);
#endif
    

    cout << "save filter ok . file = " << m_strFlyFileOut << endl;

    return true;
}

#endif


bool depthImage::depthToPlyColor(const std::string &depthImageSrc, const std::string &colorImage, const string &config, const string &outputDir
	//double disp,    //用于算基线baseline 1
	//double step,    //算视差 0.03
	//double label, //标签数量
	//double focus,   //焦点 50mm
	//double fdis
	)    //
{

    parseArguments(config);

	string baseName = FileLibrary::getInstance()->getFileNameFromPath(depthImageSrc);
	size_t extPos = baseName.find_last_of('.');
	if (extPos != string::npos)
	{
		baseName = baseName.substr(0, extPos);
	}

	string outputRoot = outputDir.empty() ? FileLibrary::getInstance()->getFileParentPath(depthImageSrc) : outputDir;
	m_strFlyFileOut = FileLibrary::getInstance()->combineFilePath(outputRoot, baseName + "_rgb.ply");
	string m_strFlyFileOutNORGB = FileLibrary::getInstance()->combineFilePath(outputRoot, baseName + "_norgb.ply");

	std::string depthPath = depthImageSrc;
	std::string rgbPath = colorImage;
	// 定义点云类型
	typedef pcl::PointXYZRGB Point;
	typedef pcl::PointCloud<Point> PointCloud;

	// 相机内参
	const double camera_factor = 1000; //单位
	double camera_cx = 0.0;
	double camera_cy = 0.0;

	double f = m_focus;   //焦距 单位毫米
	double F = m_fdis; //两点之间视差的像素偏移差值 单位像素
	double u = 4.8 / 1000.0; //像素点尺寸 单位米
	double baseline = m_disp * (4.8 / 1000.0);  //视差基线  单位米

	// 图像矩阵
	cv::Mat rgb, depth;
	rgb = cv::imread(rgbPath);
	depth = cv::imread(depthPath, -1);

	camera_cx = depth.cols / 2.0;  //中心点
	camera_cy = depth.rows / 2.0;  //中心点

	PointCloud::Ptr cloud(new PointCloud);
	// 遍历深度图
	for (int m = 0; m < depth.rows; m++)
	{
		for (int n = 0; n <depth.cols; n++)
		{
			// 获取深度图中(m,n)处的值
			ushort d = depth.ptr<ushort>(m)[n];

			double l = d / 100.0;
			//double v = (label * step) * u; //
			double v = ((l - m_label / 2) * m_step) * u;    //视差值
			if (fabs(v) < 0.00001)
			{

				continue;
			}			

			Point p;
#if 0
            p.z = double(d) / camera_factor;
            p.x = (n - camera_cx) * p.z / m_focus;
            p.y = (m - camera_cy) * p.z / m_focus;
#endif
			// z = baseline * f / v;
			p.z = baseline * f * F * 1000;
			p.z = p.z / (v * F * 1000 * u + baseline * f);
            // p.z = p.z / (v * F * 1000 + baseline * f);
			p.z = p.z / camera_factor; //转成单位米

			p.x = ((n - camera_cx) * p.z) * u / f;
			p.y = ((m - camera_cy) * p.z) * u / f;

			//p.x *= -1;
			p.y *= -1;
			p.z *= -1;
			
			p.b = rgb.ptr<uchar>(m)[n * 3];
			p.g = rgb.ptr<uchar>(m)[n * 3 + 1];
			p.r = rgb.ptr<uchar>(m)[n * 3 + 2];

			cloud->points.push_back(p);

		
		}
	}

	// 设置并保存点云
	cloud->height = 1;
	cloud->width = cloud->points.size();
	//cloud->height = depth.rows;
	//cloud->width = depth.cols;

    cout << COUT_PREFIX << "point cloud size = " << cloud->points.size() << endl;
	cloud->is_dense = false;
	//pcl::io::savePCDFile(pcdFileOutPut, *cloud);
	pcl::io::savePLYFile(m_strFlyFileOut, *cloud);

#if 0	

    pcl::PointCloud<pcl::PointXYZRGB>::Ptr cloud_filtered(new pcl::PointCloud<pcl::PointXYZRGB>);
    pcl::PointCloud<pcl::PointXYZRGB>::Ptr cloud_piont(new pcl::PointCloud<pcl::PointXYZRGB>);


	//统计滤波器
    if (meank != 0)
    {
        pcl::StatisticalOutlierRemoval<pcl::PointXYZRGB> sor;  //创建滤波器对象
        sor.setInputCloud(cloud);                     //设置待滤波的点云
	    sor.setMeanK(meank);                                //设置在进行统计时考虑查询点临近点数
	    sor.setStddevMulThresh(stddevmulthresh);            //设置判断是否为离群点的阀值
        sor.filter(*cloud_piont);                       //存储
        cout << COUT_PREFIX << "统计滤波器 size = " << cloud_piont->points.size() << endl;
        if (cloud_piont->points.size() == 0)
        {
            return false;
        }

    }
    else
    {

        pcl::copyPoint(cloud, cloud_piont);
    }
    
    //pcl::copyPoint(cloud_piont, cloud_filtered);

	//直通滤波器
	// Create the filtering object  
    if (m_passThrough == 1)
    {
        pcl::PointCloud<pcl::PointXYZRGB>::Ptr tmpPointCloud(new pcl::PointCloud<pcl::PointXYZRGB>);

        pcl::PassThrough<pcl::PointXYZRGB> pass;
        pass.setInputCloud(cloud_piont);
	    pass.setFilterFieldName("z");
        if (m_movelLabel == 0)
        {
            pass.setFilterLimits(mina, minb);                   //0.000115

        }
        else
        {
            pass.setFilterLimits(mina - (0.00011*m_movelLabel++), minb);                   //0.000115
        }
	    //pass.setFilterLimitsNegative (true);  
        pass.filter(*tmpPointCloud);

        cout << COUT_PREFIX << "直通滤波器 size = " << tmpPointCloud->points.size() << endl;
        cloud_piont->clear();
        cloud_piont->resize(tmpPointCloud->points.size());
        pcl::copyPoint(tmpPointCloud,cloud_piont);

    }
   

	//体素滤波器
	//pcl::PointCloud<pcl::PointXYZ>::Ptr cloud_filtered2(new pcl::PointCloud<pcl::PointXYZ>);

	//pcl::ApproximateVoxelGrid<pcl::PointXYZ> avg;
	//avg.setInputCloud(cloud_filtered);
	//avg.setLeafSize(0.01, 0.01, 0.01);
	//avg.setDownsampleAllData(true);
	//avg.filter(*cloud_filtered2);
	//
	//cout << "体素滤波器 size = " << cloud_filtered2->points.size() << endl;


	// 半径过滤器
    if (radius != 0.0)
    {
        pcl::PointCloud<pcl::PointXYZRGB>::Ptr RadiusOutlierRemoval_cloud_filtered(new pcl::PointCloud<pcl::PointXYZRGB>);
        pcl::RadiusOutlierRemoval<pcl::PointXYZRGB> outrem;
       
        outrem.setInputCloud(cloud_piont);
	    outrem.setRadiusSearch(radius); // 0.00005 
	    outrem.setMinNeighborsInRadius(minNeighborsInRadius);        //3 
    
	    outrem.filter(*RadiusOutlierRemoval_cloud_filtered);

        cout << COUT_PREFIX << "半径过滤器 size = " << RadiusOutlierRemoval_cloud_filtered->points.size() << endl;
        if (RadiusOutlierRemoval_cloud_filtered->points.size() == 0)
        {
            return false;
        }
        //cloud_piont->clear();
        //cloud_piont->resize(RadiusOutlierRemoval_cloud_filtered->points.size());
        cloud_piont = RadiusOutlierRemoval_cloud_filtered;
        //pcl::copyPoint(RadiusOutlierRemoval_cloud_filtered,cloud_piont);
    }

	//string filter_ply = m_strFlyFileOut.substr(0,m_strFlyFileOut.length() - 4)+ "_filter.ply";
    pcl::io::savePLYFile(m_strFlyFileOutNORGB, *cloud_piont);

    //Eigen::Matrix< float, 4, 1 > 	centroid;
    //pcl::compute3DCentroid(*cloud_piont, centroid);

    //cout.precision(6);
    ////cout << COUT_PREFIX << "x:" << centroid.x() << "    Y:" << centroid.y() << "    Z:" << centroid.z() << endl;


    //Eigen::Affine3f tMatrix = Eigen::Affine3f::Identity();


    ////pcl::getTransformation(0, 0, 0, 0, 0, 0, tMatrix);

    //tMatrix.translation() << centroid.x() * -1, centroid.y() * -1, centroid.z() * -1;

    //pcl::PointCloud<pcl::PointXYZRGB>::Ptr out_pointcloud(new pcl::PointCloud<pcl::PointXYZRGB>);

    //pcl::transformPointCloud(*cloud_piont, *out_pointcloud, tMatrix);  

    //pcl::io::savePLYFile(m_strFlyFileOutNORGB, *out_pointcloud);

    
#endif


    cout << COUT_PREFIX << "save filter ok . file = " << m_strFlyFileOut << endl;

	return true;
}
bool depthImage::depthToPcdCloreFileCorrectOrientation(const std::string &depthImageSrc, const std::string &colorImage)
{
    std::string pcdFileOutPut =  depthImageSrc.substr(0, depthImageSrc.find_last_of('.')) + "_rgb.pcd";
    std::string plyFileOutPut = depthImageSrc.substr(0, depthImageSrc.find_last_of('.')) + "_rgb.ply";
    std::string depthPath =  depthImageSrc;
    std::string rgbPath =  colorImage;
    // 定义点云类型
    typedef pcl::PointXYZRGB Point;
    typedef pcl::PointCloud<Point> PointCloud;

    // 相机内参
    const double camera_factor = 1000;
    //const double camera_cx = 325.5;   //相机的光圈中心
    //const double camera_cy = 253.5;
    //const double camera_fx = 500;   //相机在两个轴上的焦距， 影响z 方向的深度 大小， 越大 深度距离越大。
    //const double camera_fy = 500;

    /*const double camera_cx = 253;
    const double camera_cy = 184.5;
    const double camera_fx = 1157.407407407407;
    const double camera_fy = 1157.407407407407;*/


    /*const double camera_fx = 50000.0 / (4.8 * 7.23);
    const double camera_fy = 50000.0 / (4.8 * 7.23);*/
    const double camera_fx = 1500;   //相机在两个轴上的焦距， 影响z 方向的深度 大小， 越大 深度距离越大。
    const double camera_fy = 1500;


    // 读取./data/rgb.png和./data/depth.png，并转化为点云

    // 图像矩阵
    cv::Mat rgb, depth;
    // 使用cv::imread()来读取图像
    rgb = cv::imread(rgbPath);
    // rgb 图像是8UC3的彩色图像
    // depth 是16UC1的单通道图像，注意flags设置-1,表示读取原始数据不做任何修改
    depth = cv::imread(depthPath, -1);
    double camera_cx = depth.cols / 2.0;
    double camera_cy = depth.rows / 2.0;

    // 点云变量
    // 使用智能指针，创建一个空点云。这种指针用完会自动释放。
    PointCloud::Ptr cloud(new PointCloud);

    // 遍历深度图
    for (int m = 0; m < depth.rows; m++)
    for (int n = 0; n <depth.cols; n++)
    {
        // 获取深度图中(m,n)处的值
        ushort d = depth.ptr<ushort>(m)[n];
        //uchar d = depth.ptr<uchar>(m)[n];
        // d 可能没有值，若如此，跳过此点

        //d = d >> 8;

        if (d == 0)
            continue;
        // d 存在值，则向点云增加一个点
        Point p;

        // 计算这个点的空间坐标
        p.z = double(d) / camera_factor;
        p.z *= -1;

        //调整x方向 
        int j = depth.cols - n - 1;
        p.x = (j - camera_cx) * p.z / camera_fx;
        //p.x = (n - camera_cx) * p.z / camera_fx;
        //p.x = (n - camera_cx) * p.z / camera_fx;
        //p.x *= -1;

        //调整y方向
        //p.y = (m - camera_cy) * p.z / camera_fy;
        int k = depth.rows - m - 1;
        p.y = (k - camera_cy) * p.z / camera_fy;
        //p.y = (m - camera_cy) * p.z / camera_fx;

        //调整z 方向


        // 从rgb图像中获取它的颜色
        // rgb是三通道的BGR格式图，所以按下面的顺序获取颜色
        p.b = rgb.ptr<uchar>(m)[n * 3];
        p.g = rgb.ptr<uchar>(m)[n * 3 + 1];
        p.r = rgb.ptr<uchar>(m)[n * 3 + 2];
        if (p.g - p.r>30 && p.g - p.b>30)
        {
            continue;
        }

        p.y *= -1;
        p.z *= -1;


        // 把p加入到点云中
        cloud->points.push_back(p);
    }
    // 设置并保存点云
    //cloud->height = depth.rows;
    //cloud->width = depth.cols;//cloud->points.size();

    cloud->height = 1;
    cloud->width = cloud->points.size();

    cout << "point cloud size = " << cloud->points.size() << endl;
    cloud->is_dense = false;

    //pcl::io::savePCDFile(pcdFileOutPut, *cloud);
    pcl::io::savePLYFile(plyFileOutPut, *cloud);


    m_strFlyFileOut = plyFileOutPut;
    //showPointCloudRGB(cloud);

    cloud->clear();
    cout << "Point cloud saved." << endl;
    return 0;
}




bool depthImage::parseArguments(const string &configfile){


	//m_strTexturepng = plyfile.substr(0, plyfile.find_last_of("_"))+".png";// parentdir + "\\" + flyname.substr(0, flyname.length() - 4) + ".png";

  
	if (!FileLibrary::getInstance()->isFileExists(configfile))
	{
        cout << COUT_PREFIX << "config=" <<configfile << endl;
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
		if (line.find("disp=") != string::npos)
		{		

			m_disp = atof(line.substr(pos, line.length() - pos).c_str());
			
		}else if (line.find("step=") != string::npos)
		{
			m_step = atof(line.substr(pos, line.length() - pos).c_str());
			
		}
		else if (line.find("label=") != string::npos)
		{
			m_label = atof(line.substr(pos, line.length() - pos).c_str());

		}
		else if (line.find("focus=") != string::npos)
		{
			m_focus = atof(line.substr(pos, line.length() - pos).c_str());

		}
		else if (line.find("greenRGB=") != string::npos)
		{
			m_greenRGB = atoi(line.substr(pos, line.length() - pos).c_str());

		}
		else if (line.find("fdis=") != string::npos)
		{
			m_fdis = atof(line.substr(pos, line.length() - pos).c_str());

		}else if (line.find("meanK=") != std::string::npos)
		{
			meank = atoi(line.substr(pos, line.length() - pos).c_str());
		}
		else if (line.find("stddevMulThresh=") != std::string::npos)
		{
			stddevmulthresh = atof(line.substr(pos, line.length() - pos).c_str());
		}
		else if(line.find("radiussearch=") != std::string::npos)
		{
			radius = atof(line.substr(pos, line.length() - pos).c_str());
		}
		else if(line.find("minNeighborInRadius=") != std::string::npos)
		{
			minNeighborsInRadius = atof(line.substr(pos, line.length() - pos).c_str());
        }
        else if (line.find("min=") != std::string::npos)
        {
            mina = atof(line.substr(pos, line.length() - pos).c_str());
        }
        else if (line.find("max=") != std::string::npos)
        {
            minb = atof(line.substr(pos, line.length() - pos).c_str());
        }
        else if (line.find("passThrough=") != std::string::npos)
        {
            m_passThrough = atoi(line.substr(pos, line.length() - pos).c_str());
        }
	}
	iff.close();
	cout << endl;
	return true;
}
