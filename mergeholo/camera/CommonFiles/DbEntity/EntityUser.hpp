
////////////////////////////////////////////////////////////////////////////////
//
// File Name:	F:/share/EntityUser.hpp
// Class Name:	CEntityUser
// Description:	
// Author:		
// Date:		周三 3月 20 2024
// Comment:     此文件由程序自动生成，非必要或者不了解机制请不要擅自修改！！！！！
//              由1.0.0.2版本创建,使用时请核对。
////////////////////////////////////////////////////////////////////////////////

#ifndef ENTITY_USER_HPP
#define ENTITY_USER_HPP

#pragma once
#include "DbCommonHeader.h"
#include "EntityBase.hpp"

class CEntityUser : public CEntityBase
{
////////////////////////////第0部分 变量类型声明
public:
    using EntityPtr = std::shared_ptr<CEntityUser>;
    using DataVector = std::vector<CEntityUser*>;
    using DataVectorSafe = std::vector<EntityPtr>;

////////////////////////////第一部分 用一个宏来写这个类的基础函数支持，具体如下
    // 必带宏， 生成构造析构函数，必要的虚函数实现，等等
    // 第一个参数是这个类名，
    // 第二个参数是对应的数据库中的表明，大小写不敏感
    // 第三个参数是如果支持复制的话，此数据对应的复制类型，现在可以不用，所以暂时写成NONE
    ALL_OBJECT_FUNCTION_WITH_COPYTYPE (CEntityUser, ("User"),DB_COPYTYPE_NONE)

///////////////////////////第二部分，声明本表字段，这些字段和数据库字段顺序要对应，不然可能出错

public:
    int m_iSeq; //
	std::string m_strname; //
	std::vector<unsigned char> m_vctpwd; //
	std::string m_strdesc; //
	std::string m_strphone; //
	std::string m_strsex; //
	std::vector<unsigned char> m_vctpwdhash; //
	
//////////////////////////第三部分，必须实现的几个虚函数
protected:
    // 初始化类成员，构造函数中调用，顺序可以随意，可以不初始化m_iId,但是写了也无妨
    virtual void init ()
    {
        init_value (m_iId);
        init_value (m_iSeq);
		init_value (m_strname);
		init_value (m_vctpwd);
		init_value (m_strdesc);
		init_value (m_strphone);
		init_value (m_strsex);
		init_value (m_vctpwdhash);
		
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
        pRecordSet->get_value(iIndex++,m_iSeq);
		pRecordSet->get_value(iIndex++,m_strname);
		pRecordSet->get_value(iIndex++,m_vctpwd);
		pRecordSet->get_value(iIndex++,m_strdesc);
		pRecordSet->get_value(iIndex++,m_strphone);
		pRecordSet->get_value(iIndex++,m_strsex);
		pRecordSet->get_value(iIndex++,m_vctpwdhash);
		
    }

    // 将数据组织成字符串，供各种语句使用，
    // 用这张表举例子，插入语句要写成 insert into User values( null, 'name1', 'type1', true);
    // 更新语句要写成 update User set name='name2', type='type2', is_checked=false where id = 1;
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
            7,
            //需要连接的字符串个数，不包括id， 有几个就写几个，这个有6个，所以是6
            // 上一行写的是几，从这里开始，就要有几个，不然就会出错
            // 这里的顺序必须按照数据库中字段顺序来写，不然insert语句会出错
            init_sql("Seq",m_iSeq,bWithFieldName),
			init_sql("name",m_strname,bWithFieldName),
			init_sql("pwd",m_vctpwd,bWithFieldName),
			init_sql("desc",m_strdesc,bWithFieldName),
			init_sql("phone",m_strphone,bWithFieldName),
                        init_sql("sex",m_strsex,bWithFieldName),
                        init_sql("pwdhash",m_vctpwdhash,bWithFieldName)
            );
    }

    virtual bool saveBlob()
    {
        saveBlobImpl("pwd",m_vctpwd);
        saveBlobImpl("pwdhash",m_vctpwdhash);
        return true;
    }

/////////////////////////////////第四部分，赋值操作符，
public:
    // 复制记录
    virtual CEntityBase &operator= (CEntityUser &o)
    {
        CEntityUser *po = dynamic_cast<CEntityUser*>(&o);
        if (po)
        {
            //////////////////////这个函数只在这里修改就行了，顺序无所谓
            DB_ASSIGNMENT_OPERATER_FIELD(m_iSeq, po);
			DB_ASSIGNMENT_OPERATER_FIELD(m_strname, po);
			DB_ASSIGNMENT_OPERATER_FIELD(m_vctpwd, po);
			DB_ASSIGNMENT_OPERATER_FIELD(m_strdesc, po);
			DB_ASSIGNMENT_OPERATER_FIELD(m_strphone, po);
			DB_ASSIGNMENT_OPERATER_FIELD(m_strsex, po);
			DB_ASSIGNMENT_OPERATER_FIELD(m_vctpwdhash, po);
			
        }

        return CEntityBase::operator= (o);
    }
};

#endif // ENTITY_USER_HPP
