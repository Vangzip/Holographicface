////////////////////////////////////////////////////////////////////////////////
//
// File Name:	CommonQtCtrlOperate.hpp
// Class Name:  CCommonQtCtrlOperate
// Description:	封装一些仅在qt中使用的控件操作
// Author:		罗志彬
// Date:		2019年4月22日
//
////////////////////////////////////////////////////////////////////////////////

#ifndef COMMON_QTCTRL_OPERATE_H
#define COMMON_QTCTRL_OPERATE_H

#include <QColor>
#include <QPalette>
#include <QMenu>
#include <list>
#include <QRect>
#include <QStyleOptionViewItem>
#include <QStyleOptionButton>
#include <QApplication>
#include <QTreeWidget>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QRadioButton>
#include <QToolButton>
#include <QTextEdit>
#include <map>
#include <QComboBox>
#include <QDateTimeEdit>
#include "CommonString.hpp"
#include <QLabel>
#include <QDesktopWidget>
#include <QPainter>
#define MENU_POPUP          (0x00000001 << 0)
#define MENU_DISABLE        (0x00000001 << 1)

#define TABLE_ITEM_CHECKABLE    (0x00000001 << 0)
#define TABLE_ITEM_CHECKED      (0x00000001 << 1)
#define TABLE_ITEM_EDITABLE     (0x00000001 << 2)
#define TABLE_ITEM_EDITING      (0x00000001 << 3)


#define MENU_SPERATOR_STRING(parent) std::make_tuple(-1,parent ,0 ,"","","")
#define MENU_SPERATOR_ICON(parent) std::make_tuple(-1,parent ,0 ,"","",QIcon())

class CCommonQtCtrlOperate
{ 
public:

    // 设置一个widget的背景颜色
    template<typename T >
    static void SetWidgetColor(T* pWidget, QPalette::ColorRole role, const QColor &color)
    {
        // 获取控件调色板
        auto pal = pWidget->palette();

        // 设置调色板颜色
        pal.setColor(role, color);
        // 设置自动填充背景
        pWidget->setAutoFillBackground(true);

        // 设置调色板
        pWidget->setPalette(pal);
    }

    template<typename T>
    static QColor GetWidgetColor(T* pWidget, QPalette::ColorRole role)
    {
        // 获取控件调色板
        auto pal = pWidget->palette();
        return pal.color(role);
    }

    //////////////////////////////根据输入的字符串来创建一个菜单
    /// 字符串格式为 id,pid,ispopup,text,slot,icon
    /// id: 菜单项本身的识别id，用于创建，如果设置为-1，则添加一个菜单分隔符
    /// pid：菜单项的父级id，当父级为popup时生效
    /// ispopup：菜单项是否含有子项，也就是是否为popup
    /// text：菜单项显示文字
    /// slot：菜单项响应函数，是SLOT(func())形式的字符串
    /// icon：图标文件地址
//    static QMenu* GenMenu(QObject* pOwner, std::initializer_list<const char*> lst)
//    {
//        // 初始化列表是空的，说明没传东西进来，直接返回
//        if( !lst.size() )
//        {
//            return nullptr;
//        }

//        // 为调用基础函数初始化参数结构体
//        std::list<const char* > > lstItem;

//        // 开始遍历初始化列表
//        for( auto& item : lst)
//        {
//            if( item )
//            {
//                lst.push_back(item);
//            }
//        }
//        // 调用基础函数进行处理
//        return GenMenu(pOwner,lstItem);
//    }

//    template < template < class ... T> class container_type>
//    static QMenu* GenMenu(QObject* pOwner, container_type<const char*> container )
//    {
//        if( !container.size() )
//        {
//            return nullptr;
//        }

//        container_type _temp;
//        for(auto* item : container )
//        {
//            if( item )
//            {
//                // 将输入字符串拆分为数据组
//                auto v = CCommonString::SplitStdString<std::vector>(item,',');

//                // 如果传入数据不够数，忽略这一条记录
//                if( v.size() <6 )
//                {
//                    continue;
//                }
//                // 根据设定，获取对应的数值，
//                int iId = atoi(v[0].c_str());
//                int iPid = atoi(v[1].c_str());
//                bool bIsPop = !!atoi(v[2].c_str());
//                std::string strText = v[3];
//                std::string strFunction = v[4];
//                std::string strIcon = v[5];
//                // 构造结构体
//                _temp.push_back(std::make_tuple(iId, iPid, bIsPop,strText, strFunction, strIcon) );
//            }
//        }

//        return GenMenu(pOwner,_temp);
//    }

//    // 通过字符串信息创建菜单
//    static QMenu* GenMenu(QObject* pOwner, std::string& strInit )
//    {
//        // 空串，返回
//        if( strInit.empty() )
//        {
//            return nullptr;
//        }

//        std::vector<std::string> vctItems = CCommonString::SplitStdString<std::vector>(strInit,'|');
//        if(！vctItems.size() )
//        {

//        }

//    }



