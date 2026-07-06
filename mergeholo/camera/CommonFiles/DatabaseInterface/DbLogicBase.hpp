////////////////////////////////////////////////////////////////////////////////
//
// File Name:	DbLogicBase.hpp
// Class Name:  CDbLogic
// Description:	基础数据库逻辑接口类，封装操作数据库表中数据的方法
// Author:		罗志彬
// Date:		2019年4月22日
//
////////////////////////////////////////////////////////////////////////////////

#ifndef DBLOGIC_H
#define DBLOGIC_H

#include "DbInterface.hpp"
#include <vector>
#include <memory>

class CEntityBase;

#ifndef ENTITY_BASE_MACRO
#define ENTITY_BASE_MACRO

// 新的条件宏，T为true，即左右相等，F为false，即左右不等
#define QC_T(l,r) std::make_tuple(std::string(l),r,1)
#define QC_F(l,r) std::make_tuple(std::string(l),r,-1)
#define QC_LT(l,r) std::make_tuple(std::string(l),r,2)
#define QC_LF(l,r) std::make_tuple(std::string(l),r,-2)

#endif
class CDbLogicBase
{
public:
	// 打开数据库
    CDbLogicBase() :m_db(nullptr)
	{
	}

    virtual ~CDbLogicBase()
	{

	}

	/////////////// 纯虚函数，平台相关函数，由派生类实现，不允许直接调用
protected:
	// 用于判断数据库是否打开，平台相关
	virtual bool IsDBOpenedImpl() = 0;
	// 用于打开数据库，平台相关，数据库类型相关
	virtual bool OpenDbImpl(const std::string& strPath) = 0;
	// 用于关闭数据库，平台相关，数据库类型相关
	virtual bool CloseDbImpl() = 0;
	// 用于获取一个uuid，平台相关
	virtual std::string GetUuidImpl() = 0;
	
	//
	virtual std::string GetAppPathImpl() = 0;


#ifdef LOGIC_USE_BACKUP
	// 用于新建数据库的时候，进行新建处理
	virtual std::string NewDbImpl() = 0;
	// 用于创建临时目录，平台相关
	virtual bool CreatePathImpl(const std::string& strPath) = 0;
	// 用于拷贝文件，平台相关
	virtual bool CopyFileImpl(const std::string& strSrc,
		const std::string& strDst) = 0;
	// 用于清除一个目录，平台相关
	virtual bool ClearDirectoryImpl(std::string& strDir, bool bIncludeSelf) = 0;
	// 用于获得另存为的路径，平台相关
	virtual std::string GetSavePathImpl() = 0;
#endif
	/////////////// 通用功能函数，
public:

	// 判断数据库是否打开
	inline bool IsDBOpened()
	{
		// 直接返回派生类实现的结果
		return IsDBOpenedImpl();
	}

	// 连接数据库
	bool OpenDB(std::string strFile)
	{
		//
		if (strFile.empty())
		{
			m_strLastError = "数据库文件名为空！";
			return false;
		}

		// 关闭数据库
		CloseDB();

		// 设置属性
		m_strFile = strFile;
		m_bSaved = true;

		
		
#ifdef LOGIC_USE_BACKUP
                m_strCurrentTempPath = GetAppPathImpl()+"Roaming/"  +GetUuidImpl() + "/";
		m_strCurrentFile = m_strCurrentTempPath + "_current";
		m_strCurrentFileTmp = m_strCurrentFile + "_tmp";
		m_iOperateCurrent = 0;
		m_iOperateMax = 0;

		// 创建临时工作目录
		if( ! CreatePathImpl(m_strCurrentTempPath) )
		{
			m_strLastError = "无法创建临时目录！";
			return false;
		}

		// 向临时目录中复制临时文件
		if( ! CopyFileImpl (m_strFile, m_strCurrentFile) )
		{
			m_strLastError = "无法复制临时文件！";
			return false;
		}
#else
		m_strCurrentFile = strFile;
#endif
		// 打开数据库
		bool bRet = OpenDbImpl(m_strCurrentFile);
		if (!bRet)
		{// 如果打开失败，
			// 记录错误信息并返回
			m_strLastError  = m_db->GetLastError();
			CloseDB ();
		}
		return bRet;
	}
	
#ifdef LOGIC_USE_BACKUP
        // 新建数据库文件
        bool NewDB()
        {
                // 关闭数据库
                CloseDB ();

                // 设置属性
                m_strFile = "";
                m_strCurrentTempPath = GetAppPathImpl() + "Roaming/"  + GetUuidImpl() + "/";
                m_strCurrentFile = m_strCurrentTempPath + "_current";
                m_strCurrentFileTmp = m_strCurrentFile + "_tmp";
                m_iOperateCurrent = 0;
                m_iOperateMax = 0;
                m_bSaved = true;

                // 创建临时工作目录
                if( ! CreatePathImpl(m_strCurrentTempPath) )
                {
                        m_strLastError = "无法创建临时目录！";
                        return false;
                }

                // 获得模板文件的数据库路径
                std::string strTemplateDbFile = NewDbImpl();
                if( strTemplateDbFile.empty() )
                {
                        m_strLastError = "新建数据库文件失败";
                        return false;
                }

                // 向临时目录中复制临时文件
                if( ! CopyFileImpl (strTemplateDbFile, m_strCurrentFile) )
                {
                        m_strLastError = "无法复制临时文件！";
                        return false;
                }
                // 打开数据库
                bool bRet = OpenDbImpl(m_strCurrentFile);
                if (!bRet)
                {// 如果打开失败，
                        // 记录错误信息并返回
                        m_strLastError  = m_db->GetLastError();
                        CloseDB ();
                }
                return bRet;
        }

