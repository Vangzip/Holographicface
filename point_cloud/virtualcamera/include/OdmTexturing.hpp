#pragma once

// STL
#include <iostream>
#include <fstream>

// PCL
#include <pcl/point_types.h>
#include <pcl/search/kdtree.h>
#include <pcl/surface/texture_mapping.h>

// OpenCV

// Modified PCL functions
#include "modifiedPclFunctions.hpp"

// Logging
#include "Logger.hpp"
#include "base.h"
/*!
 * \brief The Coords struct     Coordinate class used in recursiveFindCoordinates for OdmTexturing::sortPatches().
 */
struct Coords
{
    // Coordinates for row and column
    float r_, c_;

    // If coordinates have been placed
    bool success_;

    Coords()
    {
        r_ = 0.0;
        c_ = 0.0;
        success_ = false;
    }
};

/*!
 * \brief The Patch struct      Struct to hold all faces connected and with the same optimal camera.
 */
struct Patch
{
    std::vector<size_t> faces_;
    float minu_, minv_, maxu_, maxv_;
    Coords c_;
    bool placed_;
    int materialIndex_;
    int optimalCameraIndex_;

    Patch()
    {
        placed_ = false;
        faces_ = std::vector<size_t>(0);
        minu_ = std::numeric_limits<double>::infinity();
        minv_ = std::numeric_limits<double>::infinity();
        maxu_ = 0.0;
        maxv_ = 0.0;
        optimalCameraIndex_ = -1;
        materialIndex_ = 0;
    }
};

/*!
 * \brief The Node struct       Node class for acceleration structure in OdmTexturing::sortPatches().
 */
struct Node
{
    float r_, c_, width_, height_;
    bool used_;
    Node* rgt_;
    Node* lft_;

    Node()
    {
        r_ = 0.0;
        c_ = 0.0;
        width_ = 1.0;
        height_ = 1.0;
        used_ = false;
        rgt_ = NULL;
        lft_ = NULL;
    }

    Node(const Node &n)
    {
        r_ = n.r_;
        c_ = n.c_;
        used_ = n.used_;
        width_ = n.width_;
        height_ = n.height_;
        rgt_ = n.rgt_;
        lft_ = n.lft_;
    }
};

/*!
 * \brief   The OdmTexturing class is used to create textures to a welded ply-mesh using the camera
 *          positions from pmvs as input. The result is stored in an obj-file with corresponding
 *          mtl-file and the textures saved as jpg.
 */
class OdmTexturing
{
public:
    OdmTexturing();
    OdmTexturing(const std::string &inputply, const std::string &inputtexture);
    ~OdmTexturing();


    /*!
     * \brief   run   Runs the texturing functionality using the provided input arguments.
     *                For a list of the accepted arguments, please see the main page documentation or
     *                call the program with parameter "-help".
     * \param   argc  Application argument count.
     * \param   argv  Argument values.
     * \return  0     if successful.
     */
    int run(float);

private: 
    /*!
     * \brief loadMesh          Loads a PLY-file containing vertices and faces.
     */
    void loadMesh();

    /*!
     * \brief createTextures    Creates textures to the mesh.
     */
    void createTextures();

    /*!
     * \brief writeObjFile      Writes the textured mesh to file on the OBJ format.
     */
    void writeObjFile();



    std::string imagesPath_;        /**< Path to the folder with all images in the image list. */
    std::string imagesListPath_;    /**< Path to the image list. */
    std::string inputModelPath_;    /**< Path to the ply-file containing the mesh to be textured. */
    std::string outputFolder_;      /**< Path to the folder to store the output mesh and textures. */

    int nrTextures_;             /**< The number of textures created. */

    pcl::TextureMesh::Ptr mesh_;    /**< PCL Texture Mesh */
    std::vector<Patch> patches_;    /**< The vector containing all patches */
    pcl::texture_mapping::CameraVector cameras_;    /**< The vector containing all cameras. */

    float m_focal;
};

class OdmTexturingException : public std::exception
{

public:
    OdmTexturingException() : message("Error in OdmTexturing") {}
    OdmTexturingException(std::string msgInit) : message("Error in OdmTexturing:\n" + msgInit) {}
    ~OdmTexturingException() throw() {}
    virtual const char* what() const throw() {return message.c_str(); }

private:
    std::string message;        /**< The error message. */
};
