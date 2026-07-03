
// GenerateimagesDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "Generateimages.h"
#include "GenerateimagesDlg.h"
#include "afxdialogex.h"
#include <map>
using namespace std;

#define THREADS 20

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

std::map<string, cv::Mat> allFileMatMap;

//在cmd窗口中显示输出信息
void InitConsole()
{
    int nRet = 0;
    FILE* fp;
    AllocConsole();
    nRet = _open_osfhandle((intptr_t)GetStdHandle(STD_OUTPUT_HANDLE), _O_TEXT);
    fp = _fdopen(nRet, "w");
    *stdout = *fp;
    setvbuf(stdout, NULL, _IONBF, 0);
}

// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// 对话框数据
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
   
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
END_MESSAGE_MAP()


// CGenerateimagesDlg 对话框
struct Param
{
    cv::Mat *img;
    int index;
    size_t r, c;   //每个图片要读取的当前行列位置
    size_t outW, outH; //输出图片的宽高
    //vector<int> filenum;  //图片的行号
   
    list<string> filelist;    //图片列表

};// params[10];

void printParam(Param *p){
    cout <<"index:"<< p->index <<", r:"<<p->r<<", c:"<<p->c<<", outw:"<<p->outW<<", outH:"<<p->outH<< endl;

};
DWORD WINAPI threadFunc(LPVOID arg)
{
//UINT threadFunc(LPVOID arg){
    Param *p = (Param *)arg;
    if (p->filelist.size() == 0)
    {
        cout << "file list is null ." << endl;
        return 0;
    }
    auto it = p->filelist.begin();
    size_t r = p->r;
    size_t c = p->c;
    size_t outH;// = p->index * 10 + 10;
    size_t lines = p->outH / THREADS;

    outH = p->index *lines + lines;

    if (p->index +1 == THREADS)
    {
        outH += p->outH % THREADS;
    }
   


    if ((outH - p->index * lines) * p->outW != p->filelist.size())
    {
        cout << "file numbers error ." << endl;
        return NULL;
    }
    
    //printParam(p);
    for (size_t i = p->index * lines; i < outH; i++)  //输出行
    {
        for (size_t j = 0; j < p->outW; j++)    //输出列
        {

            string file = *it++;
            //cout << "file: " << file << endl;
            int pos = file.find_last_of("\\") + 1;
            file = file.substr(pos, file.length()-pos);
            if (allFileMatMap.find(file) == allFileMatMap.end()){
                cout << "not find : "<< file << endl;
                continue;
            }
            cv::Mat rgb = allFileMatMap.find(file)->second;//cv::imread(file, cv::IMREAD_ANYDEPTH | cv::IMREAD_ANYCOLOR);

            uchar b = rgb.ptr<uchar>(r)[c * 3];
            uchar g = rgb.ptr<uchar>(r)[c * 3 + 1];
            uchar rr = rgb.ptr<uchar>(r)[c * 3 + 2]; 

            cv::Vec4b &rgba = p->img->at<cv::Vec4b>(i, j);
            rgba[0] = b;
            rgba[1] = g;
            rgba[2] = rr;

        }
    }
    delete p;

    return NULL;
};


