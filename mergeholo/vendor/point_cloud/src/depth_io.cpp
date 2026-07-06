#include "depth_io.h"
#include <cmath>


depthImage::depthImage(int movelabel){
    m_movelLabel = movelabel;
    m_passThrough = 0;

};


depthImage::~depthImage(){}
#if 0
bool depthImage::depthToPlyColor(const std::string &depthImageSrc, const std::string &colorImage,
    double disp,    //鐢ㄤ簬绠楀熀绾縝aseline 1
    double step,    //绠楄宸?0.03
    double label, //鏍囩鏁伴噺
    double focus,   //鐒︾偣 50mm
    double fdis)    //
{

    std::string directoryPath = FileLibrary::getInstance()->getFileParentPath(depthImageSrc)+"\\";

    std::string pcdFileOutPut =  depthImageSrc.substr(0, depthImageSrc.find_last_of('.')) + "_rgb.pcd";
    m_strFlyFileOut = depthImageSrc.substr(0, depthImageSrc.find_last_of('.')) + "_rgb.ply";
    string m_strFlyFileOutNORGB = depthImageSrc.substr(0, depthImageSrc.find_last_of('.')) + "_norgb.ply";
    m_strDepthFileOut = depthImageSrc.substr(0, depthImageSrc.find_last_of('.')) + "_depth.png";

    std::string depthPath = depthImageSrc;
    std::string rgbPath = colorImage;
    // 瀹氫箟鐐逛簯绫诲瀷
    typedef pcl::PointXYZRGB Point;
    typedef pcl::PointCloud<Point> PointCloud;

    // 鐩告満鍐呭弬
    const double camera_factor = 1000; //鍗曚綅
    double camera_cx = 0.0;
    double camera_cy = 0.0;

    double f = focus;   //鐒﹁窛 鍗曚綅姣背
    double F = fdis; //涓ょ偣涔嬮棿瑙嗗樊鐨勫儚绱犲亸绉诲樊鍊?鍗曚綅鍍忕礌
    double u = 4.8 / 1000.0; //鍍忕礌鐐瑰昂瀵?鍗曚綅绫?    double baseline = disp * (4.8 / 1000.0);  //瑙嗗樊鍩虹嚎  鍗曚綅绫?
    // 鍥惧儚鐭╅樀
    cv::Mat rgb, depth;
    rgb = cv::imread(rgbPath);
    depth = cv::imread(depthPath, -1);

    if (rgb.empty())
    {
        cout << COUT_PREFIX << "RGB image empty: " << rgbPath << endl;
        return false;
    }

    if (depth.empty())
    {
        cout << COUT_PREFIX << "Depth image empty: " << depthPath << endl;
        return false;
    }

    cout << COUT_PREFIX << "RGB cols=" << rgb.cols
        << ", rows=" << rgb.rows
        << ", channels=" << rgb.channels()
        << ", type=" << rgb.type()
        << endl;

    cout << COUT_PREFIX << "Depth cols=" << depth.cols
        << ", rows=" << depth.rows
        << ", channels=" << depth.channels()
        << ", type=" << depth.type()
        << endl;

    camera_cx = depth.cols / 2.0;  //涓績鐐?    camera_cy = depth.rows / 2.0;  //涓績鐐?
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

    // 閬嶅巻娣卞害鍥?    for (int m = 0; m < depth.rows; m++)
    {
        for (int n = 0; n <depth.cols; n++)
        {
            // 鑾峰彇娣卞害鍥句腑(m,n)澶勭殑鍊?            ushort d = depth.ptr<ushort>(m)[n];

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

                ////uv鍧愭爣
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
            p.z = p.z / camera_factor; //杞垚鍗曚綅绫?
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
            //杩囨护缁胯壊
            //if (p.g - p.r>30 && p.g - p.b>30)
            //if (p.g - p.r>5 && p.g - p.b>5)
            if (p.g - p.r>10 && p.g - p.b>10)

            {
                continue;
            }
            cloud->points.push_back(p);

            ////uv鍧愭爣
            //int index = m * depth.cols + n;
            //uv[index++] = p.x;
            //uv[index++] = p.y;
            //uv[index++] = p.z;
            //uv[index++] = m;
            //uv[index++] = n;
        }
    }

    // Set and save the point cloud.
    cloud->height = 1;
    cloud->width = cloud->points.size();
    //cloud->height = depth.rows;
    //cloud->width = depth.cols;

    cout << "point cloud size = " << cloud->points.size() << endl;
    cloud->is_dense = false;
    //pcl::io::savePCDFile(pcdFileOutPut, *cloud);
    pcl::io::savePLYFile(m_strFlyFileOut, *cloud);
#if 1
    //鐐逛簯婊ゆ尝澶勭悊.
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

    //缁熻婊ゆ尝鍣?    pcl::StatisticalOutlierRemoval<pcl::PointXYZ> sor;  //鍒涘缓婊ゆ尝鍣ㄥ璞?    sor.setInputCloud(cloud_piont);                     //璁剧疆寰呮护娉㈢殑鐐逛簯
    sor.setMeanK(meank);                                //璁剧疆鍦ㄨ繘琛岀粺璁℃椂鑰冭檻鏌ヨ鐐逛复杩戠偣鏁?    sor.setStddevMulThresh(stddevmulthresh);            //璁剧疆鍒ゆ柇鏄惁涓虹缇ょ偣鐨勯榾鍊?    sor.filter(*cloud_filtered);                        //瀛樺偍

    cout << "缁熻婊ゆ尝鍣?size = " << cloud_filtered->points.size() << endl;
    //鐩撮€氭护娉㈠櫒
    //// Create the filtering object
    //pcl::PassThrough<pcl::PointXYZ> pass;
    //pass.setInputCloud(cloud);
    //pass.setFilterFieldName("z");
    //pass.setFilterLimits(-0.5, 0);
    ////pass.setFilterLimitsNegative (true);
    //pass.filter(*cloud_filtered);

    //浣撶礌婊ゆ尝鍣?    //pcl::PointCloud<pcl::PointXYZ>::Ptr cloud_filtered2(new pcl::PointCloud<pcl::PointXYZ>);

    //pcl::ApproximateVoxelGrid<pcl::PointXYZ> avg;
    //avg.setInputCloud(cloud_filtered);
    //avg.setLeafSize(0.01, 0.01, 0.01);
    //avg.setDownsampleAllData(true);
    //avg.filter(*cloud_filtered2);
    //
    //cout << "浣撶礌婊ゆ尝鍣?size = " << cloud_filtered2->points.size() << endl;


    // 鍗婂緞杩囨护鍣?    pcl::PointCloud<pcl::PointXYZ>::Ptr RadiusOutlierRemoval_cloud_filtered(new pcl::PointCloud<pcl::PointXYZ>);
    pcl::RadiusOutlierRemoval<pcl::PointXYZ> outrem;
    outrem.setInputCloud(cloud_filtered);
    outrem.setRadiusSearch(radius); // 0.00005
    outrem.setMinNeighborsInRadius(minNeighborsInRadius);        //3

    outrem.filter(*RadiusOutlierRemoval_cloud_filtered);

    cout << "鍗婂緞杩囨护鍣?size = " << RadiusOutlierRemoval_cloud_filtered->points.size() << endl;

    //string filter_ply = m_strFlyFileOut.substr(0,m_strFlyFileOut.length() - 4)+ "_filter.ply";
    pcl::io::savePLYFile(m_strFlyFileOutNORGB, *RadiusOutlierRemoval_cloud_filtered);
#endif


    cout << "save filter ok . file = " << m_strFlyFileOut << endl;

    return true;
}

