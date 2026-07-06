////////////////////////////////////////////////////////////////////////////////
//
// File Name:	TabManager.hpp
// Class Name:  CTabManager
// Description:	用于管理标签页
// Author:	罗志彬
// Date:	2019年4月22日
//
////////////////////////////////////////////////////////////////////////////////

#ifndef QT_TABMGR_HPP
#define QT_TABMGR_HPP

#include <QWidget>
#include <QTabWidget>
#include <QGroupBox>
template<typename widget_type >
class CMainpageManager
{
public:
    CMainpageManager( widget_type* pWidget = nullptr )
        : m_pWidget(pWidget )
    {

    }
public:

    // 设置tab控件
    void SetWidget( widget_type* pWidget )
    {
        m_pWidget = pWidget;
    }

    // 根据名称获得应该添加的控件
    QWidget* GetWidgetByName( QString strName )
    {

        if( m_cstrTagNameUserMgr == strName )
        {
            return new CDlgUserManager;
        }
        else if( m_cstrTagNameLruMgr == strName )
        {
            return new CDlgLruMgr;
        }
        else if( m_cstrTagNameSruMgr == strName )
        {
            return new CDlgSruMgr;
        }
        else if( m_cstrTagNameFaultMgr == strName )
        {
            return new CDlgFaultMgr;
        }
        else if( m_cstrTagNameLruDataMgr == strName || m_cstrTagNameTestDataMgr == strName )
        {
            return new CDlgDataMgr;
        }
        else if( m_cstrTagNameDeal == strName )
        {
            return new CExecuteDignose;
        }
        else if( m_cstrTagNameNewTask == strName )
        {
            return new CDlgTaskInput;
        }
        else if( m_cstrTagNameMaintenanceMgr == strName )
        {
            return new CDlgMaintenanceInfo;
        }
        else if( m_cstrTagNameTestItemMgr == strName )
        {
            return new CDlgTestItemMgr;
        }
        else if( m_cstrTagNameTaskList == strName )
        {
            return new CDlgTaskList;
        }
        else if( m_cstrTagNameCaseMgr == strName )
        {
            return new CDlgCaseMgr;
        }
        else if( m_cstrTagNameFunctionMoudle == strName )
        {
            return new CDlgLruFunctionRelation;
        }
        else if ( m_cstrTagNameLocationMgr == strName )
        {
            return new CDlgLocationMgr;
        }
        else if( m_cstrTagNameSeverMain == strName )
        {
            return new CDlgServerMainpage;
        }
        else if( m_cstrTagNameOption == strName )
        {
            return new CDlgOption;
        }
        return nullptr;
    }

public:
    const QString m_cstrTagNameUserMgr = QObject::tr("用户管理");
    const QString m_cstrTagNameLruMgr = QObject::tr("LRU管理");
    const QString m_cstrTagNameSruMgr = QObject::tr("SRU管理");
    const QString m_cstrTagNameFaultMgr = QObject::tr("故障管理");
    const QString m_cstrTagNameDataMgr = QObject::tr("数据管理");
    const QString m_cstrTagNameLruDataMgr = QObject::tr("部件数据管理");
    const QString m_cstrTagNameTestDataMgr = QObject::tr("测试数据管理");
    const QString m_cstrTagNameDeal = QObject::tr("任务处理");
    const QString m_cstrTagNameNewTask = QObject::tr("新建任务");
    const QString m_cstrTagNameMaintenanceMgr = QObject::tr("工艺管理");
    const QString m_cstrTagNameTestItemMgr = QObject::tr("测试内容管理");
    const QString m_cstrTagNameTaskList = QObject::tr("任务列表");
    const QString m_cstrTagNameCaseMgr = QObject::tr("案例管理");
    const QString m_cstrTagNameFunctionMoudle = QObject::tr("功能模块管理");
    const QString m_cstrTagNameSoftwareMgr = QObject::tr("软件管理");
    const QString m_cstrTagNameLocationMgr = QObject::tr("位置管理");
    const QString m_cstrTagNameSeverMain = QObject::tr("首页");
    const QString m_cstrTagNameOption= QObject::tr("系统配置");

protected:
    widget_type* m_pWidget = nullptr;
};


class CButtonMenuManager : public CMainpageManager<QGroupBox>
{
public:
    using CMainpageManager<QGroupBox>::CMainpageManager;
public:
    bool Open( QString strName, std::function<void(QWidget*)> funcCallBack = nullptr )
    {


        QWidget* pNewWidget = m_mapTagToWidget[strName];
        if( !pNewWidget  )
        {
            pNewWidget = GetWidgetByName(strName );
            m_mapTagToWidget[strName] = pNewWidget;
        }
        else
        {
            if( m_cstrTagNameDeal == strName )
            {// 如果是执行任务，强制重新分配
                delete pNewWidget;
                pNewWidget = GetWidgetByName(strName );
                m_mapTagToWidget[strName] = pNewWidget;
            }
        }
        if( !pNewWidget )
        {// 匹配失败了，返回
            return false;
        }

        if( funcCallBack )
        {
            funcCallBack(pNewWidget);
        }
        return true;
    }
public:
    std::map<QString, QWidget*> m_mapTagToWidget;
};

class CTabManager : public CMainpageManager<QTabWidget>
{
public:
    using CMainpageManager<QTabWidget>::CMainpageManager;
public:
    void CloseTab( QString strName )
    {
        // 未指定父控件，返回
        if( !m_pWidget )
        {
            return;
        }

        // 遍历查找已经存在的控件名
        for( int i=0; i< m_pWidget->count(); i++ )
        {
            if( strName == m_pWidget->tabText(i) )
            {// 如果找到了，就设置为当前，然后退出
//                m_pWidget->setCurrentIndex(i);
//                if( funcCallBack )
//                {
//                    funcCallBack(m_pWidget->widget(i));
//                }
//                return true ;
                m_pWidget->removeTab(i);
            }
        }

    }

    // 打开一个标签
    bool OpenTab( QString strName, std::function<void(QWidget*)> funcCallBack = nullptr )
    {
        // 未指定父控件，返回
        if( !m_pWidget )
        {
            return false;
        }

        // 遍历查找已经存在的控件名
        for( int i=0; i< m_pWidget->count(); i++ )
        {
            if( strName == m_pWidget->tabText(i) )
            {// 如果找到了，就设置为当前，然后退出
                m_pWidget->setCurrentIndex(i);
                if( funcCallBack )
                {
                    funcCallBack(m_pWidget->widget(i));
                }
                return true ;
            }
        }
        // 没找到，按照名字生成控件
        QWidget* pNewWidget = GetWidgetByName(strName );
        if( !pNewWidget )
        {// 匹配失败了，返回
            return false;
        }

        // 将生成的控件添加到界面，并设置为当前选中
        m_pWidget->addTab(pNewWidget, strName);
        m_pWidget->setCurrentWidget(pNewWidget);
        if( funcCallBack )
        {
            funcCallBack(pNewWidget);
        }
        return true;
    }

    // 获得一个标签的名字
    QString GetTabName( int iIndex )
    {
        //
        if( iIndex < 0 || iIndex >= m_pWidget->count() )
        {
            return "";
        }
        return m_pWidget->tabText(iIndex);
    }


};


#endif // DBMGR_HPP
