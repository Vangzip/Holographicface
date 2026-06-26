#include <base.h>
#include <FileLibrary.h>
#include "filter.h"


//************************************
// Method:    Rgb2Hsv
// Access:    public 
// Returns:   bool
// Describe:  根据HSV过滤点云颜色
// Parameter: float R
// Parameter: float G
// Parameter: float B
// Parameter: float maxH
// Parameter: float minH
// Parameter: float maxS
// Parameter: float minS
// Parameter: float maxV
// Parameter: float minV
//************************************
bool Rgb2Hsv(float R, float G, float B, float maxH, float minH, float maxS, float minS, float maxV, float minV)
{
	float H, S, V, b = B / 255, g = G / 255, r = R / 255;
	float min, max, delta;
	min = min(r, min(g, b));
	max = max(r, max(g, b));

	V = max; // v
	delta = V - min;
	if (V != 0) {
		S = delta / V; // s
	}
	else
	{
		// r = g = b = 0 // s = 0, v is undefined
		S = 0;
		H = -1;
		return false;
	}

	if (r == V)
		H = (g - b) / delta; // between yellow & magenta
	else if (g == V)
		H = 2 + (b - r) / delta; // between cyan & yellow
	else if (b == V)
		H = 4 + (r - g) / delta; // between magenta & cyan

	H *= 60; // degrees
	if (H < 0)
		H += 360;
	//H 120 S 1 V 1
	if (H >= minH && H <= maxH&& S >= minS && S <= maxS && V >= minV && V <= maxV)
	{
		return true;
	}

	return false;
}

// 打印帮助信息
void printUsage(const char* progName)
{
    std::cout << "\n\nUsage: " << progName << " [options] <scene.ply>\n\n"
        << "Options:\n"
        << "-------------------------------------------\n"
        << "-green (5--50) default 0. \n"
        << "-black (10-150) default 0. \n"
        << "-white num \n"
        << "-file <string>  \n"
        << "-mls  distance num \n" 
        << "-vox  num \n"
        << "-ror  num distance  \n" 
        << "-sor  K num  \n"        
		<<"-hsv filter file path\n"
        << "-h  this help\n"
        << "\n\n";
}

