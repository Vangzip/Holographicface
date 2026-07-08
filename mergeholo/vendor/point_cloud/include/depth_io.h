#pragma once

#include "base.h"
#include "FileLibrary.h"

class depthImage{
public:
    //depthImage(const std::string &depthfile);
    depthImage(int movelabel);
    ~depthImage();

    /*bool depthToPlyColor(const std::string &depthImageSrc, const std::string &colorImage, double disp = 1, double step = 0.02, double label=150, double focus = 50, double fdis = 0.6);*/
    bool depthToPlyColor(const std::string &, const std::string &, const string &, const string &outputDir = string());
    pcl::PointCloud<pcl::PointXYZRGB>::Ptr depthToPointCloudColor(const std::string &, const std::string &, const string &);
    string getFlyFile(){ return m_strFlyFileOut; };

    bool  depthToPcdCloreFileCorrectOrientation(const std::string &depthImageSrc, const std::string &colorImage);

	bool parseArguments(const string &configfile);

    bool setPiCi(int);
private:
    std::string m_strDepthFile;
    std::string m_strFlyFileOut;
    std::string m_strDepthFileOut;



    float meank, minNeighborsInRadius, m_greenRGB, mina, minb;
	double stddevmulthresh , radius;
    double m_disp, m_step, m_label, m_focus, m_fdis;
    int m_movelLabel, m_lableNum, m_passThrough;
};