    //////////////////////////////根据输入的tuple结构信息创建一个菜单
    /// tuple有六个可用域，按照索引号依次为
    /// 0: 菜单项本身的识别id，用于创建，如果设置为-1，则添加一个菜单分隔符
    /// 1：菜单项的父级id，当父级为popup时生效
    /// 2：菜单项是否含有子项，也就是是否为popup
    /// 3：菜单项显示文字
    /// 4：菜单项响应函数，是SLOT(func())形式的字符串
    /// 5：图标文件地址
    static QIcon GenIcon( QString strFile )
    {
        return QIcon(strFile);
    }

    static QIcon GenIcon( QColor color )
    {
        QPixmap pixmap;
        pixmap.load(":/Main/Icon/Menu/Empty.png");
        pixmap.fill(color);

        return GenIcon(pixmap);
    }

    static QIcon GenIcon( QPixmap pixmap )
    {
        return QIcon(pixmap);
    }

    static QIcon& GenIcon( QIcon& icon )
    {
        return icon;
    }

    template < template < class ... T> class container_type, typename icon_type>
    static QMenu *GenMenu(QObject* pOwner, container_type<std::tuple<int, int, unsigned int, QString, QString, icon_type> > container,QMenu* pMenu = new QMenu  )
    {
        // 如果容器中没有数据，返回空指针
        if( !container.size() )
        {
            return nullptr;
        }

        // 用来获取父级节点指针的映射
        std::map<int, QMenu*> mapTemp;

        // 构造根节点菜单项
        if( !pMenu )
        {
             pMenu = new QMenu();
        }

        mapTemp[-1] = pMenu;

        for( auto& item : container)
        {
            // 获得数据
            auto [iId, iPid, mask,strText,strFunction, icon] = item;

            if( iId != -1 && iPid > iId )
            {// 强行要求按顺序创建，如果pid大于自身id 了，说明父节点在之后创建，不符合规则，忽略此条
             // 除非是创建分隔符
                continue;
            }


            if( -1 == iId )
            {// 如果自身id是1，则需要添加一个分隔符

                // 搜索父节点菜单项
                auto mit = mapTemp.find(iPid);
                if( mit != mapTemp.end() )
                {
                    // 如果找到了，就为该菜单添加一个菜单分隔符
                    auto pParent = mit->second;
                    if( pParent )
                    {
                        pParent->addSeparator();
                    }
                }

                // 分隔符不需要添加其他信息，直接下一条
                continue;
            }

            //
            if( MENU_POPUP & mask )
            {// 如果这个标志位设置了，我们认为此项目是一个popup菜单，有子节点，

                // 查询父节点
                auto mit = mapTemp.find(iPid);
                if( mit == mapTemp.end() )
                {// 如果没查到，我们认为父节点信息传错了，此条忽略，
                    continue;
                }

                QMenu* pParent = mit->second;
                if( pParent )
                {
                    // 创建新的弹出菜单,并映射id
                    QMenu* pItem = new QMenu(strText);
                    pParent->addMenu(pItem);
                    mapTemp[iId] = pItem;
                    pItem->setEnabled( !( MENU_DISABLE & mask ));
                }
            }
            else
            {// 如果不是弹出菜单，我们认为他没有子项，直接创建action

                // 搜索父节点
                auto mit = mapTemp.find(iPid);
                if( mit == mapTemp.end() )
                {// 如果没查到，我们认为父节点信息传错了，此条忽略，
                    continue;
                }

                QMenu* pParent = mit->second;
                if( !pParent )
                {// 如果父节点指针为空，此条忽略，
                    continue;
                }

                // 加载图标文件，不做判断
                QIcon iconUse = GenIcon(icon);

                // 创建一个新的叶子菜单项
                auto pAction = pParent->addAction(   iconUse,// 图标
                                      strText, // 显示文字
                                      pOwner,//响应控件指针
                                      strFunction.toStdString().c_str()// 响应函数
                                    );
                if( pAction )
                {
                    pAction->setData(iId);
                    pAction->setEnabled( !( MENU_DISABLE & mask ));
                }
            }
        }

        // 将创建好的菜单返回
        return pMenu;
    }




