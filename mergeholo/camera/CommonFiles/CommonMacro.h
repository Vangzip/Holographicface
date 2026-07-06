#ifndef COMMON_MACRO_H
#define COMMON_MACRO_H

#define MAKE_SINGLE_CASE(class_name )\
public:\
    static class_name& get()\
    {\
        static class_name static_instance_##class_name;\
        return static_instance_##class_name;\
    }\
public:\
    class_name(const class_name& ) = delete;\
    class_name(const class_name&&) = delete;\
    class_name& operator =( const class_name& ) = delete;\
    class_name& operator =( const class_name&& ) = delete;


#define CUDACHECK(cmd) do {                         \
  cudaError_t e = cmd;                              \
  if( e != cudaSuccess ) {                          \
    printf("Failed: Cuda error %s:%d '%s'\n",             \
        __FILE__,__LINE__,cudaGetErrorString(e));   \
    exit(EXIT_FAILURE);                             \
  }                                                 \
} while(0)


#define NCCLCHECK(cmd) do {                         \
  ncclResult_t r = cmd;                             \
  if (r != ncclSuccess) {                            \
    printf("Failed, NCCL error %s:%d '%s'\n",             \
        __FILE__,__LINE__,ncclGetErrorString(r));   \
    exit(EXIT_FAILURE);                             \
  }                                                 \
} while(0)

#endif // COMMON_MACRO_H

#if 1
#include <QMessageBox>
using MessageBoxType = QMessageBox;
#else
#include "Widgets/DlgMessageBox.h"
using MessageBoxType = DlgMessageBox;

#endif
// log macro using

#ifdef USING_LOGGER
#include "JpLogger.h"
#define LOG_OUTPUT JP::CJpLoggerStream(JP::Test, "log/JpLog.txt")
#define LOG_ERROR  JP::CJpLoggerStream(JP::Error, "log/JpLog.txt")
#define LOG_DEBUG  JP::CJpLoggerStream(JP::Debug, "log/JpLog.txt")
#else
#if 0
#define LOG_OUTPUT(log_content) \
    qDebug()<< qPrintable(QString("[JPTest : %1] ").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz")) )<< log_content ;

#define LOG_ERROR(log_content) \
    qDebug()<< qPrintable(QString("[JPError: %1] ").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz")) )<< log_content ;

#define LOG_DEBUG( log_content ) \
    qDebug()<< qPrintable(QString("[JPDebug: %1] ").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz")) ) << log_content ;
#endif

#endif



#define SHOW_WARNING(pParent,str )   \
    MessageBoxType::warning(pParent,QObject::tr("警告"),(str) )




#define SHOW_WARNING_FORSELF( str )   \
    MessageBoxType::warning(this,tr("警告"),(str) )

#define SHOW_ERROR_FORSELF( str )   \
    MessageBoxType::critical(this,tr("错误"), (str) )

#define SHOW_SUCCESS_FORSELF( str )   \
    MessageBoxType::information(this,tr("成功"), (str) )

#define SHOW_CONFIRM_FORSELF( str ) \
    MessageBoxType::information(this,tr("确认"),(str),QMessageBox::Yes|QMessageBox::No,QMessageBox::No)

#define SHOW_CONFIRM_3_FORSELF( str ) \
    MessageBoxType::information(this,tr("确认"),(str),QMessageBox::Yes|QMessageBox::No|QMessageBox::Cancel,QMessageBox::Cancel)

#define ENSURE_DELETE( content ) \
if( QMessageBox::Yes != SHOW_CONFIRM_FORSELF(content) )\
{\
    return ;\
}

#define ENSURE_DELETE_RT( content, rt_value ) ) \
if( QMessageBox::Yes != SHOW_CONFIRM_FORSELF(content)\
{\
    return rt_value;\
}


#define DECLARE_PTR( classname ) \
public:\
    using Ptr = std::shared_ptr<classname>;\
    template<typename ... params >\
    static Ptr makePtr( params ... arg )\
    {\
        return std::make_shared<classname>(arg...);\
    }

class QThread;
#include <memory>
using QThreadPtr = std::shared_ptr<QThread>;
