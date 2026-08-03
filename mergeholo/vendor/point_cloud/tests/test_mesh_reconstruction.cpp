#include "ConverPointCloud.h"
#include "poissonmesh.hpp"

#include <pcl/io/ply_io.h>

#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <string>

#ifdef _WIN32
#include <process.h>
#endif

namespace fs = std::filesystem;

namespace {

void expect(bool condition, const char* message)
{
    if (!condition) {
        std::cerr << "check failed: " << message << '\n';
        std::exit(1);
    }
}

class TempDirectory {
public:
    TempDirectory()
    {
        const auto ticks = std::chrono::steady_clock::now().time_since_epoch().count();
#ifdef _WIN32
        const int processId = _getpid();
#else
        const int processId = 0;
#endif
        path_ = fs::temp_directory_path()
            / ("mergeholo_mesh_reconstruction_"
                + std::to_string(processId) + "_" + std::to_string(ticks));
        fs::create_directories(path_);
    }

    ~TempDirectory()
    {
        std::error_code error;
        fs::remove_all(path_, error);
    }

    fs::path writeConfig(int reconstruct) const
    {
        const fs::path path = path_ / ("mesh_" + std::to_string(reconstruct) + ".cfg");
        std::ofstream output(path);
        output
            << "reconstruct=" << reconstruct << "\n"
            << "kSearch=20\n"
            << "searchradius=0.012\n"
            << "mu=2.5\n"
            << "maximumNearestNeighbors=100\n"
            << "maximumSurfaceAngle=45\n"
            << "minimumAngle=10\n"
            << "maximumAngle=120\n"
            << "holesize=0\n"
            << "focus=2000\n"
            << "leafsize=0\n"
            << "mlsSearchRadius=0.01\n"
            << "normalsFitIter1=1\n"
            << "normalsFitIter2=1\n"
            << "neighbor_num=20\n"
            << "nearest_distance=0.01\n";
        return path;
    }

