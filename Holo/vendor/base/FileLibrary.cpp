///////////////////////////////////////////////////////////
//  FileLibrary.cpp
//  Implementation of the Class FileLibrary
//  Created on:      11-XX-2017 17:40:43
//  Original author: bfzhao
///////////////////////////////////////////////////////////

#include "FileLibrary.h"
#include <shlobj.h>
#include <shlwapi.h>
#include <algorithm>
#include <iomanip>

using namespace std;

#pragma comment(lib, "Shlwapi")

FileLibrary* FileLibrary::m_pInstance = NULL;


FileLibrary *FileLibrary::getInstance(){
    if (m_pInstance == NULL){
        m_pInstance = new FileLibrary();
    }
    return m_pInstance;
}

FileLibrary::FileLibrary(){

    srand((unsigned)time(NULL));
}

FileLibrary::~FileLibrary(){

    if (m_pInstance != NULL)
    {
        delete m_pInstance;
    }
}


std::string FileLibrary::getCurrentFilePath(){
    char Buffer[MAX_PATH] = { 0 };

    GetModuleFileNameA(NULL, Buffer, MAX_PATH);
    std::string filepath = Buffer;
    filepath = filepath.substr(0, filepath.find_last_of("\\"));
    return filepath;
}


std::string FileLibrary::convertToLinuxPath(const std::string strWinPath){

    std::string strRet = strWinPath;
	strRet.replace(strRet.begin(), strRet.end(), '\\', '/');

    return strRet;
}

std::string FileLibrary::convertToWinPath(const std::string strLinuxPath){

    std::string strRet = strLinuxPath;
    if (strRet.size() > 0)
		strRet.replace(strRet.begin(), strRet.end(), '/', '\\');

    return strRet;
}

std::string FileLibrary::combineFilePath(const std::string strParent, const std::string strFileName){
    if (strParent.length() == 0) {
        return strFileName;
    }
    else if (strFileName.length() == 0) {
        return strParent;
    }

    std::string strResult;
    const char * pstring;
    strResult = strParent;
    pstring = strResult.c_str();

    //if no / or \ at the end of path,append it
    if ((pstring[strResult.length() - 1] != '\\')
        && (pstring[strResult.length() - 1] != '/')) {
        strResult.append("\\");
    }

    //if a '/ or \ at front of file name ,skip it
    pstring = strFileName.c_str();
    for (size_t i = 0; i < strFileName.length(); i++) {
        if ((*pstring == '\\') || (*pstring == '/')) {
            pstring++;
        }
        else
            break;
    }

    strResult = strResult + pstring;

    if ((strResult.at(strResult.length() - 1) == '\\' || strResult.at(strResult.length() - 1) == '/') && strResult.size() != 1)
        strResult = strResult.substr(0, strResult.length() - 1);

    return strResult;
}

std::string FileLibrary::getFileParentPath(const std::string &strFilePath){
    std::string strResult = "";
    if (strFilePath.size() == 0)
        return strResult;

    size_t lastPos2;
    lastPos2 = strFilePath.find_last_of('\\');

    //strResult.append(strFilePath, lastPos, strFilePath.length() - lastPos);
    strResult = strFilePath.substr(0, lastPos2);
    return strResult;
}