	// 是否可以撤销
	bool CanGoHistory()
	{
		// todo 待需要的时候实现
		return false;
	}

	// 撤销
	void GoHistory()
	{
		// todo 待需要的时候实现
	}

	// 是否可以重做
	bool CanGoForward()
	{
		// todo 待需要的时候实现
		return false;
	}

	// 重做
	void GoForward()
	{
		// todo 待需要的时候实现
	}


        // 保存
        void Save()
        {
                if (!CanSave ())
                {// 如果不需要保存
                        return;
                }
                if (m_strFile.empty() )
                {// 文件名为空，说明是新建文件，需要选定一个保存路径
                        m_strFile = GetSavePathImpl();
                }
                // 关闭数据库连接，因为数据库文件时独占式打开，所以得关掉才行
                CloseDbImpl();
                // 复制被操作的临时文件到指定路径
                m_bSaved = CopyFileImpl(m_strCurrentFile, m_strFile);
                // 重新打开临时工作文件，
                OpenDbImpl(m_strCurrentFile);
        }

        // 另存为
        bool SaveAs()
        {
                // 选择一个保存路径
                std::string strSavePath = GetSavePathImpl();
                if( strSavePath.empty() )
                {// 如果路径是空的，
                        m_strLastError = "所选路径为空!";
                        return false;
                }
                return CopyFileImpl(m_strCurrentFile, strSavePath);
        }

        // 备份操作
        void BackupOperate()
        {
                // 有修改才调用，所以要把保存标志置为false
                m_bSaved = false;
                // todo 目前不提供备份功能的实现，可以增加
        }

#endif


	// 关闭数据库
	void CloseDB()
	{
		if (!IsDBOpened ())
		{// 如果数据库没打开，就直接返回
			return;
		}

		// 调用关闭
		CloseDbImpl ();
#ifdef LOGIC_USE_BACKUP
		// 删除文件
		ClearDirectoryImpl(m_strCurrentTempPath,true);
#endif
		// 重置属性
		m_strFile = ("");
		m_strCurrentTempPath = ("");
		m_strCurrentFile = ("");
		m_strCurrentFileTmp = ("");
		m_iOperateCurrent = 0;
		m_iOperateMax = 0;
		m_bSaved = true;

	}

	

   
    //////////////////////////以下实现普通通用数据操作

    // 读取一张数据表数据
    template<typename table_entity, typename ...Condition>
    bool getTable( std::vector<table_entity*>& vctOut,
                   Condition&&... args )
    {
        return table_entity(m_db).getTable(
                    *( reinterpret_cast< std::vector<CEntityBase*> *>(&(vctOut)) )
                    ,args...);
    }

    // 获取全表，按照指定字段顺序
    template<typename table_entity, typename ...Condition>
    bool getTable( std::vector<table_entity*>& vctOut, std::string strOrder,
                   Condition&&... args )
    {
        return table_entity(m_db).getTable(
                    *( reinterpret_cast< std::vector<CEntityBase*> *>(&(vctOut)) )
                    ,strOrder,args...);
    }

    template<typename table_entity, typename ...Condition>
    typename table_entity::DataVectorSafe getTableSafe(Condition&&... args)
    {
        typename table_entity::DataVectorSafe rs;
        std::vector<table_entity*> vctOut;
        if( ! getTable(vctOut, args...) )
        {
            return rs;
        }

        for(auto * p : vctOut )
        {
            typename table_entity::EntityPtr pOut;
            pOut.reset(p);
            rs.push_back(pOut);
        }

        return rs;

    }



