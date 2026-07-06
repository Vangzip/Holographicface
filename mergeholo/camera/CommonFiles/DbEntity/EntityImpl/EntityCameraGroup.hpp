
////////////////////////////////////////////////////////////////////////////////
//
// File Name:	C:/Users/Jumper/Desktop/ddd/EntityCameraGroup.hpp
// Class Name:	CEntityCameraGroup
// Description:	
// Author:		
// Date:		周二 11月 15 2022
// Comment:     此文件由程序自动生成，非必要或者不了解机制请不要擅自修改！！！！！
//              由1.0.0.2版本创建,使用时请核对。
////////////////////////////////////////////////////////////////////////////////

#ifndef ENTITY_CAMERAGROUP_HPP
#define ENTITY_CAMERAGROUP_HPP

#pragma once
#include "DbCommonHeader.h"
#include "EntityBase.hpp"

class CEntityCameraGroup : public CEntityBase
{
////////////////////////////第0部分 变量类型声明
public:
    using EntityPtr = std::shared_ptr<CEntityCameraGroup>;
    using DataVector = std::vector<CEntityCameraGroup*>;
    using DataVectorSafe = std::vector<EntityPtr>;

////////////////////////////第一部分 用一个宏来写这个类的基础函数支持，具体如下
    // 必带宏， 生成构造析构函数，必要的虚函数实现，等等
    // 第一个参数是这个类名，
    // 第二个参数是对应的数据库中的表明，大小写不敏感
    // 第三个参数是如果支持复制的话，此数据对应的复制类型，现在可以不用，所以暂时写成NONE
    ALL_OBJECT_FUNCTION_WITH_COPYTYPE (CEntityCameraGroup, ("CameraGroup"),DB_COPYTYPE_NONE)

///////////////////////////第二部分，声明本表字段，这些字段和数据库字段顺序要对应，不然可能出错

public:
    int m_iGroupIndex; //
	std::string m_strName; //
	std::string m_strDesc; //
	double m_dBaseline; //
	int m_iLeftId; //
	int m_iRightId; //
	std::vector<unsigned char> m_vctRtMatrixInner; //
	std::vector<unsigned char> m_vctRtMatrixOutter; //
	std::vector<unsigned char> m_vctremap00; //
	std::vector<unsigned char> m_vctremap01; //
	std::vector<unsigned char> m_vctremap10; //
	std::vector<unsigned char> m_vctremap11; //
	std::vector<unsigned char> m_vctQ; //
	
//////////////////////////第三部分，必须实现的几个虚函数
protected:
    // 初始化类成员，构造函数中调用，顺序可以随意，可以不初始化m_iId,但是写了也无妨
    virtual void init ()
    {
        init_value (m_iId);
        init_value (m_iGroupIndex);
		init_value (m_strName);
		init_value (m_strDesc);
		init_value (m_dBaseline);
		init_value (m_iLeftId);
		init_value (m_iRightId);
		init_value (m_vctRtMatrixInner);
		init_value (m_vctRtMatrixOutter);
		init_value (m_vctremap00);
		init_value (m_vctremap01);
		init_value (m_vctremap10);
		init_value (m_vctremap11);
		init_value (m_vctQ);
		
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
        pRecordSet->get_value(iIndex++,m_iGroupIndex);
		pRecordSet->get_value(iIndex++,m_strName);
		pRecordSet->get_value(iIndex++,m_strDesc);
		pRecordSet->get_value(iIndex++,m_dBaseline);
		pRecordSet->get_value(iIndex++,m_iLeftId);
		pRecordSet->get_value(iIndex++,m_iRightId);
		pRecordSet->get_value(iIndex++,m_vctRtMatrixInner);
		pRecordSet->get_value(iIndex++,m_vctRtMatrixOutter);
		pRecordSet->get_value(iIndex++,m_vctremap00);
		pRecordSet->get_value(iIndex++,m_vctremap01);
		pRecordSet->get_value(iIndex++,m_vctremap10);
		pRecordSet->get_value(iIndex++,m_vctremap11);
		pRecordSet->get_value(iIndex++,m_vctQ);
		
    }

