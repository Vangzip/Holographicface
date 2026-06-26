// testreadpcd.cpp : 定义控制台应用程序的入口点。
//



#include "base.h"  
#include "FileLibrary.h"
#include "ConverPointCloud.h"                 
#include <pcl/filters/fast_bilateral.h>


// 打印帮助信息
void printUsage(const char* progName)
{
    std::cout << "\n\nUsage: " << progName << " [options] \n\n"
        << "Options:\n"
        << "-------------------------------------------\n"
        << "-mesh  \n"
        << "-model   \n"
		<< "-camera   \n"
		<< "-image   \n"
		<< "-out   \n"
		<< "-textureResolution   \n"		
        << "-config <string>  \n"
        << "-h  this help\n"
        << "\n\n";
}

int main(int argc, char *argv[]){         


	bool mesh =false, model = false;//判断是否是mesh处理，还是obj模型处理
	string config/*配置文件，mesh时使用*/, plyfile/*模型文件*/;

    //解析命令行参数
    if (pcl::console::find_argument(argc, argv, "-h") >= 0)
    {
        printUsage(argv[0]);
        return 0;
    }   
	//解析mesh参数 是否是生成mesh
    if (pcl::console::parse_argument(argc, argv, "-mesh", plyfile) >= 0)
    {
        mesh = true;
    }
	//解析obj参数，是否是生成模型
    if (pcl::console::parse_argument(argc, argv, "-model", plyfile) >= 0)
    {
        model = true;
    }
	
	//配置文件路径参数解析
    if (pcl::console::parse(argc, argv, "-config", config) >= 0)
    {

    } 	
	
    if (mesh )
    {
        if (!plyfile.empty())
        {
            ConverPointCloud *convertomesh = new ConverPointCloud();
            convertomesh->meshAPI(plyfile, config);

        }
        
    }else if (model){
        if (!plyfile.empty())
        {
			OdmTexturing createmodel(plyfile);
			createmodel.run(argc, argv);

        }
       
    }



    return 0;
}

