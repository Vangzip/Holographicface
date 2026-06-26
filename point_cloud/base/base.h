

#pragma once

//#ifdef PCL_DEF
#include <pcl/pcl_base.h>
#include <pcl/point_types.h>
#include <pcl/point_cloud.h>
#include <pcl/PolygonMesh.h>
#include <pcl/visualization/pcl_visualizer.h>
#include <pcl/point_types.h>
#include <pcl/io/ply_io.h>
#include <pcl/io/png_io.h>
#include <pcl/io/obj_io.h>
#include <pcl/io/vtk_io.h>
#include <pcl/io/pcd_io.h>

#include <Eigen/Core>
#include <pcl/point_types.h>
#include <pcl/point_cloud.h>
#include <pcl/io/pcd_io.h>
#include <pcl/kdtree/kdtree_flann.h>
#include <pcl/filters/passthrough.h>
#include <pcl/filters/voxel_grid.h>
#include <pcl/features/normal_3d.h>
#include <pcl/features/fpfh.h>
#include <pcl/registration/ia_ransac.h>
#include <pcl/common/transforms.h>
#include <pcl/registration/icp.h>
#include <pcl/registration/icp_nl.h>

#include <boost/thread/thread.hpp>
#include <pcl/common/common_headers.h>
#include <pcl/range_image/range_image.h>
#include <pcl/visualization/range_image_visualizer.h>
#include <pcl/visualization/cloud_viewer.h>
#include <pcl/range_image/range_image_planar.h>
#include <pcl/visualization/pcl_visualizer.h>

#include <pcl/kdtree/kdtree_flann.h>
#include <pcl/features/feature.h>
#include <pcl/features/normal_3d.h>
#include <vtkSmartPointerBase.h>

#include <pcl/filters/bilateral.h>
#include <pcl/filters/passthrough.h>
#include <pcl/filters/approximate_voxel_grid.h>  
#include <pcl/filters/radius_outlier_removal.h>
#include <pcl/filters/radius_outlier_removal.h>
#include <pcl/filters/conditional_removal.h>
#include <pcl/filters/statistical_outlier_removal.h>

#include <pcl/segmentation/region_growing_rgb.h>

#include <pcl/surface/gp3.h>
#include <pcl/surface/mls.h>
#include <pcl/surface/poisson.h>
#include <pcl/surface/ear_clipping.h>
#include <pcl/surface/texture_mapping.h>
#include <pcl/surface/vtk_smoothing/vtk_mesh_smoothing_laplacian.h>
#include <pcl/surface/vtk_smoothing/vtk_utils.h>
#include <pcl/surface/surfel_smoothing.h>
#include <pcl/console/parse.h> //命令行参数解析
//#include <pcl/surface/vtk_smoothing/vtk_mesh_quadric_decimation.h>

#include <vtkFillHolesFilter.h>
#include <vtkPolyDataNormals.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkVRMLImporter.h>
#include <vtkDataSet.h>
#include <vtkPolyData.h>
#include <vtkSmartPointer.h>
#include <vtkProperty.h>
#include "vtkTransform.h"
#include "vtkAxesActor.h"
#include <pcl/surface/vtk_smoothing/vtk_mesh_quadric_decimation.h>


#include <vtkVRMLExporter.h>
#include <vtkCamera.h>


#include <pcl/common/centroid.h>

#if VTK_MAJOR_VERSION>=6 || (VTK_MAJOR_VERSION==5 && VTK_MINOR_VERSION>6)
#include <pcl/visualization/pcl_plotter.h>
#endif


#include <pcl/keypoints/smoothed_surfaces_keypoint.h>
#include <pcl/conversions.h>


