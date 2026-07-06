// testreadpcd.cpp : 定义控制台应用程序的入口点。
//



#include "base.h"

#include "Filelibrary.h"
#include "ConverPointCloud.h"
#include "depth_io.h"
#include <pcl/filters/fast_bilateral.h>
#include <direct.h>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <sstream>

#if 1
static bool ensureDirectoryExists(const string& dir)
{
    if (FileLibrary::getInstance()->isDirExists(dir))
    {
        return true;
    }

    _mkdir(dir.c_str());
    return FileLibrary::getInstance()->isDirExists(dir);
}

static const string MERGED_POINT_CLOUD_FILE = "merged_rgb.ply";
static const string MERGED_MESH_FILE = "merged_mesh.ply";
static const string MERGE_MANIFEST_FILE = "merge_manifest.json";
static const string MERGED_MODEL_OBJ_FILE = "merged_model.obj";
static const string MERGED_MODEL_MTL_FILE = "merged_model.mtl";
static const string MERGED_TEXTURE_FILE = "merged_texture.jpg";

struct MergeConfig
{
    float sourceVoxel;
    float targetVoxel;
    float mergedVoxel;
    float maxCorrespondenceDistance;
    float transformationEpsilon;
    float euclideanFitnessEpsilon;
    int maxIterations;
    float focalLength;

    MergeConfig()
        : sourceVoxel(0.001f),
          targetVoxel(0.001f),
          mergedVoxel(0.001f),
          maxCorrespondenceDistance(0.01f),
          transformationEpsilon(1e-8f),
          euclideanFitnessEpsilon(1e-6f),
          maxIterations(50),
          focalLength(2000.0f)
    {
    }
};

struct MergeView
{
    string viewId;
    string cloudFile;
    string imageFile;
    Eigen::Matrix4f transformToMerged;
    bool converged;
    float fitnessScore;

    MergeView()
        : transformToMerged(Eigen::Matrix4f::Identity()),
          converged(true),
          fitnessScore(0.0f)
    {
    }
};

struct ProjectedUv
{
    float u;
    float v;
    bool visible;

    ProjectedUv() : u(0.0f), v(0.0f), visible(false) {}
};

static void keepRgbCloudForCommandLifetime(PointTRGB*)
{
    // Intentionally no-op; merge keeps PCL objects alive until process exit to avoid PCL teardown crashes.
}

static PointTRGBPtr makeRgbCloudForCommandLifetime()
{
    return PointTRGBPtr(new PointTRGB, keepRgbCloudForCommandLifetime);
}

static bool hasStringSuffix(const string& value, const string& suffix)
{
    if (value.size() < suffix.size())
    {
        return false;
    }
    return value.compare(value.size() - suffix.size(), suffix.size(), suffix) == 0;
}

static string trimCopy(const string& value)
{
    size_t begin = 0;
    while (begin < value.size() && isspace(static_cast<unsigned char>(value[begin])))
    {
        ++begin;
    }

    size_t end = value.size();
    while (end > begin && isspace(static_cast<unsigned char>(value[end - 1])))
    {
        --end;
    }

    return value.substr(begin, end - begin);
}

static string removeExtension(const string& filename)
{
    size_t extPos = filename.find_last_of('.');
    if (extPos == string::npos)
    {
        return filename;
    }
    return filename.substr(0, extPos);
}

static string getViewIdFromCloudPath(const string& cloudPath)
{
    string name = FileLibrary::getInstance()->getFileNameFromPath(cloudPath);
    if (hasStringSuffix(name, "_rgb.ply"))
    {
        name = name.substr(0, name.size() - string("_rgb.ply").size());
    }
    else
    {
        name = removeExtension(name);
    }

    size_t tiffPos = name.find(".tiff");
    if (tiffPos != string::npos)
    {
        name = name.substr(0, tiffPos);
    }
    return name;
}

static string jsonEscape(const string& value)
{
    string result;
    for (size_t i = 0; i < value.size(); ++i)
    {
        if (value[i] == '\\' || value[i] == '"')
        {
            result.push_back('\\');
        }
        result.push_back(value[i]);
    }
    return result;
}

static string matrixToJson(const Eigen::Matrix4f& matrix)
{
    stringstream ss;
    ss << fixed << setprecision(9) << "[";
    for (int r = 0; r < 4; ++r)
    {
        for (int c = 0; c < 4; ++c)
        {
            if (r != 0 || c != 0)
            {
                ss << ", ";
            }
            ss << matrix(r, c);
        }
    }
    ss << "]";
    return ss.str();
}

static string extractJsonStringValue(const string& line)
{
    size_t colon = line.find(':');
    if (colon == string::npos)
    {
        return "";
    }
    size_t firstQuote = line.find('"', colon + 1);
    if (firstQuote == string::npos)
    {
        return "";
    }
    size_t secondQuote = line.find('"', firstQuote + 1);
    if (secondQuote == string::npos)
    {
        return "";
    }
    return line.substr(firstQuote + 1, secondQuote - firstQuote - 1);
}

