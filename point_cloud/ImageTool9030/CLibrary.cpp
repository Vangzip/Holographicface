#include "CLibrary.h"



string CLibrary::combineFilePath(const string strParent, const string strFileName){
    if (strParent.length() == 0) {
        return strFileName;
    }
    else if (strFileName.length() == 0) {
        return strParent;
    }

    string strResult;
    const char * pString;
    strResult = strParent;
    pString = strResult.c_str();

    //if no / or \ at the end of path,append it
    if ((pString[strResult.length() - 1] != '\\')
        && (pString[strResult.length() - 1] != '/')) {
        strResult.append("\\");
    }

    //if a '/ or \ at front of file name ,skip it
    pString = strFileName.c_str();
    for (size_t i = 0; i < strFileName.length(); i++) {
        if ((*pString == '\\') || (*pString == '/')) {
            pString++;
        }
        else
            break;
    }

    strResult = strResult + pString;

    if ((strResult.at(strResult.length() - 1) == '\\' || strResult.at(strResult.length() - 1) == '/') && strResult.size() != 1)
        strResult = strResult.substr(0, strResult.length() - 1);

    return strResult;
}

void CLibrary::getAllSubFiles(const string& strArgFilePath, list<string>& lstSubFiles, bool bIncludeDirs, bool bIncludeFiles, bool bRecursive, const string& strFilter){


    string strFindStr;
    string strLongFindStr;
    string strMyFindStr;
    BOOL bFinished = FALSE;


    string strFilePath = strArgFilePath;

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
        string strChild = combineFilePath(strFilePath, string(FileData.cFileName));

        if (FileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) //目录
        {
            if (bIncludeDirs)
                lstSubFiles.push_back(strChild);
        }
        else //文件
        {
            if (bIncludeFiles){
                string tmpname = string(FileData.cFileName);
                if (tmpname.find(strFilter) != string::npos && strFilter.size() != 0)
                    lstSubFiles.push_back(strChild);

            }
        }

        //是目录 & bRecursive 就进入递归调用   
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


string CLibrary::convString(CString str){ 
    USES_CONVERSION;
    string data = T2A(str);
    return data; 
};


vector<string> CLibrary::splitString(string str, string pattern, vector<string> &vectString){
    vector<string> ret;
    if (pattern.empty())
        return ret;

    size_t start = 0, index = str.find_first_of(pattern, 0);

    while (index != str.npos)
    {
        if (start != index)
            vectString.push_back(str.substr(start, index - start));

        start = index + 1;
        index = str.find_first_of(pattern, start);
    }

    if (!str.substr(start).empty())
        vectString.push_back(str.substr(start));

    ret = vectString;

    return ret;
}