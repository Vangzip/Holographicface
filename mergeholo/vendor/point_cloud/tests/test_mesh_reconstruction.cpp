#include "ConverPointCloud.h"

#include <pcl/io/ply_io.h>

#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
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
    expect(header.find(
        "bool createGreedMeshFromCloud(\n"
        "        const pcl::PointCloud<pcl::PointXYZRGB>::ConstPtr &cloud,\n"
        "        pcl::PolygonMesh &meshOut);") != std::string::npos,
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
    testMemoryReconstruction(2);
    testMemoryReconstruction(1);
    testUnknownMethodIsRejected();
    testReconstructionAlgorithmSignaturesHaveNoFileIo();
    testGreedyFileAndMemoryAdaptersAreEquivalent();
    testMemoryAdapterCanPersistReturnedMesh();
    std::cout << "mesh reconstruction tests passed\n";
    return 0;
}
