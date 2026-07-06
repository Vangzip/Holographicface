
////////////////////////////////////////////////////////////////////////////////
//
// File Name:	C:/Users/Jumper/Desktop/ddd/EntityImage.hpp
// Class Name:	CEntityImage
// Description:	
// Author:		
// Date:		周二 11月 15 2022
// Comment:     此文件由程序自动生成，非必要或者不了解机制请不要擅自修改！！！！！
//              由1.0.0.2版本创建,使用时请核对。
////////////////////////////////////////////////////////////////////////////////

#ifndef ENTITY_IMAGE_HPP
#define ENTITY_IMAGE_HPP

#pragma once
#include "DbCommonHeader.h"
#include "EntityBase.hpp"

class CEntityImage : public CEntityBase
{
////////////////////////////第0部分 变量类型声明
public:
    using EntityPtr = std::shared_ptr<CEntityImage>;
    using DataVector = std::vector<CEntityImage*>;
    using DataVectorSafe = std::vector<EntityPtr>;

////////////////////////////第一部分 用一个宏来写这个类的基础函数支持，具体如下
    // 必带宏， 生成构造析构函数，必要的虚函数实现，等等
    // 第一个参数是这个类名，
    // 第二个参数是对应的数据库中的表明，大小写不敏感
    // 第三个参数是如果支持复制的话，此数据对应的复制类型，现在可以不用，所以暂时写成NONE
    ALL_OBJECT_FUNCTION_WITH_COPYTYPE (CEntityImage, ("Image"),DB_COPYTYPE_NONE)

///////////////////////////第二部分，声明本表字段，这些字段和数据库字段顺序要对应，不然可能出错

public:
    long long m_llTimestamp; //
	std::string m_strPath; //
	std::vector<unsigned char> m_vctData; //
	std::vector<unsigned char> m_vctDataRGB; //
	std::vector<unsigned char> m_vctDataGray; //
	int m_iCameraId; //
	
//////////////////////////第三部分，必须实现的几个虚函数
protected:
    // 初始化类成员，构造函数中调用，顺序可以随意，可以不初始化m_iId,但是写了也无妨
    virtual void init ()
    {
        init_value (m_iId);
        init_value (m_llTimestamp);
		init_value (m_strPath);
		init_value (m_vctData);
		init_value (m_vctDataRGB);
		init_value (m_vctDataGray);
		init_value (m_iCameraId);
		
    }


    // 这个函数用来将数据从结果集中读出，并且存入对应的变量里，
    // 注意顺序必须与数据库表中的一致，必须获得m_iId
    virtual void Produce( CRecordsetMgr* pRecordSet )
    {
        if( !pRecordSet )
        {
            return ;
        }
        int iIndex = 0;
        pRecordSet->get_value(iIndex++,m_iId);
        pRecordSet->get_value(iIndex++,m_llTimestamp);
		pRecordSet->get_value(iIndex++,m_strPath);
		pRecordSet->get_value(iIndex++,m_vctData);
		pRecordSet->get_value(iIndex++,m_vctDataRGB);
		pRecordSet->get_value(iIndex++,m_vctDataGray);
		pRecordSet->get_value(iIndex++,m_iCameraId);
		
    }

    // 将数据组织成字符串，供各种语句使用，
    // 用这张表举例子，插入语句要写成 insert into Image values( null, 'name1', 'type1', true);
    // 更新语句要写成 update Image set name='name2', type='type2', is_checked=false where id = 1;
    // 所以参数为true时，会带有字段名，用逗号分隔，供update使用 name='name2', type='type2', is_checked=false
    // 参数为false是，不带字段名，逗号分隔，供insert使用 'name1', 'type1', true
    virtual std::string GenDataSql(bool bWithFieldName = false )
    {
        // 调用连接字符串，将各个数据连接起来
        // 这是一个变参函数，使用时注意，
        return CCommonString::joinString<std::string>(
            ",",// 分隔符，使用逗号， 不要修改
            true, // 排除空格
            //////////////////////////////////////////////////////新增的表从这里开始改
            6,
            //需要连接的字符串个数，不包括id， 有几个就写几个，这个有6个，所以是6
            // 上一行写的是几，从这里开始，就要有几个，不然就会出错
            // 这里的顺序必须按照数据库中字段顺序来写，不然insert语句会出错
            init_sql("Timestamp",m_llTimestamp,bWithFieldName),
			init_sql("Path",m_strPath,bWithFieldName),
			init_sql("Data",m_vctData,bWithFieldName),
			init_sql("DataRGB",m_vctDataRGB,bWithFieldName),
			init_sql("DataGray",m_vctDataGray,bWithFieldName),
			init_sql("CameraId",m_iCameraId,bWithFieldName)
            );
    }

    virtual bool saveBlob()
    {
        saveBlobImpl("Data",m_vctData);
		saveBlobImpl("DataRGB",m_vctDataRGB);
		saveBlobImpl("DataGray",m_vctDataGray);
        return true;
    }

/////////////////////////////////第四部分，赋值操作符，
public:
    // 复制记录
    virtual CEntityBase &operator= (CEntityImage &o)
    {
        CEntityImage *po = dynamic_cast<CEntityImage*>(&o);
        if (po)
        {
            //////////////////////这个函数只在这里修改就行了，顺序无所谓
            DB_ASSIGNMENT_OPERATER_FIELD(m_llTimestamp, po);
			DB_ASSIGNMENT_OPERATER_FIELD(m_strPath, po);
			DB_ASSIGNMENT_OPERATER_FIELD(m_vctData, po);
			DB_ASSIGNMENT_OPERATER_FIELD(m_vctDataRGB, po);
			DB_ASSIGNMENT_OPERATER_FIELD(m_vctDataGray, po);
			DB_ASSIGNMENT_OPERATER_FIELD(m_iCameraId, po);
			
        }

        return CEntityBase::operator= (o);
    }
};

#endif // ENTITY_IMAGE_HPP