static bool parseMatrixJsonLine(const string& line, Eigen::Matrix4f& matrix)
{
    if (line.find("identity") != string::npos)
    {
        matrix = Eigen::Matrix4f::Identity();
        return true;
    }

    size_t begin = line.find('[');
    size_t end = line.find(']', begin);
    if (begin == string::npos || end == string::npos || end <= begin)
    {
        return false;
    }

    string values = line.substr(begin + 1, end - begin - 1);
    stringstream ss(values);
    string value;
    vector<float> numbers;
    while (getline(ss, value, ','))
    {
        numbers.push_back(static_cast<float>(atof(value.c_str())));
    }

    if (numbers.size() != 16)
    {
        return false;
    }

    for (int r = 0; r < 4; ++r)
    {
        for (int c = 0; c < 4; ++c)
        {
            matrix(r, c) = numbers[r * 4 + c];
        }
    }
    return true;
}

static string resolvePathInDir(const string& dir, const string& value)
{
    if (value.empty())
    {
        return value;
    }
    if (FileLibrary::getInstance()->isFileExists(value))
    {
        return value;
    }
    return FileLibrary::getInstance()->combineFilePath(dir, value);
}

static string findImageForCloud(const string& cloudPath)
{
    string parent = FileLibrary::getInstance()->getFileParentPath(cloudPath);
    string viewId = getViewIdFromCloudPath(cloudPath);
    string cloudStem = removeExtension(FileLibrary::getInstance()->getFileNameFromPath(cloudPath));
    if (hasStringSuffix(cloudStem, "_rgb"))
    {
        cloudStem = cloudStem.substr(0, cloudStem.size() - 4);
    }

    vector<string> candidates;
    candidates.push_back(FileLibrary::getInstance()->combineFilePath(parent, viewId + ".jpg"));
    candidates.push_back(FileLibrary::getInstance()->combineFilePath(parent, viewId + ".png"));
    candidates.push_back(FileLibrary::getInstance()->combineFilePath(parent, "f" + viewId + ".jpg"));
    candidates.push_back(FileLibrary::getInstance()->combineFilePath(parent, "f" + viewId + ".png"));
    candidates.push_back(FileLibrary::getInstance()->combineFilePath(parent, cloudStem + ".jpg"));
    candidates.push_back(FileLibrary::getInstance()->combineFilePath(parent, cloudStem + ".png"));

    for (size_t i = 0; i < candidates.size(); ++i)
    {
        if (FileLibrary::getInstance()->isFileExists(candidates[i]))
        {
            return FileLibrary::getInstance()->getFileNameFromPath(candidates[i]);
        }
    }
    return "";
}

static bool parseMergeConfig(const string& configFile, MergeConfig& config)
{
    if (!FileLibrary::getInstance()->isFileExists(configFile))
    {
        return false;
    }

    ifstream input(configFile.c_str());
    string line;
    while (getline(input, line))
    {
        line = trimCopy(line);
        if (line.empty() || line[0] == '#')
        {
            continue;
        }

        size_t pos = line.find('=');
        if (pos == string::npos)
        {
            continue;
        }

        string key = trimCopy(line.substr(0, pos));
        string value = trimCopy(line.substr(pos + 1));
        float fvalue = static_cast<float>(atof(value.c_str()));

        if (key == "voxel_grid_size1" || key == "merge_source_voxel")
        {
            config.sourceVoxel = fvalue;
        }
        else if (key == "voxel_grid_size2" || key == "merge_target_voxel")
        {
            config.targetVoxel = fvalue;
        }
        else if (key == "voxel_grid_size3" || key == "merge_voxel" || key == "merged_voxel")
        {
            config.mergedVoxel = fvalue;
        }
        else if (key == "leafsize" && config.mergedVoxel == 0.001f)
        {
            config.mergedVoxel = fvalue;
        }
        else if (key == "max_correspondence_distance")
        {
            config.maxCorrespondenceDistance = fvalue;
        }
        else if (key == "nr_iterations")
        {
            config.maxIterations = atoi(value.c_str());
        }
        else if (key == "euclidean")
        {
            config.euclideanFitnessEpsilon = fvalue;
        }
        else if (key == "transformation_epsilon")
        {
            config.transformationEpsilon = fvalue;
        }
        else if (key == "focus")
        {
            config.focalLength = fvalue;
        }
    }

    return true;
}

static bool loadRgbCloud(const string& path, PointTRGBPtr cloud)
{
    if (pcl::io::loadPLYFile<pcl::PointXYZRGB>(path, *cloud) != 0)
    {
        cout << "Error: Failed to load point cloud: " << path << endl;
        return false;
    }
    if (cloud->points.empty())
    {
        cout << "Error: Point cloud is empty: " << path << endl;
        return false;
    }
    return true;
}

static void voxelFilterRgb(PointTRGBPtr input, PointTRGBPtr output, float leafSize)
{
    if (leafSize <= 0.0f)
    {
        pcl::copyPointCloud(*input, *output);
        return;
    }

    // Keep the PCL filter alive for the process lifetime. On this PCL build,
    // some teardown paths can crash after successful filtering.
    pcl::VoxelGrid<pcl::PointXYZRGB>* voxel = new pcl::VoxelGrid<pcl::PointXYZRGB>();
    voxel->setInputCloud(input);
    voxel->setLeafSize(leafSize, leafSize, leafSize);
    voxel->filter(*output);
}

