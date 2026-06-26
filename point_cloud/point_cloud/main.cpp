// testreadpcd.cpp : 定义控制台应用程序的入口点。
//



#include "base.h"

#include "Filelibrary.h"
#include "ConverPointCloud.h"
#include "depth_io.h"
#include <pcl/filters/fast_bilateral.h>
#include <direct.h>

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
        
        // Build RGB image path: parent directory + base name + .jpg
        string rgb_png = FileLibrary::getInstance()->getFileParentPath(depthfile) + "\\" + baseName + ".jpg";
        
        cout << "  Depth image: " << depthfile << endl;
        cout << "  RGB image: " << rgb_png << endl;
        
        if (!FileLibrary::getInstance()->isFileExists(rgb_png))
        {
            cout << "  Error: RGB file not found: " << rgb_png << endl;
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

    ConverPointCloud* convertomesh = new ConverPointCloud();

    list<string>listfile;
    int num = 1;
    string plyfile1, plyfile2;
    
    cout << "Searching for _rgb.ply files in directory..." << endl;
    FileLibrary::getInstance()->getAllSubFiles(dir, listfile, false, true, false, "_rgb.ply");
    
    int totalFiles = listfile.size();
    cout << "Found " << totalFiles << " point cloud file(s)" << endl;
    
    if (totalFiles == 0) {
        cout << "Warning: No _rgb.ply files found in the specified directory!" << endl;
        delete convertomesh;
        return;
    }
    
    list<string>::iterator it = listfile.begin();
    for (it; it != listfile.end(); it++, num++) {
        string depthfile = *it;
        string filename = FileLibrary::getInstance()->getFileNameFromPath(depthfile);

        cout << "\n[" << num << "/" << totalFiles << "] Processing: " << filename << endl;
        cout << "  Full path: " << depthfile << endl;
        cout << "  Generating mesh..." << endl;

        bool result = convertomesh->meshAPI(depthfile, config, outputDir);
        
        if (result) {
            cout << "  Success: Mesh generated for " << filename << endl;
        }
        else {
            cout << "  Error: Mesh generation failed for " << filename << endl;
            cout << "  Possible causes:" << endl;
            cout << "    - Point cloud missing normals" << endl;
            cout << "    - Point cloud too sparse or insufficient points" << endl;
            cout << "    - Incorrect reconstruction parameters" << endl;
            cout << "  Suggestion: Check the error messages above and adjust config parameters." << endl;
        }
    }
    
    cout << "\n========================================" << endl;
    cout << "Mesh Generation Process Completed" << endl;
    cout << "Total files processed: " << totalFiles << endl;
    cout << "========================================" << endl;
    
    delete convertomesh;
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

    ConverPointCloud* convertomesh = new ConverPointCloud();
    list<string>listfile;
    
    cout << "Searching for _mesh.ply files in directory..." << endl;
    FileLibrary::getInstance()->getAllSubFiles(dir, listfile, false, true, false, "_mesh.ply");
    
    int totalFiles = listfile.size();
    cout << "Found " << totalFiles << " mesh file(s)" << endl;
    
    if (totalFiles == 0) {
        cout << "Warning: No _mesh.ply files found in the specified directory!" << endl;
        delete convertomesh;
        return;
    }
    
    list<string>::iterator it = listfile.begin();
    int num = 1;
    int successCount = 0;
    int failCount = 0;
    
    for (it; it != listfile.end(); it++, num++) {
        string depthfile = *it;
        string filename = FileLibrary::getInstance()->getFileNameFromPath(depthfile);
        
        cout << "\n[" << num << "/" << totalFiles << "] Processing: " << filename << endl;
        cout << "  Full path: " << depthfile << endl;
        
        if (!FileLibrary::getInstance()->isFileExists(depthfile))
        {
            cout << "  Error: Mesh file not found: " << depthfile << endl;
            failCount++;
            continue;
        }

        cout << "  Generating textured model..." << endl;
        if (convertomesh->modelAPI(depthfile, config)) {
            cout << "  Success: Textured model generated for " << filename << endl;
            successCount++;
        }
        else {
            cout << "  Failed: Unable to generate textured model for " << filename << endl;
            failCount++;
        }
    }

    cout << "\n========================================" << endl;
    cout << "Textured Model Generation Process Completed" << endl;
    cout << "Total files processed: " << totalFiles << endl;
    cout << "Successfully processed: " << successCount << endl;
    cout << "Failed: " << failCount << endl;
    cout << "========================================" << endl;

    delete convertomesh;
}


int main(int argc, char* argv[]) {
    cout << "========================================" << endl;
    cout << "3D Holographic Face Processing Tool" << endl;
    cout << "========================================" << endl;

    if (argc < 3) {
        cout << "\nUsage:" << endl;
        cout << "  Depth to Point Cloud:  program.exe -point <directory> -config <config_file>" << endl;
        cout << "  Point Cloud to Mesh:   program.exe -mesh <directory> -config <config_file>" << endl;
        cout << "  Mesh to Textured Model: program.exe -model <directory> -config <config_file>" << endl;
        cout << "\nExample:" << endl;
        cout << "  program.exe -point \"C:\\data\" -config \"C:\\data\\config.cfg\"" << endl;
        system("pause");
        return 0;
    }

    string mesh, config;
    bool model = false, point = false , mesh_flag = false;
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
    if (pcl::console::parse_argument(argc, argv, "-model", dir) >= 0)
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
        cout << "  -mesh   : Convert point clouds to mesh" << endl;
        cout << "  -model  : Generate textured models from mesh" << endl;
    }

    cout << "\n========================================" << endl;
    cout << "Program execution completed." << endl;
    cout << "========================================" << endl;
    system("pause");

    return 0;
}
