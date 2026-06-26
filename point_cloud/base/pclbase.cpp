#include "pclbase.h"

PCLBASE* PCLBASE::m_pInstance = NULL;


PCLBASE::PCLBASE(){  }

PCLBASE *PCLBASE::getInstance(){
    if (m_pInstance == NULL){
        m_pInstance = new PCLBASE();
    }
    return m_pInstance;
}





void PCLBASE::normalsMovingLeastSquares(PointTRGBPtr cloud, PointTRGBN &mls_points, float radius){

    pcl::MovingLeastSquares<PointXYZRGB, PointXYZRGBNormal> mls;
    pcl::search::KdTree<PointXYZRGB>::Ptr tree(new pcl::search::KdTree<PointXYZRGB>);
    tree->setInputCloud(cloud);

    // Set parameters
    mls.setInputCloud(cloud);
    //mls.setPolynomialFit(true); //
    mls.setComputeNormals(true);
    mls.setSearchMethod(tree);
    mls.setPointDensity(30);
    mls.setPolynomialOrder(4);
    mls.setSearchRadius(radius); // 0.001jingnan //
    mls.setUpsamplingMethod(mls.NONE);
    // Reconstruct
    mls.process(mls_points);    
}


bool PCLBASE::PCDtoPLYconvertor(const string & input_filename, const string& output_filename)
{
    pcl::PCLPointCloud2 cloud;
    if (loadPCDFile(input_filename, cloud) < 0)
    {
        cout << "Error: cannot load the PCD file!!!" << endl;
        return false;
    }

    PLYWriter writer;
    writer.write(output_filename, cloud, Eigen::Vector4f::Zero(), Eigen::Quaternionf::Identity(), true, true);
    
    return true;
}






bool PCLBASE::getPointFromPointNormal(pcl::PointCloud<pcl::PointXYZRGB>::Ptr cloud, pcl::PointCloud<PointXYZRGBNormal> &mls_points){

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

bool PCLBASE::nearestKSearchNormal(pcl::PointCloud<PointXYZRGBNormal> &mls_points,float normalIter, float neighbooNum, float distanceNum){

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

    //����ھ�K���㷨��
    for (size_t iterNum = 0; iterNum < normalIter; iterNum++)
    {
        tmpNormalPoint.clear();
        tmpNormalPoint.points.resize(mls_points.points.size());


        for (size_t i = 0; i < mls_points.points.size(); i++)
        {

            std::vector<int> indices(neighbooNum);
            std::vector<float> distance(distanceNum); //0.01
            //����VTK/OpenGL��û�д洢NaN��ʽ
            pcl::PointXYZRGB current_point;
            current_point.x = mls_points.points[i].x;
            current_point.y = mls_points.points[i].y;
            current_point.z = mls_points.points[i].z;
            search.nearestKSearch(current_point, neighbooNum, indices, distance);//����current_pointΪѡ�еĵ� 

            for (size_t Knum = 0; Knum < neighbooNum; Knum++)
            {
                int neightId = indices[Knum];
                pcl::PointXYZRGBNormal neightborpoint = mls_points.points[neightId];
                //�ھӷ��� * ��ǰ�㷨��
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


        //���߱�׼��
        calculateVertexNormal(mls_points);
        //cout << "end iter : "<< iterNum << endl;
    }

    return true;
}

bool PCLBASE::calculateVertexNormal(pcl::PointCloud<pcl::PointXYZRGBNormal> &mls_points){

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