static PointTRGBPtr mergeWithTransformedRgbCloud(PointTRGBPtr target,
                                                 PointTRGBPtr source,
                                                 const Eigen::Matrix4f& transform)
{
    PointTRGBPtr combined = makeRgbCloudForCommandLifetime();
    combined->points.reserve(target->points.size() + source->points.size());
    for (size_t i = 0; i < target->points.size(); ++i)
    {
        combined->points.push_back(target->points[i]);
    }

    for (size_t i = 0; i < source->points.size(); ++i)
    {
        pcl::PointXYZRGB point = source->points[i];
        Eigen::Vector4f position(point.x, point.y, point.z, 1.0f);
        Eigen::Vector4f transformed = transform * position;
        point.x = transformed.x();
        point.y = transformed.y();
        point.z = transformed.z();
        combined->points.push_back(point);
    }
    combined->width = static_cast<unsigned int>(combined->points.size());
    combined->height = 1;
    combined->is_dense = false;
    return combined;
}

static bool estimateTransformToMerged(PointTRGBPtr sourceCloud,
                                      PointTRGBPtr targetCloud,
                                      const MergeConfig& config,
                                      Eigen::Matrix4f& transform,
                                      bool& converged,
                                      float& fitnessScore)
{
    PointTRGBPtr sourceForIcp = makeRgbCloudForCommandLifetime();
    PointTRGBPtr targetForIcp = makeRgbCloudForCommandLifetime();
    voxelFilterRgb(sourceCloud, sourceForIcp, config.sourceVoxel);
    voxelFilterRgb(targetCloud, targetForIcp, config.targetVoxel);

    if (sourceForIcp->points.empty() || targetForIcp->points.empty())
    {
        cout << "Error: Empty cloud after voxel filtering during merge." << endl;
        return false;
    }

    // PCL 1.12 in this project can crash when the ICP object is destroyed after align().
    // Keep it alive for the command lifetime; merge view count is small and this keeps the pipeline runnable.
    pcl::IterativeClosestPoint<pcl::PointXYZRGB, pcl::PointXYZRGB>* icp =
        new pcl::IterativeClosestPoint<pcl::PointXYZRGB, pcl::PointXYZRGB>();
    icp->setInputSource(sourceForIcp);
    icp->setInputTarget(targetForIcp);
    icp->setMaximumIterations(config.maxIterations);
    icp->setMaxCorrespondenceDistance(config.maxCorrespondenceDistance);
    icp->setTransformationEpsilon(config.transformationEpsilon);
    icp->setEuclideanFitnessEpsilon(config.euclideanFitnessEpsilon);

    PointTRGBPtr aligned = makeRgbCloudForCommandLifetime();
    icp->align(*aligned);

    transform = icp->getFinalTransformation();
    converged = icp->hasConverged();
    fitnessScore = static_cast<float>(icp->getFitnessScore());

    cout << "    ICP converged: " << (converged ? "true" : "false")
         << ", fitness: " << fitnessScore << endl;
    cout << "    Transform to merged:\n" << transform << endl;

    if (!converged)
    {
        cout << "Error: ICP did not converge. Refusing to merge this view with an unreliable transform." << endl;
        return false;
    }

    return !aligned->points.empty();
}

static bool writeMergeManifest(const string& dir, const vector<MergeView>& views)
{
    string manifestPath = FileLibrary::getInstance()->combineFilePath(dir, MERGE_MANIFEST_FILE);
    ofstream output(manifestPath.c_str());
    if (!output.is_open())
    {
        cout << "Error: Failed to write manifest: " << manifestPath << endl;
        return false;
    }

    output << "{\n";
    output << "  \"merged_cloud\": \"" << MERGED_POINT_CLOUD_FILE << "\",\n";
    output << "  \"merged_mesh\": \"" << MERGED_MESH_FILE << "\",\n";
    output << "  \"views\": [\n";
    for (size_t i = 0; i < views.size(); ++i)
    {
        const MergeView& view = views[i];
        output << "    {\n";
        output << "      \"view_id\": \"" << jsonEscape(view.viewId) << "\",\n";
        output << "      \"cloud\": \"" << jsonEscape(view.cloudFile) << "\",\n";
        output << "      \"image\": \"" << jsonEscape(view.imageFile) << "\",\n";
        if (view.transformToMerged.isApprox(Eigen::Matrix4f::Identity()))
        {
            output << "      \"transform_to_merged\": \"identity\",\n";
        }
        else
        {
            output << "      \"transform_to_merged\": " << matrixToJson(view.transformToMerged) << ",\n";
        }
        output << "      \"converged\": " << (view.converged ? "true" : "false") << ",\n";
        output << "      \"fitness_score\": " << fixed << setprecision(9) << view.fitnessScore << "\n";
        output << "    }" << (i + 1 < views.size() ? "," : "") << "\n";
    }
    output << "  ]\n";
    output << "}\n";

    cout << "Merge manifest saved: " << manifestPath << endl;
    return true;
}