#endif


bool depthImage::depthToPlyColor(const std::string &depthImageSrc, const std::string &colorImage, const string &config, const string &outputDir
	//double disp,    //鐢ㄤ簬绠楀熀绾縝aseline 1
	//double step,    //绠楄宸?0.03
	//double label, //鏍囩鏁伴噺
	//double focus,   //鐒︾偣 50mm
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
	// 瀹氫箟鐐逛簯绫诲瀷
	typedef pcl::PointXYZRGB Point;
	typedef pcl::PointCloud<Point> PointCloud;

	// 鐩告満鍐呭弬
	const double camera_factor = 1000; //鍗曚綅
	double camera_cx = 0.0;
	double camera_cy = 0.0;

	double f = m_focus;   //鐒﹁窛 鍗曚綅姣背
	double F = m_fdis; //涓ょ偣涔嬮棿瑙嗗樊鐨勫儚绱犲亸绉诲樊鍊?鍗曚綅鍍忕礌
	double u = 4.8 / 1000.0; //鍍忕礌鐐瑰昂瀵?鍗曚綅绫?	double baseline = m_disp * (4.8 / 1000.0);  //瑙嗗樊鍩虹嚎  鍗曚綅绫?
	// 鍥惧儚鐭╅樀
	cv::Mat rgb, depth;
	rgb = cv::imread(rgbPath);
	depth = cv::imread(depthPath, -1);

	camera_cx = depth.cols / 2.0;  //涓績鐐?	camera_cy = depth.rows / 2.0;  //涓績鐐?
	PointCloud::Ptr cloud(new PointCloud);
    /*
	// 閬嶅巻娣卞害鍥?    /*
	for (int m = 0; m < depth.rows; m++)
	{
		for (int n = 0; n <depth.cols; n++)
		{
			// 鑾峰彇娣卞害鍥句腑(m,n)澶勭殑鍊?			ushort d = depth.ptr<ushort>(m)[n];

			double l = d / 100.0;
			//double v = (label * step) * u; //
			double v = ((l - m_label / 2) * m_step) * u;    //瑙嗗樊鍊?			if (fabs(v) < 0.00001)
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
			p.z = p.z / camera_factor; //杞垚鍗曚綅绫?
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
    */
    if (rgb.empty())
    {
        cout << COUT_PREFIX << "RGB image empty: " << rgbPath << endl;
        return false;
    }

    if (depth.empty())
    {
        cout << COUT_PREFIX << "Depth image empty: " << depthPath << endl;
        return false;
    }

    if (rgb.cols != depth.cols || rgb.rows != depth.rows)
    {
        cout << COUT_PREFIX << "RGB and depth size mismatch." << endl;
        cout << COUT_PREFIX << "RGB: " << rgb.cols << " x " << rgb.rows << endl;
        cout << COUT_PREFIX << "Depth: " << depth.cols << " x " << depth.rows << endl;
        return false;
    }

    cout << COUT_PREFIX << "RGB cols=" << rgb.cols
        << ", rows=" << rgb.rows
        << ", channels=" << rgb.channels()
        << ", type=" << rgb.type()
        << endl;

    cout << COUT_PREFIX << "Depth cols=" << depth.cols
        << ", rows=" << depth.rows
        << ", channels=" << depth.channels()
        << ", type=" << depth.type()
        << endl;

    if (depth.type() != CV_32FC3)
    {
        cout << COUT_PREFIX << "Error: expected CV_32FC3 tiff, but got type="
            << depth.type() << ", channels=" << depth.channels() << endl;
        return false;
    }

    for (int m = 0; m < depth.rows; m++)
    {
        for (int n = 0; n < depth.cols; n++)
        {
            cv::Vec3f xyz = depth.at<cv::Vec3f>(m, n);

            float X = xyz[0];
            float Y = xyz[1];
            float Z = xyz[2];

            if (!std::isfinite(X) || !std::isfinite(Y) || !std::isfinite(Z))
            {
                continue;
            }

            // 鏃犳晥娣卞害 / 鑳屾櫙
            if (std::fabs(Z) < 1e-6f)
            {
                continue;
            }

            uchar b = rgb.ptr<uchar>(m)[n * 3];
            uchar g = rgb.ptr<uchar>(m)[n * 3 + 1];
            uchar r = rgb.ptr<uchar>(m)[n * 3 + 2];

            // 杩囨护鐧借壊/娴呰壊鑳屾櫙
            if (r > 240 && g > 240 && b > 240)
            {
                continue;
            }

            Point p;

            // Use the XYZ values stored in the TIFF directly.
            p.x = X;
            p.y = -Y;
            p.z = -Z;

            p.b = b;
            p.g = g;
            p.r = r;

            cloud->points.push_back(p);
        }
    }
    // Set and save the point cloud.
    cloud->height = 1;
    cloud->width = cloud->points.size();
    //cloud->height = depth.rows;
    //cloud->width = depth.cols;

    cout << COUT_PREFIX << "point cloud size = " << cloud->points.size() << endl;
	cloud->is_dense = false;
	//pcl::io::savePCDFile(pcdFileOutPut, *cloud);
	const int saveStatus = pcl::io::savePLYFile(m_strFlyFileOut, *cloud);
	cloud->clear();
	cloud.reset();
	if (saveStatus != 0)
	{
		cout << COUT_PREFIX << "save ply failed . file = " << m_strFlyFileOut << endl;
		return false;
	}

#if 0

    pcl::PointCloud<pcl::PointXYZRGB>::Ptr cloud_filtered(new pcl::PointCloud<pcl::PointXYZRGB>);
    pcl::PointCloud<pcl::PointXYZRGB>::Ptr cloud_piont(new pcl::PointCloud<pcl::PointXYZRGB>);


	//缁熻婊ゆ尝鍣?    if (meank != 0)
    {
        pcl::StatisticalOutlierRemoval<pcl::PointXYZRGB> sor;  //鍒涘缓婊ゆ尝鍣ㄥ璞?        sor.setInputCloud(cloud);                     //璁剧疆寰呮护娉㈢殑鐐逛簯
	    sor.setMeanK(meank);                                //璁剧疆鍦ㄨ繘琛岀粺璁℃椂鑰冭檻鏌ヨ鐐逛复杩戠偣鏁?	    sor.setStddevMulThresh(stddevmulthresh);            //璁剧疆鍒ゆ柇鏄惁涓虹缇ょ偣鐨勯榾鍊?        sor.filter(*cloud_piont);                       //瀛樺偍
        cout << COUT_PREFIX << "缁熻婊ゆ尝鍣?size = " << cloud_piont->points.size() << endl;
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

	//鐩撮€氭护娉㈠櫒
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

        cout << COUT_PREFIX << "鐩撮€氭护娉㈠櫒 size = " << tmpPointCloud->points.size() << endl;
        cloud_piont->clear();
        cloud_piont->resize(tmpPointCloud->points.size());
        pcl::copyPoint(tmpPointCloud,cloud_piont);

    }


	//浣撶礌婊ゆ尝鍣?	//pcl::PointCloud<pcl::PointXYZ>::Ptr cloud_filtered2(new pcl::PointCloud<pcl::PointXYZ>);

	//pcl::ApproximateVoxelGrid<pcl::PointXYZ> avg;
	//avg.setInputCloud(cloud_filtered);
	//avg.setLeafSize(0.01, 0.01, 0.01);
	//avg.setDownsampleAllData(true);
	//avg.filter(*cloud_filtered2);
	//
	//cout << "浣撶礌婊ゆ尝鍣?size = " << cloud_filtered2->points.size() << endl;


	// 鍗婂緞杩囨护鍣?    if (radius != 0.0)
    {
        pcl::PointCloud<pcl::PointXYZRGB>::Ptr RadiusOutlierRemoval_cloud_filtered(new pcl::PointCloud<pcl::PointXYZRGB>);
        pcl::RadiusOutlierRemoval<pcl::PointXYZRGB> outrem;

        outrem.setInputCloud(cloud_piont);
	    outrem.setRadiusSearch(radius); // 0.00005
	    outrem.setMinNeighborsInRadius(minNeighborsInRadius);        //3

	    outrem.filter(*RadiusOutlierRemoval_cloud_filtered);

        cout << COUT_PREFIX << "鍗婂緞杩囨护鍣?size = " << RadiusOutlierRemoval_cloud_filtered->points.size() << endl;
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
    return false;
#if 0
    std::string pcdFileOutPut =  depthImageSrc.substr(0, depthImageSrc.find_last_of('.')) + "_rgb.pcd";
    std::string plyFileOutPut = depthImageSrc.substr(0, depthImageSrc.find_last_of('.')) + "_rgb.ply";
    std::string depthPath =  depthImageSrc;
    std::string rgbPath =  colorImage;
    // 瀹氫箟鐐逛簯绫诲瀷
    typedef pcl::PointXYZRGB Point;
    typedef pcl::PointCloud<Point> PointCloud;

    // 鐩告満鍐呭弬
    const double camera_factor = 1000;
    //const double camera_cx = 325.5;   //鐩告満鐨勫厜鍦堜腑蹇?    //const double camera_cy = 253.5;
    //const double camera_fx = 500;   //鐩告満鍦ㄤ袱涓酱涓婄殑鐒﹁窛锛?褰卞搷z 鏂瑰悜鐨勬繁搴?澶у皬锛?瓒婂ぇ 娣卞害璺濈瓒婂ぇ銆?    //const double camera_fy = 500;

    /*const double camera_cx = 253;
    const double camera_cy = 184.5;
    const double camera_fx = 1157.407407407407;
    const double camera_fy = 1157.407407407407;*/


    /*const double camera_fx = 50000.0 / (4.8 * 7.23);
    const double camera_fy = 50000.0 / (4.8 * 7.23);*/
    const double camera_fx = 1500;   //鐩告満鍦ㄤ袱涓酱涓婄殑鐒﹁窛锛?褰卞搷z 鏂瑰悜鐨勬繁搴?澶у皬锛?瓒婂ぇ 娣卞害璺濈瓒婂ぇ銆?    const double camera_fy = 1500;


    // 璇诲彇./data/rgb.png鍜?/data/depth.png锛屽苟杞寲涓虹偣浜?
    // 鍥惧儚鐭╅樀
    cv::Mat rgb, depth;
    // 浣跨敤cv::imread()鏉ヨ鍙栧浘鍍?    rgb = cv::imread(rgbPath);
    // rgb 鍥惧儚鏄?UC3鐨勫僵鑹插浘鍍?    // depth 鏄?6UC1鐨勫崟閫氶亾鍥惧儚锛屾敞鎰廸lags璁剧疆-1,琛ㄧず璇诲彇鍘熷鏁版嵁涓嶅仛浠讳綍淇敼
    depth = cv::imread(depthPath, -1);

    double camera_cx = depth.cols / 2.0;
    double camera_cy = depth.rows / 2.0;

    // 鐐逛簯鍙橀噺
    // 浣跨敤鏅鸿兘鎸囬拡锛屽垱寤轰竴涓┖鐐逛簯銆傝繖绉嶆寚閽堢敤瀹屼細鑷姩閲婃斁銆?    PointCloud::Ptr cloud(new PointCloud);

    // 閬嶅巻娣卞害鍥?    for (int m = 0; m < depth.rows; m++)
    for (int n = 0; n <depth.cols; n++)
    {
        // 鑾峰彇娣卞害鍥句腑(m,n)澶勭殑鍊?        ushort d = depth.ptr<ushort>(m)[n];
        //uchar d = depth.ptr<uchar>(m)[n];
        // d 鍙兘娌℃湁鍊硷紝鑻ュ姝わ紝璺宠繃姝ょ偣

        //d = d >> 8;

        if (d == 0)
            continue;
        // d 瀛樺湪鍊硷紝鍒欏悜鐐逛簯澧炲姞涓€涓偣
        Point p;

        // 璁＄畻杩欎釜鐐圭殑绌洪棿鍧愭爣
        p.z = double(d) / camera_factor;
        p.z *= -1;

        //璋冩暣x鏂瑰悜
        int j = depth.cols - n - 1;
        p.x = (j - camera_cx) * p.z / camera_fx;
        //p.x = (n - camera_cx) * p.z / camera_fx;
        //p.x = (n - camera_cx) * p.z / camera_fx;
        //p.x *= -1;

        //璋冩暣y鏂瑰悜
        //p.y = (m - camera_cy) * p.z / camera_fy;
        int k = depth.rows - m - 1;
        p.y = (k - camera_cy) * p.z / camera_fy;
        //p.y = (m - camera_cy) * p.z / camera_fx;

        //璋冩暣z 鏂瑰悜


        // 浠巖gb鍥惧儚涓幏鍙栧畠鐨勯鑹?        // rgb鏄笁閫氶亾鐨凚GR鏍煎紡鍥撅紝鎵€浠ユ寜涓嬮潰鐨勯『搴忚幏鍙栭鑹?        p.b = rgb.ptr<uchar>(m)[n * 3];
        p.g = rgb.ptr<uchar>(m)[n * 3 + 1];
        p.r = rgb.ptr<uchar>(m)[n * 3 + 2];
        if (p.g - p.r>30 && p.g - p.b>30)
        {
            continue;
        }

        p.y *= -1;
        p.z *= -1;


        // 鎶妏鍔犲叆鍒扮偣浜戜腑
        cloud->points.push_back(p);
    }
    // 璁剧疆骞朵繚瀛樼偣浜?    //cloud->height = depth.rows;
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
#endif
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