    template<typename table_entity, typename ...Condition>
    typename table_entity::DataVectorSafe getTableSafe(std::string strOrder, Condition&&... args)
    {
        typename table_entity::DataVectorSafe rs;
        std::vector<table_entity*>& vctOut;
        if( ! getTable(vctOut,strOrder, args...) )
        {
            return rs;
        }

        for(auto * p : vctOut )
        {
            typename table_entity::EntityPtr pOut;
            pOut.reset(p);
            rs.push_back(pOut);
        }

        return rs;

    }

    // 读取第一条符合条件的数据记录
    template<typename table_entity, typename ...Condition>
    std::shared_ptr<table_entity> getRecord(Condition&&... arg )
    {
        table_entity* pEntity = new table_entity( m_db );
        if( pEntity && pEntity->fullfillData(arg...) )
        {
            return std::shared_ptr<table_entity>(pEntity);
        }
        delete pEntity;
        return nullptr;
    }

    // 读取id为指定值的数据
    template<typename table_entity >
    std::shared_ptr<table_entity> getRecord( int iId )
    {
       return getRecord<table_entity>(QC_T("ID",iId));
    }

    // 删除一条记录
    // 这个函数会把删掉的内容返回给上层
    template<typename table_entity>
    std::shared_ptr<table_entity> deleteRecord( int iId )
    {
        // 先从数据库中查询对应id的记录，因为这个id肯定是唯一的，
        auto ptr = getRecord<table_entity>(iId);
        if( !ptr.get() )
        {// 如果没查到，我们认为删除成功了
            table_entity* pItem = new table_entity(get_db());
            pItem->SetRecordId(iId);
            ptr.reset(pItem);
        }
        else
        {// 否则的话，执行删除操作
            if( !ptr->delete_self() )
            {// 如果删除失败了，报错返回
                m_strLastError = m_db->GetLastError();
                return nullptr;
            }
        }
        // 将查询结果返回去
        return std::shared_ptr<table_entity>(ptr);
    }


    // 删除符合条件的全部记录，返回删除总数量
    template<typename table_entity, typename ...Condition>
    int deleteRecord( Condition&&... condition )
    {
        std::vector<table_entity*> vctDelete;
        // 获得数据
        if( !getTable<table_entity>(vctDelete,condition...) )
        {
            return -1;
        }

        // 计算数量
        int iCnt = vctDelete.size();

        // 批量删除加速
        if( !batchDelete(vctDelete) )
        {
            return -1;
        }

        clear(vctDelete);
        return iCnt;
    }


    // 批量插入，
    template<typename table_entity>
    bool batchInsert(std::vector<table_entity*> vctIn )
    {
        return batch<table_entity>(vctIn,
        [&](table_entity* record)->std::string{return record? record->GenInsertSQL() : "";},
        [&](std::string strSql)->bool{return m_db->Insert(strSql);}
        );
    }

    // 批量修改
    template<typename table_entity>
    bool batchUpdate(std::vector<table_entity*> vctIn )
    {
        return batch<table_entity>(vctIn,
        [&](table_entity* record)->std::string{return record? record->GenUpdateSQL() : "";},
        [&](std::string strSql)->bool{return m_db->Update(strSql);}
        );
    }

    // 批量删除
    template<typename table_entity>
    bool batchDelete(std::vector<table_entity*> vctIn )
    {
        return batch<table_entity>(vctIn,
        [&](table_entity* record)->std::string{return record? record->GenDeleteSQL() : "";},
        [&](std::string strSql)->bool{return m_db->Delete(strSql);}
        );
    }

    // 获取一个不重复的新名称
    template<typename table_entity>
    std::string getNewName(std::string strDefault, std::string strField )
    {
        // 初始化临时名称
        std::string strTempName = strDefault;
        int iIndex = 1;

        // 开始循环判断
        while( true )
        {
            if( iIndex > 1000 )
            {// 一千次了还重复，就不继续了，防止耽误时间
                return "";
            }

            // 查询该名称是否存在
            auto ptr = getRecord<table_entity>(QC_T(strField,strTempName) );
            if( !ptr.get() )
            {// 如果不存在，说明可以用了，直接返回
                return strTempName;
            }
            // 否则就继续加后缀
            strTempName = strDefault +"_"+ std::to_string(iIndex++);
        }

        return "";
    }

    template<typename table_entity>
    typename table_entity::EntityPtr makeEntity()
    {
        return std::make_shared<table_entity>(m_db);
    }