static bool readMergeManifest(const string& dir, vector<MergeView>& views)
{
    string manifestPath = FileLibrary::getInstance()->combineFilePath(dir, MERGE_MANIFEST_FILE);
    if (!FileLibrary::getInstance()->isFileExists(manifestPath))
    {
        cout << "Error: Merge manifest not found: " << manifestPath << endl;
        return false;
    }

    ifstream input(manifestPath.c_str());
    string line;
    MergeView current;
    bool inView = false;

    while (getline(input, line))
    {
        if (line.find("\"view_id\"") != string::npos)
        {
            current = MergeView();
            current.viewId = extractJsonStringValue(line);
            inView = true;
        }
        else if (inView && line.find("\"cloud\"") != string::npos)
        {
            current.cloudFile = extractJsonStringValue(line);
        }
        else if (inView && line.find("\"image\"") != string::npos)
        {
            current.imageFile = extractJsonStringValue(line);
        }
        else if (inView && line.find("\"transform_to_merged\"") != string::npos)
        {
            parseMatrixJsonLine(line, current.transformToMerged);
        }
        else if (inView && line.find("\"converged\"") != string::npos)
        {
            current.converged = line.find("true") != string::npos;
        }
        else if (inView && line.find("\"fitness_score\"") != string::npos)
        {
            size_t pos = line.find(':');
            if (pos != string::npos)
            {
                current.fitnessScore = static_cast<float>(atof(line.substr(pos + 1).c_str()));
            }
        }
        else if (inView && line.find("}") != string::npos)
        {
            views.push_back(current);
            inView = false;
        }
    }

    if (views.empty())
    {
        cout << "Error: Merge manifest has no views." << endl;
        return false;
    }
    return true;
}

static ProjectedUv projectPointToView(const pcl::PointXYZ& point,
                                      const Eigen::Matrix4f& mergedToView,
                                      float focalLength,
                                      int imageWidth,
                                      int imageHeight,
                                      bool clamp)
{
    ProjectedUv result;
    Eigen::Vector4f mergedPoint(point.x, point.y, point.z, 1.0f);
    Eigen::Vector4f localPoint = mergedToView * mergedPoint;

    if (localPoint.z() >= -1e-6f)
    {
        return result;
    }

    float cx = imageWidth / 2.0f;
    float cy = imageHeight / 2.0f;
    result.u = 1.0f - ((focalLength * (localPoint.x() / localPoint.z()) + cx) / imageWidth);
    result.v = ((focalLength * (localPoint.y() / localPoint.z()) + cy) / imageHeight);

    if (clamp)
    {
        if (result.u < 0.0f) result.u = 0.0f;
        if (result.u > 1.0f) result.u = 1.0f;
        if (result.v < 0.0f) result.v = 0.0f;
        if (result.v > 1.0f) result.v = 1.0f;
        result.visible = true;
        return result;
    }

    result.visible = result.u >= 0.0f && result.u <= 1.0f && result.v >= 0.0f && result.v <= 1.0f;
    return result;
}

static int chooseBestViewForFace(const pcl::PointCloud<pcl::PointXYZ>& vertices,
                                 const pcl::Vertices& face,
                                 const vector<MergeView>& views,
                                 const vector<Eigen::Matrix4f>& mergedToViewMatrices,
                                 int imageWidth,
                                 int imageHeight,
                                 float focalLength)
{
    int bestView = -1;
    float bestScore = -1.0f;

    for (size_t viewIndex = 0; viewIndex < views.size(); ++viewIndex)
    {
        bool allVisible = true;
        vector<ProjectedUv> projected;
        for (size_t i = 0; i < face.vertices.size(); ++i)
        {
            ProjectedUv uv = projectPointToView(
                vertices.points[face.vertices[i]],
                mergedToViewMatrices[viewIndex],
                focalLength,
                imageWidth,
                imageHeight,
                false);
            if (!uv.visible)
            {
                allVisible = false;
                break;
            }
            projected.push_back(uv);
        }

        if (!allVisible || projected.size() < 3)
        {
            continue;
        }

        float area = 0.0f;
        for (size_t i = 1; i + 1 < projected.size(); ++i)
        {
            float ax = projected[i].u - projected[0].u;
            float ay = projected[i].v - projected[0].v;
            float bx = projected[i + 1].u - projected[0].u;
            float by = projected[i + 1].v - projected[0].v;
            area += fabs(ax * by - ay * bx) * 0.5f;
        }

        if (area > bestScore)
        {
            bestScore = area;
            bestView = static_cast<int>(viewIndex);
        }
    }

    return bestView >= 0 ? bestView : 0;
}

