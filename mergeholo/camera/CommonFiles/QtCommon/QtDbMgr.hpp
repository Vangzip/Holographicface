////////////////////////////////////////////////////////////////////////////////
//
// File Name:	dbmgr.hpp
// Class Name:  CDbMgr
// Description:	用于管理唯一的静态数据库对象，
// Author:		罗志彬
// Date:		2019年4月22日
//
////////////////////////////////////////////////////////////////////////////////

#ifndef QT_DBMGR_HPP
#define QT_DBMGR_HPP

#include "DbMgr.hpp"

class CDbLogic;

class CQtDbMgr : public CDbMgr<CDbLogic>
{
public:
    using CDbMgr<CDbLogic>::CDbMgr;
};

#define INIT_DB \
    auto& qtDbLogic = CQtDbMgr::GetDb();\
    if( ! qtDbLogic.IsDBOpened() )\
    {\
    return ;\
    }


#define INIT_DB_RT( rt_value ) \
    auto& qtDbLogic = CQtDbMgr::GetDb();\
    if( ! qtDbLogic.IsDBOpened() )\
    {\
    return rt_value;\
    }
#endif // DBMGR_HPP