#pragma comment(lib,"pcl_common.lib")
#pragma comment(lib,"pcl_features.lib")
#pragma comment(lib,"pcl_filters.lib")
#pragma comment(lib,"pcl_io_ply.lib")
#pragma comment(lib,"pcl_io.lib")
#pragma comment(lib,"pcl_kdtree.lib")
#pragma comment(lib,"pcl_keypoints.lib")
#pragma comment(lib,"pcl_ml.lib")
#pragma comment(lib,"pcl_octree.lib")
#pragma comment(lib,"pcl_outofcore.lib")
#pragma comment(lib,"pcl_people.lib")
#pragma comment(lib,"pcl_recognition.lib")
#pragma comment(lib,"pcl_registration.lib")
#pragma comment(lib,"pcl_sample_consensus.lib")
#pragma comment(lib,"pcl_search.lib")
#pragma comment(lib,"pcl_segmentation.lib")
#pragma comment(lib,"pcl_stereo.lib")
#pragma comment(lib,"pcl_surface.lib")
#pragma comment(lib,"pcl_tracking.lib")
#pragma comment(lib,"pcl_visualization.lib")
//#pragma comment(lib,"vtkalglib-9.1.lib")
#pragma comment(lib,"vtkChartsCore-9.1.lib")
#pragma comment(lib,"vtkCommonColor-9.1.lib")
#pragma comment(lib,"vtkCommonComputationalGeometry-9.1.lib")
#pragma comment(lib,"vtkCommonCore-9.1.lib")
#pragma comment(lib,"vtkCommonDataModel-9.1.lib")
#pragma comment(lib,"vtkCommonExecutionModel-9.1.lib")
#pragma comment(lib,"vtkCommonMath-9.1.lib")
#pragma comment(lib,"vtkCommonMisc-9.1.lib")
#pragma comment(lib,"vtkCommonSystem-9.1.lib")
#pragma comment(lib,"vtkCommonTransforms-9.1.lib")
#pragma comment(lib,"vtkDICOMParser-9.1.lib")
#pragma comment(lib,"vtkDomainsChemistry-9.1.lib")
#pragma comment(lib,"vtkexodusII-9.1.lib")
#pragma comment(lib,"vtkexpat-9.1.lib")
#pragma comment(lib,"vtkFiltersAMR-9.1.lib")
#pragma comment(lib,"vtkFiltersCore-9.1.lib")
#pragma comment(lib,"vtkFiltersExtraction-9.1.lib")
#pragma comment(lib,"vtkFiltersFlowPaths-9.1.lib")
#pragma comment(lib,"vtkFiltersGeneral-9.1.lib")
#pragma comment(lib,"vtkFiltersGeneric-9.1.lib")
#pragma comment(lib,"vtkFiltersGeometry-9.1.lib")
#pragma comment(lib,"vtkFiltersHybrid-9.1.lib")
#pragma comment(lib,"vtkFiltersHyperTree-9.1.lib")
#pragma comment(lib,"vtkFiltersImaging-9.1.lib")
#pragma comment(lib,"vtkFiltersModeling-9.1.lib")
#pragma comment(lib,"vtkFiltersParallel-9.1.lib")
#pragma comment(lib,"vtkFiltersParallelImaging-9.1.lib")
#pragma comment(lib,"vtkFiltersProgrammable-9.1.lib")
#pragma comment(lib,"vtkFiltersSelection-9.1.lib")
#pragma comment(lib,"vtkFiltersSMP-9.1.lib")
#pragma comment(lib,"vtkFiltersSources-9.1.lib")
#pragma comment(lib,"vtkFiltersStatistics-9.1.lib")
#pragma comment(lib,"vtkFiltersTexture-9.1.lib")
#pragma comment(lib,"vtkFiltersVerdict-9.1.lib")
#pragma comment(lib,"vtkfreetype-9.1.lib")
#pragma comment(lib,"vtkGeovisCore-9.1.lib")
#pragma comment(lib,"vtkhdf5-9.1.lib")
#pragma comment(lib,"vtkhdf5_hl-9.1.lib")
#pragma comment(lib,"vtkImagingColor-9.1.lib")
#pragma comment(lib,"vtkImagingCore-9.1.lib")
#pragma comment(lib,"vtkImagingFourier-9.1.lib")
#pragma comment(lib,"vtkImagingGeneral-9.1.lib")
#pragma comment(lib,"vtkImagingHybrid-9.1.lib")
#pragma comment(lib,"vtkImagingMath-9.1.lib")
#pragma comment(lib,"vtkImagingMorphological-9.1.lib")
#pragma comment(lib,"vtkImagingSources-9.1.lib")
#pragma comment(lib,"vtkImagingStatistics-9.1.lib")
#pragma comment(lib,"vtkImagingStencil-9.1.lib")
#pragma comment(lib,"vtkInfovisCore-9.1.lib")
#pragma comment(lib,"vtkInfovisLayout-9.1.lib")
#pragma comment(lib,"vtkInteractionImage-9.1.lib")
#pragma comment(lib,"vtkInteractionStyle-9.1.lib")
#pragma comment(lib,"vtkInteractionWidgets-9.1.lib")
#pragma comment(lib,"vtkIOAMR-9.1.lib")
#pragma comment(lib,"vtkIOCore-9.1.lib")
#pragma comment(lib,"vtkIOEnSight-9.1.lib")
#pragma comment(lib,"vtkIOExodus-9.1.lib")
#pragma comment(lib,"vtkIOExport-9.1.lib")
#pragma comment(lib,"vtkIOGeometry-9.1.lib")
#pragma comment(lib,"vtkIOImage-9.1.lib")
#pragma comment(lib,"vtkIOImport-9.1.lib")
#pragma comment(lib,"vtkIOInfovis-9.1.lib")
#pragma comment(lib,"vtkIOLegacy-9.1.lib")
#pragma comment(lib,"vtkIOLSDyna-9.1.lib")
#pragma comment(lib,"vtkIOMINC-9.1.lib")
#pragma comment(lib,"vtkIOMovie-9.1.lib")
#pragma comment(lib,"vtkIONetCDF-9.1.lib")
#pragma comment(lib,"vtkIOParallel-9.1.lib")
#pragma comment(lib,"vtkIOParallelXML-9.1.lib")
#pragma comment(lib,"vtkIOPLY-9.1.lib")
#pragma comment(lib,"vtkIOSQL-9.1.lib")
#pragma comment(lib,"vtkIOVideo-9.1.lib")
#pragma comment(lib,"vtkIOXML-9.1.lib")
#pragma comment(lib,"vtkIOXMLParser-9.1.lib")
#pragma comment(lib,"vtkjpeg-9.1.lib")
#pragma comment(lib,"vtkjsoncpp-9.1.lib")
#pragma comment(lib,"vtklibxml2-9.1.lib")
#pragma comment(lib,"vtkmetaio-9.1.lib")
#pragma comment(lib,"vtkNetCDF-9.1.lib")
//#pragma comment(lib,"vtkNetCDF_cxx-9.1.lib")
#pragma comment(lib,"vtkIOOggTheora-9.1.lib")
#pragma comment(lib,"vtkParallelCore-9.1.lib")
#pragma comment(lib,"vtkpng-9.1.lib")
#pragma comment(lib,"vtklibproj-9.1.lib")
#pragma comment(lib,"vtkRenderingAnnotation-9.1.lib")
#pragma comment(lib,"vtkRenderingContext2D-9.1.lib")
#pragma comment(lib,"vtkRenderingCore-9.1.lib")
#pragma comment(lib,"vtkRenderingFreeType-9.1.lib")
#pragma comment(lib,"vtkRenderingImage-9.1.lib")
#pragma comment(lib,"vtkRenderingLabel-9.1.lib")
#pragma comment(lib,"vtkRenderingLOD-9.1.lib")
#pragma comment(lib,"vtkRenderingVolume-9.1.lib")
#pragma comment(lib,"vtksqlite-9.1.lib")
#pragma comment(lib,"vtksys-9.1.lib")
#pragma comment(lib,"vtktiff-9.1.lib")
#pragma comment(lib,"vtkverdict-9.1.lib")
#pragma comment(lib,"vtkViewsContext2D-9.1.lib")
#pragma comment(lib,"vtkViewsCore-9.1.lib")
#pragma comment(lib,"vtkViewsInfovis-9.1.lib")
#pragma comment(lib,"vtkzlib-9.1.lib")