static bool writeMergedTexturedModel(const string& dir,
                                     const string& meshPath,
                                     const vector<MergeView>& views,
                                     const MergeConfig& config)
{
    pcl::PolygonMesh mesh;
    if (pcl::io::loadPLYFile(meshPath, mesh) != 0)
    {
        cout << "Error: Failed to load merged mesh: " << meshPath << endl;
        return false;
    }

    pcl::PointCloud<pcl::PointXYZ> vertices;
    pcl::fromPCLPointCloud2(mesh.cloud, vertices);
    if (vertices.points.empty() || mesh.polygons.empty())
    {
        cout << "Error: Merged mesh has no vertices or faces." << endl;
        return false;
    }

    vector<cv::Mat> sourceImages;
    for (size_t i = 0; i < views.size(); ++i)
    {
        string imagePath = resolvePathInDir(dir, views[i].imageFile);
        cv::Mat image = cv::imread(imagePath);
        if (image.empty())
        {
            cout << "Error: Failed to read view image: " << imagePath << endl;
            return false;
        }
        sourceImages.push_back(image);
    }

    int tileWidth = 0;
    int tileHeight = 0;
    for (size_t i = 0; i < sourceImages.size(); ++i)
    {
        if (sourceImages[i].cols > tileWidth) tileWidth = sourceImages[i].cols;
        if (sourceImages[i].rows > tileHeight) tileHeight = sourceImages[i].rows;
    }

    cv::Mat atlas(tileHeight, tileWidth * static_cast<int>(sourceImages.size()), CV_8UC3, cv::Scalar(0, 0, 0));
    for (size_t i = 0; i < sourceImages.size(); ++i)
    {
        cv::Mat resized;
        cv::resize(sourceImages[i], resized, cv::Size(tileWidth, tileHeight));
        cv::Rect roi(static_cast<int>(i) * tileWidth, 0, tileWidth, tileHeight);
        resized.copyTo(atlas(roi));
    }

    string texturePath = FileLibrary::getInstance()->combineFilePath(dir, MERGED_TEXTURE_FILE);
    if (!cv::imwrite(texturePath, atlas))
    {
        cout << "Error: Failed to write merged texture: " << texturePath << endl;
        return false;
    }

    vector<Eigen::Matrix4f> mergedToViewMatrices;
    for (size_t i = 0; i < views.size(); ++i)
    {
        mergedToViewMatrices.push_back(views[i].transformToMerged.inverse());
    }

    string mtlPath = FileLibrary::getInstance()->combineFilePath(dir, MERGED_MODEL_MTL_FILE);
    ofstream mtl(mtlPath.c_str());
    if (!mtl.is_open())
    {
        cout << "Error: Failed to write MTL: " << mtlPath << endl;
        return false;
    }
    mtl << "newmtl merged_material\n";
    mtl << "Ka 1.000000 1.000000 1.000000\n";
    mtl << "Kd 1.000000 1.000000 1.000000\n";
    mtl << "Ks 0.000000 0.000000 0.000000\n";
    mtl << "d 1.000000\n";
    mtl << "illum 2\n";
    mtl << "map_Kd " << MERGED_TEXTURE_FILE << "\n";
    mtl.close();

    string objPath = FileLibrary::getInstance()->combineFilePath(dir, MERGED_MODEL_OBJ_FILE);
    ofstream obj(objPath.c_str());
    if (!obj.is_open())
    {
        cout << "Error: Failed to write OBJ: " << objPath << endl;
        return false;
    }

    obj << "mtllib " << MERGED_MODEL_MTL_FILE << "\n";
    obj << "o merged_model\n";
    obj << fixed << setprecision(7);
    for (size_t i = 0; i < vertices.points.size(); ++i)
    {
        const pcl::PointXYZ& p = vertices.points[i];
        obj << "v " << p.x << " " << p.y << " " << p.z << "\n";
    }
    obj << "usemtl merged_material\n";

    int textureCoordIndex = 1;
    int assignedFaces = 0;
    for (size_t faceIndex = 0; faceIndex < mesh.polygons.size(); ++faceIndex)
    {
        const pcl::Vertices& face = mesh.polygons[faceIndex];
        if (face.vertices.size() < 3)
        {
            continue;
        }

        int viewIndex = chooseBestViewForFace(vertices, face, views, mergedToViewMatrices, tileWidth, tileHeight, config.focalLength);
        vector<int> faceTextureIndices;

        for (size_t i = 0; i < face.vertices.size(); ++i)
        {
            ProjectedUv uv = projectPointToView(
                vertices.points[face.vertices[i]],
                mergedToViewMatrices[viewIndex],
                config.focalLength,
                tileWidth,
                tileHeight,
                true);

            float atlasU = (static_cast<float>(viewIndex) + uv.u) / static_cast<float>(views.size());
            float atlasV = 1.0f - uv.v;
            obj << "vt " << atlasU << " " << atlasV << "\n";
            faceTextureIndices.push_back(textureCoordIndex++);
        }

        obj << "f";
        for (size_t i = 0; i < face.vertices.size(); ++i)
        {
            obj << " " << (face.vertices[i] + 1) << "/" << faceTextureIndices[i];
        }
        obj << "\n";
        assignedFaces++;
    }

    cout << "Merged texture saved: " << texturePath << endl;
    cout << "Merged OBJ saved: " << objPath << endl;
    cout << "Merged MTL saved: " << mtlPath << endl;
    cout << "Textured faces: " << assignedFaces << endl;
    return true;
}