    // 使用tuple 初始化列表创建菜单
    template<typename icon_type>
    static QMenu * GenMenu(QObject* pOwner, std::initializer_list<std::tuple<int, int, unsigned int, QString, QString, icon_type> > initlst, QMenu* pMenu = new QMenu )
    {
        // 如果容器中没有数据，返回空指针
        if( !initlst.size() )
        {
            return nullptr;
        }

        // 为调用基础函数初始化参数结构体
        std::list<std::tuple<int, int, unsigned int, QString,QString,icon_type > > lstItem;

        // 转换为list
        for(auto& item : initlst)
        {
            lstItem.push_back(item);
        }

        // 将创建好的菜单返回
        return GenMenu<std::list,icon_type>(pOwner, lstItem, pMenu);
    }
    ///////////////////////////////////////////////////////////////////////////////////////////
    //用于计算复选框的位置大小
    static QRect CheckBoxRect(const QStyleOptionViewItem &viewItemStyleOptions)/*const*/
    {
        //绘制按钮所需要的参数
        QStyleOptionButton checkBoxStyleOption;
        //按照给定的风格参数 返回元素子区域
        QRect checkBoxRect = QApplication::style()->subElementRect( QStyle::SE_CheckBoxIndicator, &checkBoxStyleOption);
        //返回QCheckBox坐标
        QPoint checkBoxPoint(viewItemStyleOptions.rect.x() + viewItemStyleOptions.rect.width() / 2 - checkBoxRect.width() / 2,
                             viewItemStyleOptions.rect.y() + viewItemStyleOptions.rect.height() / 2 - checkBoxRect.height() / 2);
        //返回QCheckBox几何形状
        return QRect(checkBoxPoint, checkBoxRect.size());
    }

    // 用于为一个控件添加一个子控件
    template<typename Widget_Type>
    static bool AddTabWidget( QWidget* pParent )
    {
        // 判断父节点是否有效
        if( ! pParent )
        {
            return false;
        }
        // 获得父节点布局
        auto layout = pParent->layout();
        if( !layout )
        {// 如果没有，强行添加一个，默认使用纵向，，我自己定的，
            layout = new  QVBoxLayout();
            layout->setMargin(0);
            // 设置布局
            pParent->setLayout( layout );
        }
        Widget_Type* pChild = new Widget_Type;
        layout->addWidget(pChild);
        return true;
    }

