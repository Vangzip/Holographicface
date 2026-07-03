#pragma once


#include "afxwin.h"
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <string.h>
#include <iostream>
#include <fstream>
#include <sstream>

#include <io.h>  
#include <fcntl.h>  
#include <vector>
#include <list>


#include <afxmt.h>
using namespace std;


class CLibrary{

public:
    CLibrary(){};
    ~CLibrary(){};
    string combineFilePath(const string strParent, const string strFileName);
    void getAllSubFiles(const string& strArgFilePath, list<string>& lstSubFiles, bool bIncludeDirs, bool bIncludeFiles, bool bRecursive, const string& strFilter);

    string convString(CString str);

    vector<string> splitString(string str, string pattern, vector<string> &vectString);

};