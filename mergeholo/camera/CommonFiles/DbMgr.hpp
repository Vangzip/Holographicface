////////////////////////////////////////////////////////////////////////////////
//
// File Name:	dbmgr.hpp
// Class Name:  CDbMgr
// Description:	用于管理唯一的静态数据库对象，
// Author:		罗志彬
// Date:		2019年4月22日
//
////////////////////////////////////////////////////////////////////////////////

#ifndef DBMGR_HPP
#define DBMGR_HPP

template<typename DB_LOGIC>
class CDbMgr
{
public:
    // 获得数据库对象的引用
    static DB_LOGIC& GetDb()
    {
        // 构造静态对象，进程唯一
        static DB_LOGIC oDbInstance;
        return oDbInstance;
    }

};


#endif // DBMGR_HPP