    // 清理一个树控件
    static void ClearTreeWidget(QTreeWidget* pWidget )
    {
        if( !pWidget )
        {// 判断指针有效
            return ;
        }

        // 依次删除节点
        while(pWidget->topLevelItemCount() )
        {
            auto* pItem = pWidget->topLevelItem(0);
            if( pItem )
            {
                delete pItem;
            }
        }
    }
    // 清空一个tableWidget
    static void ClearTableWidget(QTableWidget* pWidget )
    {
        if( !pWidget )
        {// 判断指针有效
            return ;
        }

        // 依次删除节点
        while(pWidget->rowCount() )
        {
            pWidget->removeRow(0);
        }
    }

    // 从控件上获取文本
    template<typename widget_type>
    static QString GetText( widget_type* pItem )
    {
        if( !pItem )
        {
            return "";
        }

        return pItem->text();
    }

    static QString GetText(QComboBox* pItem )
    {
        if( !pItem )
        {
            return "";
        }

        return pItem->currentText();
    }

    static QString GetText(QTextEdit* pItem )
    {
        if( !pItem )
        {
            return "";
        }

        return pItem->toPlainText();
    }

    // 向控件上设置文字
    template<typename widget_type, class dummy = void>
    static void SetText(widget_type* pItem, QString strText )
    {
        if( !pItem )
        {
            return ;
        }

        pItem->setText(strText);
    }

    static void SetText(QComboBox* pItem, QString strText )
    {
        if( !pItem )
        {
            return ;
        }

        pItem->setCurrentText(strText);
    }






    // int 类型和combobox交互
    static bool updateWidgetData( bool b, std::tuple<int*, QComboBox*> item )
    {
        auto[value, widget] = item;
        if( b )
        {// 获取值
            *value = widget->currentIndex();
        }
        else
        {// 刷新值
            widget->setCurrentIndex(*value);
        }
        return true;
    }

    static bool updateWidgetData( bool b, std::tuple<int*, QComboBox*, bool> item )
    {
        auto[value, widget, data] = item;
        if( !data )
        {// 如果是使用序号
            updateWidgetData(b,std::make_tuple(value,widget));
        }
        else
        {// 如果是使用数据字段
            if( b )
            {// 获取值
                //*value = widget->currentIndex();
                *value = widget->currentData().toInt();
            }
            else
            {// 刷新值
                //widget->setCurrentIndex(*value);
                for(int i=0; i< widget->count();i++ )
                {
                    if( widget->itemData(i).toInt() == *value )
                    {
                        widget->setCurrentIndex(i);
                        break;
                    }
                }
            }
        }
        return true;
    }

    // std::string 类型和界面互相交互
    template<typename widget_type>
    static bool updateWidgetData( bool b, std::tuple<std::string*,widget_type*> item )
    {
        auto[value,widget] = item;
        if( b )
        {// 获取值
            *value = GetText(widget).toStdString();
        }
        else
        {// 刷新值
            SetText(widget,QString::fromStdString(*value));
        }
        return true;
    }


    // std::string 类型和界面互相交互
    template<typename widget_type>
    static bool updateWidgetData( bool b, std::tuple<QString*,widget_type*> item )
    {
        auto[value,widget] = item;
        if( b )
        {// 获取值
            *value = GetText(widget);
        }
        else
        {// 刷新值
            SetText(widget,*value);
        }
        return true;
    }


    // std::string 类型和界面互相交互
    template<typename widget_type>
    static bool updateWidgetData( bool b, std::tuple<int*, widget_type*> item )
    {
        auto[value, widget] = item;
        if( b )
        {// 获取值
            *value = GetText(widget).toInt();
        }
        else
        {// 刷新值
            SetText(widget,QString::number(*value));
        }
        return true;
    }

    template<typename widget_type>
    static bool updateWidgetData( bool b, std::tuple<double*, widget_type*> item )
    {
        auto[value, widget] = item;
        if( b )
        {// 获取值
            *value = GetText(widget).toDouble();
        }
        else
        {// 刷新值
            SetText(widget,QString().sprintf("%.6f",*value));
        }
        return true;
    }