UINT start(LPVOID pParam){

    CGenerateimagesDlg *imageDlg = (CGenerateimagesDlg*)pParam;
    string outdir = imageDlg->m_strOutDir;
    int outW = imageDlg->m_outW;
    int outH = imageDlg->m_outH; 
    long int total_time = 0;

    clock_t tmpstart, tmpend;
    tmpstart = clock();

    list<string> filelist = imageDlg->m_FileList;
    //list<string>::iterator it = filelist.begin();
    for (auto it = filelist.begin(); it != filelist.end(); it++)
    {
        string filename = *it;
        cv::Mat rgb = cv::imread(filename, cv::IMREAD_ANYDEPTH | cv::IMREAD_ANYCOLOR);
        int pos = filename.find_last_of("\\")+1;
        filename = filename.substr(pos, filename.length() - pos);
        //cout << "filename: " << filename<<endl;

        allFileMatMap.insert(make_pair(filename,rgb));
    }
    tmpend = clock();
    cout << "load all file time: " << (tmpend - tmpstart)  << endl;
    try{
#if 1
        for (size_t r = 0; r <outH; r++) //行
        {
            stringstream tmpRows;
            if (r+1 < 10)
            {
                for (int num = 0; num< imageDlg->m_strOutH.length() - 1; num++){
                    tmpRows << "0";
                }
            }
            else if (r + 1 < 100)
            {
                for (int num = 0; num< imageDlg->m_strOutH.length() - 2; num++){
                    tmpRows << "0";
                }
            }
            else if (r + 1 < 1000)
            {
                for (int num = 0; num< imageDlg->m_strOutH.length() - 3; num++){
                    tmpRows << "0";
                }
            }
            else if (r + 1 < 10000)
            {
                for (int num = 0; num< imageDlg->m_strOutH.length() - 4; num++){
                    tmpRows << "0";
                }
            }
            tmpRows << r + 1;

            for (size_t c = 0; c <outW; c++)  //列
            {
                //拼接输出文件名
                stringstream outfiless;
                stringstream tmpCols;

                outfiless << outdir << "\\";
                /*if (r+1 < 10)
                {
                    tmpRows << "0" << r + 1;
                }
                else
                {
                    tmpRows << r + 1;
                }*/

                if (c+1 < 10)
                {
                    for (int num = 0; num< imageDlg->m_strOutW.length() - 1; num++){
                        tmpCols << "0";
                    }
                    
                }
                else if (c + 1 < 100)
                {
                    for (int num = 0; num< imageDlg->m_strOutW.length() - 2; num++){
                        tmpCols << "0";
                    }

                }else if (c + 1 < 1000)
                {
                    for (int num = 0; num< imageDlg->m_strOutW.length() - 3; num++){
                        tmpCols << "0";
                    }

                }
                else if (c + 1 < 10000)
                {
                    for (int num = 0; num< imageDlg->m_strOutW.length() - 4; num++){
                        tmpCols << "0";
                    }

                }
                
                tmpCols << c + 1;
                

                imageDlg->SetDlgItemText(IDC_STATIC5, (CString)tmpRows.str().c_str());
                imageDlg->SetDlgItemText(IDC_STATIC7, (CString)tmpCols.str().c_str());

                outfiless << tmpRows.str() << tmpCols.str() << "." << imageDlg->m_fileExt;

                string outfilepath;
                outfiless >> outfilepath;

                cv::Mat outfile(imageDlg->m_resolutionH2, imageDlg->m_resolutionW2, CV_8UC4);
                int cols = 0;
                //for (it = filelist.begin(); it != filelist.end(); it++, cols++)
                auto it = filelist.begin();
                while (!imageDlg->m_isRun)
                {
                    Sleep(1000);
                }
                clock_t start, end;
                start = clock();
#if 1
                //将文件按行数分到线程中, 每个线程处理N行像素; 遍历所有文件的对应位置 
                int Lines = imageDlg->m_resolutionH2 / THREADS;  //线程数 = 文件总行 / 10;
                int mod = imageDlg->m_resolutionH2 % THREADS;                    


                vector<HANDLE> mythreadvector;

                for (size_t i = 0; i < THREADS; i++)
                {
                    Param *p =  new Param;
                    p->img = &outfile;
                    p->r = r; p->c = c;
                    p->index = i;
                    //输出图像分辨率
                    p->outW = imageDlg->m_resolutionW2;
                    p->outH = imageDlg->m_resolutionH2;

                    if (i + 1 != THREADS)
                    {
                        for (size_t j = 0; j <Lines * imageDlg->m_resolutionW2; j++) //每个线程处理的文件数 = 行*列
                        {
                            p->filelist.push_back(*it++);
                        }
                    }
                    else  //if (mod != 0) //最后一个线程分配文件
                    {
                        for (size_t j = 0; j <(Lines + mod)* imageDlg->m_resolutionW2; j++) //每个线程处理的文件数 = 行*列  +mod
                        {
                            p->filelist.push_back(*it++);
                        }

                    }
                    //cout << "file list size: " << p->filelist.size() << endl;
                    //CWinThread* pThread = AfxBeginThread(threadFunc, &p, THREAD_PRIORITY_NORMAL);
                    HANDLE pThread = CreateThread(NULL, 0, threadFunc, p, 0, NULL);

                    mythreadvector.push_back(pThread);
                   
                }
                //end1 = clock();
                //cout << "one file time: " << (end1 - start1 )/ CLK_TCK << endl;
                auto threadIt = mythreadvector.begin();
                for (; threadIt != mythreadvector.end(); threadIt++)
                {
                    HANDLE pthread = *threadIt;
                    WaitForSingleObject(pthread, 300*1000);

                    CloseHandle(pthread);
                }
#endif

#if 0
                for (size_t i = 0; i < imageDlg->m_resolutionH2; i++)  //输出行
                {
                    for (size_t j = 0; j < imageDlg->m_resolutionW2; j++)    //输出列
                    {

                        string file = *it++;
                        //cout << "file: " << file << endl;
                        cv::Mat rgb = cv::imread(file, cv::IMREAD_ANYDEPTH | cv::IMREAD_ANYCOLOR);

                        uchar b = rgb.ptr<uchar>(r)[c * 3];
                        uchar g = rgb.ptr<uchar>(r)[c * 3 + 1];
                        uchar rr = rgb.ptr<uchar>(r)[c * 3 + 2];
                        //uchar a = rgb.ptr<uchar>(m)[n * 3 + 3];

                        cv::Vec4b &rgba = outfile.at<cv::Vec4b>(i, j);
                        rgba[0] = b;
                        rgba[1] = g;
                        rgba[2] = rr;
                        //rgba[3] = a;

                    }
                }
#endif
                vector<int>compression_params;
                compression_params.push_back(cv::IMWRITE_JPEG_QUALITY);
                compression_params.push_back(100);

                cv::Mat tmpoutmat;

                resize(outfile, tmpoutmat, cv::Size(0, 0), imageDlg->m_scale, imageDlg->m_scale, cv::INTER_NEAREST);

                cv::imwrite(outfilepath, tmpoutmat, compression_params);

                end = clock();
                //cout << "file name: " << outfilepath.substr(outfilepath.find_last_of("\\")+1,10) << ", The time was: " << (end - start)  << endl;
                total_time += (end - start);

               

            } //
            
        }
        //cout << "average time:" << total_time / (outW*outH) << endl;

#endif
    }
    catch (...){
        throw;
    
    }

    
    //imageDlg->SetDlgItemText(IDOK,(CString) "完成");
    exit(0);



    return 0;
}