// Convert depth images to point clouds
void pointcloudFunc(const string& dir,const  string& config) 
{
    cout << "========================================" << endl;
    cout << "Point Cloud Generation Process Started" << endl;
    cout << "========================================" << endl;
    cout << "Input directory: " << dir << endl;
    cout << "Config file: " << config << endl;
    cout << "----------------------------------------" << endl;

    string outputDir = dir;
    cout << "Output directory: " << outputDir << endl;
    if (!ensureDirectoryExists(outputDir))
    {
        cout << "Error: Unable to create output directory: " << outputDir << endl;
        return;
    }

    depthImage depth(0);

    list<string>listfile;
    // Find all .tiff files (depth images)
    cout << "Searching for .tiff files in directory..." << endl;
    FileLibrary::getInstance()->getAllSubFiles(dir, listfile, false, true, false, ".tiff");
    
    int totalFiles = listfile.size();
    cout << "Found " << totalFiles << " .tiff file(s)" << endl;
    
    if (totalFiles == 0) {
        cout << "Warning: No .tiff files found in the specified directory!" << endl;
        return;
    }
    
    list<string>::iterator it = listfile.begin();
    int processedCount = 0;
    int successCount = 0;
    int failCount = 0;

    for (it; it != listfile.end(); it++)
    {
        processedCount++;
        string depthfile = *it;
        string filename = FileLibrary::getInstance()->getFileNameFromPath(depthfile);
        
        cout << "\n[" << processedCount << "/" << totalFiles << "] Processing: " << filename << endl;

        if (!FileLibrary::getInstance()->isFileExists(depthfile))
        {
            cout << "  Error: Depth file not found: " << depthfile << endl;
            failCount++;
            continue;
        }

        // Extract base name from filename (remove .tiff extension)
        string baseName = filename;
        size_t pos = baseName.find(".tiff");
        if (pos != string::npos) {
            baseName = baseName.substr(0, pos);
        }
        
        string parentPath = FileLibrary::getInstance()->getFileParentPath(depthfile);
        vector<string> rgbCandidates;
        rgbCandidates.push_back(parentPath + "\\" + baseName + ".jpg");
        rgbCandidates.push_back(parentPath + "\\f" + baseName + ".jpg");
        rgbCandidates.push_back(parentPath + "\\" + baseName + ".png");
        rgbCandidates.push_back(parentPath + "\\f" + baseName + ".png");

        string rgb_png;
        for (size_t rgbIndex = 0; rgbIndex < rgbCandidates.size(); ++rgbIndex)
        {
            if (FileLibrary::getInstance()->isFileExists(rgbCandidates[rgbIndex]))
            {
                rgb_png = rgbCandidates[rgbIndex];
                break;
            }
        }
        
        cout << "  Depth image: " << depthfile << endl;
        cout << "  RGB image: " << (rgb_png.empty() ? "(not found)" : rgb_png) << endl;
        
        if (rgb_png.empty())
        {
            cout << "  Error: RGB file not found for view: " << baseName << endl;
            cout << "  Skipping this depth image..." << endl;
            failCount++;
            continue;
        }

        cout << "  Converting depth image to point cloud..." << endl;
        if (depth.depthToPlyColor(depthfile, rgb_png, config, outputDir)) {
            cout << "  Success: Point cloud file created: " << depth.getFlyFile() << endl;
            successCount++;
        }
        else {
            cout << "  Failed: Unable to create point cloud file: " << depth.getFlyFile() << endl;
            failCount++;
        }
    }
    
    cout << "\n========================================" << endl;
    cout << "Point Cloud Generation Process Completed" << endl;
    cout << "========================================" << endl;
    cout << "Total files processed: " << processedCount << endl;
    cout << "Successfully converted: " << successCount << endl;
    cout << "Failed: " << failCount << endl;
    cout << "========================================" << endl;
};
#endif
// Register and fuse point clouds into one merged cloud plus manifest.
void mergeFunc(const string& dir, const string& config)
{
    cout << "========================================" << endl;
    cout << "Point Cloud Merge Process Started" << endl;
    cout << "========================================" << endl;
    cout << "Input directory: " << dir << endl;
    cout << "Config file: " << config << endl;
    cout << "----------------------------------------" << endl;

    MergeConfig mergeConfig;
    parseMergeConfig(config, mergeConfig);

    list<string> pointCloudFiles;
    FileLibrary::getInstance()->getAllSubFiles(dir, pointCloudFiles, false, true, false, "_rgb.ply");
    pointCloudFiles.sort();

    list<string> sourcePointCloudFiles;
    for (list<string>::const_iterator it = pointCloudFiles.begin(); it != pointCloudFiles.end(); ++it)
    {
        string filename = FileLibrary::getInstance()->getFileNameFromPath(*it);
        if (filename != MERGED_POINT_CLOUD_FILE)
        {
            sourcePointCloudFiles.push_back(*it);
        }
    }
    pointCloudFiles = sourcePointCloudFiles;

    cout << "Found " << pointCloudFiles.size() << " source point cloud file(s)" << endl;
    if (pointCloudFiles.empty())
    {
        cout << "Error: No source *_rgb.ply files found. Run -point first." << endl;
        return;
    }

    vector<MergeView> views;
    PointTRGBPtr mergedCloud = makeRgbCloudForCommandLifetime();
    int index = 0;

    for (list<string>::const_iterator it = pointCloudFiles.begin(); it != pointCloudFiles.end(); ++it, ++index)
    {
        string cloudPath = *it;
        string cloudFile = FileLibrary::getInstance()->getFileNameFromPath(cloudPath);
        string viewId = getViewIdFromCloudPath(cloudPath);
        string imageFile = findImageForCloud(cloudPath);

        cout << "\n[" << (index + 1) << "/" << pointCloudFiles.size() << "] Merging: " << cloudFile << endl;
        if (imageFile.empty())
        {
            cout << "Error: Could not find RGB image for cloud: " << cloudFile << endl;
            cout << "Expected names include " << viewId << ".jpg or f" << viewId << ".jpg" << endl;
            return;
        }

        PointTRGBPtr sourceCloud = makeRgbCloudForCommandLifetime();
        if (!loadRgbCloud(cloudPath, sourceCloud))
        {
            return;
        }

        MergeView view;
        view.viewId = viewId;
        view.cloudFile = cloudFile;
        view.imageFile = imageFile;

        if (index == 0)
        {
            pcl::copyPointCloud(*sourceCloud, *mergedCloud);
            view.transformToMerged = Eigen::Matrix4f::Identity();
            view.converged = true;
            view.fitnessScore = 0.0f;
            cout << "    First cloud is the merged coordinate frame." << endl;
        }
        else
        {
            Eigen::Matrix4f transform = Eigen::Matrix4f::Identity();
            bool converged = false;
            float fitnessScore = 0.0f;
            if (!estimateTransformToMerged(sourceCloud, mergedCloud, mergeConfig, transform, converged, fitnessScore))
            {
                cout << "Error: Failed to estimate transform for " << cloudFile << endl;
                return;
            }

            mergedCloud = mergeWithTransformedRgbCloud(mergedCloud, sourceCloud, transform);

            PointTRGBPtr filteredMerged = makeRgbCloudForCommandLifetime();
            voxelFilterRgb(mergedCloud, filteredMerged, mergeConfig.mergedVoxel);
            mergedCloud = filteredMerged;

            view.transformToMerged = transform;
            view.converged = converged;
            view.fitnessScore = fitnessScore;
        }

        mergedCloud->width = static_cast<unsigned int>(mergedCloud->points.size());
        mergedCloud->height = 1;
        mergedCloud->is_dense = false;
        views.push_back(view);
        cout << "    Merged point count: " << mergedCloud->points.size() << endl;
    }

    string mergedPath = FileLibrary::getInstance()->combineFilePath(dir, MERGED_POINT_CLOUD_FILE);
    if (pcl::io::savePLYFile(mergedPath, *mergedCloud) != 0)
    {
        cout << "Error: Failed to save merged point cloud: " << mergedPath << endl;
        return;
    }

    cout << "\nMerged cloud saved: " << mergedPath << endl;
    if (!writeMergeManifest(dir, views))
    {
        return;
    }

    cout << "========================================" << endl;
    cout << "Point Cloud Merge Process Completed" << endl;
    cout << "========================================" << endl;
}

