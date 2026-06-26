// testreadpcd.cpp : 定义控制台应用程序的入口点。
//



#include "base.h"  
#include "FileLibrary.h"
#include <pcl/filters/fast_bilateral.h>
#include "include\OdmTexturing.hpp"



int main(int argc, char *argv[]){         

	
    OdmTexturing texture;

    texture.run(argc, argv);

    return 0;
}