using namespace pcl;
using namespace pcl::io;
using namespace pcl::console;

typedef pcl::PointCloud<pcl::PointXYZ> PointT;
typedef pcl::PointCloud<pcl::PointXYZRGB> PointTRGB;

//typedef pcl::PointCloud<pcl::PointXYZRGBNormal> PointXYZRGBNormal;
typedef pcl::PointCloud<pcl::PointXYZRGBNormal> PointTRGBN;


//typedef PointT::Ptr PointTPtr;
typedef PointTRGB::Ptr PointTRGBPtr;
typedef PointTRGBN::Ptr PointTRGBNPtr;

//#endif


//#ifdef OPENCV_DEF
#include <opencv2/opencv.hpp>  // 包含所有 OpenCV 模块
#pragma comment(lib, "opencv_world450.lib")   // Release 版本
//#endif // OPENCV_DEF


//osg include file
//#ifdef OSG_DEF
#include <osgDB/readfile>
#include <osgDB/writefile>

#include <osg/object>
#include <osg/Node>
#include <osg/group>
#include <osg/fog>
#include <osg/view>
#include <osg/MatrixTransform>  
#include <osg/ProxyNode>
#include <osg/Image>
#include <osg/StateSet>
#include <osg/TextureCubeMap>
#include <osg/TexGen>
#include <osg/TexEnvCombine>
#include <osg/BlendColor>
#include <osg/BlendFunc>
#include <osg/PositionAttitudeTransform>