    // 将数据组织成字符串，供各种语句使用，
    // 用这张表举例子，插入语句要写成 insert into CameraGroup values( null, 'name1', 'type1', true);
    // 更新语句要写成 update CameraGroup set name='name2', type='type2', is_checked=false where id = 1;
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
            13,
            //需要连接的字符串个数，不包括id， 有几个就写几个，这个有13个，所以是13
            // 上一行写的是几，从这里开始，就要有几个，不然就会出错
            // 这里的顺序必须按照数据库中字段顺序来写，不然insert语句会出错
            init_sql("GroupIndex",m_iGroupIndex,bWithFieldName),
			init_sql("Name",m_strName,bWithFieldName),
			init_sql("Desc",m_strDesc,bWithFieldName),
			init_sql("Baseline",m_dBaseline,bWithFieldName),
			init_sql("LeftId",m_iLeftId,bWithFieldName),
			init_sql("RightId",m_iRightId,bWithFieldName),
			init_sql("RtMatrixInner",m_vctRtMatrixInner,bWithFieldName),
			init_sql("RtMatrixOutter",m_vctRtMatrixOutter,bWithFieldName),
			init_sql("remap00",m_vctremap00,bWithFieldName),
			init_sql("remap01",m_vctremap01,bWithFieldName),
			init_sql("remap10",m_vctremap10,bWithFieldName),
			init_sql("remap11",m_vctremap11,bWithFieldName),
			init_sql("Q",m_vctQ,bWithFieldName)
            );
    }

    virtual bool saveBlob()
    {
        saveBlobImpl("RtMatrixInner",m_vctRtMatrixInner);
		saveBlobImpl("RtMatrixOutter",m_vctRtMatrixOutter);
		saveBlobImpl("remap00",m_vctremap00);
		saveBlobImpl("remap01",m_vctremap01);
		saveBlobImpl("remap10",m_vctremap10);
		saveBlobImpl("remap11",m_vctremap11);
		saveBlobImpl("Q",m_vctQ);
        return true;
    }

/////////////////////////////////第四部分，赋值操作符，
public:
    // 复制记录
    virtual CEntityBase &operator= (CEntityCameraGroup &o)
    {
        CEntityCameraGroup *po = dynamic_cast<CEntityCameraGroup*>(&o);
        if (po)
        {
            //////////////////////这个函数只在这里修改就行了，顺序无所谓
            DB_ASSIGNMENT_OPERATER_FIELD(m_iGroupIndex, po);
			DB_ASSIGNMENT_OPERATER_FIELD(m_strName, po);
			DB_ASSIGNMENT_OPERATER_FIELD(m_strDesc, po);
			DB_ASSIGNMENT_OPERATER_FIELD(m_dBaseline, po);
			DB_ASSIGNMENT_OPERATER_FIELD(m_iLeftId, po);
			DB_ASSIGNMENT_OPERATER_FIELD(m_iRightId, po);
			DB_ASSIGNMENT_OPERATER_FIELD(m_vctRtMatrixInner, po);
			DB_ASSIGNMENT_OPERATER_FIELD(m_vctRtMatrixOutter, po);
			DB_ASSIGNMENT_OPERATER_FIELD(m_vctremap00, po);
			DB_ASSIGNMENT_OPERATER_FIELD(m_vctremap01, po);
			DB_ASSIGNMENT_OPERATER_FIELD(m_vctremap10, po);
			DB_ASSIGNMENT_OPERATER_FIELD(m_vctremap11, po);
			DB_ASSIGNMENT_OPERATER_FIELD(m_vctQ, po);
			
        }

        return CEntityBase::operator= (o);
    }
};

#endif // ENTITY_CAMERAGROUP_HPP