CGenerateimagesDlg::CGenerateimagesDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CGenerateimagesDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

    //InitConsole();

    m_isRun = false;
    m_TotalPicture = 0;
    m_outH = m_outW = 0;
    m_resolutionH = m_resolutionW = m_resolutionH2 = m_resolutionW2 = 0;
    m_scale = 1.0f;
    m_isOver = false;
    m_pThread = NULL;
}

void CGenerateimagesDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
    DDX_Text(pDX, IDC_STATIC11, m_TotalPicture);
    
    SetDlgItemText(IDC_EDIT7, (CString)"1");
}

BEGIN_MESSAGE_MAP(CGenerateimagesDlg, CDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
    ON_BN_CLICKED(IDC_BUTTON1, &CGenerateimagesDlg::OnBnClickedButton1)
    ON_BN_CLICKED(IDC_BUTTON2, &CGenerateimagesDlg::OnBnClickedButton2)
    ON_BN_CLICKED(IDOK, &CGenerateimagesDlg::OnBnClickedOk)
    ON_BN_CLICKED(IDCANCEL, &CGenerateimagesDlg::OnBnClickedCancel)
    ON_BN_CLICKED(IDOK2, &CGenerateimagesDlg::OnBnClickedOk2)
END_MESSAGE_MAP()


// CGenerateimagesDlg 消息处理程序

BOOL CGenerateimagesDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO:  在此添加额外的初始化代码

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CGenerateimagesDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
        CDialog::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CGenerateimagesDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
        CDialog::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CGenerateimagesDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



//创建一个选择文件夹的对话框，返回所选路径  
static CString Show()
{
    TCHAR           szFolderPath[MAX_PATH] = { 0 };
    CString         strFolderPath = TEXT("");

    BROWSEINFO      sInfo;
    ::ZeroMemory(&sInfo, sizeof(BROWSEINFO));
    sInfo.pidlRoot = 0;
    sInfo.lpszTitle = _T("请选择一个文件夹：");
    sInfo.ulFlags = BIF_DONTGOBELOWDOMAIN | BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE | BIF_EDITBOX;
    sInfo.lpfn = NULL;

    // 显示文件夹选择对话框  
    LPITEMIDLIST lpidlBrowse = ::SHBrowseForFolder(&sInfo);
    if (lpidlBrowse != NULL)
    {
        // 取得文件夹名  
        if (::SHGetPathFromIDList(lpidlBrowse, szFolderPath))
        {
            strFolderPath = szFolderPath;
        }
    }
    if (lpidlBrowse != NULL)
    {
        ::CoTaskMemFree(lpidlBrowse);
    }

    return strFolderPath;

}

void CGenerateimagesDlg::OnBnClickedButton1()
{
    // TODO:  在此添加控件通知处理程序代码
    


    // TODO:  在此添加控件通知处理程序代码
    //弹出对话框,选择目录文件
    CString FilePathName;
    CFileDialog dlg(TRUE, (CString)"*.*", NULL, OFN_HIDEREADONLY, (CString)TEXT("*.jpg|*.jpg|*.png|*.png|*.*|*.*|"), NULL);
    if (dlg.DoModal() == IDOK)
    {
        FilePathName = dlg.GetPathName();          // 这样可以打开并获得你选择文件的完整路径
    }
    else
    {
        return;
    }

    int iEndPos = 0;
    iEndPos = FilePathName.ReverseFind('\\');

    USES_CONVERSION;
    CString DirParth = FilePathName.Left(iEndPos);
    string str = T2A(DirParth);

    m_FileList.clear();

    ////获取目录下所有指定格式的图片
    m_fileExt = T2A(dlg.GetFileExt());
    clibrary.getAllSubFiles(str, m_FileList, false, true, false, T2A(dlg.GetFileExt()));
    if (m_FileList.size() == 0)
    {
        return;
    }
    //更新界面数据
    UpdateData(TRUE);
    m_TotalPicture = m_FileList.size();
    UpdateData(FALSE);


   

    string filepath = *(m_FileList.begin());
    list<string>::iterator it = m_FileList.end();
    string name = *(--it);
    int pos = name.find_last_of("\\")+1;
    name = name.substr(pos , name.length() - pos - 4);
    //cout << name.size() << "    " << name.length() << endl;

    if (name.length() == 4)
    {
        m_resolutionH2 = m_resolutionH = atoi(name.substr(0, 2).c_str());
        m_resolutionW2 = m_resolutionW = atoi(name.substr(2, 2).c_str());
    }
    else if (name.length() == 6)
    {
        m_resolutionH2 = m_resolutionH = atoi(name.substr(0, 3).c_str());
        m_resolutionW2 = m_resolutionW = atoi(name.substr(3, 3).c_str());

    }
    else if (name.length() == 8)
    {
        m_resolutionH2 = m_resolutionH = atoi(name.substr(0, 4).c_str());
        m_resolutionW2 = m_resolutionW = atoi(name.substr(4, 4).c_str());

    }



    CImage  image;
    image.Load((CString)filepath.c_str());
    int height = image.GetHeight();
    int width = image.GetWidth();

    CString cstr;
    cstr.Format((CString)"%d", height);
    SetDlgItemText(IDC_EDIT3, cstr);
    SetDlgItemText(IDC_EDIT1, cstr);

    cstr.Format((CString)"%d", width);
    SetDlgItemText(IDC_EDIT4, cstr);
    SetDlgItemText(IDC_EDIT2, cstr);


    cstr.Format((CString)"%d", m_resolutionH);
    SetDlgItemText(IDC_EDIT6, cstr);
    cstr.Format((CString)"%d", m_resolutionW);
    SetDlgItemText(IDC_EDIT5, cstr);

}


void CGenerateimagesDlg::OnBnClickedButton2()
{
    // TODO:  在此添加控件通知处理程序代码
    USES_CONVERSION;
    CString dir = Show();

    m_strOutDir = T2A(dir);
    cout << "dir: " << m_strOutDir << endl;
}



void CGenerateimagesDlg::getImageWH(CString filepath, int w, int h){
    int cx, cy;
    CRect   rect;
    int height, width;
    CRect rect1;

    CImage  image;
    image.Load(filepath);
    height = image.GetHeight();
    width = image.GetWidth();   
}




//开始按钮功能
void CGenerateimagesDlg::OnBnClickedOk()
{
    // TODO:  在此添加控件通知处理程序代码
    //CDialog::OnOK();
    USES_CONVERSION;
    //out 行列
    CString str;
    GetDlgItemText(IDC_EDIT1, str);
    m_outH = _ttoi(str);       
    m_strOutH = T2A(str);

    GetDlgItemText(IDC_EDIT2, str);
    m_outW = _ttoi(str);
    m_strOutW = T2A(str);

    ////out 分辨率
    //GetDlgItemText(IDC_EDIT5,str);
    //m_resolutionH = _ttoi(str); 

    //GetDlgItemText(IDC_EDIT6,str);
    //m_resolutionW = _ttoi(str);

    if (m_FileList.size() == 0)
    {
        AfxMessageBox((CString)"未加载图片");
        return;
    }

    if (m_strOutDir.length() == 0)
    {
        AfxMessageBox((CString)"未选择输出文件夹");
        return;
    }

    if (m_pThread == NULL)
    {
        m_pThread = AfxBeginThread(start, this, THREAD_PRIORITY_NORMAL/*, 0, CREATE_SUSPENDED,0*/);
        
    }

    if (!m_isRun)
    {
        SetDlgItemText(IDOK,(CString)"暂停");
        m_isRun = true;
        ((CButton*)GetDlgItem(IDC_BUTTON1))->EnableWindow(FALSE);
        ((CButton*)GetDlgItem(IDC_BUTTON2))->EnableWindow(FALSE);         
        ((CEdit *)GetDlgItem(IDC_EDIT7))->SetReadOnly(TRUE);

    }
    else
    {
        SetDlgItemText(IDOK, (CString)"开始");
        m_isRun = false;
        ((CEdit *)GetDlgItem(IDC_EDIT7))->SetReadOnly(FALSE);
    }

}


void CGenerateimagesDlg::OnBnClickedCancel()
{
    // TODO:  在此添加控件通知处理程序代码
    CDialog::OnCancel();
}

//确定按钮, 手动修改输出图片分辨率
void CGenerateimagesDlg::OnBnClickedOk2()
{
    // TODO:  在此添加控件通知处理程序代码
    CString cstr;
    GetDlgItemText(IDC_EDIT7, cstr);

    m_scale = _ttof(cstr);

    //m_resolutionH *= _ttof(cstr);
    //m_resolutionW *=_ttof(cstr);

    cstr.Format((CString)"%lf", (m_resolutionH *m_scale));
    SetDlgItemText(IDC_EDIT6, cstr);
    cstr.Format((CString)"%lf", (m_resolutionW*m_scale));
    SetDlgItemText(IDC_EDIT5, cstr);

}