//************************************
// Method:    filter
// FullName:  filter
// Access:    public 
// Returns:   int
// Qualifier:  过滤点云背景色，及噪点
// Parameter: int argc
// Parameter: char * argv[]
//************************************
int filter(int argc, char *argv[]){

	int green = 0; //过滤绿色值 一般80-5，阈值越小绿色越浅
	int black = 0; //黑色阈值过滤，一般150 阈值越小 颜色越黑
	int white = 0; //白色过滤，一般220 数值越大 白色越亮
    string plyfile = ""; //输入ply文件
    string normal = ""; //点云中是否带有法线 1是
    string outfile = ""; //输出文件路径
    string m_rg = "";
    //float itnum = 1;
    bool isROR = false, isSOR = false, isVOX = false, isMLS = false, isRG = false; //各滤波状态变量
    float ror_threshold, ror_iternum, ror_radius;//半径滤波参数变量
    float vox_size = 0; //栅格滤波参数变量
    float mls_radius, mls_iterNum; //最下二乘法参数变量
    float sor_iterNum, sor_threshold, sor_neighbors; //统计滤波参数变量
    float maxH, minH, maxS, minS, maxV, minV;  //HSV参数变量 
	string hsv = "";

    //解析命令行参数
    if (pcl::console::find_argument(argc, argv, "-h") >= 0)
    {
        printUsage(argv[0]);
        return 0;
    }
    if (pcl::console::parse_argument(argc, argv, "-hsv", hsv) >= 0)
    {
		ifstream infile(hsv);
		string line;
		while (getline(infile, line))
		{
			if (int pos = line.find("H=")!=string::npos)
			{
				line = line.substr(pos+1, line.length()-pos-1);
				vector<string> list;
				FileLibrary::getInstance()->splitString(line, ",", list);
				if (list.size()==2)
				{
					maxH = atoi(list[0].c_str());
					minH = atoi(list[1].c_str());
				}
			}
			else if (int pos = line.find("S=") != string::npos)
			{
				line = line.substr(pos + 1, line.length() - pos - 1);
				vector<string> list;
				FileLibrary::getInstance()->splitString(line, ",", list);
				if (list.size() == 2)
				{
					maxS = atoi(list[0].c_str());
					minS = atoi(list[1].c_str());
				}
			}
			else if (int pos = line.find("V=") != string::npos)
			{
				line = line.substr(pos + 1, line.length() - pos - 1);
				vector<string> list;
				FileLibrary::getInstance()->splitString(line, ",", list);
				if (list.size() == 2)
				{
					maxV = atoi(list[0].c_str());
					minV = atoi(list[1].c_str());
				}
			}
		}
		infile.close();
    }

    pcl::console::parse(argc, argv, "-normal", normal);
    pcl::console::parse(argc, argv, "-green", green);
    pcl::console::parse(argc, argv, "-black", black);
    pcl::console::parse(argc, argv, "-white", white);
    pcl::console::parse(argc, argv, "-file", plyfile);

    if (pcl::console::parse(argc, argv, "-vox", vox_size) >= 0){//体素网格滤波
        isVOX = true;
    }

    if (FileLibrary::getInstance()->parse_3x_arg(argc, argv, "-ror", ror_radius, ror_threshold, ror_iternum) >= 0)//半径滤波
    {
        isROR = true;
    }

    if (FileLibrary::getInstance()->parse_3x_arg(argc, argv, "-sor", sor_neighbors, sor_threshold, sor_iterNum) >= 0){ //统计滤波
        isSOR = true;
    }

    if (FileLibrary::getInstance()->parse_2x_arg(argc, argv, "-mls", mls_radius, mls_iterNum) >= 0){//最小二乘
        isMLS = true;
    }

    if (pcl::console::parse(argc, argv, "-rg", m_rg) >= 0){
        isRG = true;
    }


    if (!FileLibrary::getInstance()->isFileExists(plyfile))
    {

        cout << COUT_PREFIX << "not find : " << plyfile << endl;
        return -1;
    }
    outfile = plyfile;

    pcl::PointCloud<pcl::PointXYZRGBNormal>::Ptr src_cloud(new pcl::PointCloud<pcl::PointXYZRGBNormal>);
    pcl::PointCloud<pcl::PointXYZRGBNormal>::Ptr tgt_cloud(new pcl::PointCloud<pcl::PointXYZRGBNormal>);

	if (pcl::io::loadPLYFile(plyfile, *src_cloud) < 0) {
		cout << COUT_PREFIX << "load ply file false ." << endl;
		return 0;
	}

    if (normal == "1")
    {
        cout << COUT_PREFIX << "src point: " << src_cloud->points.size() << endl;
        for (size_t i = 0; i < src_cloud->points.size(); i++)
        {
            PointXYZRGBNormal Point = src_cloud->points[i];

            if (green && Point.g - Point.r > green && Point.g - Point.b > green)   //绿色
            {
                continue;

            }

            if (black && (0.3*Point.r + 0.6*Point.g + 0.1*Point.b)< black)  //黑色
            {
                continue;
            }


            if (white && (0.3*Point.r + 0.6*Point.g + 0.1*Point.b) > white)  //白色过滤
            {
                continue;
            }


            if (!hsv.empty() &&Rgb2Hsv(Point.r, Point.g, Point.b,maxH,minH,maxS,minS,maxV,minV)) //HSV
            {
                continue;
            }

            tgt_cloud->points.push_back(Point);

        }

        outfile = outfile.substr(0, outfile.find_last_of(".")) + "_rgb.ply";
        cout << COUT_PREFIX << "rgb filter tgt point: " << tgt_cloud->points.size() << endl;
        pcl::copyPoint(tgt_cloud, src_cloud);

    }

    point_base pointFunc;
    if (isSOR) //统计滤波
    {
        pointFunc.sor(src_cloud, tgt_cloud, sor_neighbors, sor_threshold, sor_iterNum);
        outfile = outfile.substr(0, outfile.find_last_of(".")) + "_sor.ply";

        
    }else
    if (isROR)//半径过滤
    {
        pointFunc.ror(src_cloud, tgt_cloud, ror_radius, ror_threshold, ror_iternum);
        outfile = outfile.substr(0, outfile.find_last_of(".")) + "_ror.ply";

	}
	else
    if (isVOX)//体素网格滤波
    {
        pointFunc.vox(src_cloud, tgt_cloud, vox_size);
        outfile = outfile.substr(0, outfile.find_last_of(".")) + "_vox.ply";

	}
	else
    if (isMLS)//最小二乘
    {
        pointFunc.mls(src_cloud, tgt_cloud, mls_radius, mls_iterNum, 0);
        outfile = outfile.substr(0, outfile.find_last_of(".")) + "_mls.ply";

	}
	else
    if (isRG)
    {
        pcl::PointCloud<pcl::PointXYZRGB>::Ptr src_cloud(new pcl::PointCloud<pcl::PointXYZRGB>);
        pcl::PointCloud<pcl::PointXYZRGB>::Ptr tgt_cloud(new pcl::PointCloud<pcl::PointXYZRGB>);

        pcl::io::loadPLYFile(plyfile, *src_cloud);

        cout << COUT_PREFIX << "src point: " << src_cloud->points.size() << endl;

        pcl::search::Search<pcl::PointXYZRGB>::Ptr normaltree(new pcl::search::KdTree<pcl::PointXYZRGB>);
        pcl::PointCloud <pcl::Normal>::Ptr normals(new pcl::PointCloud <pcl::Normal>);
        pcl::NormalEstimation<pcl::PointXYZRGB, pcl::Normal> normal_estimator;
        normal_estimator.setSearchMethod(normaltree);
        normal_estimator.setInputCloud(src_cloud);
        normal_estimator.setKSearch(50);
        normal_estimator.compute(*normals);

        pcl::RegionGrowing<pcl::PointXYZRGB, pcl::Normal> reg;
        reg.setMinClusterSize(10);  //最小聚类点
        reg.setMaxClusterSize(src_cloud->points.size()); //最大聚类点
        reg.setSearchMethod(normaltree); //搜索方式
        reg.setNumberOfNeighbours(30); //设置搜索临近点个数
        reg.setInputCloud(src_cloud);
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
        outfile = outfile.substr(0, outfile.find_last_of(".")) + "_rg.ply";
        src_cloud = cloud_cluster;
    }
    cout << COUT_PREFIX << "save file path: " << outfile << endl;
    pcl::io::savePLYFile(outfile, *tgt_cloud);

    return 0;

}


//输入参数如: -file e:/test.ply -normal 1 -green 50
int main(int argc, char *argv[]){

	filter(argc, argv);
}