    template<typename widget_type>
    static bool updateWidgetData( bool b, std::tuple<float*, widget_type*> item )
    {
        auto[value, widget] = item;
        if( b )
        {// 获取值
            *value = GetText(widget).toFloat();
        }
        else
        {// 刷新值
            SetText(widget,QString().sprintf("%.6f",*value));
        }
        return true;
    }

    // qint64 和 时间编辑界面的交互
    // 因为时间现在多数存在一个qint64里，所以这个很有必要，而且存储double 也比存储字符串的时间更好
    static bool updateWidgetData( bool b, std::tuple<long long int*, QDateTimeEdit*> item )
    {
        auto[value, widget] = item;
        if( b )
        {// 获取值
            *value = widget->dateTime().toSecsSinceEpoch();
        }
        else
        {// 刷新值
            widget->setDateTime(QDateTime::fromSecsSinceEpoch(*value));
        }
        return true;
    }


    // radiobox组，和int类型索引号的转换
    template< template <typename ...> class container_type>
    static bool updateWidgetData(bool b, std::tuple<int*, container_type<QRadioButton*> > item)
    {
        auto[value, ct] = item;
        int * pValue = value;
        if( b )
        {// 获取值
            int iIndex = 0;
            std::for_each(ct.begin(), ct.end(),[&](auto item)-> void
            {
                if( item->isChecked() )
                {
                    *pValue = iIndex;
                }
                iIndex ++;
            });
        }
        else
        {// 刷新值
            int iIndex = 0;
            std::for_each(ct.begin(), ct.end(),[&](auto item)-> void
            {
                item->setChecked(*pValue == iIndex);
                iIndex ++;
            });
        }
        return true;
    }

    // 更新数据
    // 适配MFC的使用习惯，true代表从界面向变量，false代表从变量到界面
    template<typename... T>
    static void UpdateData(bool bDirection, T&&... lst )
    {
        // 初始化列表构造查询条件
      auto rs = {(updateWidgetData(bDirection,std::forward<T>(lst)))...};
      Q_UNUSED(rs);
    }

    // 如果里面的元素是控件，就直接送回上层函数去添加
    static QWidget* MakeWidgetItem( QWidget* item)
    {
        return item;
    }

    // 如果里面的元素是控件，就直接送回上层函数去添加
    static QTableWidgetItem* MakeWidgetItem( QTableWidgetItem* item)
    {
        return item;
    }

    // 添加一个ComboBox
    template<typename item_type ,typename std::enable_if<std::is_base_of<QComboBox,item_type>::value>::type* = nullptr>
    static QWidget* MakeWidgetItem( std::tuple<std::string, int,item_type*> item )
    {
        auto [strText, iId, pCb] = item;
        if( pCb )
        {
            pCb->addItem(strText,iId);
        }
        return pCb;
    }

    // 制造一个控件item
    static QTableWidgetItem* MakeWidgetItem( std::tuple<std::string, int > item )
    {
        // 获得信息
        auto[strText, iId] = item;
        // 创建item
        QTableWidgetItem* pItem = new QTableWidgetItem(strText.empty() ? "":strText.c_str());
        // 设置数据
        pItem->setData(Qt::UserRole,iId);
        // 返回结果
        return pItem;
    }

    // 制造一个控件item
    static QTableWidgetItem* MakeWidgetItem( std::tuple<std::string, std::string > item )
    {
        // 获得信息
        auto[strText, strData] = item;
        // 创建item
        QTableWidgetItem* pItem = new QTableWidgetItem(strText.empty() ? "":strText.c_str());
        // 设置数据
        pItem->setData(Qt::UserRole,QString::fromStdString(strData));
        // 返回结果
        return pItem;
    }


