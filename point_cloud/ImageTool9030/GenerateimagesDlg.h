
// GenerateimagesDlg.h : 头文件
//

#pragma once

#include "CLibrary.h"

#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgcodecs.hpp>

// CGenerateimagesDlg 对话框
class CGenerateimagesDlg : public CDialog
{
// 构造
public:
	CGenerateimagesDlg(CWnd* pParent = NULL);	// 标准构造函数

// 对话框数据
	enum { IDD = IDD_GENERATEIMAGES_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持

public:
    void getImageWH(CString filepath,int w, int h);


    list<string> getFileList(){ return m_FileList; };

    list<string> m_FileList;  //存放所有图片

    CLibrary clibrary;
    long m_TotalPicture;

    string m_strOutDir;
    CWinThread* m_pThread;

    int m_outW, m_outH; //输出图片行列
    string m_strOutW, m_strOutH;
    float m_resolutionW, m_resolutionH; //输出图片分辨率
    int  m_resolutionW2, m_resolutionH2;//实际图片行列数
    float m_scale;

    string m_fileExt;

    bool m_isRun, m_isOver;
// 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
    afx_msg void OnBnClickedButton1();
    afx_msg void OnBnClickedButton2();
    afx_msg void OnBnClickedOk();
    afx_msg void OnBnClickedCancel();
    afx_msg void OnBnClickedOk2();
};