#include <osgViewer/Viewer>
#include <osgViewer/Renderer>
#include <osgViewer/CompositeViewer>
#include <osgViewer/ViewerEventHandlers>  
#include <osgUtil/optimizer>
#include <osgUtil/HighlightMapGenerator>
#include <osgUtil/LineSegmentIntersector>
#include <osgUtil/SceneView>

#include <osgGA/StateSetManipulator>  
#include <osgGA/GUIEventAdapter>
#include <osgGA/GUIEventHandler>
#include <osgGA/TrackballManipulator>
#include <osgGA/KeySwitchMatrixManipulator>

#include <osgSim/ShapeAttribute>

#include <osgFX/Scribe>
#include <osgFX/BumpMapping>

#include <osgEarth/Map>
#include <osgEarth/MapNode>
#include <osgEarth/GeoData>
#include <osgEarth/ImageLayer>

//#include <osgEarthDrivers/gdal/GDALOptions>
//#include <osgEarthDrivers/tms/TMSOptions>
//#include <osgEarthDrivers\feature_ogr\FeatureCursorOGR>
//#include <osgEarthDrivers\feature_ogr\OGRFeatureOptions>

//#include <osgEarthDrivers/model_feature_stencil/FeatureStencilModelOptions>
//#include <osgEarthDrivers\model_feature_geom\FeatureGeomModelOptions>
//#include <osgEarthDrivers\model_simple\SimpleModelOptions>


#include <osgEarth/EarthManipulator>
#include <osgEarth/Controls>
#include <osgEarth/Common>
#include <osgEarth/Formatter>
#include <osgEarth/LatLongFormatter>
#include <osgEarth/MouseCoordsTool>
#include <osgEarth/AutoClipPlaneHandler>

#include <osgEarth/ImageOverlay>
#include <osgEarth/ImageOverlayEditor>
#include <osgEarth/PlaceNode>
#include <osgEarth/FeatureNode>
#include <osgEarth/LabelNode>
//#include <osgEarth/ScaleDecoration>
#include <osgEarth/CircleNode>
//#include <osgEarth/AnnotationEditing>

#include <osgEarth/TerrainEngineNode>
#include <osgEarth/ElevationQuery>
#include <osgEarth/StringUtils>
#include <osgEarth/Terrain>
#include <osgEarth/Registry>
#include <osgEarth/Geometry>
#include <osgEarth/GeometryRasterizer>
#include <osgEarth/ModelNode>

//#include <osgEarth/AnnotationEvents>

#include <iomanip>

using namespace osgSim;
using namespace osgEarth;
using namespace osgEarth::Util;
using namespace osgEarth::Util::Controls;
using namespace osgViewer;
// using namespace osgEarth::Symbology; // osgEarth 3.x 中已移除
// using namespace osgEarth::Drivers;   // osgEarth 3.x 中已移除
// using namespace osgEarth::Annotation; // 注意：可能已改变，建议暂时移除

//#endif



#include <vector>
#include <map>
#include <list>
#include <sstream>
#include <fstream>
#include <ostream>
#include <iostream>
#include <string>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

using namespace std;

#ifndef COUT_PREFIX
#define COUT_PREFIX FileLibrary::getInstance()->getCurrentTime() << "::" << FileLibrary::getInstance()->getFileNameFromPath(__FILE__) << "::" << __LINE__ << "::"
#endif
// 注意：Windows 可能已经定义了 max/min，所以使用 #ifndef 保护
#ifndef max
#define max(a,b) (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a,b) (((a) < (b)) ? (a) : (b))
#endif