    const fs::path& path() const
    {
        return path_;
    }

private:
    fs::path path_;
};

pcl::PointCloud<pcl::PointXYZRGB>::Ptr makeSphereCloud()
{
    constexpr double pi = 3.14159265358979323846;
    constexpr int latitudeCount = 36;
    constexpr int longitudeCount = 72;
    constexpr float radius = 0.05f;
    constexpr float centerZ = 0.15f;

    pcl::PointCloud<pcl::PointXYZRGB>::Ptr cloud(
        new pcl::PointCloud<pcl::PointXYZRGB>);
    cloud->reserve((latitudeCount - 1) * longitudeCount);
    for (int latitude = 1; latitude < latitudeCount; ++latitude) {
        const double phi = pi * latitude / latitudeCount;
        for (int longitude = 0; longitude < longitudeCount; ++longitude) {
            const double theta = 2.0 * pi * longitude / longitudeCount;
            pcl::PointXYZRGB point;
            point.x = radius * static_cast<float>(std::sin(phi) * std::cos(theta));
            point.y = radius * static_cast<float>(std::sin(phi) * std::sin(theta));
            point.z = centerZ + radius * static_cast<float>(std::cos(phi));
            point.r = static_cast<unsigned char>(40 + latitude * 4);
            point.g = static_cast<unsigned char>(40 + longitude * 2);
            point.b = 180;
            cloud->push_back(point);
        }
    }
    cloud->width = static_cast<std::uint32_t>(cloud->size());
    cloud->height = 1;
    cloud->is_dense = true;
    return cloud;
}

std::string readProjectFile(const fs::path& relativePath)
{
    const fs::path candidates[] = {
        relativePath,
        fs::current_path() / relativePath,
        fs::current_path() / ".." / ".." / ".." / relativePath
    };
    for (const fs::path& candidate : candidates) {
        std::ifstream input(candidate, std::ios::binary);
        if (input) {
            return std::string(
                std::istreambuf_iterator<char>(input),
                std::istreambuf_iterator<char>());
        }
    }
    expect(false, "project source fixture could not be located");
    return {};
}

std::size_t meshVertexCount(const pcl::PolygonMesh& mesh)
{
    return static_cast<std::size_t>(mesh.cloud.width)
        * static_cast<std::size_t>(mesh.cloud.height);
}

bool hasMeshField(const pcl::PolygonMesh& mesh, const std::string& name)
{
    for (const pcl::PCLPointField& field : mesh.cloud.fields) {
        if (field.name == name) {
            return true;
        }
    }
    return false;
}

pcl::PointCloud<pcl::PointXYZRGBNormal>::Ptr makeColorTransferSource()
{
    pcl::PointCloud<pcl::PointXYZRGBNormal>::Ptr source(
        new pcl::PointCloud<pcl::PointXYZRGBNormal>);
    const struct {
        float x;
        float y;
        unsigned char r;
        unsigned char g;
        unsigned char b;
    } values[] = {
        {0.0f, 0.0f, 255, 0, 0},
        {2.0f, 0.0f, 0, 255, 0},
        {0.0f, 2.0f, 0, 0, 255}
    };
    for (const auto& value : values) {
        pcl::PointXYZRGBNormal point;
        point.x = value.x;
        point.y = value.y;
        point.z = 0.0f;
        point.r = value.r;
        point.g = value.g;
        point.b = value.b;
        point.normal_x = 0.0f;
        point.normal_y = 0.0f;
        point.normal_z = 1.0f;
        source->push_back(point);
    }
    source->width = static_cast<std::uint32_t>(source->size());
    source->height = 1;
    source->is_dense = true;
    return source;
}

pcl::PolygonMesh makeColorTransferMesh()
{
    pcl::PointCloud<pcl::PointXYZ> vertices;
    vertices.push_back(pcl::PointXYZ(0.0f, 0.0f, 0.0f));
    vertices.push_back(pcl::PointXYZ(1.0f, 1.0f, 0.0f));
    vertices.push_back(pcl::PointXYZ(1.0f, 0.0f, 0.0f));
    vertices.width = static_cast<std::uint32_t>(vertices.size());
    vertices.height = 1;

    pcl::PolygonMesh mesh;
    pcl::toPCLPointCloud2(vertices, mesh.cloud);
    pcl::Vertices triangle;
    triangle.vertices = {0, 1, 2};
    mesh.polygons.push_back(triangle);
    return mesh;
}

void testPoissonColorTransferUsesSpatialNeighbors()
{
    const pcl::PolygonMesh geometry = makeColorTransferMesh();
    pcl::PolygonMesh colored;
    expect(transferPoissonMeshColors(
        geometry, makeColorTransferSource(), colored),
        "Poisson color transfer must succeed for a valid mesh and source cloud");
    expect(hasMeshField(colored, "rgb"),
        "Poisson color transfer must add the packed RGB field");
    expect(meshVertexCount(colored) == meshVertexCount(geometry),
        "Poisson color transfer must preserve vertex count");
    expect(colored.polygons.size() == geometry.polygons.size()
            && colored.polygons[0].vertices
                == geometry.polygons[0].vertices,
        "Poisson color transfer must preserve polygon indices");

    pcl::PointCloud<pcl::PointXYZ> geometryPoints;
    pcl::fromPCLPointCloud2(geometry.cloud, geometryPoints);
    pcl::PointCloud<pcl::PointXYZRGB> points;
    pcl::fromPCLPointCloud2(colored.cloud, points);
    expect(points.size() == geometryPoints.size(),
        "Poisson color transfer must preserve the ordered vertex list");
    for (std::size_t i = 0; i < points.size(); ++i) {
        expect(std::abs(points[i].x - geometryPoints[i].x) < 1.0e-6f
                && std::abs(points[i].y - geometryPoints[i].y) < 1.0e-6f
                && std::abs(points[i].z - geometryPoints[i].z) < 1.0e-6f,
            "Poisson color transfer must preserve ordered XYZ coordinates");
        expect(points[i].a == 255,
            "Poisson color transfer must write an opaque alpha channel");
    }
    expect(points[0].r == 255 && points[0].g == 0 && points[0].b == 0,
        "an exact coordinate match must copy the exact source color");
    expect(std::abs(static_cast<int>(points[1].r) - 85) <= 1
            && std::abs(static_cast<int>(points[1].g) - 85) <= 1
            && std::abs(static_cast<int>(points[1].b) - 85) <= 1,
        "three equidistant neighbors must produce their average color");
    expect(std::abs(static_cast<int>(points[2].r) - 104) <= 1
            && std::abs(static_cast<int>(points[2].g) - 104) <= 1
            && std::abs(static_cast<int>(points[2].b) - 47) <= 1,
        "non-equidistant neighbors must use inverse Euclidean distance weights");
}

void testPoissonColorTransferRejectsEmptySource()
{
    pcl::PolygonMesh colored;
    pcl::PointCloud<pcl::PointXYZRGBNormal>::Ptr empty(
        new pcl::PointCloud<pcl::PointXYZRGBNormal>);
    expect(!transferPoissonMeshColors(
        makeColorTransferMesh(), empty, colored),
        "Poisson color transfer must reject an empty source cloud");
    expect(colored.cloud.data.empty() && colored.polygons.empty(),
        "failed Poisson color transfer must not return a partial mesh");
}

void testPoissonColorTransferRejectsMalformedMeshSchema()
{
    pcl::PointCloud<pcl::Normal> normals;
    normals.resize(3);
    normals.width = 3;
    normals.height = 1;

    pcl::PolygonMesh malformed = makeColorTransferMesh();
    pcl::toPCLPointCloud2(normals, malformed.cloud);
    pcl::PolygonMesh colored;
    expect(!transferPoissonMeshColors(
        malformed, makeColorTransferSource(), colored),
        "Poisson color transfer must reject a mesh without XYZ fields");
    expect(colored.cloud.data.empty() && colored.polygons.empty(),
        "malformed Poisson mesh input must fail without partial output");
}

void testPoissonColorTransferRejectsNonFiniteCoordinates()
{
    pcl::PointCloud<pcl::PointXYZ> vertices;
    vertices.push_back(pcl::PointXYZ(
        std::numeric_limits<float>::quiet_NaN(), 0.0f, 0.0f));
    vertices.push_back(pcl::PointXYZ(1.0f, 1.0f, 0.0f));
    vertices.push_back(pcl::PointXYZ(1.0f, 0.0f, 0.0f));
    vertices.width = 3;
    vertices.height = 1;

    pcl::PolygonMesh malformed = makeColorTransferMesh();
    pcl::toPCLPointCloud2(vertices, malformed.cloud);
    pcl::PolygonMesh colored;
    expect(!transferPoissonMeshColors(
        malformed, makeColorTransferSource(), colored),
        "Poisson color transfer must reject non-finite vertex coordinates");
    expect(colored.cloud.data.empty() && colored.polygons.empty(),
        "non-finite Poisson mesh input must fail without partial output");
}

void testPoissonReconstructionReturnsAndPersistsRgb()
{
    const TempDirectory temp;
    const fs::path output = temp.path() / "colored_mesh.ply";
    pcl::PolygonMesh mesh;
    ConverPointCloud converter;
    expect(converter.meshAPIFromCloud(
        makeSphereCloud(),
        (temp.path() / "colored_rgb.ply").string(),
        temp.writeConfig(1).string(),
        temp.path().string(),
        &mesh,
        false),
        "Poisson RGB integration reconstruction must succeed");
    expect(hasMeshField(mesh, "rgb"),
        "successful Poisson reconstruction must return RGB vertices");
    pcl::PointCloud<pcl::PointXYZRGB> coloredPoints;
    pcl::fromPCLPointCloud2(mesh.cloud, coloredPoints);
    bool foundMeaningfulColor = false;
    bool foundColorVariation = false;
    for (const pcl::PointXYZRGB& point : coloredPoints) {
        foundMeaningfulColor = foundMeaningfulColor
            || point.r != 0 || point.g != 0 || point.b != 0;
        foundColorVariation = foundColorVariation
            || point.r != coloredPoints.front().r
            || point.g != coloredPoints.front().g
            || point.b != coloredPoints.front().b;
    }
    expect(foundMeaningfulColor && foundColorVariation,
        "successful Poisson reconstruction must transfer meaningful RGB values");
    expect(pcl::io::savePLYFile(output.string(), mesh) == 0,
        "colored Poisson mesh must persist as PLY");
    const std::string ply = readProjectFile(output);
    expect(ply.find("property uchar red") != std::string::npos
            && ply.find("property uchar green") != std::string::npos
            && ply.find("property uchar blue") != std::string::npos,
        "persisted Poisson PLY must expose red, green, and blue properties");
}

void expectNoMeshFile(const fs::path& directory)
{
    for (const fs::directory_entry& entry : fs::directory_iterator(directory)) {
        expect(entry.path().filename() != "capture_mesh.ply",
            "memory-only reconstruction must not save capture_mesh.ply");
    }
}

void testMemoryReconstruction(int method)
{
    const TempDirectory temp;
    const fs::path config = temp.writeConfig(method);
    pcl::PolygonMesh mesh;
    ConverPointCloud converter;
    const bool ok = converter.meshAPIFromCloud(
        makeSphereCloud(),
        (temp.path() / "capture_rgb.ply").string(),
        config.string(),
        temp.path().string(),
        &mesh,
        false);
    expect(ok, method == 1
        ? "Poisson memory reconstruction must succeed"
        : "Greedy memory reconstruction must succeed");
    expect(!mesh.cloud.data.empty(), "memory mesh must contain vertices");
    expect(!mesh.polygons.empty(), "memory mesh must contain polygons");
    expectNoMeshFile(temp.path());
}

void testPoissonIgnoresIsolatedPointsWithInvalidNormals()
{
    const TempDirectory temp;
    pcl::PointCloud<pcl::PointXYZRGB>::Ptr cloud = makeSphereCloud();
    pcl::PointXYZRGB isolated;
    isolated.x = 1.0f;
    isolated.y = 1.0f;
    isolated.z = 1.0f;
    isolated.r = 255;
    isolated.g = 0;
    isolated.b = 0;
    cloud->push_back(isolated);
    cloud->width = static_cast<std::uint32_t>(cloud->size());

    pcl::PolygonMesh mesh;
    ConverPointCloud converter;
    expect(converter.meshAPIFromCloud(
        cloud,
        (temp.path() / "isolated_rgb.ply").string(),
        temp.writeConfig(1).string(),
        temp.path().string(),
        &mesh,
        false),
        "Poisson reconstruction must ignore isolated points with invalid normals");
    expect(!mesh.polygons.empty(),
        "Poisson reconstruction must retain the valid connected surface");
}

void testUnknownMethodIsRejected()
{
    const TempDirectory temp;
    pcl::PolygonMesh mesh;
    ConverPointCloud converter;
    expect(!converter.meshAPIFromCloud(
        makeSphereCloud(),
        (temp.path() / "capture_rgb.ply").string(),
        temp.writeConfig(99).string(),
        temp.path().string(),
        &mesh,
        false),
        "unknown reconstruction method must fail");
}

void testReconstructionAlgorithmSignaturesHaveNoFileIo()
{
    const std::string header = readProjectFile(
        "vendor/point_cloud/include/ConverPointCloud.h");
    expect(header.find("bool createGreedMeshFromCloud(") != std::string::npos
            && header.find("const pcl::PointCloud<pcl::PointXYZRGB>::ConstPtr &cloud,")
                != std::string::npos
            && header.find("pcl::PolygonMesh &meshOut);") != std::string::npos,
        "Greedy reconstruction must accept only cloud input and mesh output");
    expect(header.find("bool createGreedMesh(const string &);") == std::string::npos,
        "Greedy file-oriented reconstruction duplicate must be removed");
    expect(header.find("bool createPoissonMesh(const string &filepath);") == std::string::npos,
        "Poisson file-oriented reconstruction duplicate must be removed");
}

void testGreedyFileAndMemoryAdaptersAreEquivalent()
{
    const TempDirectory temp;
    const fs::path config = temp.writeConfig(2);
    const auto cloud = makeSphereCloud();
    const fs::path input = temp.path() / "sample_rgb.ply";
    expect(pcl::io::savePLYFileBinary(input.string(), *cloud) == 0,
        "test input PLY must be saved");

    pcl::PolygonMesh memoryMesh;
    ConverPointCloud memoryConverter;
    expect(memoryConverter.meshAPIFromCloud(
        cloud,
        input.string(),
        config.string(),
        temp.path().string(),
        &memoryMesh,
        false),
        "Greedy memory adapter must succeed");

    ConverPointCloud fileConverter;
    expect(fileConverter.meshAPI(
        input.string(), config.string(), temp.path().string()),
        "Greedy file adapter must succeed");

    pcl::PolygonMesh fileMesh;
    const fs::path output = temp.path() / "sample_mesh.ply";
    expect(fs::exists(output), "file adapter must save sample_mesh.ply");
    expect(pcl::io::loadPLYFile(output.string(), fileMesh) == 0,
        "saved mesh must be readable");
    expect(meshVertexCount(fileMesh) == meshVertexCount(memoryMesh),
        "file and memory adapters must return the same vertex count");
    expect(fileMesh.polygons.size() == memoryMesh.polygons.size(),
        "file and memory adapters must return the same polygon count");
}

void testMemoryAdapterCanPersistReturnedMesh()
{
    const TempDirectory temp;
    pcl::PolygonMesh mesh;
    ConverPointCloud converter;
    expect(converter.meshAPIFromCloud(
        makeSphereCloud(),
        (temp.path() / "persist_rgb.ply").string(),
        temp.writeConfig(1).string(),
        temp.path().string(),
        &mesh,
        true),
        "Poisson memory adapter with persistence must succeed");
    expect(fs::exists(temp.path() / "persist_mesh.ply"),
        "memory adapter must save the already returned mesh when requested");
    expect(!mesh.polygons.empty(), "persisted memory mesh must still be returned");
}

} // namespace

int main()
{
    testPoissonColorTransferUsesSpatialNeighbors();
    testPoissonColorTransferRejectsEmptySource();
    testPoissonColorTransferRejectsMalformedMeshSchema();
    testPoissonColorTransferRejectsNonFiniteCoordinates();
    testPoissonReconstructionReturnsAndPersistsRgb();
    testMemoryReconstruction(2);
    testMemoryReconstruction(1);
    testPoissonIgnoresIsolatedPointsWithInvalidNormals();
    testUnknownMethodIsRejected();
    testReconstructionAlgorithmSignaturesHaveNoFileIo();
    testGreedyFileAndMemoryAdaptersAreEquivalent();
    testMemoryAdapterCanPersistReturnedMesh();
    std::cout << "mesh reconstruction tests passed\n";
    return 0;
}