    // 制造一个控件item
    static QTableWidgetItem* MakeWidgetItem( std::tuple<std::string, std::string, QIcon > item )
    {
        // 获得信息
        auto[strText, strData, icon] = item;
        // 创建item
        QTableWidgetItem* pItem = new QTableWidgetItem(strText.empty() ? "":strText.c_str());
        // 设置数据
        pItem->setData(Qt::UserRole,QString::fromStdString(strData));
        // 设置图标
        pItem->setIcon(icon);
        // 返回结果
        return pItem;
    }

    // 制造一个控件item
    static QTableWidgetItem* MakeWidgetItem( std::tuple<std::string, int, QIcon, unsigned int > item )
    {
        // 获得信息
        auto[strText, iId, icon, mask] = item;

        // 调用下层函数来实现
        auto* pItem = MakeWidgetItem(std::make_tuple(strText,iId, icon) );

        if( pItem )
        {
            if( mask & TABLE_ITEM_CHECKABLE )
            {// 如果设置了复选框
                // 设置为可选状态
                pItem->setFlags( pItem->flags() | Qt::ItemIsUserCheckable);

                if( mask & TABLE_ITEM_CHECKED )
                {// 如果是选中状态
                    pItem->setCheckState(Qt::Checked);
                }
                else
                {// 如果是非选中状态
                    pItem->setCheckState(Qt::Unchecked);
                }
            }
            else
            {// 否则取消标志位
                pItem->setFlags(pItem->flags() & ~(Qt::ItemIsUserCheckable));
            }
        }
        // 返回结果
        return pItem;
    }

    // 制造一个控件item
    static QTableWidgetItem* MakeWidgetItem( std::tuple<std::string, int, QIcon > item )
    {
        // 获得信息
        auto[strText, iId, icon] = item;
        // 创建item
        QTableWidgetItem* pItem = new QTableWidgetItem(strText.empty() ? "":strText.c_str());
        // 设置数据
        pItem->setData(Qt::UserRole,iId);
        // 设置图标
        pItem->setIcon(icon);
        // 返回结果
        return pItem;
    }



    static QTableWidgetItem* MakeWidgetItem( std::tuple<QString, int > item )
    {
        // 获得信息
        auto[strText, iId] = item;
        // 创建item
        QTableWidgetItem* pItem = new QTableWidgetItem(strText);
        // 设置数据
        pItem->setData(Qt::UserRole,iId);
        // 返回结果
        return pItem;
    }

    // 制造一个控件item
    static QTableWidgetItem* MakeWidgetItem( std::string strText )
    {
        // 创建item
        QTableWidgetItem* pItem = new QTableWidgetItem(strText.empty() ? "":strText.c_str());
        // 返回结果
        return pItem;
    }

    // 制造一个控件item
    template<typename data_type , typename std::enable_if<std::is_arithmetic<data_type>::value,data_type>::type* = nullptr>
    static QTableWidgetItem* MakeWidgetItem( data_type data )
    {

        std::string strText = std::to_string(data);
        // 创建item
        QTableWidgetItem* pItem = new QTableWidgetItem(strText.empty() ? "":strText.c_str());
        // 返回结果
        return pItem;
    }


    static QTableWidgetItem* MakeWidgetItem( QString strText )
    {
        // 创建item
        QTableWidgetItem* pItem = new QTableWidgetItem(strText);
        // 返回结果
        return pItem;
    }

    // 制造一个控件item
    template< typename data_type, template <typename ...> class container_type = std::vector>
    static QTableWidgetItem* MakeWidgetItem( std::tuple<std::string, container_type<data_type> > item )
    {
        // 获得信息
        auto[strText, idlist] = item;
        // 创建item
        QTableWidgetItem* pItem = new QTableWidgetItem(strText.empty() ? "":strText.c_str());
        // 设置数据
        int iIndex = 0;
        for( auto& iId: idlist )
        {
            pItem->setData(Qt::UserRole + iIndex++,iId);
        }
        // 返回结果
        return pItem;
    }