    // 将数据复制到blob字段
    template< typename data_type>
    static size_t data2blob(const std::vector<data_type>& p, std::vector<unsigned char>& vctData )
    {
        size_t sz = p.size()* sizeof(data_type);
        vctData.resize(sz);
        memcpy(vctData.data(), p.data(),sz);
        return sz;
    }

    template< typename data_type>
    static size_t data2blob(data_type* p, size_t sz , std::vector<unsigned char>& vctData )
    {
        vctData.resize(sz);
        memcpy(vctData.data(), p,sz);
        return sz;
    }


    template< typename data_type>
    static size_t blob2data(const std::vector<unsigned char>& p, std::vector<data_type>& vctData )
    {
        size_t sz = p.size() / sizeof(data_type);
        vctData.resize(sz);
        memcpy(vctData.data(), p.data(),sz*sizeof(data_type));
        return sz;
    }

    template< typename data_type>
    static std::vector<unsigned char> data2blob(const std::vector<data_type>& p)
    {
        std::vector<unsigned char> rs;
        data2blob(p,rs);
        return rs;
    }
    template< typename data_type>
    static std::vector<data_type> blob2data(const std::vector<unsigned char>& p)
    {
        std::vector<data_type> rs;
        blob2data(p,rs);
        return rs;
    }


protected:
    // 执行批量操作
    template<typename table_entity>
    inline bool batch( std::vector<table_entity*>& vctIn, //批量操作结构体
                std::function<std::string(table_entity*)> funcGenSql, // 生成sql语句的
                std::function<bool(std::string)> funcOperate // 执行批量的算子
                                )
    {

        // 检测数据库指针有效
        if( !m_db )
        {
            return false;
        }

        // 检测必要功能函数的有效性
        if( !funcOperate || !funcGenSql )
        {
            return false;
        }
#ifdef LOGIC_USE_BACKUP
        // 备份数据库
        BackupOperate();
#else
        m_bSaved = false;
#endif

        // 事务开始
        m_db->begin();
        std::string strSql;


        for( int i=0; i< vctIn.size(); i++ )
        {
            // 连接sql语句
            strSql += funcGenSql(vctIn[i]) + ";";

            if( i && !(i%m_iBatchSetp) )
            {// 如果不是第一条，并且到达了批量操作的单步记录数

                // 调用函数进行批量操作
                if( !funcOperate(strSql) )
                {// 如果失败了
                    // 回滚，报错，退出
                    m_db->rollback();
                    m_strLastError = m_db->GetLastError();
                    return false;
                }
                // 清空sql语句继续下一步
                strSql = "";
            }
        }

        // 循环结束后，还有未操作的记录
        if( !strSql.empty() )
        {
            // 调用函数进行批量操作
            if( !funcOperate(strSql) )
            {// 如果失败了
                // 回滚，报错，退出
                m_db->rollback();
                m_strLastError = m_db->GetLastError();
                return false;
            }
        }

        //提交事务
        m_db->commit();
        return true;
    }

/////////////// 内联函数
public:


    // 检查是否可保存
    inline bool CanSave ()
    {
        return !m_bSaved;
    }
    // 获取数据库访问对象指针
    inline CDbInterface *get_db ()
    {
        return m_db;
    }
    // 获取错误信息
    std::string GetErrorMsg()
    {
        return m_strLastError;
    }

    template<	typename data_type,  //第一模板参数，容器的数据类型
                // 第二模板参数，一个以数据类型和allocator为模板的容器，适配stl容器，，默认使用std::allocator
                template< typename container_data_type, typename al = std::allocator<container_data_type> >
                    class container_type = std::vector
            >
    inline static void clear( container_type<data_type*>& container )
    {
        for(auto it = container.begin();
            it != container.end();
            it ++ )
        {
            if( *it )
            {
                delete (*it);
            }
        }
        container.clear();
    }




///////////// 成员变量
protected:
    // 数据库对象
    CDbInterface *m_db;
    // 通过参数传进来的属性值
    std::string m_strFile;
    // 当前临时文件目录
    std::string m_strCurrentTempPath;
    // 当前操作的文件路径名
    std::string m_strCurrentFile;
    // 当前操作临时文件路径名
    std::string m_strCurrentFileTmp;
    // 最近一次发生的错误信息
    std::string m_strLastError;

    // 涉及回退和前进的属性值
    // 当前操作步骤标识
    int m_iOperateCurrent;
    // 最大步骤标识
    int m_iOperateMax;
    // 是否保存过
    bool m_bSaved;

    const int m_iBatchSetp = 100;
};

#endif // DBLOGIC_H
