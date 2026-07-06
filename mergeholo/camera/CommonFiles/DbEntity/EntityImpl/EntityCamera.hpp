
////////////////////////////////////////////////////////////////////////////////
//
// File Name:	C:/Users/Jumper/Desktop/EntityCamera.hpp
// Class Name:	CEntityCamera
// Description:	
// Author:		
// Date:		周一 1月 30 2023
// Comment:     此文件由程序自动生成，非必要或者不了解机制请不要擅自修改！！！！！
//              由1.0.0.2版本创建,使用时请核对。
////////////////////////////////////////////////////////////////////////////////

#ifndef ENTITY_CAMERA_HPP
#define ENTITY_CAMERA_HPP

#pragma once
#include "DbCommonHeader.h"
#include "EntityBase.hpp"

class CEntityCamera : public CEntityBase
{
////////////////////////////第0部分 变量类型声明
public:
    using EntityPtr = std::shared_ptr<CEntityCamera>;
    using DataVector = std::vector<CEntityCamera*>;
    using DataVectorSafe = std::vector<EntityPtr>;

////////////////////////////第一部分 用一个宏来写这个类的基础函数支持，具体如下
    // 必带宏， 生成构造析构函数，必要的虚函数实现，等等
    // 第一个参数是这个类名，
    // 第二个参数是对应的数据库中的表明，大小写不敏感
    // 第三个参数是如果支持复制的话，此数据对应的复制类型，现在可以不用，所以暂时写成NONE
    ALL_OBJECT_FUNCTION_WITH_COPYTYPE (CEntityCamera, ("Camera"),DB_COPYTYPE_NONE)

///////////////////////////第二部分，声明本表字段，这些字段和数据库字段顺序要对应，不然可能出错

public:
    int m_iCameraIndex; //
	std::string m_strName; //
	std::string m_strDesc; //
	double m_dfx; //
	double m_dfy; //
	double m_dcx; //
	double m_dcy; //
	double m_dk0; //
	double m_dk1; //
	double m_dk2; //
	double m_dp0; //
	double m_dp1; //
	double m_ds0; //
	int m_iStatus; //
	
//////////////////////////第三部分，必须实现的几个虚函数
protected:
    // 初始化类成员，构造函数中调用，顺序可以随意，可以不初始化m_iId,但是写了也无妨
    virtual void init ()
    {
        init_value (m_iId);
        init_value (m_iCameraIndex);
		init_value (m_strName);
		init_value (m_strDesc);
		init_value (m_dfx);
		init_value (m_dfy);
		init_value (m_dcx);
		init_value (m_dcy);
		init_value (m_dk0);
		init_value (m_dk1);
		init_value (m_dk2);
		init_value (m_dp0);
		init_value (m_dp1);
		init_value (m_ds0);
		init_value (m_iStatus);
		
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
        pRecordSet->get_value(iIndex++,m_iCameraIndex);
		pRecordSet->get_value(iIndex++,m_strName);
		pRecordSet->get_value(iIndex++,m_strDesc);
		pRecordSet->get_value(iIndex++,m_dfx);
		pRecordSet->get_value(iIndex++,m_dfy);
		pRecordSet->get_value(iIndex++,m_dcx);
		pRecordSet->get_value(iIndex++,m_dcy);
		pRecordSet->get_value(iIndex++,m_dk0);
		pRecordSet->get_value(iIndex++,m_dk1);
		pRecordSet->get_value(iIndex++,m_dk2);
		pRecordSet->get_value(iIndex++,m_dp0);
		pRecordSet->get_value(iIndex++,m_dp1);
		pRecordSet->get_value(iIndex++,m_ds0);
		pRecordSet->get_value(iIndex++,m_iStatus);
		
    }

    // 将数据组织成字符串，供各种语句使用，
    // 用这张表举例子，插入语句要写成 insert into Camera values( null, 'name1', 'type1', true);
    // 更新语句要写成 update Camera set name='name2', type='type2', is_checked=false where id = 1;
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
            14,
            //需要连接的字符串个数，不包括id， 有几个就写几个，这个有14个，所以是14
            // 上一行写的是几，从这里开始，就要有几个，不然就会出错
            // 这里的顺序必须按照数据库中字段顺序来写，不然insert语句会出错
            init_sql("CameraIndex",m_iCameraIndex,bWithFieldName),
			init_sql("Name",m_strName,bWithFieldName),
			init_sql("Desc",m_strDesc,bWithFieldName),
			init_sql("fx",m_dfx,bWithFieldName),
			init_sql("fy",m_dfy,bWithFieldName),
			init_sql("cx",m_dcx,bWithFieldName),
			init_sql("cy",m_dcy,bWithFieldName),
			init_sql("k0",m_dk0,bWithFieldName),
			init_sql("k1",m_dk1,bWithFieldName),
			init_sql("k2",m_dk2,bWithFieldName),
			init_sql("p0",m_dp0,bWithFieldName),
			init_sql("p1",m_dp1,bWithFieldName),
			init_sql("s0",m_ds0,bWithFieldName),
			init_sql("Status",m_iStatus,bWithFieldName)
            );
    }

    virtual bool saveBlob()
    {
        
        return true;
    }

/////////////////////////////////第四部分，赋值操作符，
public:
    // 复制记录
    virtual CEntityBase &operator= (CEntityCamera &o)
    {
        CEntityCamera *po = dynamic_cast<CEntityCamera*>(&o);
        if (po)
        {
            //////////////////////这个函数只在这里修改就行了，顺序无所谓
            DB_ASSIGNMENT_OPERATER_FIELD(m_iCameraIndex, po);
			DB_ASSIGNMENT_OPERATER_FIELD(m_strName, po);
			DB_ASSIGNMENT_OPERATER_FIELD(m_strDesc, po);
			DB_ASSIGNMENT_OPERATER_FIELD(m_dfx, po);
			DB_ASSIGNMENT_OPERATER_FIELD(m_dfy, po);
			DB_ASSIGNMENT_OPERATER_FIELD(m_dcx, po);
			DB_ASSIGNMENT_OPERATER_FIELD(m_dcy, po);
			DB_ASSIGNMENT_OPERATER_FIELD(m_dk0, po);
			DB_ASSIGNMENT_OPERATER_FIELD(m_dk1, po);
			DB_ASSIGNMENT_OPERATER_FIELD(m_dk2, po);
			DB_ASSIGNMENT_OPERATER_FIELD(m_dp0, po);
			DB_ASSIGNMENT_OPERATER_FIELD(m_dp1, po);
			DB_ASSIGNMENT_OPERATER_FIELD(m_ds0, po);
			DB_ASSIGNMENT_OPERATER_FIELD(m_iStatus, po);
			
        }

        return CEntityBase::operator= (o);
    }
};

#endif // ENTITY_CAMERA_HPP