    // 制造一个控件item
    template< typename data_type, template <typename ...> class container_type = std::vector>
    static QTableWidgetItem* MakeWidgetItem( std::tuple<std::string, container_type<data_type>, QIcon > item )
    {
        // 获得信息
        auto[strText, lstId, icon] = item;
        // 创建item
        QTableWidgetItem* pItem = MakeWidgetItem<data_type,container_type>(std::make_tuple(strText,lstId));
        // 设置图标
        pItem->setIcon(icon);
        // 返回结果
        return pItem;
    }


    // 如果是QWidget的派生类，就得用SetCellWidget
    template<typename item_type ,typename std::enable_if<std::is_base_of<QWidget,item_type>::value>::type* = nullptr>
    static bool SetItem( QTableWidget* pWidget, int iRow, int iCol, item_type* item )
    {
        pWidget->setCellWidget(iRow, iCol,item);
        return true;
    }

    // 如果是QTableWidgetItem的派生类，就用SetItem
    template<typename item_type ,typename std::enable_if<std::is_base_of<QTableWidgetItem,item_type>::value>::type* = nullptr>
    static bool SetItem( QTableWidget* pWidget, int iRow, int iCol, item_type* item )
    {
        pWidget->setItem(iRow, iCol,item);
        return true;
    }

    // 为列表控件添加一行
    template<typename...Content >
    static int InsertRow(QTableWidget* pWidget, int iPos, Content... contents )
    {
        // 检测控件是否有效
        if(!pWidget )
        {
            return -1;
        }

        // 计算正确的插入位置
        if( iPos <0 || iPos > pWidget->rowCount())
        {
            iPos = pWidget->rowCount();
        }

        // 插入行
        pWidget->insertRow(iPos);
        int iIndex = 0;
        // 展开参数，添加到控件上去
        auto rs = {SetItem(pWidget,iPos, iIndex++,MakeWidgetItem(std::forward<Content>(contents)))...};
        Q_UNUSED(rs);
        // 返回真实的插入位置
        return iPos;
    }


    static void setLabelBackground( QLabel* pLabel, QString strPath )
    {
        QPixmap p = QPixmap::fromImage(QImage(strPath)).scaled(pLabel->size(),Qt::KeepAspectRatio, Qt::SmoothTransformation);
        pLabel->setPixmap(p);
    }

    static void setLabelBackground( QLabel* pLabel, QImage img )
    {
        QPixmap p = QPixmap::fromImage(img.scaled(pLabel->size(),Qt::KeepAspectRatio, Qt::SmoothTransformation));
        pLabel->setPixmap(p);
    }

    static QRect getPrimeScreenRect(  )
    {
                QDesktopWidget* desktop = QApplication::desktop();
        return getScreenRect(desktop->primaryScreen());
    }
        static QRect getScreenRect ( int iScreenId )
        {
                QDesktopWidget* desktop = QApplication::desktop();
                int iCnt = desktop->screenCount();
                if( iScreenId < iCnt )
                {
                    return desktop->screenGeometry(iScreenId);
                }
            return QRect(0,0,0,0);
        }


    static QPixmap createPixmapFromRichText(const QString& richText, const QSize& size) {
        QLabel label;
        label.setText(richText);
        label.setFixedSize(size);
        label.setAlignment(Qt::AlignCenter);
        label.setStyleSheet("QLabel { background-color: transparent; }");

        QPixmap pixmap(size);
        pixmap.fill(Qt::transparent);

        QPainter painter(&pixmap);
        label.render(&painter);
        return pixmap;
    }


    static void setRichText(QToolButton* pWidget, QString strRichText )
    {
        QPixmap pixmap = createPixmapFromRichText(strRichText, pWidget->iconSize() );
        pWidget->setIcon(pixmap);
    }

};


using CQtCtrlOpt = CCommonQtCtrlOperate ;

#endif // QTSTRING_H