// Convert point clouds to mesh
void meshFunc(const string& dir, const string& config) {
    cout << "========================================" << endl;
    cout << "Mesh Generation Process Started" << endl;
    cout << "========================================" << endl;
    cout << "Input directory: " << dir << endl;
    cout << "Config file: " << config << endl;
    cout << "----------------------------------------" << endl;

    string outputDir = dir;
    cout << "Output directory: " << outputDir << endl;
    if (!ensureDirectoryExists(outputDir))
    {
        cout << "Error: Unable to create output directory: " << outputDir << endl;
        return;
    }

    string mergedPointCloudPath = FileLibrary::getInstance()->combineFilePath(outputDir, MERGED_POINT_CLOUD_FILE);
    if (!FileLibrary::getInstance()->isFileExists(mergedPointCloudPath))
    {
        cout << "Error: Merged point cloud not found: " << mergedPointCloudPath << endl;
        cout << "Run -merge before -mesh. Mesh generation will not process per-view clouds." << endl;
        return;
    }

    ConverPointCloud convertomesh;
    cout << "\nGenerating one mesh from merged point cloud..." << endl;
    bool result = convertomesh.meshAPI(mergedPointCloudPath, config, outputDir);

    if (result) {
        cout << "  Success: Mesh generated from merged point cloud." << endl;
        cout << "  Expected mesh: " << FileLibrary::getInstance()->combineFilePath(outputDir, MERGED_MESH_FILE) << endl;
    }
    else {
        cout << "  Error: Mesh generation failed for merged point cloud." << endl;
        cout << "  Possible causes:" << endl;
        cout << "    - Point cloud missing normals" << endl;
        cout << "    - Point cloud too sparse or insufficient points" << endl;
        cout << "    - Incorrect reconstruction parameters" << endl;
        cout << "  Suggestion: Check the error messages above and adjust config parameters." << endl;
    }
    
    cout << "\n========================================" << endl;
    cout << "Mesh Generation Process Completed" << endl;
    cout << "Input point cloud: " << MERGED_POINT_CLOUD_FILE << endl;
    cout << "========================================" << endl;
}