std::string FileLibrary::getFileNameFromPath(const std::string &strFilePath){

    std::string strResult = "";
    if (strFilePath.size() == 0)
        return strResult;

    //get the last \ or '/'
    size_t lastPos1, lastPos2, lastPos;
    lastPos1 = strFilePath.find_last_of('/');
    lastPos2 = strFilePath.find_last_of('\\');

    // neither / nor \ found.
    if (std::string::npos == lastPos1 && std::string::npos == lastPos2) {
        strResult = strFilePath;
        return strResult;
    }
    // only \ found
    else if (std::string::npos == lastPos1) {
        lastPos = lastPos2;
    }
    // only / found
    else if (std::string::npos == lastPos2) {
        lastPos = lastPos1;
    }
    else {
        lastPos = std::max(lastPos1, lastPos2);
    }

    lastPos++;

    if (lastPos == strFilePath.size())//the last is / or '\\'
    {
        strResult.append(strFilePath, 0, lastPos - 1);
        strResult = getFileNameFromPath(strResult);
    }
    else {
        //copy data after last '/' or '\\'
        strResult.append(strFilePath, lastPos, strFilePath.length() - lastPos);
    }
    if ((strResult.size() == 0) && (strFilePath.size() == 1))//root
        strResult = "\\";

    return strResult;

}
void FileLibrary::getAllSubFiles(const std::string& strArgFilePath, list<std::string>& lstSubFiles, bool bIncludeDirs, bool bIncludeFiles, bool bRecursive, const std::string& strFilter){


    std::string strFindStr;
    std::string strLongFindStr;
    std::string strMyFindStr;
    BOOL bFinished = FALSE;


    std::string strFilePath = strArgFilePath;

    if (!bIncludeDirs && !bIncludeFiles) {
        return;
    }

    WIN32_FIND_DATAA FileData;
    HANDLE hSearch;

    strFindStr = strFilePath;
    /* if (strFilter.size())
    strFindStr = combineFilePath(strFilePath, strFilter);
    else*/
    strFindStr = combineFilePath(strFilePath, "*.*");

    /* strLongFindStr = strFindStr;
    convert2LongFilePath(strLongFindStr);
    strMyFindStr = strLongFindStr;*/


    hSearch = FindFirstFileA(strFindStr.c_str(), &FileData);
    if (hSearch == INVALID_HANDLE_VALUE)
    {
        hSearch = FindFirstFileA(strFindStr.c_str(), &FileData);
        if (hSearch == INVALID_HANDLE_VALUE)
        {
            bFinished = TRUE; //该目录下没有文件
        }
    }


    while (!bFinished)
    {
        if ((strcmp(FileData.cFileName, ".") == 0)
            || (strcmp(FileData.cFileName, "..") == 0))
        {
            if (!FindNextFileA(hSearch, &FileData))
            {
                bFinished = TRUE;
            }
            continue;
        }
        std::string strChild = combineFilePath(strFilePath, std::string(FileData.cFileName));

        if (FileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) //目录
        {
            if (bIncludeDirs)
                lstSubFiles.push_back(strChild);
        }
        else //文件
        {
            if (bIncludeFiles){
                std::string tmpname = std::string(FileData.cFileName);
                if (tmpname.find(strFilter) != std::string::npos && strFilter.size() != 0 && tmpname.substr(tmpname.length() - strFilter.length(), strFilter.length()) == strFilter)
                    lstSubFiles.push_back(strChild);

            }
        }

        //是目录 & bRecursive 就进行递归搜索
        if ((FileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && bRecursive)
        {

            getAllSubFiles(strChild, lstSubFiles, bIncludeDirs, bIncludeFiles, bRecursive, strFilter);

        }

        if (!FindNextFileA(hSearch, &FileData))
        {
            bFinished = TRUE;
        }
    }
    FindClose(hSearch);

}

bool FileLibrary::isFileExists(const std::string &strArgFilePath)
{
	if (_access(strArgFilePath.c_str(), 0) == -1)
	{
		return false;
	}
	else
	{
		return true;
	}
}


bool FileLibrary::isDirExists(const std::string &strArgFilePath){
	return isFileExists(strArgFilePath);
}


bool  FileLibrary::readFile(const std::string &filepath, vector<std::string> &vecFileInfo, const std::string &type){
    if (filepath == "")
        return false;

    if (!isFileExists(filepath))
        return false;

    fstream infile;
    std::string line;

    infile.open(filepath);

    while (getline(infile, line))
    {
        if (line.find(type) != std::string::npos){
            std::string tmpStr = line.substr(line.find("=") + 1, line.length());
            vecFileInfo.push_back(tmpStr);
        }
    }

    infile.close();

    return true;
}



vector<std::string> FileLibrary::splitString(std::string str, std::string pattern, vector<std::string> &vectstring){
    vector<std::string> ret;
    if (pattern.empty())
        return ret;

    size_t start = 0, index = str.find_first_of(pattern, 0);

    while (index != str.npos)
    {
        if (start != index)
            ret.push_back(str.substr(start, index - start));

        start = index + 1;
        index = str.find_first_of(pattern, start);
    }

    if (!str.substr(start).empty())
        ret.push_back(str.substr(start));

    vectstring = ret;

    return ret;
}


std::string FileLibrary::getRand(){

    return to_string(rand());

}

std::string FileLibrary::getCurrentTime(){

    SYSTEMTIME sys_time;
    GetLocalTime(&sys_time);
    char timebug[50] = { 0 };
    sprintf_s(timebug, "%4d:%02d:%02d %02d:%02d:%02d.%03d", sys_time.wYear, sys_time.wMonth,
        sys_time.wDay, sys_time.wHour, sys_time.wMinute, sys_time.wSecond, sys_time.wMilliseconds);

    return timebug;
}


int FileLibrary::copyFile(const std::string &src, const std::string &dest){
    ifstream in;
    ofstream out;
    in.open(src, ios::binary);
    if (in.fail())
    {
        cout << "Error 1: Fail to open the source file." << endl;
        in.close();
        out.close();
        return 0;
    }
    out.open(dest, ios::binary);//打开目标文件

    if (out.fail())//创建文件失败
    {
        cout << "Error 2: Fail to create the new file." << endl;
        out.close();
        in.close();
        return 0;
    }
    else//创建文件成功
    {
        out << in.rdbuf();
        out.close();
        in.close();
        return 1;
    }
}

bool FileLibrary::filterGreen(float R, float G, float B, float green){
    if (G - R > green && G - B > green)
    {
        return true;
    }
    return false;
}

//在opencv中，H:0~180   S:0~255   V:0~255
bool FileLibrary::Rgb2Hsv(float R, float G, float B)
{
    float H, S, V, b = B / 255, g = G / 255, r = R / 255;
    float minValue, maxValue, delta;
    minValue = std::min(r, std::min(g, b));
    maxValue = std::max(r, std::max(g, b));

    V = maxValue; // v
    delta = V - minValue;
    if (V != 0) {
        S = delta / V; // s
    }
    else
    {
        // r = g = b = 0 // s = 0, v is undefined
        S = 0;
        H = -1;
        return false;
    }

    if (r == V)
        H = (g - b) / delta; // between yellow & magenta
    else if (g == V)
        H = 2 + (b - r) / delta; // between cyan & yellow
    else if (b == V)
        H = 4 + (r - g) / delta; // between magenta & cyan

    H *= 60; // degrees
    if (H < 0)
        H += 360;
    //H 120 S 1 V 1
    if (H >= 90 && H <= 180 && S >= 0 && S <= 1 && V >= 0 && V <= 1)
    {
        return true;
    }

    return false;
}
template <typename T>
T FileLibrary::convertFromstring(T &value, const std::string &s) {
    if (s.length() == 0 || s.empty())
    {
        value = 0;
        return value;
    }

    std::stringstream ss(s);
    ss >> value;
    return value;
};

template <class T> std::string FileLibrary::convertTostring(T &value, int precision){
    std::stringstream ss;
    ss << std::setprecision(precision) << value;
    return ss.str();
}


void FileLibrary::toEulerAngle(const Quate& q, double& roll, double& pitch, double& yaw)
{
    #define M_PI       3.14159265358979323846   // pi

    // roll (x-axis rotation)
    double sinr = +2.0 * (q.w * q.x + q.y * q.z);
    double cosr = +1.0 - 2.0 * (q.x * q.x + q.y * q.y);
    roll = atan2(sinr, cosr);

    // pitch (y-axis rotation)
    double sinp = +2.0 * (q.w * q.y - q.z * q.x);
    if (fabs(sinp) >= 1)
        pitch = copysign(M_PI / 2, sinp); // use 90 degrees if out of range
    else
        pitch = asin(sinp);

    // yaw (z-axis rotation)
    double siny = +2.0 * (q.w * q.z + q.x * q.y);
    double cosy = +1.0 - 2.0 * (q.y * q.y + q.z * q.z);
    yaw = atan2(siny, cosy);
}

/////////////////////////////////////////////////////////////////////////////
