#include "OdmTexturing.hpp"
#include "FileLibrary.h"

OdmTexturing::OdmTexturing()
{     
    mesh_ = pcl::TextureMeshPtr(new pcl::TextureMesh);
    patches_ = std::vector<Patch>(0);

}
OdmTexturing::OdmTexturing(const std::string &inputply, const std::string &inputtexture){


    mesh_ = pcl::TextureMeshPtr(new pcl::TextureMesh);
    patches_ = std::vector<Patch>(0);

    std::string parentdir = FileLibrary::getInstance()->getFileParentPath(inputtexture)+"\\";
    std::string camerafile = parentdir + "\\cameras.out";


    inputModelPath_ = inputply;
    outputFolder_ = parentdir;
    imagesPath_ = inputtexture;

}


OdmTexturing::~OdmTexturing()
{

}

int OdmTexturing::run(float focal_length)
{
    m_focal = focal_length;
   
    loadMesh(); 
    createTextures(); 
    writeObjFile();
    
    
    return 0;
}


void OdmTexturing::loadMesh()
{
    // Read model from ply-file
    pcl::PolygonMeshPtr plyMeshPtr(new pcl::PolygonMesh);
    if (pcl::io::loadPLYFile(inputModelPath_, *plyMeshPtr.get()) == -1)
    {
        throw OdmTexturingException("Error when reading model from:\n" + inputModelPath_ + "\n");
    }
    else
    {
        cout<<COUT_PREFIX << "Successfully loaded " << plyMeshPtr->polygons.size() << " polygons from file.\n";
    }

    // Transfer data from ply file to TextureMesh
    mesh_->cloud = plyMeshPtr->cloud;
    std::vector<pcl::Vertices> polygons;
    mesh_->tex_polygons.push_back(plyMeshPtr->polygons);


}



void OdmTexturing::createTextures()
{
    // Convert vertices to pcl::PointXYZ cloud
    pcl::PointCloud<pcl::PointXYZ>::Ptr meshCloud(new pcl::PointCloud<pcl::PointXYZ>);
    pcl::fromPCLPointCloud2(mesh_->cloud, *meshCloud);

    std::vector<std::vector<Eigen::Vector2f, Eigen::aligned_allocator<Eigen::Vector2f>> > textureCoordinatesVector = std::vector<std::vector<Eigen::Vector2f, Eigen::aligned_allocator<Eigen::Vector2f>> >( 1);
    std::vector<pcl::TexMaterial> materialVector = std::vector<pcl::TexMaterial>(1);

    cv::Mat image = cv::imread(imagesPath_);


    float focal = 0.191 / 0.79 * m_focal / 4.8 * 1000; 
    float cx = image.cols / 2.0;
    float cy = image.rows / 2.0;
    float texture_width = image.cols;
    float texture_height = image.rows;

    for (size_t i = 0; i < mesh_->tex_polygons[0].size(); i++)
    {
        Eigen::Vector2f uv1, uv2, uv3;
        

        uv1(0) = 1.0 - (float)((focal * (meshCloud->points[mesh_->tex_polygons[0][i].vertices[0]].x / meshCloud->points[mesh_->tex_polygons[0][i].vertices[0]].z) + cx) / texture_width);
        uv1(1) = 1.0 - (float)((focal * (meshCloud->points[mesh_->tex_polygons[0][i].vertices[0]].y / meshCloud->points[mesh_->tex_polygons[0][i].vertices[0]].z) + cy) / texture_height);
        if (uv1(0)<0.0 || uv1(0)>1.0 || uv1(1)<0.0 || uv1(0)>1.0)
        {
            continue;
        }
        uv2(0) = 1.0 - (float)((focal * (meshCloud->points[mesh_->tex_polygons[0][i].vertices[1]].x / meshCloud->points[mesh_->tex_polygons[0][i].vertices[1]].z) + cx) / texture_width);
        uv2(1) = 1.0 - (float)((focal * (meshCloud->points[mesh_->tex_polygons[0][i].vertices[1]].y / meshCloud->points[mesh_->tex_polygons[0][i].vertices[1]].z) + cy) / texture_height);
        if (uv2(0)<0.0 || uv2(0)>1.0 || uv2(1)<0.0 || uv2(0)>1.0)
        {
            continue;
        }
        uv3(0) = 1.0 - (float)((focal * (meshCloud->points[mesh_->tex_polygons[0][i].vertices[2]].x / meshCloud->points[mesh_->tex_polygons[0][i].vertices[2]].z) + cx) / texture_width);
        uv3(1) = 1.0 - (float)((focal * (meshCloud->points[mesh_->tex_polygons[0][i].vertices[2]].y / meshCloud->points[mesh_->tex_polygons[0][i].vertices[2]].z) + cy) / texture_height);
        if (uv3(0)<0.0 || uv3(0)>1.0 || uv3(1)<0.0 || uv3(0)>1.0)
        {
            continue;
        }
       
        // Add uv coordinates to submesh
        textureCoordinatesVector[0].push_back(uv1);
        textureCoordinatesVector[0].push_back(uv2);
        textureCoordinatesVector[0].push_back(uv3);
    }


    // Declare material and setup default values
    pcl::TexMaterial meshMaterial;
    meshMaterial.tex_Ka.r = 1.0f; meshMaterial.tex_Ka.g = 1.0f; meshMaterial.tex_Ka.b = 1.0f;
    meshMaterial.tex_Kd.r = 1.0f; meshMaterial.tex_Kd.g = 1.0f; meshMaterial.tex_Kd.b = 1.0f;
    meshMaterial.tex_Ks.r = 0.0f; meshMaterial.tex_Ks.g = 0.0f; meshMaterial.tex_Ks.b = 0.0f;
    //meshMaterial.tex_d = 1.0f; 
    
    meshMaterial.tex_Ns = 200.0f;
    meshMaterial.tex_illum = 2.0f;

    string texturename = FileLibrary::getInstance()->getFileNameFromPath(imagesPath_);
    meshMaterial.tex_file = texturename;
    texturename = texturename.substr(0, texturename.length()-4);
    meshMaterial.tex_name = texturename;
    materialVector[0] = meshMaterial;


    //mesh_->tex_polygons = faceVector;
    mesh_->tex_coordinates = textureCoordinatesVector;
    mesh_->tex_materials = materialVector;



}




void OdmTexturing::writeObjFile()
{
    string outfile = FileLibrary::getInstance()->getFileNameFromPath(inputModelPath_);
    outfile = outfile.substr(0, outfile.length() - 4)+"_model.obj";
    
    if (testsaveOBJFile(outputFolder_ + outfile, *mesh_.get(), 7) == 0)
    {
        cout <<COUT_PREFIX<< "successfully saved file : " << outfile << endl << endl;
    }
    else
    {
        cout << COUT_PREFIX << "Failed to save model.\n";
    }
}

