#include "depth_io.h"


depthImage::depthImage(int movelabel){
    m_movelLabel = movelabel; 
    m_passThrough = 0;
    m_patch = 6;
};
          
          
depthImage::~depthImage(){}


bool depthImage::setPiCi(int lablenum){

    
    return true;
}


/*
mainFocusPlane : 拍摄距离，到聚焦面
mainFocusLength ： 主透镜焦距
microFocusLength ： 微透镜焦距，0.79
microB ： 微透镜到sensor距离0.62
microDiameter ： 微透镜尺寸 0.191
*/
double depthImage::getDepthMicro2MainDis(double mainFocusPlane, double mainFocusLength, double microFocusLength, double microB, double microDiameter)
{

    double B = microB;
    double f = microFocusLength;
    double D = microDiameter;
    double d = 0.0;
    double u = 4.8 / 1000.0;

    d = u * B / D;
    //d *= 3;
    double micro_image_dmin = B - d;
    double micro_image_dmax = B + d;

    double micro_object_dmax = micro_image_dmin * f / (micro_image_dmin - f);
    double micro_object_dmin = micro_image_dmax * f / (micro_image_dmax - f);

    double bL0 = mainFocusPlane * mainFocusLength / (mainFocusPlane - mainFocusLength);
    double a0 = B * f / (B - f);
    double micro2main = bL0 - fabs(a0);

    return micro2main;

}



bool depthImage::depthToPlyColor(const std::string &depthImageSrc, const std::string &colorImage )    //
{
    //mixf=0.79mm  b=0.191mm            

	parseArguments(FileLibrary::getInstance()->getFileParentPath(depthImageSrc));

    //double bu = getDepthMicro2MainDis(200*10,m_focus,0.79,0.62,0.191);

	m_strFlyFileOut = depthImageSrc.substr(0, depthImageSrc.find_last_of('_')) + "_rgb.ply";
	string m_strFlyFileOutNORGB = depthImageSrc.substr(0, depthImageSrc.find_last_of('_')) + "_norgb.ply";

	std::string depthPath = depthImageSrc;
	std::string rgbPath = colorImage;
	// 定义点云类型
	typedef pcl::PointXYZRGB Point;
	typedef pcl::PointCloud<Point> PointCloud;

	// 相机内参                                                                 
    const float camera_factor = 1000; //单位
    float camera_cx = 0.0;
    float camera_cy = 0.0;

    float f = m_focus;   //焦距 单位毫米
    float F = m_fdis; //两点之间视差的像素偏移差值 单位像素
    float u = 4.8 / 1000.0; //像素点尺寸 单位米
    //float baseline =  m_disp * (4.8 / 1000.0);  //视差基线  单位米
    float baseline = fabs(0.191 / 0.79*m_focus);// //fabs(u * 2 / 0.79  * m_focus);// m_disp * (4.8 / 1000.0);  //视差基线  单位米

    f = 0.191 / 0.79 * m_focus;// 0.191 / 0.79 * m_focus;// 32.35;

    //cout << "微透镜到主透镜之间的距离 bu= " << bu << ",主透镜焦距 fu= " << m_focus << ", baseline= " << baseline <<", f = "<<f<< endl;

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
#if 1
            float l = d / 100.0;
            //float v = (m_label * m_step) * u; //
            float v = ((l - m_label / 2) * m_step + m_patch) * u ;    //视差值
            if (fabs(v) < 0.00001)
            {

                 continue;
            }

            Point p;
            // z = baseline * f / v;
            p.z = baseline * f / v;
            
            //p.z = p.z / (v * F * 1000 + baseline * f);
            //p.z = p.z / (v * F * u + baseline * f);

            p.z = p.z / camera_factor; //转成单位米
#endif

#if 0
			float l = d / 100.0;
            //float v = (m_label * m_step) * u; //
            float v = ((l - m_label / 2) * m_step) * u;    //视差值
			if (fabs(v) < 0.00001)
			{

				continue;
			}			

			Point p;
			// z = baseline * f / v;
			p.z = baseline * f * F * 1000;
            //p.z = p.z / (v * F * 1000 + baseline * f);

            p.z = p.z /  (v * F * 1000 * u + baseline * f);
			
            p.z = p.z / camera_factor; //转成单位米
#endif
            p.x = ((n - camera_cx) * p.z) * u / f;
            p.y = ((m - camera_cy) * p.z) * u / f;

			//p.x *= -1;
			p.y *= -1;
			p.z *= -1;

            p.b = rgb.ptr<uchar>(m)[n * 3];
			p.g = rgb.ptr<uchar>(m)[n * 3 + 1];
			p.r = rgb.ptr<uchar>(m)[n * 3 + 2];
			
            //过滤绿色, 从配置文件读取绿色阈值
            if (m_greenRGB != 0 && p.g - p.r>m_greenRGB && p.g - p.b>m_greenRGB)
			{
				continue;
			}

			cloud->points.push_back(p);

		
		}
	}

	// 设置并保存点云
	cloud->height = 1;
	cloud->width = cloud->points.size();
	//cloud->height = depth.rows;
	//cloud->width = depth.cols;

	cloud->is_dense = false;

    pcl::io::savePLYFile(m_strFlyFileOut, *cloud);
    cout << COUT_PREFIX << "point cloud size = " << cloud->points.size() << endl;
    
#if 0	//点云预处理

    pcl::PointCloud<pcl::PointXYZRGB>::Ptr cloud_filtered(new pcl::PointCloud<pcl::PointXYZRGB>);
    pcl::PointCloud<pcl::PointXYZRGB>::Ptr cloud_piont(new pcl::PointCloud<pcl::PointXYZRGB>);


	//统计滤波器
    pcl::StatisticalOutlierRemoval<pcl::PointXYZRGB> sor;  //创建滤波器对象
    sor.setInputCloud(cloud);                     //设置待滤波的点云
	sor.setMeanK(meank);                                //设置在进行统计时考虑查询点临近点数
	sor.setStddevMulThresh(stddevmulthresh);            //设置判断是否为离群点的阀值
    sor.filter(*cloud_piont);                        //存储
    
    //pcl::copyPoint(cloud_piont, cloud_filtered);

    cout << COUT_PREFIX << "统计滤波器 size = " << cloud_piont->points.size() << endl;
    if (cloud_piont->points.size() == 0)
    {
        return false;
    }
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
    if (cloud_piont->points.size() == 0)
    {
        cout << COUT_PREFIX << "point size :0" << endl;
        return false;
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

    cout <<COUT_PREFIX<< "save filter ok . file = " << m_strFlyFileOutNORGB << endl;
#endif



	return true;
}




bool depthImage::parseArguments(const string &configParentdir){


	//m_strTexturepng = plyfile.substr(0, plyfile.find_last_of("_"))+".png";// parentdir + "\\" + flyname.substr(0, flyname.length() - 4) + ".png";

	string configfile = FileLibrary::getInstance()->getFileParentPath(configParentdir) + "\\config.cfg";
	if (!FileLibrary::getInstance()->isFileExists(configfile))
	{
		cout << COUT_PREFIX << "config file no find. parent =" << configParentdir<<",config=" <<configfile << endl;
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
        else if (line.find("patch=") != std::string::npos)
        {
            m_patch = atoi(line.substr(pos, line.length() - pos).c_str());
        }



        


	}
	iff.close();
	cout << endl;
	return true;
}