// Generate textured model from mesh
//************************************
// Method:    modelFunc
// Access:    public 
// Returns:   void
// Describe:  Process mesh files in directory and generate textured models
// Parameter: const string & dir
//************************************
void modelFunc(const string& dir, const string& config) {
    cout << "========================================" << endl;
    cout << "Textured Model Generation Process Started" << endl;
    cout << "========================================" << endl;
    cout << "Input directory: " << dir << endl;
    cout << "Config file: " << config << endl;
    cout << "----------------------------------------" << endl;

    vector<MergeView> views;
    if (!readMergeManifest(dir, views))
    {
        return;
    }

    MergeConfig modelConfig;
    parseMergeConfig(config, modelConfig);

    string meshPath = FileLibrary::getInstance()->combineFilePath(dir, MERGED_MESH_FILE);
    if (!FileLibrary::getInstance()->isFileExists(meshPath))
    {
        cout << "Error: Merged mesh not found: " << meshPath << endl;
        cout << "Run -mesh before -model/-texture." << endl;
        return;
    }

    if (!writeMergedTexturedModel(dir, meshPath, views, modelConfig))
    {
        cout << "Failed: Unable to generate merged textured model." << endl;
        return;
    }

    cout << "\n========================================" << endl;
    cout << "Textured Model Generation Process Completed" << endl;
    cout << "Output OBJ: " << MERGED_MODEL_OBJ_FILE << endl;
    cout << "Output MTL: " << MERGED_MODEL_MTL_FILE << endl;
    cout << "Output texture: " << MERGED_TEXTURE_FILE << endl;
    cout << "========================================" << endl;
}


int main(int argc, char* argv[]) {
    cout << "========================================" << endl;
    cout << "3D Holographic Face Processing Tool" << endl;
    cout << "========================================" << endl;

    if (argc < 3) {
        cout << "\nUsage:" << endl;
        cout << "  Depth to Point Cloud:  program.exe -point <directory> -config <config_file>" << endl;
        cout << "  Merge Point Clouds:    program.exe -merge <directory> -config <config_file>" << endl;
        cout << "  Merged Cloud to Mesh:  program.exe -mesh <directory> -config <config_file>" << endl;
        cout << "  Mesh to Textured Model: program.exe -model <directory> -config <config_file>" << endl;
        cout << "  Mesh to Textured Model: program.exe -texture <directory> -config <config_file>" << endl;
        cout << "\nExample:" << endl;
        cout << "  program.exe -point \"C:\\data\" -config \"C:\\data\\config.cfg\"" << endl;
        cout << "  program.exe -merge \"C:\\data\" -config \"C:\\data\\merge.cfg\"" << endl;
        system("pause");
        return 0;
    }

    string mesh, config;
    bool model = false, point = false, mesh_flag = false, merge = false;
    string dir;

    cout << "\nParsing command line arguments..." << endl;
    
    if (pcl::console::parse_argument(argc, argv, "-point", dir) >= 0)
    {
    	point = true;
        cout << "  Mode: Depth to Point Cloud" << endl;
        cout << "  Directory: " << dir << endl;
    }
    if (pcl::console::parse_argument(argc, argv, "-mesh", dir) >= 0)
    {
        mesh_flag = true;
        cout << "  Mode: Point Cloud to Mesh" << endl;
        cout << "  Directory: " << dir << endl;
    }
    if (pcl::console::parse_argument(argc, argv, "-merge", dir) >= 0)
    {
        merge = true;
        cout << "  Mode: Point Cloud Registration and Merge" << endl;
        cout << "  Directory: " << dir << endl;
    }
    if (pcl::console::parse_argument(argc, argv, "-model", dir) >= 0)
    {
        model = true;
        cout << "  Mode: Mesh to Textured Model" << endl;
        cout << "  Directory: " << dir << endl;
    }
    if (pcl::console::parse_argument(argc, argv, "-texture", dir) >= 0)
    {
        model = true;
        cout << "  Mode: Mesh to Textured Model" << endl;
        cout << "  Directory: " << dir << endl;
    }

    if (pcl::console::parse_argument(argc, argv, "-config", config) >= 0)
    {
        cout << "  Config file: " << config << endl;
        
        // Verify config file exists
        if (!FileLibrary::getInstance()->isFileExists(config)) {
            cout << "\nError: Config file not found: " << config << endl;
            system("pause");
            return -1;
        }
    }
    else {
        cout << "\nError: Missing required parameter -config <config_file_path>" << endl;
        cout << "Please specify the configuration file path using -config option." << endl;
        system("pause");
        return -1;
    }

    // Verify input directory exists
    if (!FileLibrary::getInstance()->isDirExists(dir)) {
        cout << "\nError: Input directory not found: " << dir << endl;
        system("pause");
        return -1;
    }

    cout << "\nStarting processing..." << endl;
    cout << "----------------------------------------" << endl;

    if (point)
    {
        pointcloudFunc(dir, config);
    }
    else if (merge)
    {
        mergeFunc(dir, config);
    }
    else if (mesh_flag)
    {
        meshFunc(dir, config);
    }
    else if (model) {
        modelFunc(dir, config);
    }
    else {
        cout << "\nError: Must specify one of the following modes:" << endl;
        cout << "  -point  : Convert depth images to point clouds" << endl;
        cout << "  -merge  : Register and fuse point clouds" << endl;
        cout << "  -mesh   : Convert merged point cloud to mesh" << endl;
        cout << "  -model  : Generate textured model from merged mesh" << endl;
        cout << "  -texture: Generate textured model from merged mesh" << endl;
    }

    cout << "\n========================================" << endl;
    cout << "Program execution completed." << endl;
    cout << "========================================" << endl;
    system("pause");

    return 0;
}
