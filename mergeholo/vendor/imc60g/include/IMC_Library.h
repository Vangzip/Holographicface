/**
 * @file IMC_Library.h
 * @brief 运动控制二代卡用户指令API
 * @copyright Copyright (c) 2025  Inovance
 *******************************************************************/

#pragma once
#if (_LINUX_)
#define IMC_API unsigned int
#else
#define IMC_API extern "C" unsigned int __stdcall ///< API返回类型
#endif

// 数据结构定义

/// @addtogroup EtherCAT
/// @{

/// @struct TMasterInfo
/// @brief  主站信息结构体
typedef struct
{
    unsigned int cycleTime; ///< 规划周期
    short sysHwCfg;         ///< 总线配置类型： bit0:Ecat0; bit1:Ecat1; bit2:端子板
    short aliasMode;        ///< 别名模式: 0 未开启, 1 开启
    short pdoLen;           ///< pdo总长度
    short slaveCnt;         ///< 从站个数
    short axisCnt;          ///< 轴个数
    short dioModuleCnt;     ///< dio模块个数
    short aioModuleCnt;     ///< aio模块个数
    short regModuleCnt;     ///< reg模块个数
    short diCnt;            ///< di数量
    short doCnt;            ///< do数量
    short aiCnt;            ///< ai,AD数量
    short aoCnt;            ///< ao, DA数量
    short regInCnt;         ///< reg输入数量
    short regOutCnt;        ///< reg输出数量
    short encCnt;           ///< enc通道数量
    short align;            ///< 预留
} TMasterInfo;

/// @struct TSlaveInfo
/// @brief  从站信息结构体
typedef struct
{
    short devType;            ///< 设备类型
    unsigned int vendorId;    ///< 厂商ID
    unsigned int productCode; ///< 产品编码
    unsigned int revisionNo;  ///< 修订版本
    short axChn;              ///< 从站起始通道号
    short axisCnt;            ///< 当前从站轴数
    short actStation;         ///< 当前从站对应的实际站号
    unsigned int aliasNo;     ///< 别名
    unsigned int align;       ///< 预留
} TSlaveInfo;

/// @struct THomingPara
/// @brief  回零参数结构体
typedef struct
{
    short homeMethod;     ///< 回原点方法
    unsigned int highVel; ///< 高速搜索减速点速度 pulse/s
    unsigned int lowVel;  ///< 搜索原点低速速度 pulse/s
    unsigned int acc;     ///< 加速度 pulse/s^2
    int offset;           ///< 回原点后的零点偏执 pulse
} THomingPara;
/// @}

/// @addtogroup LocalMultCompare
/// @{
/// @struct TMultiCmpData
/// @brief  多维位置比较数据结构体
typedef struct
{
    int compareData[3]; /**< 多维比较的位置，若为二维比较输出，则compareData[0]，compareData[1]对应设置的各位置源通道的比较值，将compareData[2]值写0 */
} TMultiCmpData;
/// @}

/// @addtogroup Axis
/// @{

/// @struct TMtPara
/// @brief  规划运动限制参数结构体
typedef struct
{
    double bgVel;    ///< 起始速度值     参数范围[0,doubleMax) unit/s
    double maxVel;   ///< 最大速度       参数范围(0,doubleMax) unit/s
    double maxAcc;   ///< 最大加速度     参数范围(0,doubleMax) unit/s^2
    double maxDec;   ///< 最大减速度     参数范围(0,doubleMax) unit/s^2
    double maxJerk;  ///< 最大加加速度   参数范围(0,doubleMax) unit/s^3
    double stopDec;  ///< 平滑停止减速度 参数范围(0,maxDec] unit/s^2
    double eStopDec; ///< 急停减速度     参数范围(0,maxDec] unit/s^2
} TMtPara;

/// @struct TAxAttriPara
/// @brief  轴属性配置结构体
typedef struct
{
    short arrivalBand;          ///< 到位误差 pulse
    unsigned short arrivalTime; ///< 到位保持时间 周期
    int errorLmt;               ///< 最大跟随误差 pulse
    int softPosLimitPos;        ///< 软正限位 pulse
    int softNegLimitPos;        ///< 软负限位 pulse
} TAxAttriPara;

/// @struct TAxCheckEn
/// @brief  轴安全检查配置结构体
typedef struct
{
    short alarmEn;    ///< 报警是否有效标志
    short softLmtEn;  ///< 软限位是否有效标志
    short hwLmtEn;    ///< 硬限位是否有效标志
    short errorLmtEn; ///< 跟随误差是否检查标志
} TAxCheckEn;
/// @}

/// @addtogroup Crd
/// @{
/// @struct TCrdAdvParam
/// @brief 插补高级参数结构体
typedef struct
{
    short userVelMode;       ///< 用户速度规划模式: 0 系统前瞻速度规划, 1 用户设定速度规划(默认: 0)
    short transMode;         ///< 过渡模式 0: 无过渡 1: 圆弧过渡 (默认:1)
    short noDataProtect;     ///< 数据断流保护: 0不保护, 1保护 (默认: 1)
    short circAccChangeEn;   ///< 圆弧变加速使能: 0不变加速, 1变加速 (默认: 0)
    short noCoplaneCircOptm; ///< 异面圆弧优化: 0不开启, 1开启
    double turnCoef;         ///< 拐弯系数: [0.01~100](默认: 1.0)
    double tol;              ///< 插补精度: [0,1e9](默认: 0 , 单位取决于设置的当量)
} TCrdAdvParam;
/// @}

/// @addtogroup Gear
/// @{
/// @struct TGearParam
/// @brief  电子齿轮参数结构体
typedef struct
{
    int masterScale;    ///< 主轴齿数: (0,intMax]
    int slaveScale;     ///< 从轴齿数: [intMin,0) || (0,intMax], 从站齿数为负表示反向跟随
    short masterNo;     ///< 主轴索引: 依据主轴类型定义范围，轴规划或轴编码器 [0,63]，端子板编码器[0,2],EtherCAT编码器[0,7]
    short masterType;   ///< 主轴类型: 0 轴规划, 1 轴编码器, 10 端子板编码器, 11 EtherCAT编码器
    short dirMode;      ///< 方向模式: 0 跟随方向不限制, 1 正向绝对位置跟随, 2 负向绝对位置跟随, 3 正向相对位置跟随, 4 负向相对位置跟随
    int masterSlopeDis; ///< 主轴离合区:[0,intMax], 0表示没有加速过程，直接跟随
} TGearParam;
/// @}

/// @addtogroup CompensateTable
/// @{
/// @struct TTableCompParam
/// @brief  表补偿参数结构体
typedef struct
{
    short tableId;    ///< 表索引:[0,当前表补偿的表个数(默认为1) - 1]
    short dimension;  ///< 补偿维数:{1,2,3}
    short srcAxNo[3]; ///< 参考轴号:[0,63],数组有效的元素与补偿维数有关,如补偿维数=1,仅用到srcAxNo[0], 下同
    short srcType[3]; ///< 参考轴类型:0 轴规划, 1 轴编码器
    int startPos[3];  ///< 补偿起始位置:[intMin,intMax]
    int count[3];     ///< 补偿点个数:[2,40000] （有效的元素的乘积不得超过40000，且各不小于2)
    int step[3];      ///< 补偿间隔:[1,intMax]
} TTableCompParam;
/// @}

/// @addtogroup Plc
/// @{
/// @struct TCompileInfo
/// @brief  程序编译信息结构体
typedef struct CompileInfo
{
    char* pFileName; ///< 文件名
    short* pLineNo;  ///< 行号
    char* pMessage;  ///< 消息
} TCompileInfo;

/// @struct TThreadSts
/// @brief  程序运行状态结构体
typedef struct ThreadSts
{
    short run;     ///< 运行状态
    short error;   ///< 错误
    double result; ///< 结果
    short line;    ///< 行号
} TThreadSts;

/// @struct TVarInfo
/// @brief  程序变量属性结构体
typedef struct VarInfo
{
    short id;       ///< 变量ID
    short dataType; ///< 数据类型
    char name[32];  ///< 变量名
} TVarInfo;

typedef struct
{
    short diType;
    short index;
    short reverse;
} TBindDi;

typedef struct
{
    short doType;
    short index;
    short reverse;
} TBindDo;

typedef struct
{
    short timerType;
    int delay;
    short inputVarId;
} TBindTimer;

typedef struct
{
    short counterType;
    short edge;
    int init;
    int target;
    int begin;
    int end;
    short dir;
    int unit;
    short inputVarId;
    short resetVarId;
} TBindCounter;

typedef struct
{
    short flankType;
    short inputVarId;
} TBindFlank;

typedef struct
{
    short setVarId;
    short resetVarId;
} TBindSrff;

/// @}

/// @addtogroup EventStruct
/// @brief  事件功能相关结构体
/// @{
/// @struct TEvent
/// @brief  事件相关结构体
typedef struct
{
    short type;        ///< 事件类型
    short index;       ///< 事件索引
    unsigned int loop; ///< 事件最大允许触发次数
    double value;      ///< 设置比较值
    short condition;   ///< 事件条件
} TEvent;

/// @struct TTaskCrdStart
/// @brief  插补坐标系号任务结构体
typedef struct
{
    short crdNo; ///< 插补坐标系号
} TTaskCrdStart;

typedef struct
{
    short crdNo;    ///< 插补坐标系号
    short stopType; ///< 插补坐标系停止类型
} TTaskCrdStop;

/// @struct TTaskStartPtp
/// @brief  启动Ptp任务结构体
typedef struct
{
    short axNo;    ///< 轴号
    short posType; ///< 位置类型
    double tgtPos; ///< 目标位置
} TTaskStartPtp;

/// @struct TTaskStartJog
/// @brief  启动Jog任务结构体
typedef struct
{
    short axNo;    ///< 轴号
    double tgtVel; ///< 目标速度
} TTaskStartJog;

/// @struct TTaskStartGear
/// @brief  启动Gear任务结构体
typedef struct
{
    short axNo; ///< 轴号
} TTaskStartGear;

/// @struct TTaskStartPt
/// @brief  启动Pt任务结构体
typedef struct
{
    short sysNo; ///< Pt系统号
} TTaskStartPt;

/// @struct TTaskStartPvt
/// @brief  启动Pvt任务结构体
typedef struct
{
    short axNum;      ///< 轴号
    short axArray[8]; ///< 轴号数组
} TTaskStartPvt;

/// @struct TTaskUpdatePtpMvPara
/// @brief  更新PtpMv参数任务结构体
typedef struct
{
    short axNo;    ///< 轴号
    double tgtVel; ///< 目标速度
    double tgtAcc; ///< 目标加速度
    double tgtDec; ///< 目标减速度
} TTaskUpdatePtpMvPara;

/// @struct TTaskUpdateJogMvPara
/// @brief  更新JogMv参数任务结构体
typedef struct
{
    short axNo;    ///< 轴号
    double tgtVel; ///< 目标速度
    double tgtAcc; ///< 目标加速度
    double tgtDec; ///< 目标减速度
} TTaskUpdateJogMvPara;

/// @struct TTaskUpdatePtpTgtPos
/// @brief  更新Ptp目标位置任务结构体
typedef struct
{
    short axNo;    ///< 轴号
    double tgtPos; ///< 目标位置
} TTaskUpdatePtpTgtPos;

/// @struct TTaskSetEcatDoBit
/// @brief  设置Do输出Bit任务结构体
typedef struct
{
    short doNo;  ///< Do索引
    short value; ///< 输出值
} TTaskSetEcatDoBit;

/// @struct TTaskSetEcatGrpDo
/// @brief  设置Do输出Group任务结构体
typedef struct
{
    short groupNo; ///< DO组号
    short value;   ///< 输出值
} TTaskSetEcatGrpDo;

/// @struct TTaskSetEcatDoBitInverse
/// @brief  设置Do输出Bit翻转任务结构体
typedef struct
{
    short doIndex;     ///< DO索引
    short value;       ///< 输出值
    short inverseTime; ///< 反向时间
} TTaskSetEcatDoBitInverse;

/// @struct TTaskSetEcatDaVal
/// @brief  设置Da值任务结构体
typedef struct
{
    short daIndex; ///< DA索引
    short value;   ///< 输出值
} TTaskSetEcatDaVal;

/// @struct TTaskStopMove
/// @brief  停止任务结构体
typedef struct
{
    short axNo;     ///< 轴号
    short stopType; ///< 停止类型
} TTaskStopMove;

/// @struct TTaskPulseDo
/// @brief  脉冲输出任务结构体
typedef struct
{
    short taskPulseDoIndex; ///< 脉冲索引
    short firstLevel;       ///< 初始电平
    int highLevelTime;      ///< 高电平时间
    int lowLevelTime;       ///< 低电平时间
    short pulseNum;         ///< 脉冲数
} TTaskPulseDo;

/// @union UTask
/// @brief  任务结构体
typedef union
{
    TTaskCrdStart taskCrdStart;                       ///< 任务启动Crd
    TTaskCrdStop taskCrdStop;                         ///< 任务停止Crd
    TTaskStartPtp taskStartPtp;                       ///< 任务启动Ptp
    TTaskStartJog taskStartJog;                       ///< 任务启动Jog
    TTaskStartGear taskStartGear;                     ///< 任务启动Gear
    TTaskStartPt taskStartPt;                         ///< 任务启动Pt
    TTaskStartPvt taskStartPvt;                       ///< 任务启动Pvt
    TTaskUpdatePtpMvPara taskUpdatePtpMvPara;         ///< 任务更新PtpMv参数
    TTaskUpdateJogMvPara taskUpdateJogMvPara;         ///< 任务更新JogMv参数
    TTaskSetEcatDoBit taskSetEcatDoBit;               ///< 任务设置Do输出Bit
    TTaskSetEcatGrpDo taskSetEcatGrpDo;               ///< 任务设置Do输出Group
    TTaskSetEcatDoBitInverse taskSetEcatDoBitInverse; ///< 任务设置Do输出Bit翻转
    TTaskSetEcatDaVal taskSetEcatDaVal;               ///< 任务设置Da值
    TTaskUpdatePtpTgtPos taskUpdatePtpTgtPos;         ///< 任务更新Ptp目标位置
    TTaskPulseDo taskPulseDo;                         ///< 任务脉冲输出
    TTaskStopMove taskStopMove;                       ///< 任务停止运动
} UTask;
/// @}

/// @addtogroup Sample
/// @{
/// @struct TSamplePara
/// @brief  采样配置参数结构体
typedef struct
{
    short interval; ///< 采样时间间隔
    short trigType; ///< 触发采样类型：0 立即 1 延时 2 本地di 3 ECAT di
    short delay;    ///< 延时时间 单位：周期
    short diNo;     ///< di输入号
    short diLevel;  ///< di的触发输入值 0或1
} TSamplePara;
/// @}

// 宏定义

// 开卡类型宏定义
#define CARD_OPEN_OPTION_0                 (0) ///< 开卡选项0, 普通模式
#define CARD_OPEN_OPTION_4                 (4) ///< 开卡选项4

/// @addtogroup CardSysDef
/// @brief 板卡配置相关定义
/// @{

/// @addtogroup CardResTypeDef
/// @brief 适用于IMC_GetResCount函数
/// @{
#define MC_ECAT_DO                         (0)  ///< ecat的通用do
#define MC_LOCAL_DO                        (1)  ///< localBus的通用do
#define MC_ECAT_DI                         (11) ///< ecat的通用DI
#define MC_ECAT_AD                         (12) ///< ecat的通用AD
#define MC_ECAT_DA                         (13) ///< ecat的通用DA
#define MC_ECAT_AXIS                       (15) ///< ecat的通用AXIS
#define MC_ECAT_REG_IN                     (16) ///< ecat的通用RegIn
#define MC_ECAT_REG_OUT                    (17) ///< ecat的通用RegOut
#define MC_ECAT_ENC                        (18) ///< ecat的Enc资源

#define MC_AXIS                            (30) ///< 板卡最大轴数
#define MC_PROFILE                         (31) ///< 板卡规划轴数
#define MC_CRD_MAX_CNT                     (60) ///< 坐标系最大个数
#define MC_CRD_BUF_LEN                     (61) ///< 坐标系缓冲区长度
/// @}

/// @addtogroup CardVersionTypeDef
/// @brief 适用于IMC_GetImcCardVersion函数
/// @{
#define SOFT_VERSION                       0  ///< 总软件版本
#define DSP_VERSION                        1  ///< HAL2软件版本
#define ARM_VERSION                        2  ///< ARM软件版本
#define API_VERSION                        3  ///< API软件版本
#define FPGA_VERSION                       4  ///< FPGA固件版本
#define DSP2_VERSION                       5  ///< HAL3固件版本
#define LOCAL_VERSION                      6  ///< 端子板固件版本
#define OS_ARM_VERSION                     11 ///< ARM系统版本
#define LIB_DSP_VERSION                    12 ///< LibInfo软件版本
#define LIB_ECAT_BOARD_APP_VERSION         20 ///< 协议栈软件版本1
#define LIB_ECAT_APP_VERSION               21 ///< 协议栈软件版本2
#define LIB_ECAT_VERSION                   22 ///< 协议栈软件版本3
#define LIB_ECAT_PARSER_ENI_VERSION        23 ///< 协议栈软件版本4
/// @}
/// @}

/// @addtogroup EcatDef
/// @brief EtherCAT相关宏定义
/// @{
/// @addtogroup MasterStsDef
/// @brief 板卡ECAT主站状态定义
/// @{
#define EC_MASTER_IDLE                     (0) ///< EtherCat主站尚未初始化
#define EC_MASTER_INIT                     (1) ///< EtherCat主站初始化
#define EC_MASTER_SCAN_SLAVE               (2) ///< EtherCat主站正在扫描从站设备
#define EC_MASTER_SCAN_SLAVE_END           (3) ///< EtherCat主站扫描从站设备结束
#define EC_MASTER_SCAN_MODULES             (4) ///< EtherCat主站正在扫描从站设备MODULES
#define EC_MASTER_SCAN_MODULES_END         (5) ///< EtherCat主站扫描从站设备MODULES结束
#define EC_MASTER_OP                       (6) ///< EtherCat主站进入OP状态
#define EC_MASTER_ERR                      (7) ///< EtherCat主站链路状态有错误
/// @}

/// @addtogroup SlaveStsDef
/// @brief 板卡EtherCAT从站状态定义
/// @{
#define EC_SLAVE_STATE_UNKNOWN             (0x00) ///< 低4位状态, 表示EtherCat从站在未知状态
#define EC_SLAVE_STATE_INIT                (0x01) ///< 低4位状态, 表示EtherCat从站在初始状态
#define EC_SLAVE_STATE_PREOP               (0x02) ///< 低4位状态, 表示EtherCat从站在PREOP状态
#define EC_SLAVE_STATE_BOOT                (0x03) ///< 低4位状态, 表示EtherCat从站在BOOT状态
#define EC_SLAVE_STATE_SAFEOP              (0x04) ///< 低4位状态, 表示EtherCat从站在SAVEOP状态
#define EC_SLAVE_STATE_OP                  (0x08) ///< 低4位状态, 表示EtherCat从站在OP状态
#define EC_SLAVE_STATE_ACK_ERR             (0x10) ///< 高4位状态, 表示EtherCat从站有错误
/// @}

/// @addtogroup EcatErrorDef
/// @brief 协议栈错误码EcatErrorCode定义
/// @{
#define ERROR_CODE_NO_SUCH_SLAVE           (0x0003) ///< 配置阶段, 没有这个从站, 处理方法: 检查从站是否掉线, 重新扫描配置
#define ERROR_CODE_INVALID_PDO             (0x0004) ///< 配置阶段, 非法PDO, 处理方法: 检查从站厂家是否更新xml配置文件, 与汇川默认配置不一致
#define ERROR_CODE_INVALID_SDO             (0x0005) ///< 配置阶段, 非法SDO, 处理方法: 检查从站厂家是否更新xml配置文件, 与汇川默认配置不一致
#define ERROR_CODE_INVALID_ENTRY           (0x0006) ///< 配置阶段, 非法ENTRY, 处理方法: 检查从站厂家是否更新xml配置文件, 与汇川默认配置不一致
#define ERROR_CODE_PASECFGFAIL             (0x000D) ///< 解析XML设备配置失败, 处理方法: 设备配置文件已经被破坏, 重新扫描配置
#define ERROR_CODE_CFGREGISTFAIL           (0x0015) ///< 配置reg失败, 处理方法: 检查从站厂家是否更新xml配置文件, 与汇川默认配置不一致
#define ERROR_CODE_CFGDIFFONLINE           (0x0016) ///< 在线从站与配置不一致, 处理方法: 检查从站是否掉线, 重新扫描配置
#define ERROR_CODE_AXIS_NUM_BEYOND         (0x0017) ///< 轴配置数量超过板卡最大支持轴数;
#define ERROR_CODE_SLAVE_OFFLINE           (0x001a) ///< 从站掉线错误, 高8位为从站号, 即bit[15:8]-轴号, 处理方法: 检查从高8位数字开始从站是否掉线, 或者重新扫描配置
#define ERROR_CODE_SDOBF_NONECAT           (0x001b) ///< SDO缓冲区收到非ECAT帧错误, 处理方法: 检查线路是否接错, 是否有其他非Ethercat设备接入
#define ERROR_CODE_PORT0_NOTLINK           (0x001c) ///< 端口未接ECAT设备错误, 处理方法: 检查板卡端线路是否正常接线, 是否接线不可靠
#define ERROR_CODE_SET_CYCLETIME_PARA_ERR  (0x001e) ///< 设置周期时间参数错误, 处理方法: 错误的DC时钟配置, 只支持125us, 250us, 500us, 125us, 1000us, 2000us, 4000us, 8000us周期
#define ERROR_CODE_COE_SDO_INIT_ERR        (0x001f) ///< 在初始化阶段coe配置错误, 处理方法: 检查从站厂家是否更新xml配置文件, 与汇川默认配置不一致; 或者可能例如RTU模块配置错误
#define ERROR_CODE_SLAVE_STATE_ERR         (0x0020) ///< 从站状态错误, 高8位为轴号, 即bit[15:8]-轴号, 处理方法: 检查从高8位数字从站状态, 检查从站设备异常原因
#define ERROR_CODE_SLAVE_SII_ERR           (0x0037) ///< E2ROM 信息有误, 处理方法: 一般为从站保存的e2rom信息有误, 可以使用twincat确认并联系厂家
#define ERROR_CODE_SLAVE_NUM_BEYOND        (0x003b) ///< 从站在线超过最大64个站
/// @}

/// @addtogroup AbortCodeDef
/// @brief 适用于IMC_GetEcatSdo, IMC_SetEcatSdo函数
/// @{
#define ABORT_CODE1                        (0x05030000) ///< 从站Toggle bit 没有变化
#define ABORT_CODE2                        (0x05040000) ///< SDO 访问超时
#define ABORT_CODE3                        (0x05040001) ///< 客户端/服务器 命令非法或未知
#define ABORT_CODE4                        (0x05040005) ///< 内存溢出
#define ABORT_CODE5                        (0x06010000) ///< 不支持访问该对象
#define ABORT_CODE6                        (0x06010001) ///< 尝试去读一个只写对象
#define ABORT_CODE7                        (0x06010002) ///< 尝试去写一个只读对象
#define ABORT_CODE8                        (0x06020000) ///< 对象字典中不存在该对象
#define ABORT_CODE9                        (0x06040041) ///< 该对象不能映射成PDO
#define ABORT_CODE10                       (0x06040042) ///< 对象映射成PDO超出 PDO长度
#define ABORT_CODE11                       (0x06040043) ///< 通用参数非法
#define ABORT_CODE12                       (0x06040047) ///< 设备内不兼容
#define ABORT_CODE13                       (0x06060000) ///< 由于硬件原因访问失败
#define ABORT_CODE14                       (0x06070010) ///< 数据类型不匹配, 长度参数
#define ABORT_CODE15                       (0x06070012) ///< 数据类型不匹配, 长度太大
#define ABORT_CODE16                       (0x06070013) ///< 数据类型不匹配, 长度太小
#define ABORT_CODE17                       (0x06090011) ///< 该对象子索引不存在
#define ABORT_CODE18                       (0x06090030) ///< 参数超出范围
#define ABORT_CODE19                       (0x06090031) ///< 参数超出范围太大
#define ABORT_CODE20                       (0x06090032) ///< 参数超出范围太小
#define ABORT_CODE21                       (0x06090036) ///< 最大值小于最小值
#define ABORT_CODE22                       (0x08000000) ///< 一般错误
#define ABORT_CODE23                       (0x08000020) ///< 数据不能被传输或被保存
#define ABORT_CODE24                       (0x08000021) ///< 数据不能被传输或被保存由于本地控制
#define ABORT_CODE25                       (0x08000022) ///< 数据不能被传输或被保存由于当前状态
#define ABORT_CODE26                       (0x08000023) ///< 缺乏对象字典或者对象字典创建失败
/// @}

/// @addtogroup ServoOpModeDef
/// @brief 定义了伺服402协议中的控制模式
/// @{
#define TQ_OP_MODE                         (4)  ///< TQ模式
#define HM_OP_MODE                         (6)  ///< 回零模式
#define CSP_OP_MODE                        (8)  ///< CSP模式
#define CSV_OP_MODE                        (9)  ///< CSV模式
#define CST_OP_MODE                        (10) ///< CST模式
/// @}

/// @addtogroup EcatHomingMethodDef
/// @brief 定义了伺服402回零模式的回零方法类型
/// @{
#define HOME_NLIMT_ZINDEX                  (1)  ///< 负限位+Z信号
#define HOME_PLIMT_ZINDEX                  (2)  ///< 正限位+Z信号
#define HOME_PHOME_FEDGE_ZINDEX            (3)  ///< 正原点开关下降沿+Z信号
#define HOME_PHOME_REDGE_ZINDEX            (4)  ///< 正原点开关上升沿+Z信号
#define HOME_NHOME_FEDGE_ZINDEX            (5)  ///< 负原点开关下降沿+Z信号
#define HOME_NHOME_REDGE_ZINDEX            (6)  ///< 负原点开关上升沿+Z信号
#define HOME_PLIMT_PHOME_FEDGE_ZINDEX      (7)  ///< 正限位+正原点开关下降沿+Z信号
#define HOME_PLIMT_PHOME_REDGE_ZINDEX      (8)  ///< 正限位+正原点开关上升沿+Z信号
#define HOME_PLIMT_NHOME_REDGE_ZINDEX      (9)  ///< 正限位+负原点开关上升沿+Z信号
#define HOME_PLIMT_NHOME_FEDGE_ZINDEX      (10) ///< 正限位+负原点开关下降沿+Z信号
#define HOME_NLIMT_NHOME_FEDGE_ZINDEX      (11) ///< 负限位+负原点开关下降沿+Z信号
#define HOME_NLIMT_NHOME_REDGE_ZINDEX      (12) ///< 负限位+负原点开关上升沿+Z信号
#define HOME_NLIMT_PHOME_REDGE_ZINDEX      (13) ///< 负限位+正原点开关上升沿+Z信号
#define HOME_NLIMT_PHOME_FEDGE_ZINDEX      (14) ///< 负限位+正原点开关下降沿+Z信号
#define HOME_NLIMT                         (17) ///< 负限位
#define HOME_PLIMT                         (18) ///< 正限位
#define HOME_PHOME_FEDGE                   (19) ///< 正原点开关下降沿
#define HOME_PHOME_REDGE                   (20) ///< 正原点开关上升沿
#define HOME_NHOME_FEDGE                   (21) ///< 负原点开关下降沿
#define HOME_NHOME_REDGE                   (22) ///< 负原点开关上升沿
#define HOME_PLIMT_PHOME_FEDGE             (23) ///< 正限位+正原点开关下降沿
#define HOME_PLIMT_PHOME_REDGE             (24) ///< 正限位+正原点开关上升沿
#define HOME_PLIMT_NHOME_REDGE             (25) ///< 正限位+负原点开关上升沿
#define HOME_PLIMT_NHOME_FEDGE             (26) ///< 正限位+负原点开关下降沿
#define HOME_NLIMT_NHOME_FEDGE             (27) ///< 负限位+负原点开关下降沿
#define HOME_NLIMT_NHOME_REDGE             (28) ///< 负限位+负原点开关上升沿
#define HOME_NLIMT_PHOME_REDGE             (29) ///< 负限位+正原点开关上升沿
#define HOME_NLIMT_PHOME_FEDGE             (30) ///< 负限位+正原点开关下降沿
#define HOME_NEGZINDEX                     (33) ///< 负向Z信号
#define HOME_POSZINDEX                     (34) ///< 正向Z信号
#define HOME_CURRENT_POS                   (35) ///< 以当前位置为原点
/// @}

/// @addtogroup EcatHomingStsDef
/// @brief 定义了伺服402回零模式下的工作状态
/// @{
#define HOME_IN_PROGRESS                   (0) ///< 正在回零中
#define HOME_INTERRUPTED_OR_NOT_START      (1) ///< 回零中断或者没有开始启动
#define HOME_ATTAINED_BUT_NOT_REACH        (2) ///< 回零结束, 但没有到设定的目标位置
#define HOME_SUCESS                        (3) ///< 回零成功
#define HOME_ERR_VEL_NOT_ZERO              (4) ///< 回零中发生错误, 同时速度不为0
#define HOME_ERR_VEL_ZERO                  (5) ///< 回零中发生错误, 同时速度为0
/// @}

/// @addtogroup CSPHomingMethodDef
/// @brief 定义了板卡CSP回零模式的回零方法
/// @{
#define HOMING_ECAT_CSP_METHOD_NONE        (-1) ///< 非回零模式
#define HOMING_ECAT_CSP_METHOD_DI          (0)  ///< DI回零方式
#define HOMING_ECAT_CSP_METHOD_INDEX       (1)  ///< Index回零方式
#define HOMING_ECAT_CSP_METHOD_LIMIT_INDEX (2)  ///< 限位+Index回零方式
#define HOMING_ECAT_CSP_METHOD_LIMIT_DI    (3)  ///< 限位+Di回零方式
/// @}

/// @addtogroup CSPHomingStsDef
/// @brief 定义了板卡CSP回零模式下的工作状态
/// @{
#define HOME_CSP_STS_RUNING                (0x01) ///< CSP回零中
#define HOME_CSP_STS_STOPPING              (0x02) ///< CSP回零停止中
#define HOME_CSP_STS_STOPPED               (0x03) ///< CSP回零已停止
#define HOME_CSP_STS_FINISH                (0x04) ///< CSP回零已完成
#define HOME_CSP_STS_ERROR                 (0x05) ///< CSP回零错误
/// @}
/// @}

/// @addtogroup AxisDef
/// @brief 适用于轴相关功能
/// @{

/// @addtogroup AxDiStopTypeDef
/// @brief 适用于IMC_SetAxStopTrigPara、IMC_GetAxStopTrigPara函数
/// @{
#define CNST_DI_STOP_TYPE_ECATDI           (0) ///< EcatDI停止类型
#define CNST_DI_STOP_TYPE_PROBLE1_RF       (1) ///< 探针1上升沿或下降沿停止
#define CNST_DI_STOP_TYPE_PROBLE1_R        (2) ///< 探针1上升沿停止
#define CNST_DI_STOP_TYPE_PROBLE1_F        (3) ///< 探针1下降沿停止
/// @}

/// @addtogroup AxStsBitDef
/// @brief 适用于IMC_GetAxSts函数
/// @{
#define AX_ALARM_BIT                       (0x00000001) ///< 轴驱动报警
#define AX_SVON_BIT                        (0x00000002) ///< 伺服使能
#define AX_BUSY_BIT                        (0x00000004) ///< 轴忙状态
#define AX_ARRIVE_BIT                      (0x00000008) ///< 轴到位状态
#define AX_POSLMT_BIT                      (0x00000010) ///< 正硬限位报警
#define AX_NEGLMT_BIT                      (0x00000020) ///< 负硬限位报警
#define AX_SOFT_POSLMT_BIT                 (0x00000040) ///< 正软限位报警
#define AX_SOFT_NEGLMT_BIT                 (0x00000080) ///< 负软限位报警
#define AX_ERRPOS_BIT                      (0x00000100) ///< 轴位置误差越限标志
#define AX_EMG_STOP_BIT                    (0x00000200) ///< 运动急停标志
#define AX_ECAT_BIT                        (0x00000400) ///< 总线轴标志
#define AX_SW_ABNOR_BIT                    (0x00000800) ///< 轴异常报警(龙门)
#define AX_WARING_BIT                      (0x00001000) ///< 轴警告
#define AX_HM_BIT                          (0x00002000) ///< 原点信号状态
#define AX_UNLINK_BIT                      (0x00004000) ///< 轴掉线状态
#define AX_ECAT_TGTREACH_BIT               (0x00008000) ///< 状态字里的到位状态
/// @}

/// @addtogroup AxStopReasonDef
/// @brief 适用于IMC_GetAxStopReason函数
/// @{
#define NORMAL_STOP_STOPREASON             (0x00) ///< 轴正常停止或者在运行中
#define EMG_STOP_STOPREASON                (0x01) ///< 急停信号触发停止
#define LINK_ERROR_STOPREASON              (0x02) ///< 掉线触发停止
#define WATCHDOG_TRIG_STOPREASON           (0x03) ///< 看门狗触发停止
#define POSHWLMT_TRIG_STOPREASON           (0x04) ///< 正硬限位触发停止
#define NEGHWLMT_TRIG_STOPREASON           (0x05) ///< 负硬限位触发停止
#define POSSOFTLMT_TRIG_STOPREASON         (0x06) ///< 正软限位触发停止
#define NEGSOFTLMT_TRIG_STOPREASON         (0x07) ///< 负软限位触发停止
#define FOLLOW_ERROR_STOPREASON            (0x08) ///< 跟随误差触发停止
#define ALARM_TRIG_STOPREASON              (0x09) ///< 伺服报警触发停止
#define ABNORMAL_BIT_STOPREASON            (0x0a) ///< 轴有异常状态触发停止
#define DI_TRIG_STOPREASON                 (0x0b) ///< DI触发停止
#define PROB_TRIG_STOPREASON               (0x0c) ///< 探针触发停止
#define IMC_AXMOVESTOP_TRIG_STOPREASON     (0x0e) ///< 调用IMC_AxMoveStop触发停止
/// @}
/// @}

/// @addtogroup EventDef
/// @brief 适用于事件功能
/// @{

/// @addtogroup EventTypeDef
/// @brief 适用于事件功能
/// @{
// 事件类型定义
#define EVENT_TYPE_PRFPOS                  (1)  ///< 事件类型-规划位置
#define EVENT_TYPE_PRFVEL                  (2)  ///< 事件类型-规划速度
#define EVENT_TYPE_ENCPOS                  (3)  ///< 事件类型-编码器位置
#define EVENT_TYPE_ENCVEL                  (4)  ///< 事件类型-编码器速度
#define EVENT_TYPE_CRDPOS_X                (5)  ///< 事件类型-插补坐标系X轴位置
#define EVENT_TYPE_CRDPOS_Y                (6)  ///< 事件类型-插补坐标系Y轴位置
#define EVENT_TYPE_CRDPOS_Z                (7)  ///< 事件类型-插补坐标系Z轴位置
#define EVENT_TYPE_CRDVEL                  (8)  ///< 事件类型-插补坐标系合成速度
#define EVENT_TYPE_AXSTS_ALARM             (9)  ///< 事件类型-轴报警
#define EVENT_TYPE_AXSTS_SVON              (10) ///< 事件类型-轴伺服使能
#define EVENT_TYPE_AXSTS_BUSY              (11) ///< 事件类型-轴忙
#define EVENT_TYPE_AXSTS_ARRIVE            (12) ///< 事件类型-轴到位
#define EVENT_TYPE_AXSTS_POSLMT            (13) ///< 事件类型-轴正硬限位
#define EVENT_TYPE_AXSTS_NEGLMT            (14) ///< 事件类型-轴负硬限位
#define EVENT_TYPE_AXSTS_SOFT_POSLMT       (15) ///< 事件类型-轴正软限位
#define EVENT_TYPE_AXSTS_SOFT_NEGLMT       (16) ///< 事件类型-轴负软限位
#define EVENT_TYPE_AXSTS_ERRPOS            (17) ///< 事件类型-轴位置误差越限
#define EVENT_TYPE_AXSTS_EMGSTOP           (18) ///< 事件类型-运动急停
#define EVENT_TYPE_AXSTS_ECAT              (19) ///< 事件类型-总线轴
#define EVENT_TYPE_AXSTS_SW_ABNOR          (20) ///< 事件类型-轴异常报警(龙门)
#define EVENT_TYPE_AXSTS_WARING            (21) ///< 事件类型-轴警告
#define EVENT_TYPE_AXSTS_HM                (22) ///< 事件类型-原点信号状态
#define EVENT_TYPE_AXSTS_UNLINK            (23) ///< 事件类型-轴掉线状态
#define EVENT_TYPE_AXSTS_TGTREACH          (24) ///< 事件类型-状态字里的到位状态
#define EVENT_TYPE_CRDSTS                  (25) ///< 事件类型-插补坐标系状态
#define EVENT_TYPE_CRDID                   (26) ///< 事件类型-插补坐标系号
#define EVENT_TYPE_DI                      (27) ///< 事件类型-DI
#define EVENT_TYPE_AD                      (28) ///< 事件类型-AD
#define EVENT_TYPE_GLOBALVAL               (29) ///< 事件类型-全局变量
#define EVENT_TYPE_LOCAL_ENC               (30) ///< 事件类型-端子板编码器位置
#define EVENT_TYPE_TXPDO_2                 (31) ///< 事件类型-txpdo通用, 2字节, 读
#define EVENT_TYPE_TXPDO_4                 (32) ///< 事件类型-txpdo通用, 4字节, 读
#define EVENT_TYPE_RXPDO_2                 (33) ///< 事件类型-rxpdo通用, 2字节, 读
#define EVENT_TYPE_RXPDO_4                 (34) ///< 事件类型-rxpdo通用, 4字节, 读
/// @}

/// @addtogroup EventConditionDef
/// @brief 适用于事件功能
/// @{
// 事件条件定义
#define EVENT_CONDITION_EQ                 (1)  ///< 事件条件-变量值等于设定值
#define EVENT_CONDITION_NE                 (2)  ///< 事件条件-变量值不等于设定值
#define EVENT_CONDITION_GT                 (3)  ///< 事件条件-变量值大于设定值
#define EVENT_CONDITION_LT                 (4)  ///< 事件条件-变量值小于设定值
#define EVENT_CONDITION_CHANGE_TO          (5)  ///< 事件条件-变量值改变为设定值
#define EVENT_CONDITION_CHANGE             (6)  ///< 事件条件-变量值改变
#define EVENT_CONDITION_UP                 (7)  ///< 事件条件-变量值增大
#define EVENT_CONDITION_DOWN               (8)  ///< 事件条件-变量值减小
#define EVENT_CONDITION_REMAIN_AT          (9)  ///< 事件条件-变量值保持为设定值
#define EVENT_CONDITION_REMAIN             (10) ///< 事件条件-变量值保持不变
#define EVENT_CONDITION_CROSS_POSITIVE     (11) ///< 事件条件-变量值正向穿越设定值
#define EVENT_CONDITION_CROSS_NEGATIVE     (12) ///< 事件条件-变量值负向穿越设定值
/// @}

/// @addtogroup TaskTypeDef
/// @brief 适用于任务功能
/// @{
// 事件功能中任务类型定义
#define TASK_TYPE_CRDSTART                 (1)  ///< 任务类型-插补坐标系启动
#define TASK_TYPE_STARTPTPMOVE             (2)  ///< 任务类型-启动PTP运动
#define TASK_TYPE_STARTJOGMOVE             (3)  ///< 任务类型-启动JOG运动
#define TASK_TYPE_STARTGEARMOVE            (4)  ///< 任务类型-启动齿轮运动
#define TASK_TYPE_STARTPTMOVE              (5)  ///< 任务类型-启动PT运动
#define TASK_TYPE_STARTPVTMOVE             (6)  ///< 任务类型-启动PVT运动
#define TASK_TYPE_UPDATEPTPMOVEPARA        (7)  ///< 任务类型-更新PTP运动参数
#define TASK_TYPE_UPDATEJOGMOVEPARA        (8)  ///< 任务类型-更新JOG运动参数
#define TASK_TYPE_SETECATDOBIT             (9)  ///< 任务类型-设置ECAT通用DO
#define TASK_TYPE_SETECATGRPDO             (10) ///< 任务类型-设置ECAT通用读PDO
#define TASK_TYPE_SETECATDOBITINVERSE      (11) ///< 任务类型-设置ECAT通用DO取反
#define TASK_TYPE_SETECATDAVAL             (12) ///< 任务类型-设置ECAT通用AD
#define TASK_TYPE_UPDATEPTPTGTPOS          (13) ///< 任务类型-更新PTP目标位置
#define TASK_TYPE_PLUSEDO                  (14) ///< 任务类型-脉冲DO
#define TASK_TYPE_STOPMOVE                 (15) ///< 任务类型-停止运动
#define TASK_TYPE_CRDSTOP                  (16) ///< 任务类型-插补坐标系停止
/// @}
/// @}

/// @addtogroup SampleTypeDef
/// @brief 适用于数据采集功能
/// @{
// 单轴数据类型
#define SAMPLE_ADDRESS_TYPE_AX_PRF_POS     (0x01) ///< 轴规划位置
#define SAMPLE_ADDRESS_TYPE_AX_ENC_POS     (0x02) ///< 轴编码器位置
#define SAMPLE_ADDRESS_TYPE_AX_PRF_VEL     (0x03) ///< 轴规划速度
#define SAMPLE_ADDRESS_TYPE_AX_ENC_VEL     (0x04) ///< 轴编码器速度
#define SAMPLE_ADDRESS_TYPE_AX_PRF_ACC     (0x05) ///< 轴规划加速度
#define SAMPLE_ADDRESS_TYPE_AX_ENC_ACC     (0x06) ///< 轴编码器加速度
#define SAMPLE_ADDRESS_TYPE_PRF_POS        (0x07) ///< 规划位置
#define SAMPLE_ADDRESS_TYPE_PRF_TGT_POS    (0x08) ///< 点位目标位置
#define SAMPLE_ADDRESS_TYPE_AX_TORQ        (0x0a) ///< 轴扭矩
#define SAMPLE_ADDRESS_TYPE_AX_STS         (0x0b) ///< 轴状态
#define SAMPLE_ADDRESS_TYPE_AX_POS_ERROR   (0x0c) ///< 轴跟随误差

// 资源信号数据类型
#define SAMPLE_ADDRESS_TYPE_ECAT_DI        (0x30) ///< ECAT通用DI
#define SAMPLE_ADDRESS_TYPE_ECAT_DO        (0x31) ///< ECAT通用DO
#define SAMPLE_ADDRESS_TYPE_LOCAL_DI       (0x32) ///< 端子板DI
#define SAMPLE_ADDRESS_TYPE_LOCAL_DO       (0x33) ///< 端子板DO
#define SAMPLE_ADDRESS_TYPE_ECAT_AD        (0x34) ///< ECAT AD
#define SAMPLE_ADDRESS_TYPE_ECAT_DA        (0x35) ///< ECAT DA
#define SAMPLE_ADDRESS_TYPE_ECAT_ENC       (0x36) ///< ECAT ENC
#define SAMPLE_ADDRESS_TYPE_LOCAL_ENC      (0x37) ///< LOCAL ENC

// 总线对象字典采集
#define SAMPLE_ADDRESS_TYPE_RXPDO_2        (0x40) ///< pdo通用, 2字节, 写
#define SAMPLE_ADDRESS_TYPE_RXPDO_4        (0x41) ///< pdo通用, 4字节, 写
#define SAMPLE_ADDRESS_TYPE_6040           (0x42) ///< 控制字
#define SAMPLE_ADDRESS_TYPE_607a           (0x43) ///< 目标位置
#define SAMPLE_ADDRESS_TYPE_60ff           (0x44) ///< 目标速度
#define SAMPLE_ADDRESS_TYPE_6071           (0x45) ///< 目标力矩
#define SAMPLE_ADDRESS_TYPE_6060           (0x46) ///< 模式控制
#define SAMPLE_ADDRESS_TYPE_60fe           (0x47) ///< 数字量输出
#define SAMPLE_ADDRESS_TYPE_60b8           (0x48) ///< 探针控制字

#define SAMPLE_ADDRESS_TYPE_TXPDO_2        (0x50) ///< pdo通用, 2字节, 读
#define SAMPLE_ADDRESS_TYPE_TXPDO_4        (0x51) ///< pdo通用, 4字节, 读
#define SAMPLE_ADDRESS_TYPE_6041           (0x52) ///< 状态字
#define SAMPLE_ADDRESS_TYPE_6064           (0x53) ///< 实际位置
#define SAMPLE_ADDRESS_TYPE_606c           (0x54) ///< 实际速度
#define SAMPLE_ADDRESS_TYPE_6077           (0x55) ///< 实际力矩
#define SAMPLE_ADDRESS_TYPE_60f4           (0x56) ///< 跟随误差
#define SAMPLE_ADDRESS_TYPE_603f           (0x57) ///< 错误码
#define SAMPLE_ADDRESS_TYPE_60fd           (0x58) ///< 数字量输入
#define SAMPLE_ADDRESS_TYPE_60b9           (0x59) ///< 探针状态字
#define SAMPLE_ADDRESS_TYPE_60ba           (0x60) ///< 探针1上升沿位置
#define SAMPLE_ADDRESS_TYPE_60bb           (0x61) ///< 探针1下降沿位置
#define SAMPLE_ADDRESS_TYPE_60bc           (0x62) ///< 探针2上升沿位置
#define SAMPLE_ADDRESS_TYPE_60bd           (0x63) ///< 探针2下降沿位置

// 插补数据类型
#define SAMPLE_ADDRESS_TYPE_CRD_POSX       (0x100) ///< 插补坐标系, X轴位置
#define SAMPLE_ADDRESS_TYPE_CRD_POSY       (0x101) ///< 插补坐标系, Y轴位置
#define SAMPLE_ADDRESS_TYPE_CRD_POSZ       (0x102) ///< 插补坐标系, Z轴位置
#define SAMPLE_ADDRESS_TYPE_CRD_VEL        (0x110) ///< 插补坐标系, 合成速度

// 全局变量地址，2,4,8字节
#define SAMPLE_ADDRESS_TYPE_VAR_2          (0x300)
#define SAMPLE_ADDRESS_TYPE_VAR_4          (0x301)
#define SAMPLE_ADDRESS_TYPE_VAR_8          (0x302)

// PCI地址Bar0，2,4,8字节
#define SAMPLE_ADDRESS_TYPE_PCI0_2         (0x310)
#define SAMPLE_ADDRESS_TYPE_PCI0_4         (0x311)
#define SAMPLE_ADDRESS_TYPE_PCI0_8         (0x312)

// Hal共享地址，2,4,8字节
#define SAMPLE_ADDRESS_TYPE_SHM_2          (0x320)
#define SAMPLE_ADDRESS_TYPE_SHM_4          (0x321)
#define SAMPLE_ADDRESS_TYPE_SHM_8          (0x322)

// FPGA，In地址，2,4,8字节
#define SAMPLE_ADDRESS_TYPE_FPGAI_2        (0x330)
#define SAMPLE_ADDRESS_TYPE_FPGAI_4        (0x331)
#define SAMPLE_ADDRESS_TYPE_FPGAI_8        (0x332)

// FPGA，Out地址，2,4,8字节
#define SAMPLE_ADDRESS_TYPE_FPGAO_2        (0x338)
#define SAMPLE_ADDRESS_TYPE_FPGAO_4        (0x339)
#define SAMPLE_ADDRESS_TYPE_FPGAO_8        (0x33a)

// 通用调试采样类型，2,4,8字节
#define SAMPLE_ADDRESS_TYPE_DEBUG_2        (0x3f0)
#define SAMPLE_ADDRESS_TYPE_DEBUG_4        (0x3f1)
#define SAMPLE_ADDRESS_TYPE_DEBUG_8        (0x3f2)

// 采集触发类型
#define SAMPLE_TRIG_IMMEDIATE              (0) ///< 立即采集
#define SAMPLE_TRIG_DELAY                  (1) ///< 延时采集
#define SAMPLE_TRIG_LOCAL_DI               (2) ///< 本地DI触发
#define SAMPLE_TRIG_ECAT_DI                (3) ///< ECAT 的DI 触发
/// @}

/// @defgroup CardBasic 板卡基本操作
/// @brief 板卡基本操作
/// @{
/// @defgroup CardSysDef 板卡配置相关宏定义
/// @{
/// @defgroup CardResTypeDef 板卡卡资源类型
/// @defgroup CardVersionTypeDef 板卡版本号定义
/// @}

/// @defgroup CardOpen 板卡开启与关闭
/// @brief 板卡开启与关闭
/// @{
/**
 * @brief  获取板卡数量
 * @param  pCardsNum        当前连接的板卡数量, 单台设备最多支持4张板卡
 * @param  pCardIndexArray  获取每张卡对应卡号的数组, 板卡卡号由拨码决定, 数组大小为4
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x0001)
 *******************************************************************/
IMC_API IMC_GetCardsNum(short* pCardsNum, short* pCardIndexArray);

/**
 * @brief  按卡号开启控制卡(自动选择开卡方式)
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x0002)
 *******************************************************************/
IMC_API IMC_OpenCard(short cardIndex);

/**
 * @brief  按卡号关闭控制卡
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x0003)
 *******************************************************************/
IMC_API IMC_CloseCard(short cardIndex);

/**
 * @brief  按卡号开启控制卡(指定开卡方式)
 * @details 控制卡已打开的未关闭的情况下,不对option进行范围检查,option以已打开的方式为主
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  option           板卡开卡选项:
                            \n 0 模式0, 自动模式
                            \n 1 模式1, 兼容模式
                            \n 4 模式4, 性能模式(win7不适用)
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x0004)
 *******************************************************************/
IMC_API IMC_OpenCardEx(short cardIndex, short option = 4);

/**
 * @brief  获取当前开卡方式
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  option           板卡开卡选项:
 *                          \n -1 未开卡
                            \n 0  模式0, 自动模式
                            \n 1  模式1, 兼容模式
                            \n 4  模式4, 性能模式(win7不适用)
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x0005)
 *******************************************************************/
IMC_API IMC_GetOpenCardOption(short cardIndex, short* pOption);
/// @}

/// @defgroup CardConfigFile 板卡配置文件操作
/// @brief 板卡配置文件操作
/// @{
/**
 * @brief  上传控制卡的设备配置文件(包括主站使能, 规划周期, EtherCAT对象以及偏置等), 上传文件的路径在设定目录下
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  pathName         文件路径
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x0100)
 *******************************************************************/
IMC_API IMC_UpLoadDeviceConfig(short cardIndex, const char* pathName);

/**
 * @brief  下载控制器的设备配置文件(包括主站使能, 规划周期, EtherCAT对象以及偏置等), 下载文件的路径在设定目录下
 * @attention 当次下载下去之后, 非立即生效, 需要调用扫描板卡指令才能生效
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  pathName         文件路径
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x0101)
 *******************************************************************/
IMC_API IMC_DownLoadDeviceConfig(short cardIndex, const char* pathName);

/**
 * @brief  下载控制器的系统配置文件(包括轴映射、轴运动参数、IO取反滤波等硬件信号配置参数)
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  pathName         文件路径
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x0102)
 *******************************************************************/
IMC_API IMC_DownLoadSystemConfig(short cardIndex, const char* pathName);
/// @}
/// @}

/// @defgroup CardSysConfig 板卡系统配置相关函数
/// @brief 板卡系统配置相关函数
/// @{

/// @defgroup EmgConfig 板卡急停参数配置函数
/// @brief 急停参数配置函数
/// @{

/**
 * @brief  设置停止信号的滤波系数
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  filter           滤波参数, 参数范围：[0, intMax]
 * \n T = (1 + filter)*0.5ms
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x0202)
 *******************************************************************/
IMC_API IMC_SetEmgFilter(short cardIndex, short filter);

/**
 * @brief  获取停止信号的滤波系数
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  pFilter          滤波参数, 参数范围：[0, intMax] \n T = (1 + filter)*0.5ms
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x0203)
 *******************************************************************/
IMC_API IMC_GetEmgFilter(short cardIndex, short* pFilter);

/**
 * @brief  设置急停信号触发的电平取反
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  inverse          取反标志：\n 0：不取反 \n 1：取反
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x0204)
 *******************************************************************/
IMC_API IMC_SetEmgTrigLevelInv(short cardIndex, short inverse);

/**
 * @brief  获取急停信号触发的电平取反
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  pInverse         取反标志：\n 0：不取反 \n 1：取反
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x0205)
 *******************************************************************/
IMC_API IMC_GetEmgTrigLevelInv(short cardIndex, short* pInverse);

/**
 * @brief  获取急停状态
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  pSts             急停触发状态, 按bit位表示急停触发类型
 *                          \n bit0: 硬件急停触发
 *                          \n bit1: 总线通讯异常
 *                          \n bit2: 端子板通讯异常
 *                          \n bit3: 看门狗触发
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x0206)
 *******************************************************************/
IMC_API IMC_GetEmgSts(short cardIndex, short* pSts);

/**
 * @brief  设置急停时是否复位所有 DO 输出的开关使能
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  enable           使能状态：\n 0：不复位 \n 1：复位
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x0207)
 *******************************************************************/
IMC_API IMC_SetEmgDoResetFlag(short cardIndex, short enable);

/**
 * @brief  读取急停时是否复位所有 DO 输出的开关使能
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  pEnable          使能状态：\n 0：不复位 \n 1：复位
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x0208)
 *******************************************************************/
IMC_API IMC_GetEmgDoResetFlag(short cardIndex, short* pEnable);

/**
 * @brief  设置是否忽略板卡的EtherCAT连接状态
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  flag             1表示忽略, 0表示不忽略
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x020b)
 *******************************************************************/
IMC_API IMC_SetEcatLinkStsIgnore(short cardIndex, short flag);
/// @}

/// @defgroup Watchdog 板卡看门狗参数配置函数
/// @brief 看门狗参数配置函数
/// @{
/**
 * @brief  设置看门狗, 在报警时, 进行急停动作。主要用于检查应用层软件是否卡死。
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  feedTime         看门狗时间, 在计数 feedtime 没再次喂狗时, 将触发急停动作。参数范围：[1,30000] 单位：ms
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x0300)
 *******************************************************************/
IMC_API IMC_OpenWatchDog(short cardIndex, int feedTime);

/**
 * @brief  喂看门狗操作
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x0301)
 *******************************************************************/
IMC_API IMC_FeedWatchDog(short cardIndex);

/**
 * @brief  关闭看门狗检查
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x0302)
 *******************************************************************/
IMC_API IMC_CloseWatchDog(short cardIndex);
/// @}

/// @defgroup CardTimeInfo 板卡内部时钟信息
/// @brief 板卡内部时钟信息
/// @{

/**
 * @brief  获取板卡的CPU负载
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  pLoadRatio       控制器的计算负载率(0,100)%
 * @warning 建议负载率不要超过60%
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x0400)
 *******************************************************************/
IMC_API IMC_GetCalcLoadRatio(short cardIndex, double* pLoadRatio);

/**
 * @brief  根据Type类型, 获取板卡对应的固件版本
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  type             版本类型:
 *                          \n (0)     总软件版本
 *                          \n (1)     HAL2软件版本
 *                          \n (2)     ARM软件版本
 *                          \n (3)     API软件版本
 *                          \n (4)     FPGA固件版本
 *                          \n (5)     HAL3固件版本
 *                          \n (6)     端子板固件版本
 *                          \n (11)    ARM系统版本
 *                          \n (12)    LibInfo软件版本
 *                          \n (20)    协议栈软件版本1
 *                          \n (21)    协议栈软件版本2
 *                          \n (22)    协议栈软件版本3
 *                          \n (23)    协议栈软件版本4
 * @param  pVersion         版本信息接收数组, 数组长度为20, 各字段含义如下:
 *                          \n pVersion[0]: VER0
 *                          \n pVersion[1]: VER1
 *                          \n pVersion[2]: VER2
 *                          \n pVersion[3]: VER3
 *                          \n pVersion[4]: YEAR
 *                          \n pVersion[5]: MONTH
 *                          \n pVersion[6]: DAY
 *                          \n pVersion[7-19]: RESERVED
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x0600)
 *******************************************************************/
IMC_API IMC_GetVersion(short cardIndex, short type, short* pVersion);

/// @}

/// @defgroup CardMasterInfo 板卡主站配置信息
/// @brief 板卡主站配置信息
/// @{

/**
 * @brief  获取主站启用配置, 从xml配置文件获取
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  pHwCfg           主站启用配置(按位表示) \n bit0：主站0使能 \n bit1：主站1使能 \n bit2：端子板使能
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x0605)
 *******************************************************************/
IMC_API IMC_GetMasterCfgXml(short cardIndex, short* pHwCfg);

/**
 * @brief  获取板卡的网口硬件连接状态
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  pLinkSts         网口连接状态(按位表示) \n bit0：主站0网口状态 \n bit1：主站1网口状态 \n bit2：端子板网口状态
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x0607)
 *******************************************************************/
IMC_API IMC_GetHwPortLinkSts(short cardIndex, short* pLinkSts);

/**
 * @brief  获取板卡的网口硬件连接状态(Ex指令)
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  pLinkSts         网口连接状态(按位表示) \n bit0：主站0网口状态 \n bit1：主站1网口状态 \n bit2：端子板网口状态
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x0608)
 *******************************************************************/
IMC_API IMC_GetHwPortLinkStsEx(short cardIndex, short* pLinkSts);
/// @}

/// @defgroup CardReset 板卡复位功能
/// @brief 板卡复位功能
/// @{
/**
 * @brief  复位控制卡系统参数
 * @details 控制卡规划层软件资源复位
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x0700)
 *******************************************************************/
IMC_API IMC_ResetSysPara(short cardIndex);

/// @}

/// @defgroup UserCode 板卡用户配置
/// @brief 用户配置
/// @{
/**
 * @brief  设置用户密码
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  len              密码长度, 长度范围：[1,1024]
 * @param  pCode            用户密码
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x0800)
 *******************************************************************/
IMC_API IMC_WriteUserCode(short cardIndex, char* pCode, short len);

/**
 * @brief  检验设置的用户密码
 * @param  cardIndex        	板卡卡号, 参数范围：[0,3]
 * @param  len              	密码长度, 长度范围：[1,1024]
 * @param  pCode            	用户密码
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x0801)
 *******************************************************************/
IMC_API IMC_CheckUserCode(short cardIndex, char* pCode, short len);

/**
 * @brief  写入用户数据
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  offset           数据偏移,共开辟了10K内存空间用于存储数据
 * @param  len              数据长度
 * @param  pData            用户数据
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x0802)
 *******************************************************************/
IMC_API IMC_WriteUserData(short cardIndex, short offset, char* pData, short len);

/**
 * @brief  读取用户数据
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  offset           数据偏移,共开辟了10K内存空间用于存储数据
 * @param  len              数据长度
 * @param  pData            用户数据
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x0803)
 *******************************************************************/
IMC_API IMC_ReadUserData(short cardIndex, short offset, char* pData, short len);
/// @}
/// @}

/// @defgroup EtherCAT 板卡EtherCAT功能
/// @brief 板卡EtherCAT功能
/// @{
/// @defgroup EcatDef 板卡EtherCAT相关宏定义
/// @{
/// @defgroup MasterStsDef 板卡EtherCAT主站状态定义
/// @defgroup SlaveStsDef 板卡EtherCAT从站状态定义
/// @defgroup EcatErrorDef 板卡协议栈错误码EcatErrorCode定义
/// @defgroup AbortCodeDef 板卡协议栈SDO错误码AbortCode定义
/// @defgroup ServoOpModeDef 板卡伺服控制模式
/// @defgroup EcatHomingMethodDef 板卡伺服402回零方法类型
/// @defgroup EcatHomingStsDef 板卡伺服402回零工作状态
/// @defgroup CSPHomingMethodDef 板卡CSP回零方法
/// @defgroup CSPHomingStsDef 板卡CSP回零工作状态
/// @}

/// @defgroup EcatInit 板卡EtherCAT初始化功能
/// @brief 板卡EtherCAT初始化功能
/// @{

/**
 * @brief  初始化板卡通讯, 主站进入OP状态
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x1000)
 *******************************************************************/
IMC_API IMC_InitEcatComm(short cardIndex);

/**
 * @brief  开启板卡通讯, 获取EtherCAT总线资源, 建立主站与各从站之间的通讯
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x1001)
 *******************************************************************/
IMC_API IMC_StartEcatComm(short cardIndex);

/**
 * @brief  关闭板卡通讯, 主站退出OP状态
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x1002)
 *******************************************************************/
IMC_API IMC_DelEcatComm(short cardIndex);

/**
 * @brief  初始化总线, 直到主站进入OP状态, 随后扫描EtherCAT总线资源, 并建立主站与各从站之间的通讯 (该函数会阻塞在EtherCAT的总线扫描同步阶段)
 * @warning 该指令阻塞执行
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  waitTime         总线OP等待超时时间(单位 s, 预设为40s)
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x1003)
 *******************************************************************/
IMC_API IMC_ScanCardEcat(short cardIndex, short waitTime = 40);

/// @}

/// @defgroup EcatInfo 板卡EtherCAT主站信息获取函数
/// @brief 板卡EtherCAT主站信息获取函数
/// @{

/**
 * @brief  获取EtherCAT主站的状态
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  pStatus          获取的主站状态, 具体信息请参考\ref MasterStsDef "主站状态类型"
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x1010)
 *******************************************************************/
IMC_API IMC_GetEcatMasterSts(short cardIndex, unsigned int* pStatus);

/**
 * @brief  获取EtherCAT协议栈通讯错误码
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  pErrCode         获取到的错误码, 具体信息请参考\ref EcatErrorDef "协议栈通讯错误码"
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x1011)
 *******************************************************************/
IMC_API IMC_GetEcatErrCode(short cardIndex, unsigned int* pErrCode);

/**
 * @brief  获取EtherCAT主站的状态(Ex指令)
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  pStatus          获取的主站状态, 具体信息请参考\ref MasterStsDef "主站状态类型"
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x1020)
 *******************************************************************/
IMC_API IMC_GetEcatMasterStsEx(short cardIndex, unsigned int* pStatus);

/**
 * @brief  获取EtherCAT协议栈通讯错误码(Ex指令)
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  pErrCode         获取到的错误码, 具体信息请参考\ref EcatErrorDef "协议栈通讯错误码"
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x1021)
 *******************************************************************/
IMC_API IMC_GetEcatErrCodeEx(short cardIndex, unsigned int* pErrCode);

/**
 * @brief  获取EtherCAT从站当前通讯阶段INIT、PreOP、SafeOp、OP
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  slave            操作站点号
 * @param  pCurStatus       获取到的从站状态, 具体信息请参考\ref SlaveStsDef "从站状态类型"
 * @param  pReqStatus       获取到的从站状态, 具体信息请参考\ref SlaveStsDef "从站状态类型"
 * @param  pErrCode         获取到的从站错误码, 具体信息请参考\ref EcatErrorDef "协议栈通讯错误码"
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x1012)
 *******************************************************************/
IMC_API IMC_GetEcatSlaveSts(short cardIndex, short slave, short* pCurStatus, short* pReqStatus, short* pErrCode);

/**
 * @brief  获取EtherCAT主站信息(包含通讯周期, 从站个数, 资源数量等)
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  ptMasterInfo     获取的主站信息, 具体信息请参考\ref TMasterInfo "主站信息数据结构"
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x1014)
 *******************************************************************/
IMC_API IMC_GetEcatMasterInfo(short cardIndex, TMasterInfo* ptMasterInfo);

/**
 * @brief  获取EtherCAT从站信息(包含设备类型, 对应轴号, 从站别名等)
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  slave       操作站点号
 * @param  ptSlaveInfo      获取的从站信息, 具体信息请参考\ref TSlaveInfo "从站信息数据结构"
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x1015)
 *******************************************************************/
IMC_API IMC_GetEcatSlaveInfo(short cardIndex, short slave, TSlaveInfo* ptSlaveInfo);

/**
 * @brief  获取总线从站的连接状态
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  pSts             未开启别名：
 *                          \n 0: 存在掉线从站
 *                          \n 1: 所有从站在线
 *                          \n 开启别名：
 *                          \n 0: 所有从站掉线
 *                          \n 1: 所有从站在线
 *                          \n 2: 部分从站掉线
 * @param  pMask    从站OP状态bit位表示，数组长度为4，按bit位分别表示第0~128从站的状态
 *                          \n bit值为0: 表示对应从站不在线
 *                          \n bit值为1: 表示对应从站在线
 * @par 指令码              (0x1330)
 *******************************************************************/
IMC_API IMC_GetEcatSlaveOpSts(short cardIndex, unsigned short* pSts, unsigned int* pMask);

/// @}

/// @defgroup EcatPDO 板卡EtherCAT主站PDO功能
/// @brief 板卡EtherCAT主站PDO功能
/// @{

/**
 * @brief  根据站号读取EtherCAT从站的PDO数据
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  slave            从站站号
 * @param  index            索引
 * @param  subIndex         子索引
 * @param  pData            读取数据
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x1104)
 *******************************************************************/
IMC_API IMC_GetEcatSlavePdoData(short cardIndex, short slave, unsigned short index, unsigned short subIndex, unsigned char* pData);

/**
 * @brief  根据站号写入EtherCAT从站的PDO数据, 仅支持非轴从站, 非DIO, AIO, RegInOut类型Pdo
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  slave            从站站号
 * @param  index            索引
 * @param  subIndex         子索引
 * @param  pData            写入数据
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x1105)
 *******************************************************************/
IMC_API IMC_SetEcatSlavePdoData(short cardIndex, short slave, unsigned short index, unsigned short subIndex, unsigned char* pData);

/**
 * @brief  根据轴号获取EtherCAT从站的PDO数据
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  index            索引
 * @param  pData            获取数据
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x1106)
 *******************************************************************/
IMC_API IMC_GetEcatAxPdoData(short cardIndex, short axNo, unsigned short index, unsigned char* pData);

/**
 * @brief  根据轴号写入EtherCAT从站的PDO数据
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  index            索引
 * @param  pData            发送数据
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x1107)
 *******************************************************************/
IMC_API IMC_SetEcatAxPdoData(short cardIndex, short axNo, unsigned short index, unsigned char* pData);

/**
 * @brief  根据索引和子索引读取从站Pdo的相关信息
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  slave            从站站号
 * @param  index            索引
 * @param  subIndex         子索引
 * @param  pDevType         Pdo对象设备类型
 *                          \n 0:SERVO
 *                          \n 1:DO
 *                          \n 2:DI
 *                          \n 3:DA
 *                          \n 4:AD
 *                          \n 5:REG_IN
 *                          \n 6:REG_OUT
 *                          \n 7:CR_ST
 *                          \n 8:GENERAL
 *                          \n 9:ENC
 * @param  pBitOffset       Pdo全局偏移(站点起始偏移+站点相对偏移)
 * @param  pBitLen          PdoBit长度
 * @param  pDir             Pdo传输方向 0:RxPdo 1:TxPdo
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x1108)
 *******************************************************************/
IMC_API IMC_GetEcatSlavePdoEntry(short cardIndex, short slave, unsigned short index, unsigned short subIndex, char* pDevType, short* pBitOffset, short* pBitLen, short* pDir);
/// @}

/// @defgroup EcatSDO 板卡EtherCAT主站SDO功能
/// @brief 板卡EtherCAT主站SDO功能
/// @{
/**
 * @brief  根据站号获取EtherCAT从站的SDO数据
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  slave            从站站号
 * @param  index            索引
 * @param  subIndex         子索引
 * @param  pData            接收数据地址
 * @param  dataSize         接收数据大小 单位：byte
 * @param  pResultSize      实际返回数据大小 单位：byte
 * @param  pAbortCode       舍弃代码, 具体意义请参考\ref AbortCodeDef "SDO访问舍弃码"
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x1120)
 *******************************************************************/
IMC_API IMC_GetEcatSlaveSdo(short cardIndex, short slave, unsigned short index, unsigned short subIndex, unsigned char* pData, unsigned int dataSize, unsigned int* pResultSize, unsigned int* pAbortCode);

/**
 * @brief  根据站号写入EtherCAT从站的SDO数据
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  slave            从站站号
 * @param  index            索引
 * @param  subIndex         子索引
 * @param  pData            发送数据地址
 * @param  dataSize         发送数据大小 单位：byte
 * @param  pAbortCode       舍弃代码, 具体意义请参考\ref AbortCodeDef "SDO访问舍弃码"
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x1121)
 *******************************************************************/
IMC_API IMC_SetEcatSlaveSdo(short cardIndex, short slave, unsigned short index, unsigned short subIndex, unsigned char* pData, unsigned int dataSize, unsigned int* pAbortCode);

/**
 * @brief  根据轴号获取EtherCAT从站的SDO数据
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  index            索引
 * @param  subIndex         子索引
 * @param  pData            接收数据地址
 * @param  dataSize         接收数据大小 单位：byte
 * @param  pResultSize      实际返回数据大小 单位：byte
 * @param  pAbortCode       舍弃代码, 具体意义请参考\ref AbortCodeDef "SDO访问舍弃
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x1122)
 *******************************************************************/
IMC_API IMC_GetEcatAxSdo(short cardIndex, short axNo, unsigned short index, unsigned short subIndex, unsigned char* pData, unsigned int dataSize, unsigned int* pResultSize, unsigned int* pAbortCode);

/**
 * @brief  根据轴号写入EtherCAT从站的SDO数据
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  index            索引
 * @param  subIndex         子索引
 * @param  pData            发送数据地址
 * @param  dataSize         发送数据大小 单位：byte
 * @param  pAbortCode       舍弃代码, 具体意义请参考\ref AbortCodeDef "SDO访问舍弃码"
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x1123)
 *******************************************************************/
IMC_API IMC_SetEcatAxSdo(short cardIndex, short axNo, unsigned short index, unsigned short subIndex, unsigned char* pData, unsigned int dataSize, unsigned int* pAbortCode);
/// @}

/// @defgroup EcatAxisMap 板卡EtherCAT主站轴映射功能
/// @brief 板卡EtherCAT主站轴映射功能
/// @{
/**
 * @brief  根据轴号获取轴所在从站的站号以及Slot号
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  pSlave           获取的从站站号
 * @param  pSlotIndex       获取的Slot号
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x1300)
 *******************************************************************/
IMC_API IMC_GetEcatAxStation(short cardIndex, short axNo, short* pSlave, short* pSlotIndex);

/**
 * @brief  根据从站号以及Slot号获取对应的轴通道号
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  slave            操作的从站站号
 * @param  slotIndex        操作的Slot号
 * @param  pAxChn           获取的轴通道号
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x1301)
 *******************************************************************/
IMC_API IMC_GetEcatSlaveAxChn(short cardIndex, short slave, short slotIndex, short* pAxChn);

/**
 * @brief  根据从站别名号获取EtherCAT从站的SDO数据
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  aliasNo            从站别名号
 * @param  index            索引
 * @param  subIndex         子索引
 * @param  pData            接收数据地址
 * @param  dataSize         接收数据大小 单位：byte
 * @param  pResultSize      实际返回数据大小 单位：byte
 * @param  pAbortCode       舍弃代码, 具体意义请参考\ref AbortCodeDef "SDO访问舍弃码"
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x1322)
 *******************************************************************/
IMC_API IMC_GetEcatAliasSdo(short cardIndex, unsigned int aliasNo, unsigned short index, unsigned short subIndex, unsigned char* pData, unsigned int dataSize, unsigned int* pResultSize, unsigned int* pAbortCode);

/**
 * @brief  根据从站别名号写入EtherCAT从站的SDO数据
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  aliasNo            从站别名号
 * @param  index            索引
 * @param  subIndex         子索引
 * @param  pData            发送数据地址
 * @param  dataSize         发送数据大小 单位：byte
 * @param  pAbortCode       舍弃代码, 具体意义请参考\ref AbortCodeDef "SDO访问舍弃码"
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x1323)
 *******************************************************************/
IMC_API IMC_SetEcatAliasSdo(short cardIndex, unsigned int aliasNo, unsigned short index, unsigned short subIndex, unsigned char* pData, unsigned int dataSize, unsigned int* pAbortCode);

/// @}

/// @defgroup EcatIO 板卡EtherCAT主站IO功能
/// @brief 板卡EtherCAT主站DI/DO/AD/DA/Reg32In/Reg32Out寄存器功能
/// @{
/**
 * @brief  设置EtherCAT的DO输出状态保持, 断线后复位总线保持上一次状态
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  isHold           是否开启DO状态断线保持功能, 重启总线DO不复位 0-不开启 1-开启
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x1400)
 *******************************************************************/
IMC_API IMC_SetEcatDoStsHold(short cardIndex, short isHold);

/// @defgroup EcatIO 板卡EtherCAT主站IO功能
/// @brief 板卡EtherCAT主站DI/DO/AD/DA/Reg32In/Reg32Out寄存器功能
/// @{
/**
 * @brief  获取EtherCAT的DO输出状态保持状态
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  pIsHold          是否开启断线保持功能, 重启总线DO不复位 0-不开启 1-开启
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x1401)
 *******************************************************************/
IMC_API IMC_GetEcatDoStsHold(short cardIndex, short* pIsHold);

/**
 * @brief  获取EtherCAT的DI输入状态, 按16bits为一组进行获取
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  packIndex        操作DI组号, 取值范围：(0,128), 每组16 bit, 按照实际资源获取
 * @param  pValue           接收获取状态(按位进行解析, 每个bit表示对应的DI状态)
 * @param  packCnt          状态获取组数
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x1410)
 *******************************************************************/
IMC_API IMC_GetEcatDi(short cardIndex, short packIndex, unsigned short* pValue, short packCnt);

/**
 * @brief  设置EtherCAT的DO输出状态, 按16bits为一组进行设置
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  packIndex        操作DO组号, 取值范围：[0,127], 每组16 bit, 按照实际资源设置
 * @param  pValue           输出状态设置(按位进行设置, 每个bit表示对应的DO状态)
 * @param  packCnt          输出设置组数
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x1411)
 *******************************************************************/
IMC_API IMC_SetEcatDo(short cardIndex, short packIndex, unsigned short* pValue, short packCnt);

/**
 * @brief  获取EtherCAT的DO输出状态, 按16bits为一组进行获取
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  packIndex        操作DO组号, 取值范围：[0,127], 每组16 bit, 按照实际资源设置
 * @param  pValue           输出状态获取(按位进行解析, 每个bit表示对应的DO状态)
 * @param  packCnt          状态获取组数
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x1412)
 *******************************************************************/
IMC_API IMC_GetEcatDo(short cardIndex, short packIndex, unsigned short* pValue, short packCnt);

/**
 * @brief  获取EtherCAT的DI输入状态, 按位进行获取
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  diIndex          操作DI号, 取值范围：[0,2047], 按照实际资源获取
 * @param  pValue           输入状态
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x1413)
 *******************************************************************/
IMC_API IMC_GetEcatDiBit(short cardIndex, short diIndex, short* pValue);

/**
 * @brief  获取EtherCAT的DO输出状态, 按位进行获取
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  doIndex          操作DO号, 取值范围：[0,2047], 按照实际资源获取
 * @param  pValue           输出状态, 参数含义：0 关闭，其他 开启
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x1414)
 *******************************************************************/
IMC_API IMC_GetEcatDoBit(short cardIndex, short doIndex, short* pValue);

/**
 * @brief  设置EtherCAT的DO输出状态, 按位进行设置
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  doIndex          操作DO号, 取值范围：[0,2047], 按照实际资源设置
 * @param  value            输出状态, 参数含义：0 关闭，其他 开启
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x1415)
 *******************************************************************/
IMC_API IMC_SetEcatDoBit(short cardIndex, short doIndex, short value);

/**
 * @brief  设置EtherCAT的DO输出状态以及生效延时, 按位进行设置
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  doIndex          操作DO号, 取值范围：[0,2047], 按照实际资源设置
 * @param  value            输出状态, 参数含义：0 关闭，其他 开启
 * @param  inverseTime      延时取反状态, 取值范围：[0,4000], 单位ms
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x1416)
 *******************************************************************/
IMC_API IMC_SetEcatDoBitInverse(short cardIndex, short doIndex, short value, short inverseTime);

/**
 * @brief  根据通道号获取EtherCAT对应通道的AD值
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  adIndex          AD通道号, 取值范围：[0,63], 按照实际资源获取
 * @param  pValue           获取的AD输入值
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x1420)
 *******************************************************************/
IMC_API IMC_GetEcatAdVal(short cardIndex, short adIndex, short* pValue);

/**
 * @brief  根据通道号设置EtherCAT对应通道的DA值
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  daIndex          DA通道号, 取值范围：[0,63], 按照实际资源设置
 * @param  Value            设置的DA输出值
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x1421)
 *******************************************************************/
IMC_API IMC_SetEcatDaVal(short cardIndex, short daIndex, short Value);

/**
 * @brief  根据通道号获取EtherCAT对应通道的DA值
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  daIndex          DA通道号, 取值范围：[0,63], 按照实际资源获取
 * @param  pValue           获取的DA输出值
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x1422)
 *******************************************************************/
IMC_API IMC_GetEcatDaVal(short cardIndex, short daIndex, short* pValue);

/**
 * @brief  根据通道号获取EtherCAT对应通道的RegIn值
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  regIndex         RegIn通道号, 取值范围：[0,31], 按照实际资源获取
 * @param  pValue           获取的RegIn输入值
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x1424)
 *******************************************************************/
IMC_API IMC_GetEcatRegInVal(short cardIndex, short regIndex, float* pValue);

/**
 * @brief  根据通道号设置EtherCAT对应通道的RegOut值
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  regIndex         RegOut通道号, 取值范围：[0,31], 按照实际资源设置
 * @param  Value            设置的RegOut输出值
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x1426)
 *******************************************************************/
IMC_API IMC_SetEcatRegOutVal(short cardIndex, short regIndex, float Value);

/**
 * @brief  根据通道号获取EtherCAT对应通道的RegOut值
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  regIndex         RegOut通道号, 取值范围：[0,31], 按照实际资源获取
 * @param  pValue           获取的RegOut输出值
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x1425)
 *******************************************************************/
IMC_API IMC_GetEcatRegOutVal(short cardIndex, short regIndex, float* pValue);

/**
 * @brief  根据通道号批量获取EtherCAT对应数量的AD值
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  adIndex          AD通道号, 取值范围：[0,63], 按照实际资源获取
 * @param  pValueArray      获取的AD通道输入值
 * @param  count            批量获取的通道数量
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x1430)
 *******************************************************************/
IMC_API IMC_GetEcatAd(short cardIndex, short adIndex, short* pValueArray, short count = 1);

/**
 * @brief  根据通道号批量设置EtherCAT对应数量的DA值
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  daIndex          DA通道号, 取值范围：[0,63], 按照实际资源设置
 * @param  pValueArray      设置的DA通道输出值
 * @param  count            批量获取的通道数量
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x1431)
 *******************************************************************/
IMC_API IMC_SetEcatDa(short cardIndex, short daIndex, short* pValueArray, short count = 1);

/**
 * @brief  根据通道号批量获取EtherCAT对应数量的DA值
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  daIndex          DA通道号, 取值范围：[0,63], 按照实际资源设置
 * @param  pValueArray      获取的DA通道输出值
 * @param  count            批量获取的通道数量
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x1432)
 *******************************************************************/
IMC_API IMC_GetEcatDa(short cardIndex, short daIndex, short* pValueArray, short count = 1);

/**
 * @brief  根据通道号批量获取EtherCAT对应数量的RegIn值
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  regIndex         RegIn通道号, 取值范围：[0,31], 按照实际资源获取
 * @param  pValueArray      获取的RegIn输入值
 * @param  count            批量获取的通道数量
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x1434)
 *******************************************************************/
IMC_API IMC_GetEcatRegIn(short cardIndex, short regIndex, float* pValueArray, short count = 1);

/**
 * @brief  根据通道号批量设置EtherCAT对应数量的RegOut值
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  regIndex         RegOut通道号, 取值范围：[0,31], 按照实际资源设置
 * @param  pValueArray      设置的RegOut输出值
 * @param  count            批量获取的通道数量
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x1436)
 *******************************************************************/
IMC_API IMC_SetEcatRegOut(short cardIndex, short regIndex, float* pValueArray, short count = 1);

/**
 * @brief  根据通道号批量获取EtherCAT对应数量的RegOut值
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  regIndex         RegOut通道号, 取值范围：[0,31], 按照实际资源设置
 * @param  pValueArray      获取的RegOut输出值
 * @param  count            批量获取的通道数量
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x1435)
 *******************************************************************/
IMC_API IMC_GetEcatRegOut(short cardIndex, short regIndex, float* pValueArray, short count = 1);
/// @}

/// @defgroup EcatEnc 板卡EtherCAT主站Enc资源
/// @brief 板卡EtherCAT编码器资源操作
/// @{
/**
 * @brief  设置Ecat编码器计数方向
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  encIndex         表示Ecat编码器通道号：[0,7]
 * @param  pDirArray        设置Ecat编码器计数方向的取反参数数组, 按照编码器通道索引顺序排列, 0：不取反（正向计数） 1：取反（反向计数）
 * @param  count            设置的通道数量, N 表示同时设置 N 个编码器通道的计数方向, 设置参数在 pDirArray 数组中
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x1440)
 *******************************************************************/
IMC_API IMC_SetEcatEncDir(short cardIndex, short encIndex, short* pDirArray, short count = 1);

/**
 * @brief  读取Ecat编码器计数方向
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  encIndex         表示Ecat编码器通道号：[0,7]
 * @param  pDirArray        读取Ecat编码器计数方向的参数数组, 按照编码器通道索引顺序排列, 0：不取反（正向计数） 1：取反（反向计数）
 * @param  count            读取的通道数量, N 表示同时读取 N 个编码器通道的计数方向, 读取参数在 pDirArray 数组中
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x1441)
 *******************************************************************/
IMC_API IMC_GetEcatEncDir(short cardIndex, short encIndex, short* pDirArray, short count = 1);

/**
 * @brief  设置Ecat编码器值
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  encIndex         设定的Ecat编码器通道号：[0,7]
 * @param  pEncPosArray     读取的Ecat编码器值数组
 * @param  count            读取的Ecat编码器通道数量, N 表示同时读取 N 个编码器通道的值, 读取参数在 pEncPosArray 数组中
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x1442)
 *******************************************************************/
IMC_API IMC_SetEcatEncPos(short cardIndex, short encIndex, int* pEncPosArray, short count = 1);

/**
 * @brief  读取Ecat编码器值
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  encIndex         读取的Ecat编码器通道号：[0,7]
 * @param  pEncPosArray     读取的Ecat编码器值数组
 * @param  count            读取的Ecat编码器通道数量, N 表示同时读取 N 个编码器通道的值, 读取参数在 pEncPosArray 数组中
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x1443)
 *******************************************************************/
IMC_API IMC_GetEcatEncPos(short cardIndex, short encIndex, int* pEncPosArray, short count = 1);

/**
 * @brief  读取Ecat编码器原始值（PDO原始编码数据）
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  encIndex         读取的Ecat编码器通道号：[0,7]
 * @param  pEncPosArray     读取的Ecat编码器值数组
 * @param  count            读取的Ecat编码器通道数量, N 表示同时读取 N 个编码器通道的值, 读取参数在 pEncPosArray 数组中
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x1444)
 *******************************************************************/
IMC_API IMC_GetEcatEncPosRaw(short cardIndex, short encIndex, int* pEncPosArray, short count = 1);

/**
 * @brief  读取Ecat编码器速度
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  encIndex         读取的Ecat编码器通道号：[0,7]
 * @param  pEncVelArray     读取的Ecat编码器速度数组
 * @param  count            读取的Ecat编码器通道数量, N 表示同时读取 N 个编码器通道的速度, 读取参数在 pEncVelArray 数组中
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x1445)
 *******************************************************************/
IMC_API IMC_GetEcatEncVel(short cardIndex, short encIndex, int* pEncVelArray, short count = 1);
/// @}

/// @defgroup EcatAxisPDO 板卡EtherCAT轴PDO操作
/// @brief 板卡EtherCAT轴常用PDO操作（模式切换、AxIO读写、参数配置等功能）
/// @{
/**
 * @brief  设置EtherCAT类型轴控制模式0x6060
 * @details 该条指令不支持设置回零模式，需使用IMC_SetEcatHomingMode指令切换回零状态
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  ctrlMode         控制模式, 具体信息请参考\ref ServoOpModeDef "伺服控制模式"
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x1600)
 *******************************************************************/
IMC_API IMC_SetEcatAxMode(short cardIndex, short axNo, short ctrlMode);

/**
 * @brief  读取EtherCAT类型轴控制模式
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  pCtrlMode        控制模式, 具体信息请参考\ref ServoOpModeDef "伺服控制模式"
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x1601)
 *******************************************************************/
IMC_API IMC_GetEcatAxMode(short cardIndex, short axNo, short* pCtrlMode);

/**
 * @brief  获取EtherCAT类型轴的对应的数字量输入,pdo必须配置0x60fd
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  pDiVal           轴的数字量输入, 其值所对应的意义请参考相应的伺服手册
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x1602)
 *******************************************************************/
IMC_API IMC_GetEcatAxDi(short cardIndex, short axNo, unsigned int* pDiVal);

/**
 * @brief  设置EtherCAT类型轴的对应的数字量输出,pdo必须配置0x60fe：01
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  doVal            轴的数字量输出, 其值所对应的意义请参考相应的伺服手册
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x1603)
 *******************************************************************/
IMC_API IMC_SetEcatAxDo(short cardIndex, short axNo, unsigned int doVal);

/**
 * @brief  获取EtherCAT类型轴的对应的数字量输输出,pdo必须配置0x60fe：01
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  pDoVal           轴的数字量输出, 其值所对应的意义请参考相应的伺服手册
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x1604)
 *******************************************************************/
IMC_API IMC_GetEcatAxDo(short cardIndex, short axNo, unsigned int* pDoVal);

/**
 * @brief  获取EtherCAT类型轴的对应的错误码,pdo必须配置0x603f
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  pErrCode         返回的错误码, 参考对应的伺服手册
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x1605)
 *******************************************************************/
IMC_API IMC_GetEcatAxErrCode(short cardIndex, short axNo, unsigned short* pErrCode);

/**
 * @brief  设置EtherCAT类型轴的最大速度限制,pdo必须配置0x607f
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  maxVel           最大速度限制 (注意：具体指的意义请参考对应的伺服相关手册)
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x1606)
 *******************************************************************/
IMC_API IMC_SetEcatAxMaxVelLmt(short cardIndex, short axNo, unsigned int maxVel);

/**
 * @brief  获取EtherCAT类型轴的最大速度限制,pdo必须配置0x607f
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  pMaxVel          最大速度限制 (注意：具体指的意义请参考对应的伺服相关手册)
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x1607)
 *******************************************************************/
IMC_API IMC_GetEcatAxMaxVelLmt(short cardIndex, short axNo, unsigned int* pMaxVel);

/**
 * @brief  设置 EtherCAT 类型轴的对应的正向力矩限制,pdo必须配置0x60e0
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  posTorqLmt       正向力矩限制 (注意：具体指的意义请参考对应的伺服相关手册)
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x1608)
 *******************************************************************/
IMC_API IMC_SetEcatAxPosTorqLmt(short cardIndex, short axNo, unsigned short posTorqLmt);

/**
 * @brief  获取 EtherCAT 类型轴的对应的正向力矩限制,pdo必须配置0x60e0
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  pPosTorqLmt      正向力矩限制 (注意：具体指的意义请参考对应的伺服相关手册)
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x1609)
 *******************************************************************/
IMC_API IMC_GetEcatAxPosTorqLmt(short cardIndex, short axNo, unsigned short* pPosTorqLmt);

/**
 * @brief  设置 EtherCAT 类型轴的对应的负向力矩限制,pdo必须配置0x60e1
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  negTorqLmt       负向力矩限制 (注意：具体指的意义请参考对应的伺服相关手册)
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x160a)
 *******************************************************************/
IMC_API IMC_SetEcatAxNegTorqLmt(short cardIndex, short axNo, unsigned short negTorqLmt);

/**
 * @brief  获取 EtherCAT 类型轴的对应的负向力矩限制,pdo必须配置0x60e1
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  pNegTorqLmt      负向力矩限制 (注意：具体指的意义请参考对应的伺服相关手册)
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x160b)
 *******************************************************************/
IMC_API IMC_GetEcatAxNegTorqLmt(short cardIndex, short axNo, unsigned short* pNegTorqLmt);

/**
 * @brief  设置 EtherCAT 类型轴的最大力矩限制,pdo必须配置0x6072
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  maxTorqLmt       最大力矩限制 (注意：具体指的意义请参考对应的伺服相关手册)
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x160c)
 *******************************************************************/
IMC_API IMC_SetEcatAxMaxTorqLmt(short cardIndex, short axNo, unsigned short maxTorqLmt);

/**
 * @brief  获取 EtherCAT 类型轴的最大力矩限制,pdo必须配置0x6072
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  pMaxTorqLmt      最大力矩限制 (注意：具体指的意义请参考对应的伺服相关手册)
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x160d)
 *******************************************************************/
IMC_API IMC_GetEcatAxMaxTorqLmt(short cardIndex, short axNo, unsigned short* pMaxTorqLmt);

/**
 * @brief  设置 EtherCAT 类型轴的使能检测时间，使能命令后检测时间内未等待到使能状态，则自动下使能
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  waitPeriod      等待时间，单位周期，参数范围：[0,20000]
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x1680)
 *******************************************************************/
IMC_API IMC_SetEcatAxOnThreshold(short cardIndex, short axNo, unsigned short waitPeriod);

/**
 * @brief  设置 EtherCAT 类型轴的使能检测时间，使能命令后检测时间内未等待到使能状态，则自动下使能
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  pWaitPeriod      等待时间，单位周期
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x1681)
 *******************************************************************/
IMC_API IMC_GetEcatAxOnThreshold(short cardIndex, short axNo, unsigned short* pWaitPeriod);

/**
 * @brief  设置 EtherCAT 类型轴的CSV或CST切换到其他控制模式的最大反馈速度，反馈速度大于该速度时不允许切换到其他控制模式
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  lmtVel           限制的最大速度, 单位unit/cycle
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x1682)
 *******************************************************************/
IMC_API IMC_SetEcatAxSwitchModeVelLmt(short cardIndex, short axNo, int lmtVel);

/**
 * @brief  获取 EtherCAT 类型轴的CSV或CST切换到其他控制模式的最大反馈速度，反馈速度大于该速度时不允许切换到其他控制模式
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  pLmtVel          限制的最大速度, 单位unit/cycle
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x1683)
 *******************************************************************/
IMC_API IMC_GetEcatAxSwitchModeVelLmt(short cardIndex, short axNo, int* pLmtVel);

/// @}

/// @defgroup EcatAxisCSV 板卡EtherCAT轴CSV模式操作
/// @brief 板卡EtherCAT轴CSV模式操作
/// @{
/**
 * @brief  启动 CSV 速度规划
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  tgtVel           目标速度, 取值范围：[-maxVel,maxVel]
 * @param  acc              加速度, 取值范围：[0,maxAcc]
 * @param  prfType          0：T 型速度规划 \n 1：S 型速度规划
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x1700)
 *******************************************************************/
IMC_API IMC_StartEcatAxCsvPrf(short cardIndex, short axNo, int tgtVel, int acc, short prfType);

/**
 * @brief  更新 CSV 速度规划的目标速度
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  tgtVel           目标速度, 取值范围：[-maxVel,maxVel]
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x1701)
 *******************************************************************/
IMC_API IMC_UpdateEcatAxCsvPrf(short cardIndex, short axNo, int tgtVel);

/**
 * @brief  获取 CSV 速度规划的状态
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  pStatus          规划状态 \n 0 停止状态 \n 1 规划中 \n 2 停止过程中
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x1702)
 *******************************************************************/
IMC_API IMC_GetEcatAxCsvPrfSts(short cardIndex, short axNo, short* pStatus);
/// @}

/// @defgroup EcatAxisCST 板卡EtherCAT轴CST模式操作
/// @brief 板卡EtherCAT轴CST模式操作
/// @{
/**
 * @brief  启动 CST 线性转矩规划
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  tgtTorq          目标转矩, 参数范围及单位参考对应的伺服手册
 * @param  time             从当前转矩变化至目标转矩需要的时间, 取值范围：(0,intMax]单位：ms
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x1720)
 *******************************************************************/
IMC_API IMC_StartEcatAxTorqPrf(short cardIndex, short axNo, short tgtTorq, short time);

/**
 * @brief  获取 EtherCAT 类型轴的实际转矩, 根据count值可一次获取多个轴的扭矩
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  pActTorqArray    获取的实际转矩数组, 按照轴号顺序排列, 参数范围及单位参考对应的伺服手册
 * @param  count            获取的轴数量, 按照实际资源读取, 参数范围：[1,32]
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x1721)
 *******************************************************************/
IMC_API IMC_GetEcatAxActTorq(short cardIndex, short axNo, short* pActTorqArray, short count = 1);

/**
 * @brief  设置 EtherCAT 类型轴的对应的转矩斜坡值
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  torqSlope        转矩斜坡值 (注意：具体指的意义请参考对应的伺服相关手册)
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x1724)
 *******************************************************************/
IMC_API IMC_SetEcatAxTorqSlope(short cardIndex, short axNo, unsigned int torqSlope);

/**
 * @brief  获取 EtherCAT 类型轴的对应的转矩斜坡值
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  pTorqSlope       转矩斜坡值 (注意：具体指的意义请参考对应的伺服相关手册)
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x1725)
 *******************************************************************/
IMC_API IMC_GetEcatAxTorqSlope(short cardIndex, short axNo, unsigned int* pTorqSlope);

/**
 * @brief  设置 EtherCAT 类型轴的对应的目标转矩
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  tgtTorq          目标转矩, 参数范围及单位参考对应的伺服手册
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x1726)
 *******************************************************************/
IMC_API IMC_SetEcatAxTgtTorq(short cardIndex, short axNo, short tgtTorq);

/**
 * @brief  获取 EtherCAT 类型轴的对应的目标转矩
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  pTgtTorq         目标转矩值, 参数范围及单位参考对应的伺服手册
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x1727)
 *******************************************************************/
IMC_API IMC_GetEcatAxTgtTorq(short cardIndex, short axNo, short* pTgtTorq);
/// @}

/// @defgroup EcatAxisCapt 板卡EtherCAT轴探针捕获功能
/// @brief 板卡EtherCAT轴探针捕获功能
/// @{
/**
 * @brief  设置 EtherCAT 类型轴的位置捕获类型
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  trigType         触发源 \n 0：外部信号触发(伺服的固定 DI) \n 1：Z 信号触发
 * @param  edge             触发边沿 \n 0：下降沿触发 \n 1：上升沿触发 \n 2：双沿触发
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x1750)
 *******************************************************************/
IMC_API IMC_SetEcatAxCapt(short cardIndex, short axNo, short trigType, short edge);

/**
 * @brief  获取 EtherCAT 类型轴的位置捕获类型
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  pTrigType        触发源 \n 0：外部信号触发(伺服的固定 DI) \n 1：Z 信号触发
 * @param  pEdge            触发边沿 \n 0：下降沿触发 \n 1：上升沿触发 \n 2：双沿触发
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x1751)
 *******************************************************************/
IMC_API IMC_GetEcatAxCapt(short cardIndex, short axNo, short* pTrigType, short* pEdge);

/**
 * @brief  获取 EtherCAT 类型轴的位置捕获状态
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  pSts             捕获状态 \n 0：未触发捕获 \n 1：已触发捕获
 * @param  pPosVal          上升沿捕获的位置, 单位：pulse
 * @param  pNegVal          下降沿捕获的位置, 单位：pulse
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x1752)
 *******************************************************************/
IMC_API IMC_GetEcatAxCaptStatus(short cardIndex, short axNo, short* pSts, int* pPosVal, int* pNegVal);

/**
 * @brief  设置Ecat轴的探针控制字0x60B8
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  probeFun         设置探针控制字0x60B8, 详见CiA 402: Object 60B8h: Touch probe function
 * @attention               连续捕获bit位置为1时开始连续捕获位置缓存, 置为0时停止连续捕获位置缓存
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x1760)
 *******************************************************************/
IMC_API IMC_SetEcatAxProbeFun(short cardIndex, short axNo, short probeFun);

/**
 * @brief  读取Ecat轴的探针控制字0x60B8
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  pProbeFun        读取探针控制字0x60B8, 详见CiA 402: Object 60B8h: Touch probe function
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x1761)
 *******************************************************************/
IMC_API IMC_GetEcatAxProbeFun(short cardIndex, short axNo, short* pProbeFun);

/**
 * @brief  读取Ecat轴的探针状态字0x60B9, 以及当前探针捕获位置
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  pProbeSts        读取探针状态字0x60B9, 详见CiA 402: Object 60B9h: Touch probe status
 * @param  pProbe1Pos       读取探针1捕获上升沿位置0x60BA, 详见CiA 402: Object 60BAh: Touch probe pos1 pos value
 * @param  pProbe1Neg       读取探针1捕获下降沿位置0x60BB, 详见CiA 402: Object 60BBh: Touch probe pos1 neg value
 * @param  pProbe2Pos       读取探针2捕获上升沿位置0x60BC, 详见CiA 402: Object 60BCh: Touch probe pos2 pos value
 * @param  pProbe2Neg       读取探针2捕获下降沿位置0x60BD, 详见CiA 402: Object 60BDh: Touch probe pos2 neg value
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x1762)
 *******************************************************************/
IMC_API IMC_GetEcatAxProbeSts(short cardIndex, short axNo, short* pProbeSts, int* pProbe1Pos, int* pProbe1Neg, int* pProbe2Pos, int* pProbe2Neg);

/**
 * @brief  设置Ecat轴探针连续捕获次数
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  pCountArray      设置探针各边沿待捕获的次数数组, 排列顺序为: {pos1, neg1, pos2, neg2}
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x1763)
 *******************************************************************/
IMC_API IMC_SetEcatAxProbeContinousCount(short cardIndex, short axNo, short* pCountArray);

/**
 * @brief  获取Ecat轴探针连续捕获次数
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  pCountArray      获取探针各边沿待捕获的次数数组, 排列顺序为: {pos1, neg1, pos2, neg2}
 * @param  pTrigArray       获取探针各边沿已触发的次数数组, 排列顺序为: {pos1, neg1, pos2, neg2}
 * @param  pRestArray       获取探针各边沿剩余捕获次数数组, 排列顺序为: {pos1, neg1, pos2, neg2}
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x1764)
 *******************************************************************/
IMC_API IMC_GetEcatAxProbeContinousCount(short cardIndex, short axNo, short* pCountArray, short* pTrigArray, short* pRestArray);

/**
 * @brief  获取Ecat轴探针连续捕获位置
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  probeType        边沿类型 \n 0：pos1 \n 1：neg1 \n 2：pos2 \n 3：neg2
 * @param  pPosArray        捕获位置的存储数组
 * @attention               连续捕获位置缓存表满或停止连续捕获后, 需将全部数据取出或清空缓存表后才能重新开始缓存连续捕获位置
 * @param  count            获取位置数量, 范围：[1,4096]
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x1765)
 *******************************************************************/
IMC_API IMC_GetEcatAxProbeContinousData(short cardIndex, short axNo, short probeType, int* pPosArray, short count);

/**
 * @brief  设置Ecat轴探针连续捕获位置缓存表绑定
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  tableIndex       设置表号, 参数范围：[0,7]
 * @attention               板卡初始化时默认将表0~7绑定到对应轴号的Ecat轴, 可调用此接口修改表绑定
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x1766)
 *******************************************************************/
IMC_API IMC_SetEcatAxProbeContinousTable(short cardIndex, short axNo, short tableIndex);

/**
 * @brief  读取Ecat轴探针连续捕获位置缓存表绑定
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  pTableIndex      读取表号, 参数范围：[0,7]
 * @attention               板卡初始化时默认将表0~7绑定到对应轴号的Ecat轴, 可调用此接口修改表绑定
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x1767)
 *******************************************************************/
IMC_API IMC_GetEcatAxProbeContinousTable(short cardIndex, short axNo, short* pTableIndex);
/**
 * @brief  清除Ecat轴的连续位置捕获状态
 *
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x1757)
 *******************************************************************/
IMC_API IMC_ClrEcatAxCaptStatus(short cardIndex, short axNo);

/**
 * @brief  非标指令，解锁定制从站
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  slave            操作从站号
 * @param  type             操作从站类型，type= 1，汇川定制伺服
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0xe100)
 *******************************************************************/
IMC_API IMC_UnlockEcatCustomSlave(short cardIndex, short slave, short type);

// 非标，获取系统处理ecat时间，不区分主站
IMC_API IMC_GetEcatTime(short cardIndex, long long* pSysTime);

/// @}
/// @}

/// @defgroup HEcatFunc 板卡H主站函数
/// @brief H主站函数
/// @{

/// @defgroup HMasterCfg 板卡H主站系统配置函数
/// @brief 板卡H主站系统配置函数
/// @{
/**
 * @brief  获取板卡的CPU负载
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  pLoadRatio       控制器的计算负载率：(0,100)%
 * @warning 建议负载率不要超过60%
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x0410)
 *******************************************************************/
IMC_API IMC_H_GetCalcLoadRatio(short cardIndex, double* pLoadRatio);

/**
 * @brief  复位控制卡系统参数
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x0710)
 *******************************************************************/
IMC_API IMC_H_ResetSysPara(short cardIndex);
/// @}

/// @defgroup HEtherCAT 板卡H主站功能
/// @brief 板卡H主站功能
/// @{
/**
 * @brief  初始化板卡通讯, 主站进入OP状态
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x1800)
 *******************************************************************/
IMC_API IMC_H_InitEcatComm(short cardIndex);

/**
 * @brief  开启板卡通讯, 初始化EtherCAT总线资源, 建立主站与各从站之间的通讯
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x1801)
 *******************************************************************/
IMC_API IMC_H_StartEcatComm(short cardIndex);

/**
 * @brief  关闭板卡通讯, 主站退出OP状态
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x1802)
 *******************************************************************/
IMC_API IMC_H_DelEcatComm(short cardIndex);

/**
 * @brief  初始化总线, 直到主站进入OP状态, 随后扫描EtherCAT总线资源, 并建立主站与各从站之间的通讯 (该函数会阻塞在EtherCAT的总线扫描同步阶段)
 * @warning 该指令阻塞执行
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  waitTime         总线OP等待超时时间(单位 s, 预设为40s)
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x1803)
 *******************************************************************/
IMC_API IMC_H_ScanCardEcat(short cardIndex, short waitTime = 40);

/**
 * @brief  获取EtherCAT主站的状态
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  pStatus          获取的主站状态, 具体信息请参考\ref MasterStsDef "主站状态类型"
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x1810)
 *******************************************************************/
IMC_API IMC_H_GetEcatMasterSts(short cardIndex, unsigned int* pStatus);

/**
 * @brief  获取EtherCAT协议栈通讯错误码
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  pErrCode         获取到的错误码, 具体信息请参考\ref EcatErrorDef "协议栈通讯错误码"
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x1811)
 *******************************************************************/
IMC_API IMC_H_GetEcatErrCode(short cardIndex, unsigned int* pErrCode);

/**
 * @brief  获取EtherCAT主站的状态(Ex指令)
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  pStatus          获取的主站状态, 具体信息请参考\ref MasterStsDef "主站状态类型"
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x1820)
 *******************************************************************/
IMC_API IMC_H_GetEcatMasterStsEx(short cardIndex, unsigned int* pStatus);

/**
 * @brief  获取EtherCAT协议栈通讯错误码(Ex指令)
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  pErrCode         获取到的错误码, 具体信息请参考\ref EcatErrorDef "协议栈通讯错误码"
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x1821)
 *******************************************************************/
IMC_API IMC_H_GetEcatErrCodeEx(short cardIndex, unsigned int* pErrCode);

/**
 * @brief  获取EtherCAT从站当前通讯阶段, INIT, PerOP, SafeOP, OP
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  slave            操作站点号
 * @param  pCurStatus       获取到的从站状态, 具体信息请参考\ref SlaveStsDef "从站状态类型"
 * @param  pReqStatus       获取到的从站状态, 具体信息请参考\ref SlaveStsDef "从站状态类型"
 * @param  pErrCode         获取到的从站错误码, 具体信息请参考\ref EcatErrorDef "协议栈通讯错误码"
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x1812)
 *******************************************************************/
IMC_API IMC_H_GetEcatSlaveSts(short cardIndex, short slave, short* pCurStatus, short* pReqStatus, short* pErrCode);

/**
 * @brief  获取EtherCAT主站信息(包含通讯周期, 从站个数, 资源数量等)
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  ptMasterInfo     获取的主站信息, 具体信息请参考\ref TMasterInfo "主站信息数据结构"
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x1814)
 *******************************************************************/
IMC_API IMC_H_GetEcatMasterInfo(short cardIndex, TMasterInfo* ptMasterInfo);

/**
 * @brief  获取EtherCAT从站信息(包含设备类型, 对应轴号, 从站别名等)
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  slave            操作站点号
 * @param  ptSlaveInfo      获取的从站信息, 具体信息请参考\ref TSlaveInfo "从站信息数据结构"
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x1815)
 *******************************************************************/
IMC_API IMC_H_GetEcatSlaveInfo(short cardIndex, short slave, TSlaveInfo* ptSlaveInfo);

/**
 * @brief  根据站号读取EtherCAT从站的PDO数据
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  slave            从站站号
 * @param  index            索引
 * @param  subIndex         子索引
 * @param  pData            读取数据
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x1904)
 *******************************************************************/
IMC_API IMC_H_GetEcatSlavePdoData(short cardIndex, short slave, unsigned short index, unsigned short subIndex, unsigned char* pData);

/**
 * @brief  根据站号写入EtherCAT从站的PDO数据
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  slave            从站站号
 * @param  index            索引
 * @param  subIndex         子索引
 * @param  pData            写入数据
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x1905)
 *******************************************************************/
IMC_API IMC_H_SetEcatSlavePdoData(short cardIndex, short slave, unsigned short index, unsigned short subIndex, unsigned char* pData);

/**
 * @brief  根据站号获取EtherCAT从站的SDO数据
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  slave            从站站号
 * @param  index            索引
 * @param  subIndex         子索引
 * @param  pData            接收数据地址
 * @param  dataSize         接收数据大小 单位：byte
 * @param  pResultSize      实际返回数据大小 单位：byte
 * @param  pAbortCode       舍弃代码, 具体意义请参考\ref AbortCodeDef "SDO访问舍弃码"
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x1920)
 *******************************************************************/
IMC_API IMC_H_GetEcatSlaveSdo(short cardIndex, short slave, unsigned short index, unsigned short subIndex, unsigned char* pData, unsigned int dataSize, unsigned int* pResultSize, unsigned int* pAbortCode);

/**
 * @brief  根据站号写入EtherCAT从站的SDO数据
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  slave            从站站号
 * @param  index            索引
 * @param  subIndex         子索引
 * @param  pData            发送数据地址
 * @param  dataSize         发送数据大小 单位：byte
 * @param  pAbortCode       舍弃代码, 具体意义请参考\ref AbortCodeDef "SDO访问舍弃码"
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x1921)
 *******************************************************************/
IMC_API IMC_H_SetEcatSlaveSdo(short cardIndex, short slave, unsigned short index, unsigned short subIndex, unsigned char* pData, unsigned int dataSize, unsigned int* pAbortCode);

/**
 * @brief  获取EtherCAT的DI输入状态, 按16bits为一组进行获取
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  packIndex        操作DI组号, 取值范围：[0,127], 每组16 bit, 按照实际资源获取
 * @param  pValue           接收获取状态(按位进行解析, 每个bit表示对应的DI状态)
 * @param  packCnt          状态获取组数
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x1a00)
 *******************************************************************/
IMC_API IMC_H_GetEcatDi(short cardIndex, short packIndex, unsigned short* pValue, short packCnt);

/**
 * @brief  设置EtherCAT的DO输出状态, 按16bits为一组进行设置
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  packIndex        操作DO组号, 取值范围：[0,127], 每组16 bit, 按照实际资源设置
 * @param  pValue           输出状态设置(按位进行设置, 每个bit表示对应的DO状态)
 * @param  packCnt          输出设置组数
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x1a02)
 *******************************************************************/
IMC_API IMC_H_SetEcatDo(short cardIndex, short packIndex, unsigned short* pValue, short packCnt);

/**
 * @brief  获取EtherCAT的DO输出状态, 按16bits为一组进行获取
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  packIndex        操作DO组号, 取值范围：[0,127], 每组16 bit, 按照实际资源设置
 * @param  pValue           输出状态获取(按位进行解析, 每个bit表示对应的DO状态)
 * @param  packCnt          状态获取组数
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x1a01)
 *******************************************************************/
IMC_API IMC_H_GetEcatDo(short cardIndex, short packIndex, unsigned short* pValue, short packCnt);

/**
 * @brief  获取EtherCAT的DI输入状态, 按位进行获取
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  diIndex          操作DI号, 取值范围：[0,2047], 按照实际资源获取
 * @param  pValue           输入状态
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x1a03)
 *******************************************************************/
IMC_API IMC_H_GetEcatDiBit(short cardIndex, short diIndex, short* pValue);

/**
 * @brief  获取EtherCAT的DO输出状态, 按位进行获取
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  doIndex          操作DO号, 取值范围：[0,2047], 按照实际资源获取
 * @param  pValue           输出状态, 参数含义：0 关闭，其他 开启
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x1a05)
 *******************************************************************/
IMC_API IMC_H_GetEcatDoBit(short cardIndex, short doIndex, short* pValue);

/**
 * @brief  设置EtherCAT的DO输出状态, 按位进行设置
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  doIndex          操作DO号, 取值范围：[0,2047], 按照实际资源设置
 * @param  value            输出状态, 参数含义：0 关闭，其他 开启
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x1a04)
 *******************************************************************/
IMC_API IMC_H_SetEcatDoBit(short cardIndex, short doIndex, short value);

/**
 * @brief  设置EtherCAT的DO输出状态保持
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  isHold           DO输出状态保持标志 0-不保持 1-保持
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x1a06)
 *******************************************************************/
IMC_API IMC_H_SetEcatDoStsHold(short cardIndex, short isHold);

/**
 * @brief  获取EtherCAT的DO输出状态保持
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  pIsHold          获取的DO输出状态保持标志 0-不保持 1-保持
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x1a06)
 *******************************************************************/
IMC_API IMC_H_GetEcatDoStsHold(short cardIndex, short* pIsHold);

/**
 * @brief  根据通道号获取EtherCAT对应通道的AD值
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  adIndex          AD通道号, 取值范围：[0,63], 按照实际资源获取
 * @param  pValue           获取的AD输入值
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x1a20)
 *******************************************************************/
IMC_API IMC_H_GetEcatAdVal(short cardIndex, short adIndex, short* pValue);

/**
 * @brief  根据通道号设置EtherCAT对应通道的DA值
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  daIndex          DA通道号, 取值范围：[0,63], 按照实际资源设置
 * @param  Value            设置的DA输出值
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x1a21)
 *******************************************************************/
IMC_API IMC_H_SetEcatDaVal(short cardIndex, short daIndex, short Value);

/**
 * @brief  根据通道号获取EtherCAT对应通道的DA值
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  daIndex          DA通道号, 取值范围：[0,63], 按照实际资源获取
 * @param  pValue           获取的DA输出值
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x1a22)
 *******************************************************************/
IMC_API IMC_H_GetEcatDaVal(short cardIndex, short daIndex, short* pValue);

/**
 * @brief  根据通道号获取EtherCAT对应通道的RegIn值
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  regIndex         RegIn通道号, 取值范围：[0,31], 按照实际资源获取
 * @param  pValue           获取的RegIn输入值
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x1a24)
 *******************************************************************/
IMC_API IMC_H_GetEcatRegInVal(short cardIndex, short regIndex, float* pValue);

/**
 * @brief  根据通道号设置EtherCAT对应通道的RegOut值
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  regIndex         RegOut通道号, 取值范围：[0,31], 按照实际资源设置
 * @param  Value            设置的RegOut输出值
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x1a26)
 *******************************************************************/
IMC_API IMC_H_SetEcatRegOutVal(short cardIndex, short regIndex, float Value);

/**
 * @brief  根据通道号获取EtherCAT对应通道的RegOut值
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  regIndex         RegOut通道号, 取值范围：[0,31], 按照实际资源获取
 * @param  pValue           获取的RegOut输出值
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x1a25)
 *******************************************************************/
IMC_API IMC_H_GetEcatRegOutVal(short cardIndex, short regIndex, float* pValue);
/**
 * @brief  根据通道号批量获取EtherCAT对应数量的AD值
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  adIndex          AD通道号, 取值范围：[0,63], 按照实际资源获取
 * @param  pValueArray      获取的AD通道输入值
 * @param  count            批量获取的通道数量
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x1a30)
 *******************************************************************/
IMC_API IMC_H_GetEcatAd(short cardIndex, short adIndex, short* pValueArray, short count = 1);

/**
 * @brief  根据通道号批量设置EtherCAT对应数量的DA值
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  daIndex          DA通道号, 取值范围：[0,63], 按照实际资源设置
 * @param  pValueArray      设置的DA通道输出值
 * @param  count            批量获取的通道数量
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x1a31)
 *******************************************************************/
IMC_API IMC_H_SetEcatDa(short cardIndex, short daIndex, short* pValueArray, short count = 1);

/**
 * @brief  根据通道号批量获取EtherCAT对应数量的DA值
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  daIndex          DA通道号, 取值范围：[0,63], 按照实际资源设置
 * @param  pValueArray      获取的DA通道输出值
 * @param  count            批量获取的通道数量
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x1a32)
 *******************************************************************/
IMC_API IMC_H_GetEcatDa(short cardIndex, short daIndex, short* pValueArray, short count = 1);

/**
 * @brief  根据通道号批量获取EtherCAT对应数量的RegIn值
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  regIndex         RegIn通道号, 取值范围：[0,31], 按照实际资源获取
 * @param  pValueArray      获取的RegIn输入值
 * @param  count            批量获取的通道数量
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x1a34)
 *******************************************************************/
IMC_API IMC_H_GetEcatRegIn(short cardIndex, short regIndex, float* pValueArray, short count = 1);

/**
 * @brief  根据通道号批量设置EtherCAT对应数量的RegOut值
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  regIndex         RegOut通道号, 取值范围：[0,31], 按照实际资源设置
 * @param  pValueArray      设置的RegOut输出值
 * @param  count            批量获取的通道数量
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x1a35)
 *******************************************************************/
IMC_API IMC_H_SetEcatRegOut(short cardIndex, short regIndex, float* pValueArray, short count = 1);

/**
 * @brief  根据通道号批量获取EtherCAT对应数量的RegOut值
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  regIndex         RegOut通道号, 取值范围：[0,31], 按照实际资源设置
 * @param  pValueArray      获取的RegOut输出值
 * @param  count            批量获取的通道数量
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x1a36)
 *******************************************************************/
IMC_API IMC_H_GetEcatRegOut(short cardIndex, short regIndex, float* pValueArray, short count = 1);
/// @}
/// @}

/// @defgroup LocalBus 板卡端子板功能
/// @brief 板卡端子板功能
/// @{

/// @defgroup LocalInfo 板卡端子板信息获取
/// @brief 板卡端子板信息获取
/// @{
/**
 * @brief  获取端子板的工作状态
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  pValue           端子板工作状态 \n 0：端子板不在线 \n 1：端子板初始化中 \n 2：端子板初始化完成 \n 3：端子板初始化失败
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x2001)
 *******************************************************************/
IMC_API IMC_GetLocalBoardWorkSts(short cardIndex, short* pValue);
/// @}

/// @defgroup LocalParam 板卡端子板参数设置
/// @brief 板卡端子板参数设置
/// @{
/**
 * @brief  设置端子板 DI 滤波时间
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  filterTime       设置的滤波参数, 参数：0:10±1us 1:100±10us 2:400±40us 3:700±70us 4:1000±100us
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x2100)
 *******************************************************************/
IMC_API IMC_SetLocalDiFilterTime(short cardIndex, unsigned short filterTime);

/**
 * @brief  读取端子板 DI 滤波时间
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  pFilterTime      读取的滤波参数, 参数：0:10±1us 1:100±10us 2:400±40us 3:700±70us 4:1000±100us
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x2101)
 *******************************************************************/
IMC_API IMC_GetLocalDiFilterTime(short cardIndex, unsigned short* pFilterTime);

/**
 * @brief  设置端子板编码器滤波参数
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  filterDepth      滤波深度：3 或 5
 * @param  filterCoef       滤波系数：[0,127]
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x2102)
 *******************************************************************/
IMC_API IMC_SetLocalEncFilterPara(short cardIndex, short filterDepth, short filterCoef);

/**
 * @brief  读取端子板编码器滤波参数
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  pFilterDepth     滤波深度：3 或 5
 * @param  pFilterCoef      滤波系数：[0,127]
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x2103)
 *******************************************************************/
IMC_API IMC_GetLocalEncFilterPara(short cardIndex, short* pFilterDepth, short* pFilterCoef);

/**
 * @brief  设置端子板编码器计数方向
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  encIndex         表示端子板的编码器通道索引：[0,2]
 * @param  pDirArray        设置端子板编码器计数方向的取反参数数组, 按照编码器通道索引顺序排列, 0：不取反（正向计数） 1：取反（反向计数）
 * @param  count            设置的通道数量, N 表示同时设置 N 个编码器通道的计数方向, 设置参数在 pDirArray 数组中
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x2108)
 *******************************************************************/
IMC_API IMC_SetLocalEncDir(short cardIndex, short encIndex, short* pDirArray, short count = 1);

/**
 * @brief  读取端子板编码器计数方向
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  encIndex         表示端子板的编码器通道索引：[0,2]
 * @param  pDirArray        读取端子板编码器计数方向的参数数组, 按照编码器通道索引顺序排列, 0：不取反（正向计数） 1：取反（反向计数）
 * @param  count            读取的通道数量, N 表示同时读取 N 个编码器通道的计数方向, 读取参数在 pDirArray 数组中
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x2109)
 *******************************************************************/
IMC_API IMC_GetLocalEncDir(short cardIndex, short encIndex, short* pDirArray, short count = 1);
/// @}

/// @defgroup LocalDIO 板卡端子板IO操作
/// @brief 板卡端子板IO操作
/// @{
/**
 * @brief  获取本地 DI 值
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  pValue           本地 DI 的对应值, 低8bit有效
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x2200)
 *******************************************************************/
IMC_API IMC_GetLocalDi(short cardIndex, short* pValue);

/**
 * @brief  设置本地 DO 值
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  value            本地 DO 的对应值, 低8bit有效
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x2201)
 *******************************************************************/
IMC_API IMC_SetLocalDo(short cardIndex, short value);

/**
 * @brief  读取本地 DO 值
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  pValue           本地 DO 的对应值, 低8bit有效
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x2202)
 *******************************************************************/
IMC_API IMC_GetLocalDo(short cardIndex, short* pValue);

/**
 * @brief  按位获取本地第 diIndex 个 DI 值
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  diIndex          DI 序号：[0,7]
 * @param  pValue           读取对应 DI 的输入状态
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x2203)
 *******************************************************************/
IMC_API IMC_GetLocalDiBit(short cardIndex, short diIndex, short* pValue);

/**
 * @brief  按位设置本地第 doIndex 个 DO 值
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  doIndex          DO 序号：[0,7]
 * @param  value            设置对应 DO 的输出状态
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x2204)
 *******************************************************************/
IMC_API IMC_SetLocalDoBit(short cardIndex, short doIndex, short value);

/**
 * @brief  按位设置本地第 doIndex 个 DO 值
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  doIndex          DO 序号：[0,7]
 * @param  pValue           设置对应 DO 的输出状态
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x2205)
 *******************************************************************/
IMC_API IMC_GetLocalDoBit(short cardIndex, short doIndex, short* pValue);

/**
 * @brief  按位脉冲输出本地第 doIndex 个 DO
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  doIndex          DO 序号：[0,7]
 * @param enable            输出使能
 * @param highLevelTime     脉冲高电平持续时间, 参数范围：[1,4000]
 * @param lowLevelTime      脉冲低电平持续时间, 参数范围：[1,4000]
 * @param pulseNum          输出脉冲数量
 * @param firstLevel        第一个脉冲电平
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x2206)
 *******************************************************************/
IMC_API IMC_SetLocalDoBitPulse(short cardIndex, short doIndex, short enable, unsigned short highLevelTime, unsigned short lowLevelTime, short pulseNum, short firstLevel);
/// @}

/// @defgroup LocalEnc 板卡端子板编码器操作
/// @brief 板卡端子板编码器操作
/// @{
/**
 * @brief  设置编码器值
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  encIndex         设定的编码器通道索引：[0,2]
 * @param  encPos           设定的编码器值
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x2220)
 *******************************************************************/
IMC_API IMC_SetLocalEncPos(short cardIndex, short encIndex, int encPos);

/**
 * @brief  读取编码器值
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  encIndex         读取的编码器通道：[0,2]
 * @param  pEncPosArray     读取的编码器值数组
 * @param  count            读取的编码器通道数量, N 表示同时读取 N 个编码器通道的值, 读取参数在 pEncPosArray 数组中
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x2221)
 *******************************************************************/
IMC_API IMC_GetLocalEncPos(short cardIndex, short encIndex, int* pEncPosArray, short count = 1);

/**
 * @brief  读取编码器速度
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  encIndex         读取的编码器通道：[0,2]
 * @param  pEncVelArray     读取的编码器速度数组
 * @param  count            读取的编码器通道数量, N 表示同时读取 N 个编码器通道的速度, 读取参数在 pEncVelArray 数组中
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x2222)
 *******************************************************************/
IMC_API IMC_GetLocalEncVel(short cardIndex, short encIndex, int* pEncVelArray, short count = 1);
/// @}

/// @defgroup LocalCapt 板卡端子板捕获功能
/// @brief 板卡端子板捕获功能
/// @{
/**
 * @brief  设置端子板编码器位置捕获模式, 配置捕获参数
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  encIndex         编码器通道号：[0,2]
 * @param  trigType         触发类型：\n 0：index触发 \n 3：外部探针(DI7)触发
 * @param  edge				获取触发沿类型：\n 0：上升沿 \n 1：下降沿 \n 2：双沿触发
 * @retval #EXE_SUCCESS     指令执行成功
 * @par 指令码              (0x2300)
 *******************************************************************/
IMC_API IMC_SetLocalCaptMode(short cardIndex, short encIndex, short trigType, short edge);

/**
 * @brief  获取端子板编码器位置捕获模式配置参数
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  encIndex         编码器通道号：[0,2]
 * @param  pTrigType        触发类型 \n 0：index触发 \n 3：外部探针(DI7)触发
 * @param  pEdge			获取触发沿类型 \n 0：上升沿 \n 1：下降沿 \n 2：双沿触发
 * @retval #EXE_SUCCESS     指令执行成功
 * @par 指令码              (0x2301)
 *******************************************************************/
IMC_API IMC_GetLocalCaptMode(short cardIndex, short encIndex, short* pTrigType, short* pEdge);

/**
 * @brief  获取端子板编码器位置捕获状态
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  encIndex         编码器通道号
 * @param  pSts             触发状态 \n 0：未触发捕获 \n 1：已触发捕获
 * @param  pPosVal          上升沿捕获的位置
 * @param  pNegVal          下降沿捕获的位置
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x2302)
 *******************************************************************/
IMC_API IMC_GetLocalCaptStatus(short cardIndex, short encIndex, short* pSts, int* pPosVal, int* pNegVal);

/**
 * @brief  设置端子板编码器连续位置捕获模式, 配置连续捕获参数
 * @attention  连续捕获模式下配置单次捕获，连续捕获会被打断，重新执行连续捕获可复位
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  encIndex         编码器通道号
 * @param  trigType         触发类型 \n 0：index触发 \n 1：外部信号触发（DI7）
 * @param  edge             获取触发沿类型 \n 0：上升沿 \n 1：下降沿 \n 2：双沿触发
 * @param  captCount        设置捕获的次数：[1,1024] 注:单边沿最大1024，双边沿最大512次
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x2303)
 *******************************************************************/
IMC_API IMC_SetLocalCaptRepeatMode(short cardIndex, short encIndex, short trigType, short edge, short captCount);

/**
 * @brief  设置端子板编码器连续位置捕获模式配置参数
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  encIndex         编码器通道号
 * @param  pTrigType        触发类型 \n 0：index触发 \n 1：外部信号触发（DI7）
 * @param  pEdge            获取触发沿类型 \n 0：上升沿 \n 1：下降沿 \n 2：双沿触发
 * @param  pCaptCount       设置捕获的次数：[1,1024]
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x2304)
 *******************************************************************/
IMC_API IMC_GetLocalCaptRepeatMode(short cardIndex, short encIndex, short* pTrigType, short* pEdge, short* pCaptCount);

/**
 * @brief  获取端子板编码器连续位置捕获状态
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  encIndex         编码器通道号
 * @param  pCaptSts         触发状态 \n 0：连续捕获完成 \n 1：连续探针捕获中
 * @param  pCaptCount       已记录连续捕获位置个数
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x2305)
 *******************************************************************/
IMC_API IMC_GetLocalCaptRepeatStatus(short cardIndex, short encIndex, short* pCaptSts, short* pCaptCount);

/**
 * @brief  获取端子板编码器连续位置捕获位置
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  encIndex         编码器通道号
 * @param  startNo          获取位置起始索引
 * @param  count            获取位置数量, 按照实际设置的捕获次数获取
 * @param  pCaptPosArray    捕获位置的存储数组 (双沿触发时, 上升下降沿捕获位置交替存储)
 * @attention 双沿沿捕获时, 开辟存储数组长度应为 IMC_SetLocalCaptRepeatMode 设置 的 captCount 的 2 倍
 *         \n 因为每次捕获的同时锁存了上升沿和下降沿的位置, 数据存储按(上升沿 / 下降沿)(上升沿 / 下降沿)存放
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x2306)
 *******************************************************************/
IMC_API IMC_GetLocalCaptRepeatPos(short cardIndex, short encIndex, short startNo, short count, int* pCaptPosArray);
/// @}

/// @defgroup LocalPWM 板卡端子板PWM功能
/// @brief 板卡端子板PWM功能
/// @{
/**
 * @brief  设置端子板PWM的输出参数
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  chn              PWM输出通道：[0,2]
 * @param  frequency        输出频率, 参数范围：[1,4000000]单位(Hz)
 * @param  dutyRatio        输出占空比, 参数范围：[0,100]%
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x2400)
 *******************************************************************/
IMC_API IMC_SetLocalPwmPara(short cardIndex, short chn, int frequency, double dutyRatio);

/**
 * @brief  获取端子板PWM的输出参数
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  chn              PWM输出通道：[0,2]
 * @param  pFrequency       输出频率, 参数范围：[1,4000000]单位(Hz)
 * @param  pDutyRatio       输出占空比, 参数范围：[0,100]%
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x2401)
 *******************************************************************/
IMC_API IMC_GetLocalPwmPara(short cardIndex, short chn, int* pFrequency, double* pDutyRatio);

/**
 * @brief  设置端子板PWM的输出频率
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  chn              PWM输出通道：[0,2]
 * @param  frequency        输出频率, 参数范围：[1,4000000]单位(Hz)
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x2402)
 *******************************************************************/
IMC_API IMC_SetLocalPwmFrq(short cardIndex, short chn, int frequency);

/**
 * @brief  设置端子板PWM的输出占空比
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  chn              PWM输出通道：[0,2]
 * @param  dutyRatio        输出占空比, 参数范围：[0,100]%
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x2403)
 *******************************************************************/
IMC_API IMC_SetLocalPwmDuty(short cardIndex, short chn, double dutyRatio);

/**
 * @brief  设置端子板PWM输出的开关及延时时间
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  chn              PWM输出通道：[0,2]
 * @param  onOff            开关：0 关 1 开
 * @param  delay            操作延时时间, 参数范围：[0,uintMax]单位(μs)
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x2404)
 *******************************************************************/
IMC_API IMC_SetLocalPwmOnOffDelay(short cardIndex, short chn, short onOff, unsigned short delay);
/// @}

/// @defgroup LocalCompare 板卡端子板位置比较功能
/// @brief 板卡端子板位置比较功能
/// @{
/**
 * @brief  设置端子板位置比较输出源的配置
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  chn              设置的比较输出通道号：[0,3]
 * @param  compSrc          比较源的编码器通道
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x2501)
 *******************************************************************/
IMC_API IMC_SetLocalCmpSrcCfg(short cardIndex, short chn, short compSrc);

/**
 * @brief  设置端子板比较物理信号输出的配置
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  chn              设置的比较输出通道号：[0,3]
 * @param  ctrlMode         输出控制模式：0 手动输出 1 比较输出
 * @param  stLevel          电平输出是否取反(0 不取反 1 取反)
 * @param  outputType       比较输出的类型：0 输出脉冲 1 输出电平
 * @param  pulseWidth       输出脉冲的宽度：[1,uintMax]单位μs（仅在输出类型为脉冲式有效）
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x2502)
 *******************************************************************/
IMC_API IMC_SetLocalCmpOutputCfg(short cardIndex, short chn, short ctrlMode, short stLevel, short outputType, unsigned short pulseWidth);

/**
 * @brief  设置端子板比较数据的类型
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  chn              设置的比较输出通道号：[0,3]
 * @param  type             比较数据类型：\n 0：静态获取数据类型比较，即启动比较后不可继续压入比较数据
 *                                        \n 1：动态获取数据类型比较，即启动比较后还可继续压入比较数据
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x2503)
 *******************************************************************/
IMC_API IMC_SetLocalCmpDataType(short cardIndex, short chn, short type);

/**
 * @brief  获取端子板比较数据的类型
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  chn              设置的比较输出通道号：[0,3]
 * @param  pType            比较数据类型：\n 0：静态获取数据类型比较，即启动比较后不可继续压入比较数据
 *                                        \n 1：动态获取数据类型比较，即启动比较后还可继续压入比较数据
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x2504)
 *******************************************************************/
IMC_API IMC_GetLocalCmpDataType(short cardIndex, short chn, short* pType);

/**
 * @brief  设置端子板位置比较的位置类型
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  chn              设置的比较输出通道号：[0,3]
 * @param  type             位置类型：0 相对位置比较  1 绝对位置比较
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x2505)
 *******************************************************************/
IMC_API IMC_SetLocalCmpPosType(short cardIndex, short chn, short type);

/**
 * @brief  获取端子板位置比较的位置类型
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  chn              设置的比较输出通道号：[0,3]
 * @param  pType            位置类型：0 相对位置比较  1 绝对位置比较
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x2506)
 *******************************************************************/
IMC_API IMC_GetLocalCmpPosType(short cardIndex, short chn, short* pType);

/**
 * @brief  设置本地输出 EX-O bit0 ~ 3 位位置比较输出类型
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  doIndex          序号：[0,3]
 *                          \n 0：指DO0(Y0)输出通道编号
 *                          \n 1：指DO1(Y1)输出通道编号
 *                          \n 2：指DO2(Y2)输出通道编号
 *                          \n 3：指DO3(Y3)输出通道编号
 * @param  type             输出类型
 *                          \n 0：通用DO (默认值) 输出类型
 *                          \n 1：位置比较输出类型
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x2507)
 *******************************************************************/
IMC_API IMC_SetLocalGpoType(short cardIndex, short doIndex, short type);

/**
 * @brief  获取本地输出 EX-O bit0 ~ 3 位位置比较输出类型配置
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  doIndex          序号：[0,3]
 *                          \n 0：指DO0(Y0)输出通道编号
 *                          \n 1：指DO1(Y1)输出通道编号
 *                          \n 2：指DO2(Y2)输出通道编号
 *                          \n 3：指DO3(Y3)输出通道编号
 * @param  pType             输出类型
 *                          \n 0：通用DO (默认值) 输出类型
 *                          \n 1：位置比较输出类型
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x2508)
 *******************************************************************/
IMC_API IMC_GetLocalGpoType(short cardIndex, short doIndex, short* pType);

/**
 * @brief  获取端子板位置比较的状态
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  chn              设置的比较输出通道号：[0,3]
 * @param  pCmpSts          位置比较的运行状态,不同位代表不同意义
 *                          \n bit0位置比较输出状态标志 0：比较触发完成 1：比较触发进行中
 *                          \n bit1比较数据通道错误标志 0：无错误 1：数据通道错误
 *                          \n bit2位置比较数据错误标志 0：无数据错误 1：比较位置已过
 *                          \n bit3位置比较过点错误标志 0：无过点错误 1：信号覆盖，发生过点错误
 * @param  pCmpCount        比较输出的次数
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x2509)
 *******************************************************************/
IMC_API IMC_GetLocalCmpSts(short cardIndex, short chn, short* pCmpSts, unsigned int* pCmpCount);

/**
 * @brief  设置端子板手动输出单个脉冲或电平
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  chn              设置的比较输出通道号：[0,3]
 * @param  outval           手动输出值：
 *                          \n 脉冲输出模式：0 无输出  1 输出一个脉冲
 *                          \n 电平输出模式：0 输出低电平 1 输出高电平
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x2513)
 *******************************************************************/
IMC_API IMC_SetLocalCmpManualOut(short cardIndex, short chn, short outval);

/**
 * @brief  设置端子板手动输出多脉冲
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  chn              设置的比较输出通道号：[0,3]
 * @param  pulseNum         输出脉冲次数 [1,uintMax]
 * @param  pulseCycle       输出脉冲周期 [0,uintMax]单位μs (脉冲周期需大于设定的脉冲宽度)
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x2514)
 *******************************************************************/
IMC_API IMC_SetLocalCmpManualMultiOut(short cardIndex, short chn, unsigned int pulseNum, unsigned short pulseCycle);

/**
 * @brief  设置端子板等间距线性比较输出
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  chn              设置的比较输出通道号：[0,3]
 * @param  intrvalLen       等距输出间隔, 单位：pulse
 * @param  cmpCount			等距比较输出的次数
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x2515)
 *******************************************************************/
IMC_API IMC_SetLocalCmpLinearOut(short cardIndex, short chn, int intrvalLen, unsigned int cmpCount);

/**
 * @brief  停止端子板比较输出
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  chn              设置的比较输出通道号：[0,3]
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x2510)
 *******************************************************************/
IMC_API IMC_StopLocalCmpOut(short cardIndex, short chn);

/**
 * @brief  启动端子板比较输出
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  chn              设置的比较输出通道号：[0,3]
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x2511)
 *******************************************************************/
IMC_API IMC_StartLocalCmpOut(short cardIndex, short chn);

/**
 * @brief  设置端子板一维位置比较输出数据
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  chn              设置的比较输出通道号：[0,3]
 * @param  compCount        一次压入的一维位置比较数据数量,( 注：单次传输的个数最大 50, 若比较个数大于 50, 则需要分批进行数据的压入 )
 * @param  pPosBufArray     比较位置数据数组
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x2512)
 *******************************************************************/
IMC_API IMC_SetLocalCmpData(short cardIndex, short chn, short compCount, int* pPosBufArray);

/**
 * @brief  设置端子板比较物理信号输出的配置
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  chn              设置的比较输出通道号：[0,3]
 * @param  ctrlMode         输出控制模式：0 手动输出 1 比较输出
 * @param  stLevel          电平输出是否取反(0 不取反 1 取反)
 * @param  outputType       比较输出的类型：0 输出脉冲 1 输出电平
 * @param  pulseWidth       输出脉冲的宽度：[1,uintMax]单位（仅在输出类型为脉冲式有效）
 * @param  pulseUnit        输出脉冲的单位：0-1us 1-10us 2-100us 3-1ms 4-10ms 5-100ms
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x250a)
 *******************************************************************/
IMC_API IMC_SetLocalCmpOutputCfgEx(short cardIndex, short chn, short ctrlMode, short stLevel, short outputType, unsigned short pulseWidth, short pulseUnit);

/// @}

/// @defgroup LocalMultCompare 板卡端子板多维位置比较功能
/// @brief 板卡端子板多维位置比较功能
/// @{
/**
 * @brief  设置端子板位置比较输出源的配置
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  chn              设置的比较输出通道号：0 为多维比较输出
 * @param  dimension        比较的维数, 参数范围 [2,3], 1表示一维比较, N表示多维位置比较
 * @param  pCompSrcArray    比较源的编码器通道数组
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x2530)
 *******************************************************************/
IMC_API IMC_SetLocalMultiCmpSrcCfg(short cardIndex, short chn, short dimension, short* pCompSrcArray);

/**
 * @brief  设置端子板比较物理信号输出的配置
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  chn              设置的比较输出通道号：0 为多维比较输出
 * @param  stLevel          电平输出是否取反(0 不取反 1 取反)
 * @param  outputType       比较输出的类型：0 输出脉冲 1 输出电平
 * @param  pulseWidth       输出脉冲的宽度：[1,uintMax]单位μs（仅在输出类型为脉冲式有效）
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x2531)
 *******************************************************************/
IMC_API IMC_SetLocalMultiCmpOutputCfg(short cardIndex, short chn, short stLevel, short outputType, unsigned short pulseWidth);

/**
 * @brief  获取端子板位置比较的状态
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  chn              设置的比较输出通道号：0 为多维比较输出
 * @param  pCmpSts          位置比较的运行状态,不同位代表不同意义
 *                          \n bit0位置比较输出状态标志 0：比较触发完成 1：比较触发进行中
 *                          \n bit1比较数据通道错误标志 0：无错误 1：数据通道错误
 *                          \n bit2位置比较数据错误标志 0：无数据错误 1：比较位置已过
 *                          \n bit3位置比较过点错误标志 0：无过点错误 1：信号覆盖，发生过点错误
 * @param  pCmpCount        比较输出的次数
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x2532)
 *******************************************************************/
IMC_API IMC_GetLocalMultiCmpSts(short cardIndex, short chn, short* pCmpSts, unsigned int* pCmpCount);

/**
 * @brief  设置端子板比较数据的类型
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  chn              设置的比较输出通道号：0 为多维比较输出
 * @param  type             比较数据类型：\n 0：静态获取数据类型比较，即启动比较后不可继续压入比较数据
 *                                        \n 1：动态获取数据类型比较，即启动比较后还可继续压入比较数据
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x2533)
 *******************************************************************/
IMC_API IMC_SetLocalMultiCmpDataType(short cardIndex, short chn, short type);

/**
 * @brief  获取端子板比较数据的类型
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  chn              设置的比较输出通道号：0 为多维比较输出
 * @param  pType            比较数据类型：\n 0：静态获取数据类型比较，即启动比较后不可继续压入比较数据
 *                                        \n 1：动态获取数据类型比较，即启动比较后还可继续压入比较数据
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x2534)
 *******************************************************************/
IMC_API IMC_GetLocalMultiCmpDataType(short cardIndex, short chn, short* pType);

/**
 * @brief  设置端子板多维位置比较参数
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  chn              设置的比较输出通道号：0 为多维比较输出
 * @param  syncDeltaPos     比较位置容差范围, 单位pulse
 * @param  outPinType       映射输出的端口类型
 *                          \n 0：无映射
 *                          \n 1：PWM0端口
 *                          \n 2：CMP0一维比较端口
 *                          \n 3：DO4(Y4)输出端口
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x2540)
 *******************************************************************/
IMC_API IMC_SetLocalMultiCmpPara(short cardIndex, short chn, unsigned short syncDeltaPos, short outPinType);

/**
 * @brief  获取端子板多维位置比较参数
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  chn              设置的比较输出通道号：0 为多维比较输出
 * @param  pSyncDeltaPos    比较位置容差范围, 单位pulse
 * @param  pOutPinType      获取的输出的端口类型，默认为0（无映射）
 *                          \n 0：无映射
 *                          \n 1：PWM0端口
 *                          \n 2：CMP0一维比较端口
 *                          \n 3：DO4(Y4)输出端口
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x2541)
 *******************************************************************/
IMC_API IMC_GetLocalMultiCmpPara(short cardIndex, short chn, unsigned short* pSyncDeltaPos, short* pOutPinType);

/**
 * @brief  设置多维比较位置点
 * @param  cardIndex         板卡卡号, 参数范围：[0,3]
 * @param  chn              设置的比较输出通道号：0 为多维比较输出
 * @param  pComparaDataArray 多维位置比较点, 数据类型详见\ref TMultiCmpData "多维位置比较数据结构体"
 * @param  count             一次压入的多维位置比较数据数量, ( 注：单次传输的个数最大 20, 若比较个数大于20, 则分批进行数据的压入)
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x2542)
 *******************************************************************/
IMC_API IMC_SetLocalMultiCmpData(short cardIndex, short chn, TMultiCmpData* pComparaDataArray, short count);

/**
 * @brief  启动端子板多维位置比较输出
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  chn              设置的比较输出通道号：0 为多维比较输出
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x2536)
 *******************************************************************/
IMC_API IMC_StartLocalMultiCmpOut(short cardIndex, short chn);

/**
 * @brief  停止端子板多维位置比较输出
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  chn              设置的比较输出通道号：0 为多维比较输出
 *                            \n 注：停止多维比较会释放多维比较占用的输出端口类型（默认值0）
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x2535)
 *******************************************************************/
IMC_API IMC_StopLocalMultiCmpOut(short cardIndex, short chn);

/**
 * @brief  设置端子板比较物理信号输出的配置
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  chn              设置的比较输出通道号：0 为多维比较输出
 * @param  stLevel          电平输出是否取反(0 不取反 1 取反)
 * @param  outputType       比较输出的类型：0 输出脉冲 1 输出电平
 * @param  pulseWidth       输出脉冲的宽度
 * @param  pulseUnit        输出脉冲的单位 0-1us 1-10us 2-100us 3-1ms 4-10ms 5-100ms
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x2531)
 *******************************************************************/
IMC_API IMC_SetLocalMultiCmpOutputCfgEx(short cardIndex, short chn, short stLevel, short outputType, unsigned short pulseWidth, short pulseUnit);

/// @}

/// @defgroup LocalPSO 板卡端子板PSO功能
/// @brief 板卡端子板PSO功能
/// @{
/**
 * @brief  设置端子板PSO功能参数
 * @param  cardIndex         板卡卡号, 参数范围：[0,3]
 * @param  chn               PSO输出通道号：1为PSO输出
 * @param  dimension         维数：[1,3]
 * @param  pinType           输出端口类型
 *                          \n 0: 无映射
 *                          \n 1：PWM1端口
 *                          \n 2：CMP1一维比较端口
 *                          \n 3：DO5(Y5)输出端口
 * @param  pPsoPosIndexArray 编码器位置源通道号数组, 参数范围：[0,2], 数组大小根据dimension决定
 * @param  outPlsWidth       PSO输出的脉冲宽度, 参数范围：[1,uintMax]单位(由基频决定，默认us)
 * @param  syncDeltaPos      PSO输出间隔(合成)距离, 参数范围：[1,2147483647] 单位(pulse)
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x2600)
 *******************************************************************/
IMC_API IMC_SetLocalPSOPara(short cardIndex, short chn, short dimension, short pinType, short* pPsoPosIndexArray, unsigned short outPlsWidth, int syncDeltaPos);

/**
 * @brief  获取端子板PSO功能参数配置
 * @param  cardIndex         板卡卡号, 参数范围：[0,3]
 * @param  chn               PSO输出通道号：1为PSO输出
 * @param  pDimension         维数：[1,3]
 * @param  pPinType           输出端口映射
 *                          \n 0: 无映射
 *                          \n 1：PWM1端口
 *                          \n 2：CMP1一维比较端口
 *                          \n 3：DO5(Y5)输出端口
 * @param  pPsoPosIndexArray 编码器位置源通道号数组, 参数范围：[0,2], 数组大小根据dimension决定
 * @param  pOutPlsWidth      PSO输出的脉冲宽度, 参数范围：[1,uintMax]单位(由基频决定，默认us)
 * @param  pSyncDeltaPos     PSO输出间隔(合成)距离, [1,2147483647] 单位(pulse)
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x2601)
 *******************************************************************/
IMC_API IMC_GetLocalPSOPara(short cardIndex, short chn, short* pDimension, short* pPinType, short* pPsoPosIndexArray, unsigned short* pOutPlsWidth, int* pSyncDeltaPos);

/**
 * @brief  设置PSO基频
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  chn               PSO输出通道号：1为PSO输出
 * @param  baseFrq          设置PSO通道基频, 参数范围：[0,5] \n 0:1us \n 1:10us \n 2:100us \n 3:1ms \n 4:10ms \n 5:100ms
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x2605)
 *******************************************************************/
IMC_API IMC_SetLocalPSOBaseFrq(short cardIndex, short chn, short baseFrq);

/**
 * @brief  获取PSO基频
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  chn               PSO输出通道号：1为PSO输出
 * @param  pBaseFrq         设置PSO通道基频, 参数范围：[0,5] \n 0:1us \n 1:10us \n 2:100us \n 3:1ms \n 4:10ms \n 5:100ms
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x2606)
 *******************************************************************/
IMC_API IMC_GetLocalPSOBaseFrq(short cardIndex, short chn, short* pBaseFrq);

/**
 * @brief  启动PSO输出
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  chn               PSO输出通道号：1为PSO输出
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x2602)
 *******************************************************************/
IMC_API IMC_StartLocalPSO(short cardIndex, short chn);

/**
 * @brief  停止PSO输出
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  chn               PSO输出通道号：1为PSO输出
 *                          \n 注：停止PSO会释放占用的输出端口类型（默认值0）
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x2603)
 *******************************************************************/
IMC_API IMC_StopLocalPSO(short cardIndex, short chn);

/**
 * @brief  获取PSO输出状态
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  chn               PSO输出通道号：1为PSO输出
 * @param  pSts			    PSO运行状态：0-停止 1-进行中
 * @param  pPsoCount        PSO已输出次数
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x2604)
 *******************************************************************/
IMC_API IMC_GetLocalPSOSts(short cardIndex, short chn, short* pSts, unsigned int* pPsoCount);

/**
 * @brief  设置PSO输出窗口配置
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  chn              PSO输出通道号：1为PSO输出
 * @param  enable			PSO窗口使能模式：0-通用模式 1-起点模式（以起点位置开始输出）2-窗口模式（窗口内正常输出）
 * @param  startPos         PSO 窗口起点位置
 * @param  endPos           PSO 窗口结束位置
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x2607)
 *******************************************************************/
IMC_API IMC_SetLocalPSOWindow(short cardIndex, short chn, short enable, int startPos, int endPos);

/**
 * @brief  获取PSO输出窗口配置
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  chn              PSO输出通道号：1为PSO输出
 * @param  pEnable			PSO窗口使能模式：0-通用模式 1-起点模式（以起点位置开始输出）2-窗口模式（窗口内正常输出）
 * @param  pStartPos        PSO 窗口起点位置
 * @param  pEndPos          PSO 窗口结束位置
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x2608)
 *******************************************************************/
IMC_API IMC_GetLocalPSOWindow(short cardIndex, short chn, short* pEnable, int* pStartPos, int* pEndPos);
/// @}
/// @}

/// @defgroup Axis 板卡单轴
/// @brief 板卡轴操作相关函数
/// @{
/// @defgroup AxisDef 板卡轴相关宏定义
/// @{
/// @defgroup AxDiStopTypeDef 板卡轴停止类型
/// @defgroup AxStsBitDef 板卡轴状态字定义
/// @defgroup AxStopReasonDef 板卡轴停止原因定义
/// @}

/// @defgroup AxisConfig 板卡轴安全配置
/// @brief 板卡轴安全配置
/// @{

/**
 * @brief  设置控制器单轴安全运行参数
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  pMtPara          轴运行参数, 具体信息请参考\ref TMtPara "单轴安全运行参数"
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x3002)
 *******************************************************************/
IMC_API IMC_SetAxMaxMtPara(short cardIndex, short axNo, TMtPara* pMtPara);

/**
 * @brief  读取控制器单轴安全运行参数
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  pMtPara          轴运行参数, 具体信息请参考\ref TMtPara "单轴安全运行参数"
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x3003)
 *******************************************************************/
IMC_API IMC_GetAxMaxMtPara(short cardIndex, short axNo, TMtPara* pMtPara);

/**
 * @brief  设置轴当量参数
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  pAxEquArray      轴当量参数, 默认值为1, 可以根据count值一次设置多个轴
 * @param  count            设置的轴数量, 按照实际资源设置, 参数范围：[1,32]
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x3004)
 *******************************************************************/
IMC_API IMC_SetAxEquiv(short cardIndex, short axNo, double* pAxEquArray, short count = 1);

/**
 * @brief  获取轴当量参数
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  pAxEquArray      轴当量参数, 默认值为1, 可以根据count值一次获取多个轴
 * @param  count            获取的轴数量, 按照实际资源获取, 参数范围：[1,32]
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x3005)
 *******************************************************************/
IMC_API IMC_GetAxEquiv(short cardIndex, short axNo, double* pAxEquArray, short count = 1);

/**
 * @brief  设置轴输出绑定
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  axType           轴类型
 *                          \n 1：EtherCAT轴
 *                          \n -1：虚轴, 表示输出未绑定
 * @param  outputChn        输出通道：对于EtherCAT轴, 通道范围：[0,31]
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x3006)
 *******************************************************************/
IMC_API IMC_SetAxBondCfg(short cardIndex, short axNo, short axType, short outputChn);

/**
 * @brief  获取轴输出绑定
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  pAxType          轴类型
 *                          \n 1：EtherCAT轴
 *                          \n -1：虚轴, 表示输出未绑定
 * @param  pOutputChn       输出通道：对于EtherCAT轴, 通道范围：[0,31]
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x3007)
 *******************************************************************/
IMC_API IMC_GetAxBondCfg(short cardIndex, short axNo, short* pAxType, short* pOutputChn);

/**
 * @brief  复位轴绑定状态, 让所有轴恢复到虚轴状态
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x3008)
 *******************************************************************/
IMC_API IMC_ResetAxBondCfg(short cardIndex);

/**
 * @brief  设置轴属性参数, 包含软限位、到位误差、最大跟随误差
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  pAxAttriPara     轴属性配置, 具体信息请参考\ref TAxAttriPara "轴属性配置结构体"
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x3009)
 *******************************************************************/
IMC_API IMC_SetAxAttriPara(short cardIndex, short axNo, TAxAttriPara* pAxAttriPara);

/**
 * @brief  获取轴属性参数, 包含软限位、到位误差、最大跟随误差
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  pAxAttriPara     轴属性配置, 具体信息请参考\ref TAxAttriPara "轴属性配置结构体"
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x300a)
 *******************************************************************/
IMC_API IMC_GetAxAttriPara(short cardIndex, short axNo, TAxAttriPara* pAxAttriPara);

/**
 * @brief  设置轴软限位
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  positive         正软限位值, 单位(unit)
 * @param  negative         负软限位值, 单位(unit)
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x300b)
 *******************************************************************/
IMC_API IMC_SetAxSoftLimit(short cardIndex, short axNo, int positive, int negative);

/**
 * @brief  获取轴软限位
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  pPositive        正软限位值, 单位(unit)
 * @param  pNegative        负软限位值, 单位(unit)
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x300c)
 *******************************************************************/
IMC_API IMC_GetAxSoftLimit(short cardIndex, short axNo, int* pPositive, int* pNegative);

/**
 * @brief  设置轴到位检查参数
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  arrivalBand      到位误差带, 参数范围：[0, intMax]单位(unit)
 * @param  arrivalTime      到位保持时间, 参数范围：[0, uintMax]单位(周期)
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x300d)
 *******************************************************************/
IMC_API IMC_SetAxArrivalBand(short cardIndex, short axNo, short arrivalBand, unsigned short arrivalTime);

/**
 * @brief  获取轴到位检查参数
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  pArrivalBand     到位误差带, 参数范围：[0, intMax]单位(unit)
 * @param  pArrivalTime     到位保持时间, 参数范围：[0, uintMax]单位(周期)
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x300e)
 *******************************************************************/
IMC_API IMC_GetAxArrivalBand(short cardIndex, short axNo, short* pArrivalBand, unsigned short* pArrivalTime);

/**
 * @brief  设置轴最大跟随误差
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  errorPos         最大跟随误差, 单位(unit)
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x300f)
 *******************************************************************/
IMC_API IMC_SetAxErrorPos(short cardIndex, short axNo, int errorPos);

/**
 * @brief  获取轴最大跟随误差
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  pErrorPos        最大跟随误差, 单位(unit)
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x3010)
 *******************************************************************/
IMC_API IMC_GetAxErrorPos(short cardIndex, short axNo, int* pErrorPos);

/**
 * @brief  设置轴安全检查使能综合配置
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  pAxCheckEn       轴安全检查配置, 具体信息请参考\ref TAxCheckEn "轴安全检查配置结构体"
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x3013)
 *******************************************************************/
IMC_API IMC_SetAxSafeCheck(short cardIndex, short axNo, TAxCheckEn* pAxCheckEn);

/**
 * @brief  获取轴安全检查使能综合配置
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  pAxCheckEn       轴安全检查配置, 具体信息请参考\ref TAxCheckEn "轴安全检查配置结构体"
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x3014)
 *******************************************************************/
IMC_API IMC_GetAxSafeCheck(short cardIndex, short axNo, TAxCheckEn* pAxCheckEn);

/**
 * @brief  设置轴报警检查使能配置
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  enable           使能状态
 * @param  count            设置的轴数量, 按照实际资源设置, 参数范围：[1,32]
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x3015)
 *******************************************************************/
IMC_API IMC_SetAxAlarmCheck(short cardIndex, short axNo, short enable, short count = 1);

/**
 * @brief  设置轴软限位检查使能配置
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  enable           使能状态
 * @param  count            设置的轴数量, 按照实际资源设置, 参数范围：[1,32]
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x3016)
 *******************************************************************/
IMC_API IMC_SetAxSoftLmtsCheck(short cardIndex, short axNo, short enable, short count = 1);

/**
 * @brief  设置轴硬限位检查使能配置
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  enable           使能状态
 * @param  count            设置的轴数量, 按照实际资源设置, 参数范围：[1,32]
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x3017)
 *******************************************************************/
IMC_API IMC_SetHwLmtsCheck(short cardIndex, short axNo, short enable, short count = 1);

/**
 * @brief  设置轴跟随误差位检查使能配置
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  enable           使能状态
 * @param  count            设置的轴数量, 按照实际资源设置, 参数范围：[1,32]
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x3018)
 *******************************************************************/
IMC_API IMC_SetAxErrPosCheck(short cardIndex, short axNo, short enable, short count = 1);

/**
 * @brief  设定轴停止的平滑停止减速度和急停减速度
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  decSmoothStop    设定的平滑停止减速度, 参数范围：(0,maxDec]单位(unit/s^2)
 * @param  decAbruptStop    设定的急停减速度, 参数范围：(0,maxDec]单位(unit/s^2)
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x3019)
 *******************************************************************/
IMC_API IMC_SetAxStopDec(short cardIndex, short axNo, double decSmoothStop, double decAbruptStop);

/**
 * @brief  获取轴停止的平滑停止减速度和急停减速度
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  pDecSmoothStop   获取的平滑停止减速度, 参数范围：(0,maxDec]单位(unit/s^2)
 * @param  pDecAbruptStop   获取的急停减速度, 参数范围：(0,maxDec]单位(unit/s^2)
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x301a)
 *******************************************************************/
IMC_API IMC_GetAxStopDec(short cardIndex, short axNo, double* pDecSmoothStop, double* pDecAbruptStop);

/**
 * @brief  设定轴急停减速限制时间
 * @details 急停时根据当前速度和设定的急停减速度计算停止时间, 若超过设定的限制时间, 则按照当前限制时间重新计算减速度, 否则按照设定的减速度执行
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  decLmtTime       设定的急停减速限制时间, 取值范围：[0, 1000]单位(ms), 默认值为100ms
 *                          \n 设定为0时, 不做加速度限制, 将按照急停加速度急停, 不建议用户取消限制
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x301b)
 *******************************************************************/
IMC_API IMC_SetAxEmgMaxDecLmt(short cardIndex, short axNo, unsigned short decLmtTime);

/**
 * @brief  获取轴急停减速限制时间
 * @details 急停时根据当前速度和设定的急停减速度计算停止时间, 若超过设定的限制时间, 则按照当前限制时间重新计算加速度, 否则按照设定的加速度执行
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  pDecLmtTime      获取的急停减速限制时间, 取值范围：[0, 1000]单位(ms), 默认值为100ms
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x301c)
 *******************************************************************/
IMC_API IMC_GetAxEmgMaxDecLmt(short cardIndex, short axNo, unsigned short* pDecLmtTime);

/**
 * @brief  设置轴的结束速度
 * @details 点位运动时，运动规划结束速度可以不为0，注意合理设置结束速度值，运动规划完成后，结束速度直接突跳速度为0，可能引起机台抖动
 * @param  cardIndex 板卡卡号, 参数范围：[0,3]
 * @param  axNo 操作轴号, 参数范围：[0,63]
 * @param  endVel 设定的结束速度值，单位pluse/s
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x301d)
 *******************************************************************/
IMC_API IMC_SetAxEndVel(short cardIndex, short axNo, double endVel);

/**
 * @brief  获取轴的结束速度设定
 * @param  cardIndex 板卡卡号, 参数范围：[0,3]
 * @param  axNo 操作轴号, 参数范围：[0,63]
 * @param  pEndVel 获取的结束速度值，单位pluse/s
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x301e)
 *******************************************************************/
IMC_API IMC_GetAxEndVel(short cardIndex, short axNo, double* pEndVel);
/// @}

/// @defgroup AxisStatus 板卡轴状态监控
/// @brief 轴状态监控
/// @{
/**
 * @brief  获取轴规划模式, 该值中包含单轴模式和多轴模式的信息
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  pPrfModeArray    轴规划模式数组, 规划模式定义
 *                          \n 0：未处于任何规划模式
 *                          \n 1：点对点规划模式
 *                          \n 2：Jog规划模式
 *                          \n 3：电子齿轮规划模式
 *                          \n 4：电子凸轮规划模式
 *                          \n 5：PVT规划模式
 *                          \n 6：龙门规划模式
 *                          \n 7：手轮规划模式
 *                          \n 9：点位连续规划模式
 *                          \n 11：插补同步轴规划模式
 *                          \n 15：回零模式
 *                          \n 17：坐标系插补规划模式(多轴模式)
 *                          \n 18：多轴同步规划模式(多轴模式)
 *                          \n 19：绑定PT规划模式(多轴模式)
 * @param  count            读取的轴数量, 按照实际资源获取, 参数范围：[1,32]
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x3100)
 *******************************************************************/
IMC_API IMC_GetAxPrfMode(short cardIndex, short axNo, short* pPrfModeArray, short count = 1);

/**
 * @brief  获取轴状态
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  pAxStsArray      获取的轴状态, 按bit解析, 各bit的定义见\ref AxStsBitDef "轴状态位定义"
 * @param  count            读取的轴数量, 按照实际资源获取, 参数范围：[1,32]
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x3101)
 *******************************************************************/
IMC_API IMC_GetAxSts(short cardIndex, short axNo, int* pAxStsArray, short count = 1);

/**
 * @brief  清除轴的错误状态, 当发生报警时, 需要调用该指令进行清除, 但如果实际物理报警还在, 则无法清除
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  count            清除的轴数量, 按照实际资源设置, 参数范围：[1,32]
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x3102)
 *******************************************************************/
IMC_API IMC_ClrAxSts(short cardIndex, short axNo, short count = 1);

/**
 * @brief  获取轴规划位置
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  pPrfPosArray     获取轴规划位置数组, 单位(unit), 根据count值定义数组的大小
 * @param  count            读取的轴数量, 按照实际资源获取, 参数范围：[1,32]
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x3103)
 *******************************************************************/
IMC_API IMC_GetAxPrfPos(short cardIndex, short axNo, double* pPrfPosArray, short count = 1);

/**
 * @brief  获取轴规划速度
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  pPrfVelArray     获取轴规划速度数组, 单位(unit/s), 根据count值定义数组的大小
 * @param  count            读取的轴数量, 按照实际资源获取, 参数范围：[1,32]
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x3104)
 *******************************************************************/
IMC_API IMC_GetAxPrfVel(short cardIndex, short axNo, double* pPrfVelArray, short count = 1);

/**
 * @brief  获取轴规划加速度
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  pPrfAccArray     获取轴规划加速度数组, 单位(unit/s^2), 根据count值定义数组的大小
 * @param  count            读取的轴数量, 按照实际资源获取, 参数范围：[1,32]
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x3105)
 *******************************************************************/
IMC_API IMC_GetAxPrfAcc(short cardIndex, short axNo, double* pPrfAccArray, short count = 1);

/**
 * @brief  获取轴编码器反馈位置
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  pEncPosArray     获取轴编码器反馈位置数组, 单位(pulse), 根据count值定义数组的大小
 * @param  count            读取的轴数量, 按照实际资源获取, 参数范围：[1,32]
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x3106)
 *******************************************************************/
IMC_API IMC_GetAxEncPos(short cardIndex, short axNo, double* pEncPosArray, short count = 1);

/**
 * @brief  获取轴编码器反馈速度
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  pEncVelArray     获取轴编码器反馈速度数组, 单位(pulse/s), 根据count值定义数组的大小
 * @param  count            读取的轴数量, 按照实际资源获取, 参数范围：[1,32]
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x3107)
 *******************************************************************/
IMC_API IMC_GetAxEncVel(short cardIndex, short axNo, double* pEncVelArray, short count = 1);

/**
 * @brief  获取轴编码器反馈加速度
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  pEncAccArray     获取轴编码器反馈加速度数组, 单位(pulse/s^2), 根据count值定义数组的大小
 * @param  count            读取的轴数量, 按照实际资源获取, 参数范围：[1,32]
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x3108)
 *******************************************************************/
IMC_API IMC_GetAxEncAcc(short cardIndex, short axNo, double* pEncAccArray, short count = 1);

/**
 * @brief  获取指定轴是否均已到位的状态
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axMask           获取的轴, bit位对应轴号：bit0 ~ 31 分别对应轴顺序
 * @param  pSts             获取的轴到位状态, 当所有轴到位后pSts等于1
 * @param  groupNo          获取的组数, 0表示1~31轴 1表示32~64轴
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x3110)
 *******************************************************************/
IMC_API IMC_GetMultiAxArrivalSts(short cardIndex, unsigned int axMask, short* pSts, short groupNo);

/**
 * @brief  获取轴规划位置(32位整形指令)
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  pPrfPosArray     获取轴规划位置数组, 单位(unit), 根据count值定义数组的大小
 * @param  count            读取的轴数量, 按照实际资源获取, 参数范围：[1,32]
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x3120)
 *******************************************************************/
IMC_API IMC_GetAxPrfPos32(short cardIndex, short axNo, int* pPrfPosArray, short count = 1);

/**
 * @brief  获取轴编码器反馈位置(32位整形指令)
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  pEncPosArray     获取轴编码器反馈位置数组, 单位(pulse), 根据count值定义数组的大
 * @param  count            读取的轴数量, 按照实际资源获取, 参数范围：[1,32]
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x3121)
 *******************************************************************/
IMC_API IMC_GetAxEncPos32(short cardIndex, short axNo, int* pEncPosArray, short count = 1);

/**
 * @brief  获取轴规划位置(脉冲位置指令)
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  pPrfPosArray     获取轴规划位置数组, 单位(pluse), 根据count值定义数组的大小
 * @param  count            读取的轴数量, 按照实际资源获取, 参数范围：[1,32]
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x3122)
 *******************************************************************/
IMC_API IMC_GetPrfPos(short cardIndex, short axNo, double* pPrfPosArray, short count = 1);

/**
 * @brief  获取轴编码器原点位置
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  pOrgEncPosArray  获取轴编码器原点位置数组, 单位(pulse), 根据count值定义数组的大
 * @param  count            读取的轴数量, 按照实际资源获取, 参数范围：[1,32]
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x3123)
 *******************************************************************/
IMC_API IMC_GetAxOrgEncPos(short cardIndex, short axNo, int* pOrgEncPosArray, short count = 1);

/**
 * @brief  获取轴停止原因
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  pAxStopReason    获取轴停止原因数组, 根据count值定义数组的大小, 停止原因的定义见\ref AxStopReasonDef "板卡轴停止原因定义"
 * @param  count            读取的轴数量, 按照实际资源获取, 参数范围：[1,32]
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x3112)
 *******************************************************************/
IMC_API IMC_GetAxStopReason(short cardIndex, short axNo, short* pAxStopReason, short count = 1);

/**
 * @brief  获取轴状态(Ex指令)
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  pAxStsArray           获取的轴状态, 按bit解析, 各bit的定义见\ref AxStsBitDef "轴状态位定义"
 * @param  count            读取的轴数量, 按照实际资源获取, 参数范围：[1,32]
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x3130)
 *******************************************************************/
IMC_API IMC_GetAxStsEx(short cardIndex, short axNo, int* pAxStsArray, short count = 1);

/**
 * @brief  获取轴规划位置(Ex指令)
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  pPrfPosArray     获取轴规划位置数组, 单位(unit), 根据count值定义数组的大小
 * @param  count            读取的轴数量, 按照实际资源获取, 参数范围：[1,32]
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x3131)
 *******************************************************************/
IMC_API IMC_GetAxPrfPosEx(short cardIndex, short axNo, double* pPrfPosArray, short count = 1);

/**
 * @brief  获取轴编码器反馈位置(Ex指令)
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  pEncPosArray     获取轴编码器反馈位置数组, 单位(pulse), 根据count值定义数组的大小
 * @param  count            读取的轴数量, 按照实际资源获取, 参数范围：[1,32]
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x3132)
 *******************************************************************/
IMC_API IMC_GetAxEncPosEx(short cardIndex, short axNo, double* pEncPosArray, short count = 1);

/**
 * @brief  获取 EtherCAT 类型轴的实际转矩, 根据count值可一次获取多个轴的扭矩(Ex类型指令)
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  pActTorqArray    获取的实际转矩数组, 按照轴号顺序排列, 参数范围及单位参考对应的伺服手册
 * @param  count            获取的轴数量, 按照实际资源读取, 参数范围：[1,32]
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x3133)
 *******************************************************************/
IMC_API IMC_GetAxActTorqEx(short cardIndex, short axNo, short* pActTorqArray, short count = 1);
/// @}

/// @defgroup AxisControl 板卡轴功能控制
/// @brief 轴功能控制
/// @{
/**
 * @brief  使能伺服
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  count            使能的轴数量, 按照实际资源设置, 参数范围：[1,32]
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x3200)
 *******************************************************************/
IMC_API IMC_ServoOn(short cardIndex, short axNo, short count = 1);

/**
 * @brief  关闭伺服
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  count            关闭的轴数量, 按照实际资源设置, 参数范围：[1,32]
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x3201)
 *******************************************************************/
IMC_API IMC_ServoOff(short cardIndex, short axNo, short count = 1);

/**
 * @brief  停止单轴轴运动
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  stopType         停止类型
 *                          \n 0：平滑停止
 *                          \n 1：急停
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x3202)
 *******************************************************************/
IMC_API IMC_StopMove(short cardIndex, short axNo, short stopType);

/**
 * @brief  按位停止单轴轴运动
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axMask           按位设置要操作的轴, 按bit位设置：bit0 ~ 31 分别对应轴 0 ~ 31
 * @param  stopTypeBits     按位设置轴的停止类型, 按bit位设置：bit0 ~ 31 分别对应轴 0 ~ 31
 *                          \n 0：平滑停止
 *                          \n 1：急停
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x3203)
 *******************************************************************/
IMC_API IMC_StopMoveBits(short cardIndex, unsigned int axMask, unsigned int stopTypeBits);

/**
 * @brief  设置轴的触发停止参数
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  uselessFlag      是否开启：0 不开启, 1 开启
 * @param  bitNo            仅在 diType 为 0 时有效, 表示停止 DI 在 0x60fd 中 bit 位
 * @param  stopType         停止类型, 0 平滑停止,  1 急停停止
 * @param  diType           DI 类型, 具体信息请参考\ref AxDiStopTypeDef "轴DI停止类型定义"
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x3204)
 *******************************************************************/
IMC_API IMC_SetAxStopTrigPara(short cardIndex, short axNo, short uselessFlag, short bitNo, short stopType, short diType);

/**
 * @brief  获取轴的触发停止参数
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  pUselessFlag     是否开启：0 不开启, 1 开启
 * @param  pBitNo           仅在 diType 为 0 时有效, 表示停止 DI 在 0x60fd 中 bit 位
 * @param  pStopType        停止类型, 0 平滑停止,  1 急停停止
 * @param  pDiType          DI 类型, 具体信息请参考\ref AxDiStopTypeDef "轴DI停止类型定义"
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x3205)
 *******************************************************************/
IMC_API IMC_GetAxStopTrigPara(short cardIndex, short axNo, short* pUselessFlag, short* pBitNo, short* pStopType, short* pDiType);

/**
 * @brief  设置当前轴位置为用户指定位置
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  setPos           轴设定位置, 参数范围：[intMin,intMax]
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x3206)
 *******************************************************************/
IMC_API IMC_SetAxCurPos(short cardIndex, short axNo, double setPos);

/**
 * @brief  同步轴位置, 将轴的规划位置与编码器位置同步
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x3207)
 *******************************************************************/
IMC_API IMC_SyncAxPos(short cardIndex, short axNo);
/// @}

/// @defgroup AxisMovePara 板卡轴运动参数
/// @brief 板卡轴运动参数
/// @{
/**
 * @brief  设置单轴运动参数
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  vel              单轴目标规划速度, 参数范围：(0,maxVel]单位(unit/s)
 * @param  acc              单轴规划加速度, 参数范围：(0,maxAcc]单位(unit/s^2)
 * @param  dec              单轴规划减速度, 参数范围：(0,maxDec]单位(unit/s^2)
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x3300)
 *******************************************************************/
IMC_API IMC_SetAxMvPara(short cardIndex, short axNo, double vel, double acc, double dec);

/**
 * @brief  获取单轴运动参数
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  pVel             单轴目标规划速度, 参数范围：(0,maxVel]单位(unit/s)
 * @param  pAcc             单轴规划加速度, 参数范围：(0,maxAcc]单位(unit/s^2)
 * @param  pDec             单轴规划减速度, 参数范围：(0,maxDec]单位(unit/s^2)
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x3301)
 *******************************************************************/
IMC_API IMC_GetAxMvPara(short cardIndex, short axNo, double* pVel, double* pAcc, double* pDec);

/**
 * @brief  设定单轴速度规划类型
 * @details velType = 1时,ratio 越大, 则 S 越接近 T 型速度规划, 冲击也越大; 反之, ratio 越小, 则规划越平顺, 冲击越小
 *          velType = 2时,ratio的绝对值越大, 规划越平顺, 冲击越小
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  velType          规划类型
 *                          \n 0：T形速度规划
 *                          \n 1：S型速度规划
 * @param  ratio            velType = 1时,S型曲线使用,控制加速度梯形比例, 加速度梯形比例为1:ratio:1, 参数范围：[0.01,100]
 *                          velType = 2时,S型曲线使用,控制加加速度(跃度)平滑变化的时间比例, 加加速度从0变化至最大值的时间比例为ratio/2, 负值表示严格按照设置的加加速度规划 参数范围：[-100,100]
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x3302)
 *******************************************************************/
IMC_API IMC_SetAxVelType(short cardIndex, short axNo, short velType, double ratio);

/**
 * @brief  获取单轴速度规划类型
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  pVelType         规划类型
 *                          \n 0：T形速度规划
 *                          \n 1：S型速度规划
 * @param  pRatio           velType = 1时,S型曲线使用,控制加速度梯形比例, 加速度梯形比例为1:ratio:1, 参数范围：[0.01,100]
 *                          velType = 2时,S型曲线使用,控制加加速度(跃度)平滑变化的时间比例, 加加速度从0变化至最大值的时间比例为ratio/2, 负值表示严格按照设置的加加速度规划 参数范围：[-100,100]
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x3303)
 *******************************************************************/
IMC_API IMC_GetAxVelType(short cardIndex, short axNo, short* pVelType, double* pRatio);


/**
 * @brief  设置单轴运动参数
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  vs               单轴起始规划速度, 参数范围：(0,maxVel]单位(unit/s)
 * @param  vm               单轴最大规划速度, 参数范围：(0,maxVel]单位(unit/s)
 * @param  ve               单轴结束规划速度, 参数范围：(0,maxVel]单位(unit/s)
 * @param  acc              单轴规划加速度, 参数范围：(0,maxAcc]单位(unit/s^2)
 * @param  dec              单轴规划减速度, 参数范围：(0,maxDec]单位(unit/s^2)
 * @param  Jacc             单轴规划加速段跃度, 参数范围：(0,maxJerk]单位(unit/s^3)
 * @param  Jdec             单轴规划减速段跃度, 参数范围：(0,maxJerk]单位(unit/s^3)
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x3308)
 *******************************************************************/
IMC_API IMC_SetAxMvParaEx2(short cardIndex, short axNo, double vs, double vm, double ve, double acc, double dec, double Jacc, double Jdec);

/**
 * @brief  获取单轴运动参数
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  pVs              单轴起始规划速度, 参数范围：(0,maxVel]单位(unit/s)
 * @param  pVm              单轴最大规划速度, 参数范围：(0,maxVel]单位(unit/s)
 * @param  pVe              单轴结束规划速度, 参数范围：(0,maxVel]单位(unit/s)
 * @param  pAcc             单轴规划加速度, 参数范围：(0,maxAcc]单位(unit/s^2)
 * @param  pDec             单轴规划减速度, 参数范围：(0,maxDec]单位(unit/s^2)
 * @param  pJacc            单轴规划加速段跃度, 参数范围：(0,maxJerk]单位(unit/s^3)
 * @param  pJdec            单轴规划减速段跃度, 参数范围：(0,maxJerk]单位(unit/s^3)
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x3309)
 *******************************************************************/
IMC_API IMC_GetAxMvParaEx2(short cardIndex, short axNo, double* pVs, double* pVm, double* pVe, double* pAcc, double* pDec, double* pJacc, double* pJdec);

/// @}
/// @}

/// @defgroup BasicMotion 板卡单轴运动功能
/// @brief 单轴运动功能
/// @{

/// @defgroup EcatAxisHoming 板卡EtherCAT轴回零功能
/// @brief 板卡EtherCAT轴402回零功能
/// @{
/**
 * @brief  设置EtherCAT类型轴402回零参数
 * @param  cardIndex        板卡卡号, 参数范围[0 ~ 3]
 * @param  axNo             操作轴号, 参数范围[0 ~ 63]
 * @param  method           回零方式, 具体信息请参考\ref EcatHomingMethodDef "402回零方式"
 * @param  highVel          回零高速 (pulse/s)
 * @param  lowVel           回零低速 (pulse/s)
 * @param  acc              回零加速度 (pulse/s^2)
 * @param  offset           零点偏移 (pulse)
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x1500)
 *******************************************************************/
IMC_API IMC_SetEcatHomingPara(short cardIndex, short axNo, short method, unsigned int highVel, unsigned int lowVel, unsigned int acc, int offset);

/**
 * @brief  将EtherCAT类型轴设置为回零模式
 * @param  cardIndex        板卡卡号, 参数范围[0 ~ 3]
 * @param  axNo             操作轴号, 参数范围[0 ~ 63]
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x1501)
 *******************************************************************/
IMC_API IMC_SetEcatHomingMode(short cardIndex, short axNo);

/**
 * @brief  控制EtherCAT类型轴开始402回零
 * @param  cardIndex        板卡卡号, 参数范围[0 ~ 3]
 * @param  axNo             操作轴号, 参数范围[0 ~ 63]
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x1502)
 *******************************************************************/
IMC_API IMC_StartEcatHoming(short cardIndex, short axNo);

/**
 * @brief  控制EtherCAT类型轴停止402回零
 * @param  cardIndex        板卡卡号, 参数范围[0 ~ 3]
 * @param  axNo             操作轴号, 参数范围[0 ~ 63]
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x1503)
 *******************************************************************/
IMC_API IMC_StopEcatHoming(short cardIndex, short axNo);

/**
 * @brief  控制EtherCAT类型轴退出回零模式
 * @param  cardIndex        板卡卡号, 参数范围[0 ~ 3]
 * @param  axNo             操作轴号, 参数范围[0 ~ 63]
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x1504)
 *******************************************************************/
IMC_API IMC_ExitEcatHomingMode(short cardIndex, short axNo);

/**
 * @brief  获取EtherCAT类型轴的402回零状态
 * @param  cardIndex        板卡卡号, 参数范围[0 ~ 3]
 * @param  axNo             操作轴号, 参数范围[0 ~ 63]
 * @param  pStatus          回零状态
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x1505)
 *******************************************************************/
IMC_API IMC_GetHomingStatus(short cardIndex, short axNo, short* pStatus);

/**
 * @brief  EtherCAT类型轴402回零合成函数, 包含设定回零参数, 切回零模式以及启动回零运动
 * @param  cardIndex        板卡卡号, 参数范围[0 ~ 3]
 * @param  axNo             操作轴号, 参数范围[0 ~ 63]
 * @param  pHomingPara      回零参数结构体, 具体信息请参考\ref THomingPara "402回零参数结构体"
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x1508)
 *******************************************************************/
IMC_API IMC_StartHoming(short cardIndex, short axNo, THomingPara* pHomingPara);

/**
 * @brief  完成402回零动作合成函数, 包含停止回零, 退出ecat回零模式, 暂默认切到CSP模式
 * @param  cardIndex        板卡卡号, 参数范围[0 ~ 3]
 * @param  axNo             操作轴号, 参数范围[0 ~ 63]
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x1509)
 *******************************************************************/
IMC_API IMC_FinishHoming(short cardIndex, short axNo);

/**
 * @brief  设置 CSP 模式下回原点中 DI 触发对应 0x60FD(digitalInput)中的 bit 位置
 * @param  cardIndex        板卡卡号, 参数范围[0 ~ 3]
 * @param  axNo             操作轴号, 参数范围[0 ~ 63]
 * @param  prbDiBitNo       对应的 bit 位置 范围: [0 ~ 31]
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x1540)
 *******************************************************************/
IMC_API IMC_SetEcatAxProbeMaskBit(short cardIndex, short axNo, short prbDiBitNo);

/**
 * @brief  获取 CSP 模式下回原点中 DI 触发对应 0x60FD(digitalInput)中的 bit 位置
 * @param  cardIndex        板卡卡号, 参数范围[0 ~ 3]
 * @param  axNo             操作轴号, 参数范围[0 ~ 63]
 * @param  pPrbDiBitNo      对应的 bit 位置 范围: [0 ~ 31]
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x1541)
 *******************************************************************/
IMC_API IMC_GetEcatAxProbeMaskBit(short cardIndex, short axNo, short* pPrbDiBitNo);

/**
 * @brief  开始EtherCAT类型轴CSP模式规划回零
 * @param  cardIndex        板卡卡号, 参数范围[0 ~ 3]
 * @param  axNo             操作轴号, 参数范围[0 ~ 63]
 * @param  method           回零方法, 具体信息请参考\ref CSPHomingMethodDef "CSP规划回零方式"
 * @param  dir              回零方向 (1:正向 0:反向)
 * @param  level            回零信号触发边沿 (1:上升沿 0:下降沿)
 * @param  hVel             回零高速 (pulse/s)
 * @param  lVel             回零低速 (pulse/s)
 * @param  acc              回零加速度 (pulse/s^2)
 * @param  offset           零点偏移 (pulse)
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x1542)
 *******************************************************************/
IMC_API IMC_StartEcatAxCSPHoming(short cardIndex, short axNo, short method, short dir, short level, double hVel, double lVel, double acc, double offset);

/**
 * @brief  停止EtherCAT类型轴CSP模式规划回零
 * @param  cardIndex        板卡卡号, 参数范围[0 ~ 3]
 * @param  axNo             操作轴号, 参数范围[0 ~ 63]
 * @param  stopMode         停止方式 (1:急停 0:平滑停止)
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x1543)
 *******************************************************************/
IMC_API IMC_StopEcatAxCSPHoming(short cardIndex, short axNo, short stopMode);

/**
 * @brief  获取EtherCAT类型轴CSP模式规划回零状态
 * @param  cardIndex        板卡卡号, 参数范围[0 ~ 3]
 * @param  axNo             操作轴号, 参数范围[0 ~ 63]
 * @param  pHomingMethod    当前回零方法, 具体信息请参考\ref CSPHomingMethodDef "CSP规划回零方式"
 * @param  pHomingState     当前回零状态, 具体信息请参考\ref CSPHomingStsDef "CSP规划回零状态"
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x1544)
 *******************************************************************/
IMC_API IMC_GetEcatAxCSPHomingSts(short cardIndex, short axNo, short* pHomingMethod, short* pHomingState);

/**
 * @brief  完成EtherCAT类型轴CSP模式规划回零
 * @param  cardIndex        板卡卡号, 参数范围[0 ~ 3]
 * @param  axNo             操作轴号, 参数范围[0 ~ 63]
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x1545)
 *******************************************************************/
IMC_API IMC_FinishEcatAxCSPHoming(short cardIndex, short axNo);
/// @}

/// @defgroup PTP 板卡PTP运动模式相关接口
/// @brief 点位运动模式相关接口
/// @{
/**
 * @brief  启动单轴PTP运动
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  tgtPos           PTP运动目标位置
 * @param  posType          位置类型 0：表示绝对位置 1：表示相对位置
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x3400)
 *******************************************************************/
IMC_API IMC_StartPtpMove(short cardIndex, short axNo, double tgtPos, short posType = 0);

/**
 * @brief  启动多轴PTP的运动
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNum            同时启动的轴数量：[1,16]
 * @param  pAxNoArray       启动轴的各轴轴号
 * @param  pTgtPosArray     启动轴的各轴目标位置
 * @param  pPosTypeArray    启动轴的各轴位置类型 0：表示绝对位置 1：表示相对位置
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x3401)
 *******************************************************************/
IMC_API IMC_StartMultiPtpMove(short cardIndex, short axNum, short* pAxNoArray, double* pTgtPosArray, short* pPosTypeArray);

/**
 * @brief  在线更新PTP目标位置
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  tgtPos           PTP运动目标位置
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x3402)
 *******************************************************************/
IMC_API IMC_UpdatePtpTgtPos(short cardIndex, short axNo, double tgtPos);

/**
 * @brief  获取PTP目标位置
 * @details PTP运动的目标位置会随在线更新目标及停止PTP运动时同步更新
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  pTgtPos          PTP运动目标位置
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x3403)
 *******************************************************************/
IMC_API IMC_GetPtpTgtPos(short cardIndex, short axNo, double* pTgtPos);

/**
 * @brief  在线更新PTP运动参数
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  tgtVel           PTP目标速度, 参数范围：(0,maxVel]单位(unit/s)
 * @param  acc              PTP规划加速度, 参数范围：(0,maxAcc]单位(unit/s^2)
 * @param  dec              PTP规划减速度, 参数范围：(0,maxDec]单位(unit/s^2)
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x3404)
 *******************************************************************/
IMC_API IMC_UpdatePtpMvPara(short cardIndex, short axNo, double tgtVel, double acc, double dec);

/**
 * @brief  在线更新PTP目标速度
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  tgtVel           PTP目标速度, 参数范围：(0,maxVel]单位(unit/s)
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x3405)
 *******************************************************************/
IMC_API IMC_UpdatePtpTgtVel(short cardIndex, short axNo, double tgtVel);

/**
 * @brief  切换PTP规划模式
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x3406)
 *******************************************************************/
IMC_API IMC_PtpPrf(short cardIndex, short axNo);

IMC_API IMC_StartPtpMoveDiag(short cardIndex, short axNo, double tgtPos, short posType, double* pPrfTime, long long* pSysTime);

/**
 * @brief  PTP运动时间计算
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  velType          规划类型
 *                          \n 0：T形速度规划
 *                          \n 1：S型速度规划
 *                          \n 2：暂不支持
 * @param  dist             目标位移, 参数范围：(0,doubleMax]单位(unit)
 * @param  vs               起始速度, 参数范围：(0,maxVel]单位(unit/s)
 * @param  vm               目标速度, 参数范围：(0,maxVel]单位(unit/s)
 * @param  ve               末速度, 参数范围：(0,maxVel]单位(unit/s)
 * @param  acc              规划加速度, 参数范围：(0,maxAcc]单位(unit/s^2)
 * @param  dec              规划减速度, 参数范围：(0,maxDec]单位(unit/s^2)
 * @param  Ja               规划加速段加加速度, 参数范围：(0,maxJerk]单位(unit/s^3)
 * @param  Jd               规划减速段加加速度, 参数范围：(0,maxJerk]单位(unit/s^3)
 * @param  ratio            velType = 1时,S型曲线使用,控制加速度梯形比例, 加速度梯形比例为1:ratio:1, 参数范围：[0.01,100]
 *                          velType = 2时,S型曲线使用,控制加加速度(跃度)平滑变化的时间比例, 加加速度从0变化至最大值的时间比例为ratio/2, 负值表示严格按照设置的加加速度规划 参数范围：[-100,100]
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x340F)
 *******************************************************************/
IMC_API IMC_PtpPrfTimeCalc(short cardIndex, double* pPrfTime, short velType, double dist, double vs, double vm, double ve, double acc, double dec, double Ja = 499999993495552.00, double Jd = 499999993495552.00, double ratio = 0.01);


/// @}

/// @defgroup Jog 板卡Jog运动模式相关接口
/// @brief 点动运动模式相关接口
/// @{
/**
 * @brief  启动单轴JOG运动
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  tgVel            JOG目标速度, 参数范围：[-maxVel,maxVel]单位(unit/s)
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x3500)
 *******************************************************************/
IMC_API IMC_StartJogMove(short cardIndex, short axNo, double tgVel);

/**
 * @brief  启动多轴JOG运动
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNum            同时启动的轴数量：[1,16]
 * @param  pAxNoArray       启动轴的各轴轴号
 * @param  pTgtVelArray     启动轴的各轴目标速度
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x3501)
 *******************************************************************/
IMC_API IMC_StartMultiJogMove(short cardIndex, short axNum, short* pAxNoArray, double* pTgtVelArray);

/**
 * @brief  在线更新JOG目标速度
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  tgVel            JOG目标速度, 参数范围：[-maxVel,maxVel]单位(unit/s)
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x3502)
 *******************************************************************/
IMC_API IMC_UpdateJogTgtVel(short cardIndex, short axNo, double tgVel);

/**
 * @brief  在线更新JOG运动参数
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  tgVel            JOG目标速度, 参数范围：[-maxVel,maxVel]单位(unit/s)
 * @param  acc              JOG规划加速度, 参数范围：(0,maxAcc]单位(unit/s^2)
 * @param  dec              JOG规划减速度, 参数范围：(0,maxDec]单位(unit/s^2)
 * @attention               JOG规划减速度仅作用于JOG在线变速减速过程，JOG停止减速度由IMC_StopMove类型决定
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x3503)
 *******************************************************************/
IMC_API IMC_UpdateJogMvPara(short cardIndex, short axNo, double tgVel, double acc, double dec);

/**
 * @brief  切换JOG规划模式
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x3504)
 *******************************************************************/
IMC_API IMC_JogPrf(short cardIndex, short axNo);
/// @}

/// @defgroup PTPS 板卡PTPS运动模式相关接口
/// @brief 阶梯运动模式相关接口
/// @{
/**
 * @brief  切换PTPS规划模式
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x3480)
 *******************************************************************/
IMC_API IMC_PrfPtps(short cardIndex, short axNo);

/**
 * @brief  设置PTPS运动启动速度
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  beginVel         启动速度, 参数范围：[0,系统最大运行参数bgVel]单位(unit/s)
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x3481)
 *******************************************************************/
IMC_API IMC_SetPtpsBeginVel(short cardIndex, short axNo, double beginVel);

/**
 * @brief  设置PTPS运动结束速度
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  endVel           结束速度, 参数范围：[0,系统最大运行参数endVel]单位(unit/s)
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x3482)
 *******************************************************************/
IMC_API IMC_SetPtpsEndVel(short cardIndex, short axNo, double endVel);

/**
 * @brief  设置PTPS运动参数
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  tgVel            目标速度, 参数范围：（0,maxVel]单位(unit/s)
 * @param  acc              规划加速度, 参数范围：(0,maxAcc]单位(unit/s^2)
 * @param  dec              规划减速度, 参数范围：(0,maxDec]单位(unit/s^2)
 * @attention               规划加减速度作用于PTPS规划过程（包括软着陆与软启动），停止减速度由IMC_StopMove类型决定
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x3483)
 *******************************************************************/
IMC_API IMC_SetPtpsMovePara(short cardIndex, short axNo, double tgtVel, double acc, double dec);

/**
 * @brief  设置PTPS运动参数
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  tgVel            目标速度, 参数范围：（0,maxVel]单位(unit/s)
 * @param  startAcc         目标加速度, 参数范围：(0,maxAcc]单位(unit/s^2)
 * @param  stopDec          目标减速度, 参数范围：(0,maxDec]单位(unit/s^2)
 * @param  tgtAcc           目标加速度, 参数范围：(0,maxAcc]单位(unit/s^2)
 * @param  tgtDec           目标减速度, 参数范围：(0,maxDec]单位(unit/s^2)
 * @attention               目标加减速度作用于PTPS规划过程,启停加减速度作用于软启动和软着陆过程，停止减速度由IMC_StopMove类型决定
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x3490)
 *******************************************************************/
IMC_API IMC_SetPtpsMoveParaEx(short cardIndex, short axNo, double tgtVel, double startAcc, double stopDec, double tgtAcc, double tgtDec);

/**
 * @brief  启动单轴PTPS快速规划
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  tgtPos           目标位置
 * @param  posType          位置类型：0-绝对位置 1-相对位置
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x3484)
 *******************************************************************/
IMC_API IMC_StartPtpsMove(short cardIndex, short axNo, double tgtPos, short posType);

/**
 * @brief  启动单轴PTPS软启动规划
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  tgtPos           目标位置
 * @param  startSlowPos     软启动位置
 * @param  startSlowVel     软启动速度
 * @param  posType          位置类型：0-绝对位置 1-相对位置
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x3485)
 *******************************************************************/
IMC_API IMC_StartPtpsMoveA(short cardIndex, short axNo, double tgtPos, double startSlowPos, double startSlowVel, short posType);

/**
 * @brief  启动单轴PTPS软着陆规划
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  tgtPos           目标位置
 * @param  stopSlowPos      软着陆位置
 * @param  stopSlowVel      软着陆速度
 * @param  posType          位置类型：0-绝对位置 1-相对位置
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x3486)
 *******************************************************************/
IMC_API IMC_StartPtpsMoveD(short cardIndex, short axNo, double tgtPos, double stopSlowPos, double stopSlowVel, short posType);

/**
 * @brief  启动单轴PTPS多段运动规划，可实现软启动及软着陆
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  tgtPos           目标位置
 * @param  startSlowPos     软启动位置
 * @param  startSlowVel     软启动速度
 * @param  stopSlowPos      软着陆位置
 * @param  stopSlowVel      软着陆速度
 * @param  posType          位置类型：0-绝对位置 1-相对位置
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x3487)
 *******************************************************************/
IMC_API IMC_StartPtpsMoveM(short cardIndex, short axNo, double tgtPos, double startSlowPos, double startSlowVel, double stopSlowPos, double stopSlowVel, short posType);

/**
 * @brief  在线更新PTPS目标位置，无法实现软着陆
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  tgtPos           目标位置, 绝对位置
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x3488)
 *******************************************************************/
IMC_API IMC_UpdatePtpsMovePos(short cardIndex, short axNo, double tgtPos);

/**
 * @brief  在线更新PTPS目标速度，无法实现软着陆
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  tgtVel           目标速度, 参数范围：(0,maxVel]单位(unit/s)
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x3489)
 *******************************************************************/
IMC_API IMC_UpdatePtpsMoveVel(short cardIndex, short axNo, double tgtVel);

/**
 * @brief  在线更新PTPS运动参数，无法实现软着陆
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  tgtVel           目标速度, 参数范围：(0,maxVel]单位(unit/s)
 * @param  acc              规划加速度, 参数范围：(0,maxAcc]单位(unit/s^2)
 * @param  dec              规划减速度, 参数范围：(0,maxDec]单位(unit/s^2)
 * @attention               规划减速度仅作用于规划减速过程，停止减速度由IMC_StopMove类型决定
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x348a)
 *******************************************************************/
IMC_API IMC_UpdatePtpsMovePara(short cardIndex, short axNo, double tgtVel, double acc, double dec);

/**
 * @brief  在线更新PTPS软着陆规划，实现软着陆
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  tgtPos           目标位置
 * @param  stopSlowPos      软着陆位置
 * @param  stopSlowVel      软着陆速度
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x348b)
 *******************************************************************/
IMC_API IMC_UpdatePtpsMoveD(short cardIndex, short axNo, double tgtPos, double stopSlowPos, double stopSlowVel);

/**
 * @brief  获取PTPS运动启动速度
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  pBeginVel        获取到的启动速度
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x348c)
 *******************************************************************/
IMC_API IMC_GetPtpsBeginVel(short cardIndex, short axNo, double* pBeginVel);

/**
 * @brief  获取PTPS运动结束速度
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  pEendVel         获取到的结束速度
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x348d)
 *******************************************************************/
IMC_API IMC_GetPtpsEndVel(short cardIndex, short axNo, double* pEendVel);

/**
 * @brief  获取PTPS运动参数
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  pTgtVel          获取到的目标速度
 * @param  pAcc             获取到的规划加速度
 * @param  pDec             获取到的规划减速度
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x348e)
 *******************************************************************/
IMC_API IMC_GetPtpsMovePara(short cardIndex, short axNo, double* pTgtVel, double* pAcc, double* pDec);

/**
 * @brief  获取PTPS运动参数
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  tgVel            目标速度, 参数范围：（0,maxVel]单位(unit/s)
 * @param  pStartAcc        启动加速度, 参数范围：(0,maxAcc]单位(unit/s^2)
 * @param  pStopDec         停止减速度, 参数范围：(0,maxDec]单位(unit/s^2)
 * @param  pTgtAcc          规划加速度, 参数范围：(0,maxAcc]单位(unit/s^2)
 * @param  pTgtDec          规划减速度, 参数范围：(0,maxDec]单位(unit/s^2)
 * @attention               目标加减速度作用于软启动和软着陆过程，规划加减速度作用于PTPS规划过程，停止减速度由IMC_StopMove类型决定
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x3490)
 *******************************************************************/
IMC_API IMC_GetPtpsMoveParaEx(short cardIndex, short axNo, double* pTgtVel, double* pStartAcc, double* pStopDec, double* pTgtAcc, double* pTgtDec);

/// @}

/// @defgroup Gear 板卡Gear运动模式相关接口
/// @brief 电子齿轮运动模式相关接口
/// @{
/**
 * @brief  切换为电子齿轮模式
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x3600)
 *******************************************************************/
IMC_API IMC_GearPrf(short cardIndex, short axNo);

/**
 * @brief  设置电子齿轮参数
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  pGearParam       电子齿轮参数, 详细信息请参考\ref TGearParam "电子齿轮参数结构体"
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x3601)
 *******************************************************************/
IMC_API IMC_GearSetParam(short cardIndex, short axNo, TGearParam* pGearParam);

/**
 * @brief  获取电子齿轮参数
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  pGearParam       电子齿轮参数, 详细信息请参考\ref TGearParam "电子齿轮参数结构体"
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x3602)
 *******************************************************************/
IMC_API IMC_GearGetParam(short cardIndex, short axNo, TGearParam* pGearParam);

/**
 * @brief  启动电子齿轮运动
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x3605)
 *******************************************************************/
IMC_API IMC_GearStart(short cardIndex, short axNo);

/**
 * @brief  停止电子齿轮运动
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  stopType         停止类型：0 正常停止 1 急停停止
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x3606)
 *******************************************************************/
IMC_API IMC_GearStop(short cardIndex, short axNo, short stopType);

/**
 * @brief  获取电子齿轮状态
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  pStatus          电子齿轮运动状态 0：停止状态 1：离合区运行状态 2：同步运行状态 3：减速停止状态 4：等待启动状态
 * @param  pErr             错误状态 :1：轴报警或限位触发 2：轴跟随误差报警 3：主轴超过离合区未完成同步 4：同步运行状态下超过最大速度
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x3607)
 *******************************************************************/
IMC_API IMC_GearGetStatus(short cardIndex, short axNo, short* pStatus, short* pErr);

/**
 * @brief  更新电子齿轮比
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  masterScale      主轴齿数, 参数范围：(0,intMax]
 * @param  slaveScale       从轴齿数, 参数范围：[intMin,0) || (0,intMax]
 * @param  masterDis        离合区距离, 参数范围：[0,intMax]
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x3608)
 *******************************************************************/
IMC_API IMC_GearUpdateScale(short cardIndex, short axNo, int masterScale, int slaveScale, int masterDis);
/// @}

/// @defgroup PVT 板卡PVT运动模式相关接口
/// @brief PVT运动模式相关接口
/// @{
/**
 * @brief  切换为PVT模式
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x3700)
 *******************************************************************/
IMC_API IMC_PvtPrf(short cardIndex, short axNo);

/**
 * @brief  选择PVT数据表
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  tableId          数据表ID, 参数范围：[0,7]
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x3701)
 *******************************************************************/
IMC_API IMC_PvtTableSelect(short cardIndex, short axNo, short tableId);

/**
 * @brief  启动PVT运动
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNum            操作轴数, 参数范围：[1,64]
 * @param  pAxArray         操作轴号列表, 参数范围：[0,63]
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x3702)
 *******************************************************************/
IMC_API IMC_PvtStart(short cardIndex, short axNum, short* pAxArray);

/**
 * @brief  获取PVT状态
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  pTableId         当前正在使用的数据表ID, 参数范围：[0,7]
 * @param  pTime            操作轴已经运动的时间
 * @param  count            操作轴数, 参数范围：[1,53]
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x3703)
 *******************************************************************/
IMC_API IMC_PvtGetStatus(short cardIndex, short axNo, short* pTableId, double* pTime, short count = 1);

/**
 * @brief  设置PVT循环
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  loop             目标循环次数, 参数范围：[0,intMax]
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x3704)
 *******************************************************************/
IMC_API IMC_PvtSetLoop(short cardIndex, short axNo, int loop);

/**
 * @brief  获取PVT循环
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  pCurLoop         当前循环次数, 参数范围：[0,intMax]
 * @param  pLoop            目标循环次数, 参数范围：[0,intMax]
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x3705)
 *******************************************************************/
IMC_API IMC_PvtGetLoop(short cardIndex, short axNo, int* pCurLoop, int* pLoop);

/**
 * @brief  向PVT的指定数据表传送PT数据
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  tableId          目标表ID, 参数范围：[0,7]
 * @param  count            数据个数, 参数范围：[2,2048]
 * @param  P                位置, 单位：pulse
 * @param  T                时间, 单位：ms
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x3706)
 *******************************************************************/
IMC_API IMC_PvtTablePt(short cardIndex, short tableId, int count, double* P, double* T);

/**
 * @brief  向PVT的指定数据表传送数据, 采用PVT描述方式
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  tableId          目标表ID, 参数范围：[0,7]
 * @param  count            数据个数, 参数范围：[2,2048]
 * @param  P                位置, 单位：pulse
 * @param  V                速度, 单位：pulse/ms
 * @param  T                时间, 单位：ms
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x3707)
 *******************************************************************/
IMC_API IMC_PvtTablePvt(short cardIndex, short tableId, int count, double* P, double* V, double* T);

/**
 * @brief  向PVT的指定数据表传送数据, 采用Percent描述方式(1段Percent描述方式消耗1~3个缓存空间)
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  tableId          目标表ID, 参数范围：[0,7]
 * @param  count            数据个数, 参数范围：[2,2048]
 * @param  P                位置, 单位：pulse
 * @param  T                时间, 单位：ms
 * @param  percent          百分比, 参数范围：[0,100], 单位：%
 * @param  startVel         起点速度, 单位：pulse/ms
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x3708)
 *******************************************************************/
IMC_API IMC_PvtTablePercent(short cardIndex, short tableId, int count, double* P, double* T, double* percent, double startVel = 0);

/**
 * @brief  向PVT的指定数据表传送数据, 采用Complete描述方式
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  tableId          目标表ID, 参数范围：[0,7]
 * @param  count            数据个数, 参数范围：[2,2048]
 * @param  P                位置, 单位：pulse
 * @param  T                时间, 单位：ms
 * @param  startVel         起点速度, 单位：pulse/ms
 * @param  endVel           终点速度, 单位：pulse/ms
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x3709)
 *******************************************************************/
IMC_API IMC_PvtTableComplete(short cardIndex, short tableId, int count, double* P, double* T, double startVel = 0, double endVel = 0);
/// @}

/// @defgroup CAM 板卡CAM运动模式相关接口
/// @brief 凸轮运动模式相关接口
/// @{
///
/**
 * @brief  设置凸轮规划模式
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x3800)
 *******************************************************************/
IMC_API IMC_CamPrf(short cardIndex, short axNo);

/**
 * @brief  凸轮参数设置
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  masterType       主轴类型: 0 轴规划, 1 轴编码器, 10 端子板编码器, 11 EtherCAT编码器
 * @param  masterNo         主轴索引: 依据主轴类型定义范围，轴规划或轴编码器 [0,63]，端子板编码器[0,2],EtherCAT编码器[0,7]
 * @param  dirMode          方向模式: 0 跟随方向不限制, 1 正向绝对位置跟随, 2 负向绝对位置跟随
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x3801)
 *******************************************************************/
IMC_API IMC_CamSetParam(short cardIndex, short axNo, short masterType, short masterNo, short dirMode = 0);

/**
 * @brief  凸轮参数获取
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  pMasterType      主轴类型: 0 轴规划, 1 轴编码器, 10 端子板编码器, 11 EtherCAT编码器
 * @param  pMasterNo        主轴索引: 依据主轴类型定义范围，轴规划或轴编码器 [0,63]，端子板编码器[0,2],EtherCAT编码器[0,7]
 * @param  pDirMode         方向模式: 0 跟随方向不限制, 1 正向绝对位置跟随, 2 负向绝对位置跟随
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x3802)
 *******************************************************************/
IMC_API IMC_CamGetParam(short cardIndex, short axNo, short* pMasterType, short* pMasterNo, short* pDirMode);

/**
 * @brief  凸轮表选择
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  tableId          目标表ID, 参数范围：[0,当前凸轮表个数(默认为8) - 1]
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x3803)
 *******************************************************************/
IMC_API IMC_CamTableSelect(short cardIndex, short axNo, short tableId);

/**
 * @brief  启动凸轮运动
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x3804)
 *******************************************************************/
IMC_API IMC_CamStart(short cardIndex, short axNo);

/**
 * @brief  停止凸轮运动
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  stopType         停止类型
 *                          \n 0：平滑停止
 *                          \n 1：急停
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x3805)
 *******************************************************************/
IMC_API IMC_CamStop(short cardIndex, short axNo, short stopType);

/**
 * @brief  获取凸轮状态
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  pStatus          凸轮表状态: 0 空闲, 1 等待启动, 2 加速耦合, 3 运行中, 4 平滑停止, 5 急停
 * @param  pTableId         目标表ID, 参数范围：[0,当前凸轮表个数(默认为8) - 1]
 * @param  pCurUserID       当前正在执行的段对应的UserID, 参数范围：[intMin,intMax]
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x3806)
 *******************************************************************/
IMC_API IMC_CamGetStatus(short cardIndex, short axNo, short* pStatus, short* pTableId, int* pCurUserID);

/**
 * @brief  获取凸轮当前在表中的主轴位置和从轴位置
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  pMasterPos       主轴位置
 * @param  pSlavePos        从轴位置
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x3807)
 *******************************************************************/
IMC_API IMC_CamGetCurPos(short cardIndex, short axNo, double* pMasterPos, double* pSlavePos);

/**
 * @brief  设置凸轮循环次数
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  loop             目标循环次数, 参数范围：[0,uintMax], 0 表示无限循环
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x3810)
 *******************************************************************/
IMC_API IMC_CamSetLoop(short cardIndex, short axNo, unsigned int loop);

/**
 * @brief  获取凸轮循环次数
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  pCurLoop         当前已执行次数, 参数范围：[0,uintMax]
 * @param  pLoop            目标循环次数, 参数范围：[0,uintMax], 0 表示无限循环
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x3811)
 *******************************************************************/
IMC_API IMC_CamGetLoop(short cardIndex, short axNo, unsigned int* pCurLoop, unsigned int* pLoop);

/**
 * @brief  设置凸轮启动方式
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  type             启动方式: 0 立即启动, 1 正向穿越位置, 2 负向穿越位置
 * @param  masterPos        穿越位置, 启动方式为1或2时有效, 参数范围：[intMin,intMax]
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x3812)
 *******************************************************************/
IMC_API IMC_CamSetEvent(short cardIndex, short axNo, short type, int masterPos);

/**
 * @brief  获取凸轮启动方式
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  pType            启动方式: 0 立即启动, 1 正向穿越位置, 2 负向穿越位置
 * @param  pMasterPos       穿越位置, 启动方式为1或2时有效, 参数范围：[intMin,intMax]
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x3813)
 *******************************************************************/
IMC_API IMC_CamGetEvent(short cardIndex, short axNo, short* pType, int* pMasterPos);

/**
 * @brief  主轴相位偏移(由速度参数指定)
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  phaseShift       目标相位偏移, 单位：unit
 * @param  phaseVel         相位偏移速度, 参数范围(1e-6,doubleMax] unit/s
 * @param  phaseAcc         相位偏移加速度, 参数范围(1e-6,doubleMax] unit/s^2
 * @param  phaseDec         相位偏移减速度, 参数范围(1e-6,doubleMax] unit/s^2
 * @param  phaseShiftType   目标相位偏移类型, 0 绝对偏移, 1 相对偏移
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x3820)
 *******************************************************************/
IMC_API IMC_CamPhasing(short cardIndex, short axNo, double phaseShift, double phaseVel, double phaseAcc, double phaseDec, short phaseShiftType = 0);

/**
 * @brief  主轴相位偏移(指定主轴位移完成偏移)
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  phaseShift       目标相位偏移, 单位：unit
 * @param  masterDist       完成偏移对应的主轴位移， 参数范围：[1e-6,doubleMax], 单位：unit
 * @param  accDistRatio     偏移加速段占主轴位移的百分比, 参数范围[0,100], 单位：%, 注意:accDistRatio + decDistRatio <= 100
 * @param  decDistRatio     偏移减速段占主轴位移的百分比, 参数范围[0,100], 单位：%, 注意:accDistRatio + decDistRatio <= 100
 * @param  phaseShiftType   目标相位偏移类型, 0 绝对偏移, 1 相对偏移
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x3821)
 *******************************************************************/
IMC_API IMC_CamPhasingDist(short cardIndex, short axNo, double phaseShift, double masterDist, double accDistRatio, double decDistRatio, short phaseShiftType = 0);

/**
 * @brief  停止主轴相位偏移
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x3822)
 *******************************************************************/
IMC_API IMC_CamStopPhasing(short cardIndex, short axNo);

/**
 * @brief  获取主轴相位偏移状态
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  phasingSts       相位偏移状态: 0 相位偏移静止, 1 相位偏移中
 * @param  pPhase           当前主轴相位偏移, 单位：unit
 * @param  pPhaseVel        当前主轴相位偏移速度, 单位：unit/s
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x3823)
 *******************************************************************/
IMC_API IMC_CamGetPhasing(short cardIndex, short axNo, short* phasingSts, double* pPhase, double* pPhaseVel);

/**
 * @brief  清除凸轮表数据
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  tableId          目标表ID, 参数范围：[0,当前凸轮表个数(默认为8) - 1]
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x3830)
 *******************************************************************/
IMC_API IMC_CamTableClrData(short cardIndex, short tableId);

/**
 * @brief  获取凸轮表剩余空间
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  tableId          目标表ID, 参数范围：[0,当前凸轮表个数(默认为8) - 1]
 * @param  pSpace           剩余空间, 参数范围：[2,8*2048/(当前凸轮表个数(默认为8))]
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x3831)
 *******************************************************************/
IMC_API IMC_CamTableGetSpace(short cardIndex, short tableId, int* pSpace);

/**
 * @brief  重新分配凸轮表数量和大小(总深度为8*2048)
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  tableNum         凸轮表个数, 参数范围：[1,16]
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x3832)
 *******************************************************************/
IMC_API IMC_CamTableResize(short cardIndex, short tableNum);

/**
 * @brief  获取凸轮表数量和大小信息
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  pTableNum        凸轮表个数, 参数范围：[1,16]
 * @param  pTableSize       单个凸轮表总大小
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x3833)
 *******************************************************************/
IMC_API IMC_CamTableGetInfo(short cardIndex, short* pTableNum, int* pTableSize);

/**
 * @brief  凸轮表数据压入指令-通用压入指令
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  tableId          目标表ID, 参数范围：[0,当前凸轮表个数(默认为8) - 1]
 * @param  count            数据个数, 参数范围：[2,表剩余空间]
 * @param  pCurveType       曲线类型: 0 直线, 1 抛物线, 2 百分比, 3 三次曲线, 4 五次曲线, 5 七次曲线
 * @param  pKeyPointParam   关键点参数信息, 一个关键点保留5个double数据, 总大小为count * 5。第i个关键点的数据为: pKeyPointParam[i * 5 + 0] = 主轴位置(unit), pKeyPointParam[i * 5 + 1] = 从轴位置(unit)
 *                          \n pCurveType[i] = 0时, ..., pKeyPointParam[i * 5 + 2] = pKeyPointParam[i * 5 + 3] = pKeyPointParam[i * 5 + 4] = 内部强制为0, 该段速度 = 从轴相对位移(相对上一点) / 主轴相对位移
 *                          \n pCurveType[i] = 1时, ..., pKeyPointParam[i * 5 + 2] = pKeyPointParam[i * 5 + 3] = pKeyPointParam[i * 5 + 4] = 内部强制为0, 该段速度 = 2 * 从轴相对位移(相对上一点) / 主轴相对位移 - 上一点速度
 *                          \n pCurveType[i] = 2时, ..., pKeyPointParam[i * 5 + 2] = 加速度变化时间百分比(取值[0,100]), pKeyPointParam[i * 5 + 3] = pKeyPointParam[i * 5 + 4] = 内部强制为0, 该段速度 = 2 * 从轴相对位移(相对上一点) / 主轴相对位移 - 上一点速度
 *                          \n pCurveType[i] = 3时, ..., pKeyPointParam[i * 5 + 2] = 从轴速比, pKeyPointParam[i * 5 + 3] = pKeyPointParam[i * 5 + 4] = 内部强制为0
 *                          \n pCurveType[i] = 4时, ..., pKeyPointParam[i * 5 + 2] = 从轴速比, pKeyPointParam[i * 5 + 3] = 从轴加速度比, pKeyPointParam[i * 5 + 4] = 内部强制为0
 *                          \n pCurveType[i] = 5时, ..., pKeyPointParam[i * 5 + 2] = 从轴速比, pKeyPointParam[i * 5 + 3] = 从轴加速度比, pKeyPointParam[i * 5 + 4] = 从轴跃度比
 * @param  pUserID          指向大小为count的Int32数组, pUserID[i]为对应段的UserID, 指定空指针时, 传入段的UserID为0
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x383a)
 *******************************************************************/
IMC_API IMC_CamTableData(short cardIndex, short tableId, int count, short* pCurveType, double* pKeyPointParam, int* pUserID = 0);

/**
 * @brief  凸轮表获取关键点之间指定相位的位移等信息
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  count            数据个数, 参数范围：[2,intMax]
 * @param  pCurveType       曲线类型: 0 直线, 1 抛物线, 2 百分比, 3 三次曲线, 4 五次曲线, 5 七次曲线
 * @param  pKeyPointParam   关键点参数信息, 一个关键点保留5个double数据, 总大小为count * 5。第i个关键点的数据为: pKeyPointParam[i * 5 + 0] = 主轴位置(unit), pKeyPointParam[i * 5 + 1] = 从轴位置(unit)
 *                          \n pCurveType[i] = 0时, ..., pKeyPointParam[i * 5 + 2] = pKeyPointParam[i * 5 + 3] = pKeyPointParam[i * 5 + 4] = 内部强制为0, 该段速度 = 从轴相对位移(相对上一点) / 主轴相对位移
 *                          \n pCurveType[i] = 1时, ..., pKeyPointParam[i * 5 + 2] = pKeyPointParam[i * 5 + 3] = pKeyPointParam[i * 5 + 4] = 内部强制为0, 该段速度 = 2 * 从轴相对位移(相对上一点) / 主轴相对位移 - 上一点速度
 *                          \n pCurveType[i] = 2时, ..., pKeyPointParam[i * 5 + 2] = 加速度变化时间百分比(取值[0,100]), pKeyPointParam[i * 5 + 3] = pKeyPointParam[i * 5 + 4] = 内部强制为0, 该段速度 = 2 * 从轴相对位移(相对上一点) / 主轴相对位移 - 上一点速度
 *                          \n pCurveType[i] = 3时, ..., pKeyPointParam[i * 5 + 2] = 从轴速比, pKeyPointParam[i * 5 + 3] = pKeyPointParam[i * 5 + 4] = 内部强制为0
 *                          \n pCurveType[i] = 4时, ..., pKeyPointParam[i * 5 + 2] = 从轴速比, pKeyPointParam[i * 5 + 3] = 从轴加速度比, pKeyPointParam[i * 5 + 4] = 内部强制为0
 *                          \n pCurveType[i] = 5时, ..., pKeyPointParam[i * 5 + 2] = 从轴速比, pKeyPointParam[i * 5 + 3] = 从轴加速度比, pKeyPointParam[i * 5 + 4] = 从轴跃度比
 * @param  x                主轴位置
 * @param  pOutput          输出的从轴信息, 数组长度为4的指针, pOutput[0]=从轴位置, pOutput[1]=从轴速比, pOutput[2]=从轴加速度比, pOutput[3]=从轴跃度比
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x383b)
 *******************************************************************/
IMC_API IMC_CamTableGetDist(short cardIndex, int count, short* pCurveType, double* pKeyPointParam, double x, double* pOutput);



/**
 * @brief  凸轮表数据压入指令-连续曲线压入指令  （针对条件不足的曲线）
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  tableId          目标表ID, 参数范围：[0,当前凸轮表个数(默认为8) - 1]
 * @param  curveType        曲线类型: 
 *                          \n周期边界 : 20 -保留(输入主轴位置,从轴位置), 21 -保留(输入主轴位置,从轴位置,从轴速度), 22 五次曲线(输入主轴位置,从轴位置), 
 *                          \n固定边界 : 30 -保留(输入主轴位置,从轴位置), 31 -保留(输入主轴位置,从轴位置,从轴速度), 32 五次曲线(输入主轴位置,从轴位置)
 * @param  row              数据个数(行数), 参数范围：[2,MIN(1000,表空间)]
 * @param  pKeyPointParam   关键点信息
 *                          \n周期边界 : 20 -保留(输入2列参数), 21 -保留(输入3列参数), 22 五次曲线(输入2列参数), 
 *                          \n固定边界 : 30 -保留(输入2列参数), 31 -保留(输入3列参数), 32 五次曲线(输入2列参数)
 *                          \n关键点参数信息, row行 column列,一个关键点保留2个或3个double数据, 总大小为count * column。
 *                          \npKeyPointParam[i * column + 0] = 主轴位置(unit), pKeyPointParam[i * column + 1] = 从轴位置(unit)
 *                          \n如果为3列参数, pKeyPointParam[i * column + 2] = 从轴速比
 * @param  pClampParam      固定边界条件, curveType在[30,40]范围内需要输入边界条件, 
                            \n如<32 五次曲线(输入主轴位置,从轴位置)>,pClampParam[0]=startVel ,pClampParam[1]=endVel,pClampParam[2]=startAcc ,pClampParam[3]=endAcc
 * @param  pUserID          指向大小为count的Int32数组, pUserID[i]为对应段的UserID, 指定空指针时, 传入段的UserID为0
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x383d)
 *******************************************************************/
IMC_API IMC_CamTableDataCont(short cardIndex, short tableId, short curveType, int row, double* pKeyPointParam, double* pClampParam = 0, int* pUserID = 0);

/**
 * @brief  凸轮表获取连续曲线关键点之间指定相位的位移等信息
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  x                主轴位置
 * @param  pOutput          输出的从轴信息, 数组长度为4的指针, pOutput[0]=从轴位置, pOutput[1]=从轴速比, pOutput[2]=从轴加速度比, pOutput[3]=从轴跃度比
 * @param  curveType        曲线类型: 
 *                          \n周期边界 : 20 -保留(输入主轴位置,从轴位置), 21 -保留(输入主轴位置,从轴位置,从轴速度), 22 五次曲线(输入主轴位置,从轴位置), 
 *                          \n固定边界 : 30 -保留(输入主轴位置,从轴位置), 31 -保留(输入主轴位置,从轴位置,从轴速度), 32 五次曲线(输入主轴位置,从轴位置)
 * @param  row              数据个数(行数), 参数范围：[2,MIN(1000,表空间)]
 * @param  pKeyPointParam   关键点信息
 *                          \n周期边界 : 20 -保留(输入2列参数), 21 -保留(输入3列参数), 22 五次曲线(输入2列参数), 
 *                          \n固定边界 : 30 -保留(输入2列参数), 31 -保留(输入3列参数), 32 五次曲线(输入2列参数)
 *                          \n关键点参数信息, row行 column列,一个关键点保留2个或3个double数据, 总大小为count * column。
 *                          \npKeyPointParam[i * column + 0] = 主轴位置(unit), pKeyPointParam[i * column + 1] = 从轴位置(unit)
 *                          \n如果为3列参数, pKeyPointParam[i * column + 2] = 从轴速比
 * @param  pClampParam      固定边界条件, curveType在[30,40]范围内需要输入边界条件, 
                            \n如曲线类型为<32 五次曲线(输入主轴位置,从轴位置)>,pClampParam[0]=startVel ,pClampParam[1]=endVel,pClampParam[2]=startAcc ,pClampParam[3]=endAcc
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x383e)
 *******************************************************************/
IMC_API IMC_CamTableGetDistCont(short cardIndex, double x, double* pOutput, short curveType, int row, double* pKeyPointParam, double* pClampParam = 0);

/// @}

/// @}

/// @defgroup AdvMotion 板卡多轴运动功能
/// @brief 多轴运动功能
/// @{
/// @defgroup Crd 板卡Crd运动模式相关接口
/// @brief 坐标系插补运动模式相关接口
/// @{

/**
 * @brief  创建插补坐标系, 配置坐标系插补参数
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  crdNo            坐标系号, 参数范围：[0,7]
 * @param  pMaskAxNoArray   坐标系的轴映射值, 参数范围 [-1, 板卡最大支持轴数).
 *                          \n 如果只有两个实轴坐 标系, 可以将另外的一个轴挂接虚轴
 *                          \n 例如：要建立两轴（XY）坐标系, 可将第三个轴映射值设为-1
 * @attention 注意！用户必须定义长度为 3 的数组进行传值
 * @param  lookAheadLen     需要建立的缓存队列长度大小, 参数范围 [0, 20000]
 *                          \n 当设置小于或等于1时, 表示无前瞻计算; 设置为其他值时, 表示前瞻缓冲区的大小
 * @param  estopDec         插补急停减加速度, 参数范围：(> 1e-10), 插补过程触发急停利用此减速度进行减速停止
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x4000)
 *******************************************************************/
IMC_API IMC_CrdSetMtSys(short cardIndex, short crdNo, short* pMaskAxNoArray, short lookAheadLen, double estopDec);

/**
 * @brief  获取插补坐标系配置参数
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  crdNo            坐标系号, 参数范围：[0,7]
 * @param  pMaskAxNoArray   坐标系的轴映射值, 参数范围 [-1, 板卡最大支持轴数).
 *                          \n 如果只有两个实轴坐 标系, 可以将另外的一个轴挂接虚轴
 *                          \n 例如：要建立两轴（XY）坐标系, 可将第三个轴映射值设为-1
 * @attention 注意！用户必须定义长度为 3 的数组进行传值
 * @param  pLookAheadLen    需要建立的缓存队列长度大小, 参数范围 [0, 20000]
 *                          \n 当设置为 0 时, 表示无前瞻计算; 设置为其他值时, 表示前瞻缓冲区的大小
 * @param  pEstopDec        插补急停减加速度, 参数范围：(> 1e-10), 插补过程触发急停利用此减速度进行减速停止
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x4001)
 *******************************************************************/
IMC_API IMC_CrdGetMtSysParam(short cardIndex, short crdNo, short* pMaskAxNoArray, short* pLookAheadLen, double* pEstopDec);

/**
 * @brief  删除插补坐标系
 * @attention 需对创建的坐标系进行删除, 否则接口报错
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  crdNo            坐标系号, 参数范围：[0,7]
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x4002)
 *******************************************************************/
IMC_API IMC_CrdDeleteMtSys(short cardIndex, short crdNo);

/**
 * @brief  设置插补坐标系高级参数
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  crdNo            坐标系号, 参数范围：[0,7]
 * @param  pCrdAdvParam     插补高级参数, 详细信息请参考\ref TCrdAdvParam "插补高级参数结构体"
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x4010)
 *******************************************************************/
IMC_API IMC_CrdSetAdvParam(short cardIndex, short crdNo, TCrdAdvParam* pCrdAdvParam);

/**
 * @brief  获取插补坐标系高级参数
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  crdNo            坐标系号, 参数范围：[0,7]
 * @param  pCrdAdvParam     插补高级参数, 详细信息请参考\ref TCrdAdvParam "插补高级参数结构体"
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x4011)
 *******************************************************************/
IMC_API IMC_CrdGetAdvParam(short cardIndex, short crdNo, TCrdAdvParam* pCrdAdvParam);

/**
 * @brief  插补运动指令用户模式下的运动段的始末速度设置
 * @details 该指令主要用于用户规划模式下, 指定每段运动, 段的起始和段末的速度。一般情况下, 运动段的起始速度等于上一段的段末速度
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  crdNo            坐标系号, 参数范围：[0,7]
 * @param  uStartVel        运动段起始速度, 参数范围：[0,1e9]
 * @param  uEndVel          运动段段末速度, 参数范围：[0,1e9]
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x4012)
 *******************************************************************/
IMC_API IMC_CrdSetUserVelPlan(short cardIndex, short crdNo, double uStartVel, double uEndVel);

/**
 * @brief  插补运动指令在用户规划模式下设置的运动段的始末速度的获取
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  crdNo            坐标系号, 参数范围：[0,7]
 * @param  pUStartVel       运动段起始速度, 参数范围：[0,1e9]
 * @param  pUEndVel         运动段段末速度, 参数范围：[0,1e9]
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x4013)
 *******************************************************************/
IMC_API IMC_CrdGetUserVelPlan(short cardIndex, short crdNo, double* pUStartVel, double* pUEndVel);

/**
 * @brief  设置插补平滑参数。通过设置平滑等级和平滑精度, 使插补运动轨迹更平滑
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  crdNo            坐标系号, 参数范围：[0,7]
 * @param  smoothLevel      平滑等级 [0,20], 0 表示不平滑, 大于 0 表示平滑的等级
 * @param  smoothTol        平滑处理的保持精度, 参数范围：[0, 1e6], 即轨迹的平滑处理后应保持的轨迹精度, 单位与插补位置一致
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x4014)
 *******************************************************************/
IMC_API IMC_CrdSetSmoothParam(short cardIndex, short crdNo, int smoothLevel, double smoothTol);

/**
 * @brief  获取插补平滑参数。通过设置平滑等级和平滑精度, 使插补运动轨迹更平滑
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  crdNo            坐标系号, 参数范围：[0,7]
 * @param  pSmoothLevel     平滑等级, 参数范围：[0,20], 0 表示不平滑, 大于 0 表示平滑的等级
 * @param  pSmoothTol       平滑处理的保持精度, 参数范围：[0, 1e6], 即轨迹的平滑处理后应保持的轨迹精度, 单位与插补位置一致
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x4015)
 *******************************************************************/
IMC_API IMC_CrdGetSmoothParam(short cardIndex, short crdNo, int* pSmoothLevel, double* pSmoothTol);

/**
 * @brief  开启/关闭插补时间计算模块
 * @details 开启后压入数据, 可通过IMC_CrdGetPrfTime获取规划时间和剩余时间, 开启后会影响插补压入运动段的指令耗时
 *          仅支持直线、圆弧、螺旋线、涡旋线以及IMC_CrdWaitTime指令的规划时间计算
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  crdNo            坐标系号, 参数范围：[0,7]
 * @param  enable           1 开启  0 关闭
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x4016)
 *******************************************************************/
IMC_API IMC_CrdEnablePrfTimeCalc(short cardIndex, short crdNo, short enable);

/**
 * @brief  设置插补轨迹速度
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  crdNo            坐标系号, 参数范围：[0,7]
 * @param  tgtVel           轨迹目标速度设定数值, 参数范围：[1e-6,1e9]
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x4020)
 *******************************************************************/
IMC_API IMC_CrdSetTrajVel(short cardIndex, short crdNo, double tgtVel);

/**
 * @brief  获取插补轨迹速度
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  crdNo            坐标系号, 参数范围：[0,7]
 * @param  pTgtVel          轨迹目标速度设定数值, 参数范围：[1e-6,1e9]
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x4021)
 *******************************************************************/
IMC_API IMC_CrdGetTrajVel(short cardIndex, short crdNo, double* pTgtVel);

/**
 * @brief  设置插补轨迹加速度和减速度
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  crdNo            坐标系号, 参数范围：[0,7]
 * @param  tgtAcc           轨迹目标加速度设定数值, 参数范围：[1e-6,1e9]
 * @param  tgtDec           轨迹目标减速度设定数值, 参数范围：[1e-6,1e9]
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x4022)
 *******************************************************************/
IMC_API IMC_CrdSetTrajAccAndDec(short cardIndex, short crdNo, double tgtAcc, double tgtDec);

/**
 * @brief  获取插补轨迹加速度和减速度
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  crdNo            坐标系号, 参数范围：[0,7]
 * @param  pTgtAcc          轨迹目标加速度设定数值, 参数范围：[1e-6,1e9]
 * @param  pTgtDec          轨迹目标减速度设定数值, 参数范围：[1e-6,1e9]
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x4023)
 *******************************************************************/
IMC_API IMC_CrdGetTrajAccAndDec(short cardIndex, short crdNo, double* pTgtAcc, double* pTgtDec);

/**
 * @brief  设置插补强制规划末速度降为 0 标识
 * @details 如果将强制规划末速度降为 0, 则下面所有线段末速度为 0
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  crdNo            坐标系号, 参数范围：[0,7]
 * @param  ZeroFlag         插补强制规划末速度将为 0 标识
 *                          \n 0 关闭强制降速为0标识
 *                          \n 1 开启强制降速为0标识
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x4024)
 *******************************************************************/
IMC_API IMC_CrdSetZeroFlag(short cardIndex, short crdNo, short ZeroFlag);

/**
 * @brief  获取插补强制规划末速度降为 0 标识
 * @details 如果将强制规划末速度降为 0, 则下面所有线段末速度为 0
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  crdNo            坐标系号, 参数范围：[0,7]
 * @param  pZeroFlag        插补强制规划末速度将为 0 标识
 *                          \n 0 关闭强制降速为0标识
 *                          \n 1 开启强制降速为0标识
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x4025)
 *******************************************************************/
IMC_API IMC_CrdGetZeroFlag(short cardIndex, short crdNo, short* pZeroFlag);

/**
 * @brief  设置插补运动指令的位置编程模式
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  crdNo            坐标系号, 参数范围：[0,7]
 * @param  mode             插补位置编程模式
 *                          \n 0 绝对坐标编程模式
 *                          \n 1 相对坐标编程模式
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x4026)
 *******************************************************************/
IMC_API IMC_CrdSetIncMode(short cardIndex, short crdNo, short mode);

/**
 * @brief  获取插补运动指令的位置编程模式
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  crdNo            坐标系号, 参数范围：[0,7]
 * @param  pMode            插补位置编程模式
 *                          \n 0 绝对坐标编程模式
 *                          \n 1 相对坐标编程模式
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x4027)
 *******************************************************************/
IMC_API IMC_CrdGetIncMode(short cardIndex, short crdNo, short* pMode);

/**
 * @brief  设置插补同步跟随轴的速度模式
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  crdNo            坐标系号, 参数范围：[0,7]
 * @param  mode             速度模式
 *                          \n 0：IMC_CrdSyncMove 跟随的插补运动段的首末速度为0
 *                          \n 1：IMC_CrdSyncMove 跟随的插补运动段的首末速度由前瞻决定
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x4028)
 *******************************************************************/
IMC_API IMC_CrdSetFolVelMode(short cardIndex, short crdNo, short mode);

/**
 * @brief  获取插补同步跟随轴的速度模式
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  crdNo            坐标系号, 参数范围：[0,7]
 * @param  pMode            速度模式
 *                          \n 0：IMC_CrdSyncMove 跟随的插补运动段的首末速度为0
 *                          \n 1：IMC_CrdSyncMove 跟随的插补运动段的首末速度由前瞻决定
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x4029)
 *******************************************************************/
IMC_API IMC_CrdGetFolVelMode(short cardIndex, short crdNo, short* pMode);

/**
 * @brief  设置插补运动指令的过渡精度
 * @details 该指令属于高级参数指令, 只有在启动过渡模式才生效
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  crdNo            坐标系号, 参数范围：[0,7]
 * @param  tol              轨迹过渡控制精度, 参数范围：[0,1e9]
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x402a)
 *******************************************************************/
IMC_API IMC_CrdSetTrajTol(short cardIndex, short crdNo, double tol);

/**
 * @brief  设置插补运动指令的拐弯系数。
 * @details 该指令属于高级参数指令, 只有在启动过渡模式才生效, 该参数越 小,  在过渡拐弯时速度降至越低, 参数越大, 则拐弯速度越高, 默认参数为 1
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  crdNo            坐标系号, 参数范围：[0,7]
 * @param  turnCoef         拐弯系数, 参数范围：[0.01,100]
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x402b)
 *******************************************************************/
IMC_API IMC_CrdSetTrajTurnCoef(short cardIndex, short crdNo, double turnCoef);

/**
 * @brief  三维直线插补运动
 * @attention pEndPosArray 应传入长度为 3 的数组指针
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  crdNo            坐标系号, 参数范围：[0,7]
 * @param  pEndPosArray     插补运动终点位置
 *                          \n pEndPosArray[0]：运动终点X轴位置
 *                          \n pEndPosArray[1]：运动终点Y轴位置
 *                          \n pEndPosArray[2]：运动终点Z轴位置
 * @param  userID           用户标记索引号, 无范围要求
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x4100)
 *******************************************************************/
IMC_API IMC_CrdLineXYZ(short cardIndex, short crdNo, double* pEndPosArray, int userID = 0);

/**
 * @brief  XY平面内直线插补运动
 * @attention pEndPosArray 应传入长度为 2 的数组指针
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  crdNo            坐标系号, 参数范围：[0,7]
 * @param  pEndPosArray     插补运动终点位置
 *                          \n pEndPosArray[0]：运动终点X轴位置
 *                          \n pEndPosArray[1]：运动终点Y轴位置
 * @param  userID           用户标记索引号, 无范围要求
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x4101)
 *******************************************************************/
IMC_API IMC_CrdLineXY(short cardIndex, short crdNo, double* pEndPosArray, int userID = 0);

/**
 * @brief  XZ平面内直线插补运动
 * @attention pEndPosArray 应传入长度为 2 的数组指针
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  crdNo            坐标系号, 参数范围：[0,7]
 * @param  pEndPosArray     插补运动终点位置
 *                          \n pEndPosArray[0]：运动终点Z轴位置
 *                          \n pEndPosArray[1]：运动终点X轴位置
 * @param  userID           用户标记索引号, 无范围要求
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x4102)
 *******************************************************************/
IMC_API IMC_CrdLineZX(short cardIndex, short crdNo, double* pEndPosArray, int userID = 0);

/**
 * @brief  YZ平面内直线插补运动
 * @attention pEndPosArray 应传入长度为 2 的数组指针
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  crdNo            坐标系号, 参数范围：[0,7]
 * @param  pEndPosArray     插补运动终点位置
 *                          \n pEndPosArray[0]：运动终点Y轴位置
 *                          \n pEndPosArray[1]：运动终点Z轴位置
 * @param  userID           用户标记索引号, 无范围要求
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x4103)
 *******************************************************************/
IMC_API IMC_CrdLineYZ(short cardIndex, short crdNo, double* pEndPosArray, int userID = 0);

/**
 * @brief  给定三点的三维圆弧插补
 * @details 如果输入三点共线会导致圆弧几何错误, 此函数只能画小于一圈的圆不能画整圆和多圈圆
 * @attention pMidPos 、pEndPos 应传入长度为 3 的数组指针
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  crdNo            坐标系号, 参数范围：[0,7]
 * @param  pMidPosArray     圆弧中间任意一点位置
 *                          \n pMidPosArray[0]：圆弧任意点X轴位置
 *                          \n pMidPosArray[1]：圆弧任意点Y轴位置
 *                          \n pMidPosArray[2]：圆弧任意点Z轴位置
 * @param  pEndPosArray     插补运动终点位置
 *                          \n pEndPosArray[0]：运动终点X轴位置
 *                          \n pEndPosArray[1]：运动终点Y轴位置
 *                          \n pEndPosArray[2]：运动终点Z轴位置
 * @param  userID           用户标记索引号, 无范围要求
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x4120)
 *******************************************************************/
IMC_API IMC_CrdArcThreePoint(short cardIndex, short crdNo, double* pMidPosArray, double* pEndPosArray, int userID = 0);

/**
 * @brief  给定圆心, 末点法向量的三维圆弧插补
 * @details 当 height 不为 0 的时候可衍生为螺旋线插补。当起点和终点重合的时候几何参数错误。
 *          \n 当法向量与圆所在平面夹角小于 60 度的时候几何参数错误。输入的法向量可不为单位向量
 * @attention pCenterArray, pEndPosArray, pNormalArray 应传入长度为 3 的数组指针
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  crdNo            坐标系号, 参数范围：[0,7]
 * @param  pCenterArray     圆弧圆心坐标(相对于起点的位置增量)
 *                          \n pCenterArray[0]：圆弧圆心X轴位置
 *                          \n pCenterArray[1]：圆弧圆心Y轴位置
 *                          \n pCenterArray[2]：圆弧圆心Z轴位置
 * @param  pEndPosArray     插补运动终点位置
 *                          \n pEndPosArray[0]：运动终点X轴位置
 *                          \n pEndPosArray[1]：运动终点Y轴位置
 *                          \n pEndPosArray[2]：运动终点Z轴位置
 * @param  pNormalArray     圆弧的法向量, 圆弧方向由法向量决定
 *                          \n pNormalArray[0]：圆弧的法向量X轴分量
 *                          \n pNormalArray[1]：圆弧的法向量Y轴分量
 *                          \n pNormalArray[2]：圆弧的法向量Z轴分量
 * @param  height           绝对值为螺旋线螺距高：符号代表螺旋线的垂直与圆的运动方向, 正号表示顺着法向量方向运动, 负号表示逆着法向量方向运动
 * @param  turn             圆弧运动的圈数, 参数范围：[0, 1e8]
 * @param  userID           用户标记索引号, 无范围要求
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x4121)
 *******************************************************************/
IMC_API IMC_Crd3DArcCenterNormal(short cardIndex, short crdNo, double* pCenterArray, double* pEndPosArray, double* pNormalArray, double height = 0, int turn = 0, int userID = 0);

/**
 * @brief  给定圆弧半径, 圆弧末点和圆弧法向量的三维圆弧插补
 * @details 当 height 不为 0 的时候可衍生为螺旋线插补。 此函数无法插补整圆
 * @attention pEndPosArray, pNormalArray 应传入长度为 3 的数组指针
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  crdNo            坐标系号, 参数范围：[0,7]
 * @param  radius           圆弧半径, 正号：优弧, 负号：劣弧
 * @param  pEndPosArray     插补运动终点位置
 *                          \n pEndPosArray[0]：运动终点X轴位置
 *                          \n pEndPosArray[1]：运动终点Y轴位置
 *                          \n pEndPosArray[2]：运动终点Z轴位置
 * @param  pNormalArray     圆弧的法向量, 圆弧方向由法向量决定
 *                          \n pNormalArray[0]：圆弧的法向量X轴分量
 *                          \n pNormalArray[1]：圆弧的法向量Y轴分量
 *                          \n pNormalArray[2]：圆弧的法向量Z轴分量
 * @param  height           绝对值为螺旋线螺距高：符号代表螺旋线的垂直与圆的运动方向, 正号表示顺着法向量方向运动, 负号表示逆着法向量方向运动
 * @param  turn             圆弧运动的圈数, 参数范围：[0, 1e8]
 * @param  userID           用户标记索引号, 无范围要求
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x4122)
 *******************************************************************/
IMC_API IMC_Crd3DArcRadiusNormal(short cardIndex, short crdNo, double radius, double* pEndPosArray, double* pNormalArray, double height = 0, int turn = 0, int userID = 0);

/**
 * @brief  给定圆弧圆心, 圆弧角度和圆弧法向量的三维圆弧插补
 * @details 当 height 不为 0 的时候可衍生为螺旋线插补。此函数可以画整圆
 * @attention pCenterArray, pNormalArray 应传入长度为 3 的数组指针
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  crdNo            坐标系号, 参数范围：[0,7]
 * @param  pCenterArray     圆弧圆心坐标(相对于起点的位置增量)
 *                          \n pCenterArray[0]：圆弧圆心X轴位置
 *                          \n pCenterArray[1]：圆弧圆心Y轴位置
 *                          \n pCenterArray[2]：圆弧圆心Z轴位置
 * @param  angle            圆弧一共行走的角度, 参数范围：[-1e9, 1e9]单位(rad)
 * @param  pNormalArray     圆弧的法向量, 圆弧方向由法向量决定
 *                          \n pNormalArray[0]：圆弧的法向量X轴分量
 *                          \n pNormalArray[1]：圆弧的法向量Y轴分量
 *                          \n pNormalArray[2]：圆弧的法向量Z轴分量
 * @param  height           绝对值为螺旋线螺距高：符号代表螺旋线的垂直与圆的运动方向, 正号表示顺着法向量方向运动, 负号表示逆着法向量方向运动
 * @param  userID           用户标记索引号, 无范围要求
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x4123)
 *******************************************************************/
IMC_API IMC_Crd3DArcAngleNormal(short cardIndex, short crdNo, double* pCenterArray, double angle, double* pNormalArray, double height = 0, int userID = 0);

/**
 * @brief  给定圆弧圆心和圆弧末点, XY平面内的圆弧插补
 * @attention pCenterArray, pEndPosArray 应传入长度为 2 的数组指针
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  crdNo            坐标系号, 参数范围：[0,7]
 * @param  pCenterArray     圆弧圆心坐标(相对于起点的位置增量)
 *                          \n pCenterArray[0]：圆弧圆心X轴位置
 *                          \n pCenterArray[1]：圆弧圆心Y轴位置
 * @param  pEndPosArray     插补运动终点位置
 *                          \n pEndPosArray[0]：运动终点X轴位置
 *                          \n pEndPosArray[1]：运动终点Y轴位置
 * @param  dir              圆弧运动方向
 *                          \n 1：圆弧逆时针方向运动
 *                          \n -1 圆弧顺时针方向运动
 * @param  height           绝对值为螺旋线螺距高：符号代表螺旋线的垂直与圆的运动方向, 正号表示顺着法向量方向运动, 负号表示逆着法向量方向运动
 * @param  turn             圆弧运动的圈数, 参数范围：[0, 1e8]
 * @param  userID           用户标记索引号, 无范围要求
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x4124)
 *******************************************************************/
IMC_API IMC_CrdArcCenterXYPlane(short cardIndex, short crdNo, double* pCenterArray, double* pEndPosArray, short dir, double height = 0, int turn = 0, int userID = 0);

/**
 * @brief  给定圆弧圆心和圆弧末点, YZ平面内的圆弧插补
 * @attention pCenterArray, pEndPosArray 应传入长度为 2 的数组指针
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  crdNo            坐标系号, 参数范围：[0,7]
 * @param  pCenterArray     圆弧圆心坐标(相对于起点的位置增量)
 *                          \n pCenterArray[0]：圆弧圆心Y轴位置
 *                          \n pCenterArray[1]：圆弧圆心Z轴位置
 * @param  pEndPosArray     插补运动终点位置
 *                          \n pEndPosArray[0]：运动终点Y轴位置
 *                          \n pEndPosArray[1]：运动终点Z轴位置
 * @param  dir              圆弧运动方向
 *                          \n 1：圆弧逆时针方向运动
 *                          \n -1 圆弧顺时针方向运动
 * @param  height           绝对值为螺旋线螺距高：符号代表螺旋线的垂直与圆的运动方向, 正号表示顺着法向量方向运动, 负号表示逆着法向量方向运动
 * @param  turn             圆弧运动的圈数, 参数范围：[0, 1e8]
 * @param  userID           用户标记索引号, 无范围要求
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x4125)
 *******************************************************************/
IMC_API IMC_CrdArcCenterYZPlane(short cardIndex, short crdNo, double* pCenterArray, double* pEndPosArray, short dir, double height = 0, int turn = 0, int userID = 0);

/**
 * @brief  给定圆弧圆心和圆弧末点, ZX平面内的圆弧插补
 * @attention pCenterArray, pEndPosArray 应传入长度为 2 的数组指针
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  crdNo            坐标系号, 参数范围：[0,7]
 * @param  pCenterArray     圆弧圆心坐标(相对于起点的位置增量)
 *                          \n pCenterArray[0]：圆弧圆心Z轴位置
 *                          \n pCenterArray[1]：圆弧圆心X轴位置
 * @param  pEndPosArray     插补运动终点位置
 *                          \n pEndPosArray[0]：运动终点Z轴位置
 *                          \n pEndPosArray[1]：运动终点X轴位置
 * @param  dir              圆弧运动方向
 *                          \n 1：圆弧逆时针方向运动
 *                          \n -1 圆弧顺时针方向运动
 * @param  height           绝对值为螺旋线螺距高：符号代表螺旋线的垂直与圆的运动方向, 正号表示顺着法向量方向运动, 负号表示逆着法向量方向运动
 * @param  turn             圆弧运动的圈数, 参数范围：[0, 1e8]
 * @param  userID           用户标记索引号, 无范围要求
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x4126)
 *******************************************************************/
IMC_API IMC_CrdArcCenterZXPlane(short cardIndex, short crdNo, double* pCenterArray, double* pEndPosArray, short dir, double height = 0, int turn = 0, int userID = 0);

/**
 * @brief  给定圆弧半径和圆弧末点, XY平面内的圆弧插补
 * @attention pEndPosArray 应传入长度为 2 的数组指针
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  crdNo            坐标系号, 参数范围：[0,7]
 * @param  radius           圆弧半径, 正号：优弧, 负号：劣弧
 * @param  pEndPosArray     插补运动终点位置
 *                          \n pEndPosArray[0]：运动终点X轴位置
 *                          \n pEndPosArray[1]：运动终点Y轴位置
 * @param  dir              圆弧运动方向
 *                          \n 1：圆弧逆时针方向运动
 *                          \n -1 圆弧顺时针方向运动
 * @param  height           绝对值为螺旋线螺距高：符号代表螺旋线的垂直与圆的运动方向, 正号表示顺着法向量方向运动, 负号表示逆着法向量方向运动
 * @param  turn             圆弧运动的圈数, 参数范围：[0, 1e8]
 * @param  userID           用户标记索引号, 无范围要求
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x4127)
 *******************************************************************/
IMC_API IMC_CrdArcRadiusXYPlane(short cardIndex, short crdNo, double radius, double* pEndPosArray, short dir, double height = 0, int turn = 0, int userID = 0);

/**
 * @brief  给定圆弧半径和圆弧末点, YZ平面内的圆弧插补
 * @attention pEndPosArray 应传入长度为 2 的数组指针
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  crdNo            坐标系号, 参数范围：[0,7]
 * @param  radius           圆弧半径, 正号：优弧, 负号：劣弧
 * @param  pEndPosArray     插补运动终点位置
 *                          \n pEndPosArray[0]：运动终点Y轴位置
 *                          \n pEndPosArray[1]：运动终点Z轴位置
 * @param  dir              圆弧运动方向
 *                          \n 1：圆弧逆时针方向运动
 *                          \n -1 圆弧顺时针方向运动
 * @param  height           绝对值为螺旋线螺距高：符号代表螺旋线的垂直与圆的运动方向, 正号表示顺着法向量方向运动, 负号表示逆着法向量方向运动
 * @param  turn             圆弧运动的圈数, 参数范围：[0, 1e8]
 * @param  userID           用户标记索引号, 无范围要求
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x4128)
 *******************************************************************/
IMC_API IMC_CrdArcRadiusYZPlane(short cardIndex, short crdNo, double radius, double* pEndPosArray, short dir, double height = 0, int turn = 0, int userID = 0);

/**
 * @brief  给定圆弧半径和圆弧末点, ZX平面内的圆弧插补
 * @attention pEndPosArray 应传入长度为 2 的数组指针
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  crdNo            坐标系号, 参数范围：[0,7]
 * @param  radius           圆弧半径, 正号：优弧, 负号：劣弧
 * @param  pEndPosArray     插补运动终点位置
 *                          \n pEndPosArray[0]：运动终点Z轴位置
 *                          \n pEndPosArray[1]：运动终点X轴位置
 * @param  dir              圆弧运动方向
 *                          \n 1：圆弧逆时针方向运动
 *                          \n -1 圆弧顺时针方向运动
 * @param  height           绝对值为螺旋线螺距高：符号代表螺旋线的垂直与圆的运动方向, 正号表示顺着法向量方向运动, 负号表示逆着法向量方向运动
 * @param  turn             圆弧运动的圈数, 参数范围：[0, 1e8]
 * @param  userID           用户标记索引号, 无范围要求
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x4129)
 *******************************************************************/
IMC_API IMC_CrdArcRadiusZXPlane(short cardIndex, short crdNo, double radius, double* pEndPosArray, short dir, double height = 0, int turn = 0, int userID = 0);

/**
 * @brief  给定圆弧圆心和和圆弧角度, XY平面内的圆弧插补
 * @attention pCenterArray 应传入长度为 2 的数组指针
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  crdNo            坐标系号, 参数范围：[0,7]
 * @param  pCenterArray     圆弧圆心坐标(相对于起点的位置增量)
 *                          \n pCenterArray[0]：圆弧圆心X轴位置
 *                          \n pCenterArray[1]：圆弧圆心Y轴位置
 * @param  angle            圆弧一共行走的角度, 参数范围：[-1e9, 1e9]单位(rad)
 * @param  height           绝对值为螺旋线螺距高：符号代表螺旋线的垂直与圆的运动方向, 正号表示顺着法向量方向运动, 负号表示逆着法向量方向运动
 * @param  userID           用户标记索引号, 无范围要求
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x412a)
 *******************************************************************/
IMC_API IMC_CrdArcAngleXYPlane(short cardIndex, short crdNo, double* pCenterArray, double angle, double height = 0, int userID = 0);

/**
 * @brief  给定圆弧圆心和和圆弧角度, YZ平面内的圆弧插补
 * @attention pCenterArray 应传入长度为 2 的数组指针
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  crdNo            坐标系号, 参数范围：[0,7]
 * @param  pCenterArray     圆弧圆心坐标(相对于起点的位置增量)
 *                          \n pCenterArray[0]：圆弧圆心Y轴位置
 *                          \n pCenterArray[1]：圆弧圆心Z轴位置
 * @param  angle            圆弧一共行走的角度, 参数范围：[-1e9, 1e9]单位(rad)
 * @param  height           绝对值为螺旋线螺距高：符号代表螺旋线的垂直与圆的运动方向, 正号表示顺着法向量方向运动, 负号表示逆着法向量方向运动
 * @param  userID           用户标记索引号, 无范围要求
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x412b)
 *******************************************************************/
IMC_API IMC_CrdArcAngleYZPlane(short cardIndex, short crdNo, double* pCenterArray, double angle, double height = 0, int userID = 0);

/**
 * @brief  给定圆弧圆心和和圆弧角度, ZX平面内的圆弧插补
 * @attention pCenterArray 应传入长度为 2 的数组指针
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  crdNo            坐标系号, 参数范围：[0,7]
 * @param  pCenterArray     圆弧圆心坐标(相对于起点的位置增量)
 *                          \n pCenterArray[0]：圆弧圆心Z轴位置
 *                          \n pCenterArray[1]：圆弧圆心X轴位置
 * @param  angle            圆弧一共行走的角度, 参数范围：[-1e9, 1e9]单位(rad)
 * @param  height           绝对值为螺旋线螺距高：符号代表螺旋线的垂直与圆的运动方向, 正号表示顺着法向量方向运动, 负号表示逆着法向量方向运动
 * @param  userID           用户标记索引号, 无范围要求
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x412c)
 *******************************************************************/
IMC_API IMC_CrdArcAngleZXPlane(short cardIndex, short crdNo, double* pCenterArray, double angle, double height = 0, int userID = 0);

/**
 * @brief  给定圆心, 末点法向量的三维涡旋线插补
 * @details 当 height 不为 0 的时候可衍生为涡旋线插补。当起点和终点重合的时候几何参数错误。
 *          \n 当法向量与圆所在平面夹角小于 60 度的时候几何参数错误。输入的法向量可不为单位向量
 * @attention pCenterArray, pEndPosArray, pNormalArray 应传入长度为 3 的数组指针
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  crdNo            坐标系号, 参数范围：[0,7]
 * @param  pCenterArray     涡旋线圆心坐标(相对于起点的位置增量)
 *                          \n pCenterArray[0]：涡旋线圆心X轴位置
 *                          \n pCenterArray[1]：涡旋线圆心Y轴位置
 *                          \n pCenterArray[2]：涡旋线圆心Z轴位置
 * @param  pEndPosArray     插补运动终点位置
 *                          \n pEndPosArray[0]：运动终点X轴位置
 *                          \n pEndPosArray[1]：运动终点Y轴位置
 *                          \n pEndPosArray[2]：运动终点Z轴位置
 * @param  pNormalArray     涡旋线的法向量, 涡旋线方向由法向量决定
 *                          \n pNormalArray[0]：涡旋线的法向量X轴分量
 *                          \n pNormalArray[1]：涡旋线的法向量Y轴分量
 *                          \n pNormalArray[2]：涡旋线的法向量Z轴分量
 * @param  height           绝对值为涡旋线螺距高：符号代表涡旋线的垂直与圆的运动方向, 正号表示顺着法向量方向运动, 负号表示逆着法向量方向运动
 * @param  turn             涡旋线运动的圈数, 参数范围：[0, 1e8]
 * @param  userID           用户标记索引号, 无范围要求
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x4140)
 *******************************************************************/
IMC_API IMC_Crd3DVortexCenterNormal(short cardIndex, short crdNo, double* pCenterArray, double* pEndPosArray, double* pNormalArray, double height = 0, int turn = 0, int userID = 0);

/**
 * @brief  给定涡旋线圆心和涡旋线末点, XY平面内的涡旋线插补
 * @attention pCenterArray, pEndPosArray 应传入长度为 2 的数组指针
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  crdNo            坐标系号, 参数范围：[0,7]
 * @param  pCenterArray     涡旋线圆心坐标(相对于起点的位置增量)
 *                          \n pCenterArray[0]：涡旋线圆心X轴位置
 *                          \n pCenterArray[1]：涡旋线圆心Y轴位置
 * @param  pEndPosArray     插补运动终点位置
 *                          \n pEndPosArray[0]：运动终点X轴位置
 *                          \n pEndPosArray[1]：运动终点Y轴位置
 * @param  dir              涡旋线运动方向
 *                          \n 1：涡旋线逆时针方向运动
 *                          \n -1 涡旋线顺时针方向运动
 * @param  height           绝对值为涡旋线螺距高：符号代表涡旋线的垂直与圆的运动方向, 正号表示顺着法向量方向运动, 负号表示逆着法向量方向运动
 * @param  turn             涡旋线运动的圈数, 参数范围：[0, 1e8]
 * @param  userID           用户标记索引号, 无范围要求
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x4141)
 *******************************************************************/
IMC_API IMC_CrdVortexCenterXYPlane(short cardIndex, short crdNo, double* pCenterArray, double* pEndPosArray, short dir, double height = 0, int turn = 0, int userID = 0);

/**
 * @brief  给定涡旋线圆心和涡旋线末点, YZ平面内的涡旋线插补
 * @attention pCenterArray, pEndPosArray 应传入长度为 2 的数组指针
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  crdNo            坐标系号, 参数范围：[0,7]
 * @param  pCenterArray     涡旋线圆心坐标(相对于起点的位置增量)
 *                          \n pCenterArray[0]：涡旋线圆心Y轴位置
 *                          \n pCenterArray[1]：涡旋线圆心Z轴位置
 * @param  pEndPosArray     插补运动终点位置
 *                          \n pEndPosArray[0]：运动终点Y轴位置
 *                          \n pEndPosArray[1]：运动终点Z轴位置
 * @param  dir              涡旋线运动方向
 *                          \n 1：涡旋线逆时针方向运动
 *                          \n -1 涡旋线顺时针方向运动
 * @param  height           绝对值为涡旋线螺距高：符号代表涡旋线的垂直与圆的运动方向, 正号表示顺着法向量方向运动, 负号表示逆着法向量方向运动
 * @param  turn             涡旋线运动的圈数, 参数范围：[0, 1e8]
 * @param  userID           用户标记索引号, 无范围要求
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x4142)
 *******************************************************************/
IMC_API IMC_CrdVortexCenterYZPlane(short cardIndex, short crdNo, double* pCenterArray, double* pEndPosArray, short dir, double height = 0, int turn = 0, int userID = 0);

/**
 * @brief  给定涡旋线圆心和涡旋线末点, ZX平面内的涡旋线插补
 * @attention pCenterArray, pEndPosArray 应传入长度为 2 的数组指针
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  crdNo            坐标系号, 参数范围：[0,7]
 * @param  pCenterArray     涡旋线圆心坐标(相对于起点的位置增量)
 *                          \n pCenterArray[0]：涡旋线圆心Z轴位置
 *                          \n pCenterArray[1]：涡旋线圆心X轴位置
 * @param  pEndPosArray     插补运动终点位置
 *                          \n pEndPosArray[0]：运动终点Z轴位置
 *                          \n pEndPosArray[1]：运动终点X轴位置
 * @param  dir              涡旋线运动方向
 *                          \n 1：涡旋线逆时针方向运动
 *                          \n -1 涡旋线顺时针方向运动
 * @param  height           绝对值为涡旋线螺距高：符号代表涡旋线的垂直与圆的运动方向, 正号表示顺着法向量方向运动, 负号表示逆着法向量方向运动
 * @param  turn             涡旋线运动的圈数, 参数范围：[0, 1e8]
 * @param  userID           用户标记索引号, 无范围要求
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x4143)
 *******************************************************************/
IMC_API IMC_CrdVortexCenterZXPlane(short cardIndex, short crdNo, double* pCenterArray, double* pEndPosArray, short dir, double height = 0, int turn = 0, int userID = 0);

/**
 * @brief  插补缓冲区等待延时
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  crdNo            坐标系号, 参数范围：[0,7]
 * @param  waitTime			等待时间, 参数范围：[0,30000],单位(ms) ,0 表示无限等待
 * @param  userID           用户标记索引号, 无范围要求
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x4150)
 *******************************************************************/
IMC_API IMC_CrdWaitTime(short cardIndex, short crdNo, int waitTime, int userID = 0);

/**
 * @brief  插补缓冲区等待 DI,默认超时30s
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  crdNo            坐标系号, 参数范围：[0,7]
 * @param  diNO             等待 DI 的端口号
 * @param  diType           等待 DI 的类型：0 ：EtherCAT 总线模块 DI, 1：端子板 DI
 * @param  diLevel          等待 DI 的电平, 0 低电平, 1 高电平
 * @param  userID           用户标记索引号, 无范围要求
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x4151)
 *******************************************************************/
IMC_API IMC_CrdWaitDI(short cardIndex, short crdNo, short diNO, short diType, short diLevel, int userID = 0);

/**
 * @brief  插补缓冲区等待 DI
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  crdNo            坐标系号, 参数范围：[0,7]
 * @param  diNO             等待 DI 的端口号
 * @param  diType           等待 DI 的类型：0 ：EtherCAT 总线模块 DI, 1：端子板 DI
 * @param  diLevel          等待 DI 的电平, 0 低电平, 1 高电平
 * @param  waitTime			等待时间, 参数范围：[0,30000],单位(ms) ,0 表示无限等待
 * @param  userID           用户标记索引号, 无范围要求
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x415b)
 *******************************************************************/
IMC_API IMC_CrdWaitTimeDI(short cardIndex, short crdNo, short diNO, short diType, short diLevel, int waitTime, int userID = 0);

/**
 * @brief  插补缓冲区输出 DO
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  crdNo            坐标系号, 参数范围：[0,7]
 * @param  doNO             等待 DO 的端口号
 * @param  doType           等待 DO 的类型：0 ：EtherCAT 总线模块 DO, 1：端子板 DO
 * @param  doLevel          等待 DO 的电平, 0 低电平, 1 高电平
 * @param  userID           用户标记索引号, 无范围要求
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x4152)
 *******************************************************************/
IMC_API IMC_CrdSetDO(short cardIndex, short crdNo, short doNO, short doType, short doLevel, int userID = 0);

/**
 * @brief  插补缓冲区指令, 执行到该段时, 以该段为起始, 在 waitPos 距离处输出指定的 DO 信号
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  crdNo            坐标系号, 参数范围：[0,7]
 * @param  waitPos          指定的距离位置
 *                          \n 正数：表示当前位置之后的 waitPos 处
 *                          \n 负数：表示当前位置之前的 waitPos 处
 * @param  doNO             输出 DO 的端口号
 * @param  doType           输出 DO 的类型：0 ：EtherCAT 总线模块 DO, 1：端子板 DO
 * @param  doLevel          输出 DO 的电平, 0 低电平, 1 高电平
 * @param  userID           用户标记索引号, 无范围要求
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x4153)
 *******************************************************************/
IMC_API IMC_CrdSetDistanceDO(short cardIndex, short crdNo, double waitPos, short doNO, short doType, short doLevel, int userID = 0);

/**
 * @brief  插补缓冲区指令, 执行到该段时, 以该段为起始, 延时 waitPeriod 个周期, 输出指定的 DO 信号
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  crdNo            坐标系号, 参数范围：[0,7]
 * @param  waitTime			等待时间, 参数范围：[0,4000], 单位(ms)
 * @param  doNO             输出 DO 的端口号
 * @param  doType           输出 DO 的类型：0 ：EtherCAT 总线模块 DI, 1：端子板 DI
 * @param  doLevel          输出 DO 的电平, 0 低电平, 1 高电平
 * @param  userID           用户标记索引号, 无范围要求
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x4154)
 *******************************************************************/
IMC_API IMC_CrdSetTimeDO(short cardIndex, short crdNo, int waitTime, short doNO, short doType, short doLevel, int userID = 0);

/**
 * @brief  插补缓冲区启动插补轴以外的轴 PTP 运动
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  crdNo            坐标系号, 参数范围：[0,7]
 * @param  axNo             插补轴之外需要 PTP 运动的轴号
 * @param  tgtPos           目标位置
 * @param  tgtVel           目标速度
 * @param  acc              运行加速度
 * @param  mvType           指示目标位置的类型, 0 绝对位置, 1 相对位置
 * @param  waitFlag         是否等待 PTP 完成后继续执行下一条指令。0, 不等待, 1 等待
 * @param  userID           用户标记索引号, 无范围要求
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x4155)
 *******************************************************************/
IMC_API IMC_CrdPTPMove(short cardIndex, short crdNo, short axNo, double tgtPos, double tgtVel, double acc, short mvType, short waitFlag, int userID = 0);

/**
 * @brief  插补缓冲区中启动插补轴之外的轴同步运动(相对位移)
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  crdNo            坐标系号, 参数范围：[0,7]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  syncPos          同步运动的相对位移, 相对于该轴当前位置需要运动的位移量
 * @param  userID           用户标记索引号, 无范围要求
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x4156)
 *******************************************************************/
IMC_API IMC_CrdSyncMove(short cardIndex, short crdNo, short axNo, double syncPos, int userID = 0);

/**
 * @brief  插补缓冲区中启动插补轴之外的轴同步运动(绝对位移)
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  crdNo            坐标系号, 参数范围：[0,7]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  syncTgtPos       同步运动的绝对位移
 * @param  userID           用户标记索引号, 无范围要求
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x4157)
 *******************************************************************/
IMC_API IMC_CrdSyncMoveAbs(short cardIndex, short crdNo, short axNo, double syncTgtPos, int userID = 0);

/**
 * @brief  插补缓冲区等待上一条运动指令到位，默认超时时间5s
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  crdNo            坐标系号, 参数范围：[0,7]
 * @param  userID           用户标记索引号, 无范围要求
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x4158)
 *******************************************************************/
IMC_API IMC_CrdWaitInPos(short cardIndex, short crdNo, int userID = 0);

/**
 * @brief  缓冲区插补轴外的多轴（两轴）同步运动
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  crdNo            坐标系号, 参数范围：[0,7]
 * @param  axNum            同步轴数量, 目前仅支持 2
 * @param  pAxNo            指定轴号, 仅支持两轴, pAxNo[0]、pAxNo[1]        分别为同步轴号。范围不包含插补轴
 * @param  pTgtPos          指令轴的目标相对位置, 仅支持两轴, pTgtPos[0]、pTgtPos[1]        分别为同步轴相对位置
 * @param  tgtVel           同步轴的合成速度
 * @param  tgtAcc           同步轴的合成加速度
 * @param  contiFlag        为 1 表示两轴同步运动与下一条同步运动合成速度连续, 为 0 表示两轴同步段末速度降为 0
 * @param  userID           用户标记索引号, 无范围要求
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x4159)
 *******************************************************************/
IMC_API IMC_CrdMultiSyncMove(short cardIndex, short crdNo, short axNum, short* pAxNo, double* pTgtPos, double tgtVel, double tgtAcc, short contiFlag, int userID = 0);

/**
 * @brief  插补缓存运动停止
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  crdNo            坐标系号, 参数范围：[0,7]
 * @param  userID           用户标记索引号, 无范围要求
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x415a)
 *******************************************************************/
IMC_API IMC_CrdBufStop(short cardIndex, short crdNo, int userID = 0);

/**
 * @brief  插补缓存区设置全局变量
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  crdNo            坐标系号, 参数范围：[0,7]
 * @param  index            全局变量索引, 参数范围：[0,15]
 * @param  value            设置值, 参数范围：[intMin,intMax]
 * @param  userID           用户标记索引号, 无范围要求
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x415b)
 *******************************************************************/
IMC_API IMC_CrdSetUserVal(short cardIndex, short crdNo, short index, int value, int userID = 0);

/**
 * @brief  插补缓存区等待全局变量
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  crdNo            坐标系号, 参数范围：[0,7]
 * @param  index            全局变量索引, 参数范围：[0,15]
 * @param  condition        等待条件, bit0为等于 bit1为不等于 bit2为小于 bit3为大于;如 "大于等于" condition = (1<<0 |1<<3) = 0x09
 * @param  compareValue     比较值, 与等待条件、全局变量构成判断条件,当条件为真时,当前段执行完成
 * @param  waitTime         等待时间, 参数范围：[0,30000],等待超时时间(ms),为0表示无限等待
 * @param  userID           用户标记索引号, 无范围要求
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x415c)
 *******************************************************************/
IMC_API IMC_CrdWaitUserVal(short cardIndex, short crdNo, short index, short condition, int compareValue, int waitTime, int userID = 0);

/**
 * @brief  插补缓存区输出DA
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  crdNo            坐标系号, 参数范围：[0,7]
 * @param  index            输出DA 的端口号
 * @param  value            DA输出值
 * @param  userID           用户标记索引号, 无范围要求
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x415e)
 *******************************************************************/
IMC_API IMC_CrdSetEcatDA(short cardIndex, short crdNo, short index, short value, int userID = 0);

/**
 * @brief  一次性往 DSP 队列发送 PC 部分的队列数据
 * @details 每次发送若干条, 如果完全发送完成, *pIsFinished 标 识记为 1
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  crdNo            坐标系号, 参数范围：[0,7]
 * @param  pIsFinished      发送完成标识, 1 表示发送完成。0 表示 PC 队列中仍有数据
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x4200)
 *******************************************************************/
IMC_API IMC_CrdEndData(short cardIndex, short crdNo, short* pIsFinished);

/**
 * @brief  插补运动使能函数, 调用此函数后, 会开始插补运动。如果停止插补运动, 需要调用停止运动函数 进行去插补使能
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  crdNo            坐标系号, 参数范围：[0,7]
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x4201)
 *******************************************************************/
IMC_API IMC_CrdStart(short cardIndex, short crdNo);

/**
 * @brief  多个插补运动使能函数, 调用此函数后, 会开始插补运动。如果停止插补运动, 需要调用停止运动函数 进行去插补使能
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  pCrdNo           坐标系号, 参数范围：[0,7]
 * @param  count              坐标系号, 参数范围：[0,8]
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x4207)
 *******************************************************************/
IMC_API IMC_CrdMultiStart(short cardIndex, short* pCrdNo, short count);

/**
 * @brief  调用此函数进行插补停止, 用户可以选择正常平滑停止和急停。
 * @details 正常平滑停止按照用户设定的轨迹加速度进行减速停止。急停是利用用户在建立坐标系设定的急停加速度进行停止, 停止后系统不会清空队列, 需要用户自己清空队列。
 * @attention 急停停止后, 插补会报出急停错误号, 需要手动清除此错误才能再次运行
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  crdNo            坐标系号, 参数范围：[0,7]
 * @param  stopType         停止类型：0 正常停止 1 急停停止
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x4202)
 *******************************************************************/
IMC_API IMC_CrdStop(short cardIndex, short crdNo, short stopType);

/**
 * @brief  清除缓存区压入曲线数据
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  crdNo            坐标系号, 参数范围：[0,7]
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x4203)
 *******************************************************************/
IMC_API IMC_CrdClrData(short cardIndex, short crdNo);

/**
 * @brief  清除插补错误号。可以清除 IMC_CrdGetStatus 获取到的故障号
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  crdNo            坐标系号, 参数范围：[0,7]
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x4204)
 *******************************************************************/
IMC_API IMC_CrdClrError(short cardIndex, short crdNo);

/**
 * @brief  插补倍率设定函数。插补实际进给速度 = 用户插补进给速度 × 倍率值。
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  crdNo            坐标系号, 参数范围：[0,7]
 * @param  ratio            插补倍率。参数范围 [0, 10]
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x4205)
 *******************************************************************/
IMC_API IMC_CrdSetRatio(short cardIndex, short crdNo, double ratio);

/**
 * @brief  插补倍率获取函数
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  crdNo            坐标系号, 参数范围：[0,7]
 * @param  pRatio           插补倍率。参数范围 [0, 10]
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x4206)
 *******************************************************************/
IMC_API IMC_CrdGetRatio(short cardIndex, short crdNo, double* pRatio);

/**
 * @brief  插补状态获取。bit0-bit7 是插补运行状态。bi8-bit15 是插补故障状态
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  crdNo            坐标系号, 参数范围：[0,7]
 * @param  pStatus          获取的状态
 *                          \n bit0-bit7：0 表示静止状态 1 表示运行状态
 *                          \n bi8-bit15（以下为 10 进制表示）：100：插补运行错误, 特指 DSP 插补断流
 *                          \n 101：插补急停
 *                          \n 102：逻辑故障
 *                          \n 103：同步运动轴错误
 *                          \n 1、超出 16 个轴限制
 *                          \n 2、同步轴在同步前处于运动状态
 *                          \n 3、同步轴号为插 补轴号
 *                          \n 4、同步轴在运动中出现轴报警或限位报警
 *                          \n 5、同步轴的速度超出了设定轴的最大速度
 *                          \n 104：PTP 启动不满足条件错误
 *                          \n 105：等待超时
 *                          \n 106：跟随误差报警
 *                          \n 107：插补D0同时处理数量达到上限
 *                          \n 108：超前DO距离事件没有数据
 *                          \n 109：插补运动过程中,两轴同步指令轴出现报警或限位
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x4230)
 *******************************************************************/
IMC_API IMC_CrdGetStatus(short cardIndex, short crdNo, short* pStatus);

/**
 * @brief  获取插补运动执行缓冲区最后一段的运动状态
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  crdNo            坐标系号, 参数范围：[0,7]
 * @param  pSts             到位状态 1：到位 0：没到位
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x4231)
 *******************************************************************/
IMC_API IMC_CrdGetArrivalSts(short cardIndex, short crdNo, short* pSts);

/**
 * @brief  获取插补目标位置。目标位置指的是最后压入曲线的末点位置
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  crdNo            坐标系号, 参数范围：[0,7]
 * @param  pPosArray        获取到的目标位置
 *                          \n pPosArray[0]：X轴目标位置
 *                          \n pPosArray[1]：Y轴目标位置
 *                          \n pPosArray[2]：Z轴目标位置
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x4232)
 *******************************************************************/
IMC_API IMC_CrdGetTargetPos(short cardIndex, short crdNo, double* pPosArray);

/**
 * @brief  获取插补暂停运动的位置。该位置一般用于当用户使用单轴移动过插补轴后, 需要返回继续加工时使用
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  crdNo            坐标系号, 参数范围：[0,7]
 * @param  pPosArray        获取到的暂停位置
 *                          \n pPosArray[0]：X轴暂停位置
 *                          \n pPosArray[1]：Y轴暂停位置
 *                          \n pPosArray[2]：Z轴暂停位置
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x4233)
 *******************************************************************/
IMC_API IMC_CrdGetPausePos(short cardIndex, short crdNo, double* pPosArray);

/**
 * @brief  当前插补坐标系坐标读取函数
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  crdNo            坐标系号, 参数范围：[0,7]
 * @param  pCrdPosArray     获取到的当前位置
 *                          \n pCrdPosArray[0]：X轴当前位置
 *                          \n pCrdPosArray[1]：Y轴当前位置
 *                          \n pCrdPosArray[2]：Z轴当前位置
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x4234)
 *******************************************************************/
IMC_API IMC_CrdGetPos(short cardIndex, short crdNo, double* pCrdPosArray);

/**
 * @brief  插补轨迹速度读取函数
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  crdNo            坐标系号, 参数范围：[0,7]
 * @param  pCrdVel          获取到的当前速度
 *                          \n pCrdVel[0]：X轴当前速度
 *                          \n pCrdVel[1]：Y轴当前速度
 *                          \n pCrdVel[2]：Z轴当前速度
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x4235)
 *******************************************************************/
IMC_API IMC_CrdGetVel(short cardIndex, short crdNo, double* pCrdVel);

/**
 * @brief  插补当前运动曲线用户索引获取
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  crdNo            坐标系号, 参数范围：[0,7]
 * @param  pUserID          获取到的用户标记索引号
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x4236)
 *******************************************************************/
IMC_API IMC_CrdGetUserID(short cardIndex, short crdNo, int* pUserID);

/**
 * @brief  获取队列当前的剩余余量
 * @attention 开启前瞻过渡模式后，插补段的衔接处增加的过渡段数据会占用一定插补队列空间
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  crdNo            坐标系号, 参数范围：[0,7]
 * @param  pSpace           获取的队列余量
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x4237)
 *******************************************************************/
IMC_API IMC_CrdGetSpace(short cardIndex, short crdNo, int* pSpace);

/**
 * @brief  获取 CPU 缓存队列当前的剩余空间
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  crdNo            坐标系号, 参数范围：[0,7]
 * @param  pSpace           获取的剩余空间
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x4238)
 *******************************************************************/
IMC_API IMC_CrdGetBufSpace(short cardIndex, short crdNo, int* pSpace);

/**
 * @brief  获取前瞻缓存队列当前的剩余空间
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  crdNo            坐标系号, 参数范围：[0,7]
 * @param  pSpace           获取的剩余空间
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x4239)
 *******************************************************************/
IMC_API IMC_CrdGetLookAheadSpace(short cardIndex, short crdNo, int* pSpace);

/**
 * @brief  插补缓冲区模式设置
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  crdNo            坐标系号, 参数范围：[0,7]
 * @param  bufferMode       插补缓冲区模式
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x423a)
 *******************************************************************/
IMC_API IMC_CrdSetBufferMode(short cardIndex, short crdNo, short bufferMode);

/**
 * @brief  插补缓冲区模式获取
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  crdNo            坐标系号, 参数范围：[0,7]
 * @param  pBufferMode      插补缓冲区模式
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x423b)
 *******************************************************************/
IMC_API IMC_CrdGetBufferMode(short cardIndex, short crdNo, short* pBufferMode);

/**
 * @brief  插补数据恢复
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  crdNo            坐标系号, 参数范围：[0,7]
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x423c)
 *******************************************************************/
IMC_API IMC_CrdSetDataRestore(short cardIndex, short crdNo);

/**
 * @brief  获取总插补时间及剩余时间
 * @details 需要配合IMC_CrdEnablePrfTimeCalc开启插补时间计算模块, 坐标系销毁、清除缓冲区数据(IMC_CrdClrData)时，总插补时间清零
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  crdNo            坐标系号, 参数范围：[0,7]
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x423c)
 *******************************************************************/
IMC_API IMC_CrdGetPrfTime(short cardIndex, short crdNo, double* pTotalTime, double* pRemainTime);
/// @}

/// @defgroup MultiCrd 板卡MultiCrd运动模式相关接口
/// @brief 多轴插补运动模式相关接口
/// @{

/**
 * @brief  建立多轴插补系统
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  groupNo          多轴组号, 参数范围：[0,3]
 * @param  pAxNo            多轴系统轴映射值, 参数范围 [0, 最大支持轴数）
 * @param  maxAxNum         多轴插补系统轴数, 参数范围 [2, 16]
 * @retval #EXE_SUCCESS     指令执行成功
 * @par 指令码              (0x4300)
 *******************************************************************/
IMC_API IMC_MultiSetupSys(short cardIndex, short groupNo, short* pAxNo, short maxAxNum);

/**
 * @brief  销毁多轴插补系统
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  groupNo          多轴组号, 参数范围：[0,3]
 * @retval #EXE_SUCCESS     指令执行成功
 * @par 指令码              (0x4301)
 *******************************************************************/
IMC_API IMC_MultiDeleteSys(short cardIndex, short groupNo);

/**
 * @brief  启动多轴插补运动
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  groupNo          多轴组号, 参数范围：[0,3]
 * @retval #EXE_SUCCESS     指令执行成功
 * @par 指令码              (0x4302)
 *******************************************************************/
IMC_API IMC_MultiStart(short cardIndex, short groupNo);

/**
 * @brief  停止多轴插补运动
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  groupNo          多轴组号, 参数范围：[0,3]
 * @param  stopType         停止类型：0 正常停止 1 急停停止
 * @retval #EXE_SUCCESS     指令执行成功
 * @par 指令码              (0x4303)
 *******************************************************************/
IMC_API IMC_MultiStop(short cardIndex, short groupNo, short stopType);

/**
 * @brief  获取多轴插补数据剩余空间
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  groupNo          多轴组号, 参数范围：[0,3]
 * @param  pSpace           获取的数据队列剩余空间
 * @retval #EXE_SUCCESS     指令执行成功
 * @par 指令码              (0x4322)
 *******************************************************************/
IMC_API IMC_MultiGetSpace(short cardIndex, short groupNo, int* pSpace);

/**
 * @brief  清除多轴插补系统错误状态
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  groupNo          多轴组号, 参数范围：[0,3]
 * @retval #EXE_SUCCESS     指令执行成功
 * @par 指令码              (0x4314)
 *******************************************************************/
IMC_API IMC_MultiClrData(short cardIndex, short groupNo);

/**
 * @brief  清除多轴插补数据队列
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  groupNo          多轴组号, 参数范围：[0,3]
 * @retval #EXE_SUCCESS     指令执行成功
 * @par 指令码              (0x4315)
 *******************************************************************/
IMC_API IMC_MultiClrError(short cardIndex, short groupNo);

/**
 * @brief  设置多轴插补位置类型
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  groupNo          多轴组号, 参数范围：[0,3]
 * @param  posType          位置类型：0 绝对位置 1 相对位置
 * @retval #EXE_SUCCESS     指令执行成功
 * @par 指令码              (0x4310)
 *******************************************************************/
IMC_API IMC_MultiSetPosType(short cardIndex, short groupNo, short posType);

/**
 * @brief  获取多轴插补位置类型
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  groupNo          多轴组号, 参数范围：[0,3]
 * @param  pPosType         获取的位置类型：0 绝对位置 1 相对位置
 * @retval #EXE_SUCCESS     指令执行成功
 * @par 指令码              (0x4311)
 *******************************************************************/
IMC_API IMC_MultiGetPosType(short cardIndex, short groupNo, short* pPosType);

/**
 * @brief  获取多轴插补当前段UserID
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  groupNo          多轴组号, 参数范围：[0,3]
 * @param  pUserID          获取的当前段UserID
 * @retval #EXE_SUCCESS     指令执行成功
 * @par 指令码              (0x4323)
 *******************************************************************/
IMC_API IMC_MultiGetUserID(short cardIndex, short groupNo, int* pUserID);

/**
 * @brief  获取多轴插补系统运行状态
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  groupNo          多轴组号, 参数范围：[0,3]
 * @param  pSts             获取的多轴插补系统运行状态 0-停止 1-运行
 * @param  pErrocode        获取的多轴插补系统运行错误码：
 *                          \n 1: 急停错误
 *                          \n 2: 位置错误
 *                          \n 3: DI等待超时
 *                          \n 4: 延时DO数量超限
 * @retval #EXE_SUCCESS     指令执行成功
 * @par 指令码              (0x4320)
 *******************************************************************/
IMC_API IMC_MultiGetSts(short cardIndex, short groupNo, short* pSts, short* pErrocode);

/**
 * @brief  获取多轴插补运动到位状态
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  groupNo          多轴组号, 参数范围：[0,3]
 * @param  pArrivalSts      获取的运动到位状态 0-运动未完成 1-运动完成
 * @retval #EXE_SUCCESS     指令执行成功
 * @par 指令码              (0x4321)
 *******************************************************************/
IMC_API IMC_MultiGetArrivalSts(short cardIndex, short groupNo, short* pArrivalSts);

/**
 * @brief  设置多轴插补系统运动速度倍率
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  groupNo          多轴组号, 参数范围：[0,3]
 * @param  ratio            设置的速度倍率值, 参数范围：[0.0,1.0]
 * @retval #EXE_SUCCESS     指令执行成功
 * @par 指令码              (0x4312)
 *******************************************************************/
IMC_API IMC_MultiSetRatio(short cardIndex, short groupNo, double ratio);

/**
 * @brief  获取多轴插补系统运动速度倍率
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  groupNo          多轴组号, 参数范围：[0,3]
 * @param  pRatio           获取的速度倍率值, 参数范围：[0.0,1.0]
 * @retval #EXE_SUCCESS     指令执行成功
 * @par 指令码              (0x4313)
 *******************************************************************/
IMC_API IMC_MultiGetRatio(short cardIndex, short groupNo, double* pRatio);

/**
 * @brief  获取多轴插补运动轨迹位置
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  groupNo          多轴组号, 参数范围：[0,3]
 * @param  pPos             获取到的轨迹位置值,按照轴号序列依次排列
 * @retval #EXE_SUCCESS     指令执行成功
 * @par 指令码              (0x4324)
 *******************************************************************/
IMC_API IMC_MultiGetTrajPos(short cardIndex, short groupNo, double* pPos);

/**
 * @brief  获取多轴插补运动轨迹速度
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  groupNo          多轴组号, 参数范围：[0,3]
 * @param  pVel             获取到的轨迹速度值
 * @retval #EXE_SUCCESS     指令执行成功
 * @par 指令码              (0x4325)
 *******************************************************************/
IMC_API IMC_MultiGetTrajVel(short cardIndex, short groupNo, double* pVel);

/**
 * @brief  多轴插补直线运动
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  groupNo          多轴组号, 参数范围：[0,3]
 * @param  pEndPos          目标终点位置序列,按照多轴系统的轴号序列依次设置
 * @param  trajVel          目标速度
 * @param  trajAcc          加速度
 * @param  trajDec          减速度
 * @param  blendType        过渡类型：0 叠加过渡  1 直接过渡
 * @param  blendRatio       过渡系数, 参数范围：[0.0,1.0]
 * @param  userID           当前段的userID
 * @retval #EXE_SUCCESS     指令执行成功
 * @par 指令码              (0x4330)
 *******************************************************************/
IMC_API IMC_MultiLineMove(short cardIndex, short groupNo, double* pEndPos, double trajVel, double trajAcc, double trajDec, short blendType, double blendRatio, int userID = 0);

/**
 * @brief  多轴插补等待事件
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  groupNo          多轴组号, 参数范围：[0,3]
 * @param  waitTime         等待时间, 参数范围：[1,30000] 单位ms
 * @param  userID           当前段的userID
 * @retval #EXE_SUCCESS     指令执行成功
 * @par 指令码              (0x4341)
 *******************************************************************/
IMC_API IMC_MultiWaitTime(short cardIndex, short groupNo, int waitTime, int userID);

/**
 * @brief  多轴插补等待DI输入事件
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  groupNo          多轴组号, 参数范围：[0,3]
 * @param  diIndex          输入DI信号索引, 参数范围：EcatDI[0,2047], LocalDI[0,7]
 * @param  diType           输入DI信号类型：0 EcatDI 1 LocalBusDI
 * @param  diLevel          输入DI信号电平：0 低电平 1 高电平
 * @param  waitTime         等待时间, 参数范围：[0,30000] 单位ms 设置为0，无限等待
 * @param  userID           当前段的userID
 * @retval #EXE_SUCCESS     指令执行成功
 * @par 指令码              (0x4332)
 *******************************************************************/
IMC_API IMC_MultiWaitDI(short cardIndex, short groupNo, short diIndex, short diType, short diLevel, int waitTime, int userID);

/**
 * @brief  多轴插补设置DO输出事件
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  groupNo          多轴组号, 参数范围：[0,3]
 * @param  doIndex          输出DO信号索引, 参数范围：EcatDO[0,2047], LocalDO[0,7]
 * @param  doType           输出DO信号类型：0 EcatDO 1 LocalBusDO
 * @param  doLevel          输出DO信号电平：0 低电平 1 高电平
 * @param  userID           当前段的userID
 * @retval #EXE_SUCCESS     指令执行成功
 * @par 指令码              (0x4333)
 *******************************************************************/
IMC_API IMC_MultiSetDO(short cardIndex, short groupNo, short doIndex, short doType, short doLevel, int userID);

/**
 * @brief  多轴插补设置翻转DO输出事件
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  groupNo          多轴组号, 参数范围：[0,3]
 * @param  doIndex          输出DO信号索引, 参数范围：EcatDO[0,2047], LocalDO[0,7]
 * @param  doType           输出DO信号类型：0 EcatDO 1 LocalBusDO
 * @param  doLevel          输出DO信号电平：0 低电平 1 高电平
 * @param  waitTime         翻转等待时间, 参数范围：[0,30000] 单位ms
 * @param  userID           当前段的userID
 * @retval #EXE_SUCCESS     指令执行成功
 * @par 指令码              (0x4335)
 *******************************************************************/
IMC_API IMC_MultiSetReverseDO(short cardIndex, short groupNo, short doIndex, short doType, short doLevel, int waitTime, int userID);
/// @}

/// @defgroup BandPt 板卡Pt运动模式相关接口
/// @brief 缓冲区插补运动模式相关接口
/// @{
/**
 * @brief  建立多轴捆绑 PT 运行系统
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  sysNo            PT系统号, 参数范围：[0,5]
 * @param  pMaskAxNoArray   建立PT系统包含的轴号，按数组排列
 * @param  maxAxNum         PT系统的轴数, 参数范围：[0,16]
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x5000)
 *******************************************************************/
IMC_API IMC_SetupPtPackSys(short cardIndex, short sysNo, short* pMaskAxNoArray, short maxAxNum);

/**
 * @brief  销毁捆绑 PT 运动系统
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  sysNo            PT系统号, 参数范围：[0,5]
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x5001)
 *******************************************************************/
IMC_API IMC_DeletePtPackSys(short cardIndex, short sysNo);

/**
 * @brief  设置捆绑 PT 运动位置的编程类型
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  sysNo            PT系统号, 参数范围：[0,5]
 * @param  incMode          0：绝对位置编程 1：相对位置编程
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x5010)
 *******************************************************************/
IMC_API IMC_SetPtPackIncMode(short cardIndex, short sysNo, short incMode);

/**
 * @brief  获取捆绑 PT 运动位置的编程类型
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  sysNo            PT系统号, 参数范围：[0,5]
 * @param  pIncMode         0：绝对位置编程 1：相对位置编程
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x5011)
 *******************************************************************/
IMC_API IMC_GetPtPackIncMode(short cardIndex, short sysNo, short* pIncMode);

/**
 * @brief  使能捆绑 PT 运动数据断流保护。当使能后, 根据缓冲区是否有数据, 同时判断各轴的速度是否大 于设定的阈值。
 * @details 如果系统中有任意一个轴的速度大于了设定的阈值, 则进行断流保护, 按照设定的 平滑停加速度进行停止。
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  sysNo            PT系统号, 参数范围：[0,5]
 * @param  pThresholdVelArray    系统各轴对应的速度阈值
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x5012)
 *******************************************************************/
IMC_API IMC_EnablePtPackNoDataProtect(short cardIndex, short sysNo, double* pThresholdVelArray);

/**
 * @brief  取消捆绑 PT 运动数据断流保护
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  sysNo            PT系统号, 参数范围：[0,5]
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x5013)
 *******************************************************************/
IMC_API IMC_DisablePtPackNoDataProtect(short cardIndex, short sysNo);

/**
 * @brief  获取捆绑 PT 运动数据断流保护的设置状态
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  sysNo            PT系统号, 参数范围：[0,5]
 * @param  pEnSts           保护使能状态
 * @param  pThresholdVelArray    各轴的保护速度阈值, 该参数为数组首地址
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x5014)
 *******************************************************************/
IMC_API IMC_GetPtPackNoDataProtectStatus(short cardIndex, short sysNo, short* pEnSts, double* pThresholdVelArray);

/**
 * @brief  添加捆绑 PT 运动数据
 * @details 绑定PT缓冲区最大支持4096个数据, 单条指令最多可下发256个数据
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  sysNo            PT系统号, 参数范围：[0,5]
 * @param  pPosArray        建立系统中 pAxArray[0]~ pAxArray[N] 所对应的位置。
 * @param  pTypeArray       每个数据的类型, 0 起点速度连续模式 1 匀速模式 2 末速度为0  的模式。
 * @param  T                每个数据点执行的时间, 单位：毫秒
 * @param  dataNum          数据的个数。
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x5030)
 *******************************************************************/
IMC_API IMC_AddMotionPointPtPack(short cardIndex, short sysNo, double* pPosArray, short* pTypeArray, double T, short dataNum);

/**
 * @brief  添加捆绑 PT 中的 DO 输出事件
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  sysNo            PT系统号, 参数范围：[0,5]
 * @param  doNo             DO 输出的端口号
 * @param  doType           DO 输出的类型：0：EtherCAT 类型 DO 1：本地端子板 DO
 * @param  doLevel          DO 输出电平 0：低电平 1：高电平
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x5031)
 *******************************************************************/
IMC_API IMC_AddDoPointPtPack(short cardIndex, short sysNo, short doNo, short doType, short doLevel);

/**
 * @brief  启动捆绑 PT 运动
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  sysNo            PT系统号, 参数范围：[0,5]
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x5040)
 *******************************************************************/
IMC_API IMC_StartPtPack(short cardIndex, short sysNo);

/**
 * @brief  停止捆绑 PT 运动
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  sysNo            PT系统号, 参数范围：[0,5]
 * @param  type             停止类型 0：平滑停止 1：急停
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x5041)
 *******************************************************************/
IMC_API IMC_StopPtPack(short cardIndex, short sysNo, short type);

/**
 * @brief  清除捆绑 PT 运动数据
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  sysNo            PT系统号, 参数范围：[0,5]
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x5042)
 *******************************************************************/
IMC_API IMC_ClrPtPackData(short cardIndex, short sysNo);

/**
 * @brief  清除捆绑 PT 运动错误
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  sysNo            PT系统号, 参数范围：[0,5]
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x5043)
 *******************************************************************/
IMC_API IMC_ClrPtPackError(short cardIndex, short sysNo);

/**
 * @brief  获取捆绑 PT 运动数据的空间
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  sysNo            PT系统号, 参数范围：[0,5]
 * @param  pSpace           当前系统剩余的数据空间
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x5050)
 *******************************************************************/
IMC_API IMC_GetPtPackRestSpace(short cardIndex, short sysNo, short* pSpace);

/**
 * @brief  获取捆绑 PT 运动的状态
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  sysNo            PT系统号, 参数范围：[0,5]
 * @param  pStatus          0：停止状态 1：运动状态
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x5051)
 *******************************************************************/
IMC_API IMC_GetPtPackStatus(short cardIndex, short sysNo, short* pStatus);

/**
 * @brief  获取捆绑 PT 运动错误
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  sysNo            PT系统号, 参数范围：[0,5]
 * @param  pErr             0：无错误 1：断流错误 2：系统中某一单轴出现报警错误
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x5052)
 *******************************************************************/
IMC_API IMC_GetPtPackError(short cardIndex, short sysNo, short* pErr);

/**
 * @brief  获取捆绑 PT 运动到位状态
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  sysNo            PT系统号, 参数范围：[0,5]
 * @param  pSts             到位状态, 1：到位 0：未到位
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x5053)
 *******************************************************************/
IMC_API IMC_GetPtPackArrivalSts(short cardIndex, short sysNo, short* pSts);
/// @}

/// @defgroup MoveCrd 板卡立即插补运动模式相关接口
/// @brief 立即插补运动模式相关接口
/// @{

/**
 * @brief  立即直线插补
 * @details
 * @param  cardIndex        板卡卡号, 参数范围[0 ~ 3]
 * @param  moveCrdNo        立即插补坐标系号, 参数范围 [0, 4）
 * @param  pAxArray         轴号, 参数范围 [0, 最大支持轴数）
 * @param  axNum            轴数量, 参数范围 [1, 16]
 * @param  pEndPos          各轴末点位置
 *                          \n pEndPos[0]：运动终点pAxArray[0]轴位置
 *                          \n pEndPos[1]：运动终点pAxArray[1]轴位置
 *                          \n ...
 * @param  trajVel          合成速度,   参数范围 [1e-6,1e9]
 * @param  trajAcc          合成加速度, 参数范围 [1e-6,1e9]
 * @param  trajDec          合成减速度, 参数范围 [1e-6,1e9]
 * @param  smoothCoef       平滑系数,单位:周期, 参数范围 [0,200]
 * @param  posType          位置类型：0 绝对位置 1 相对位置
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x5500)
 *******************************************************************/
IMC_API IMC_MoveCrdLine(short cardIndex, short moveCrdNo, short* pAxArray, short axNum, double* pEndPos, double trajVel, double trajAcc, double trajDec, short smoothCoef, short posType = 0);

/**
 * @brief  三点圆弧立即插补
 * @details 前3个轴以给定速度/加速度做主插补运动, 其他轴按比例跟随主运动
 * @param  cardIndex        板卡卡号, 参数范围[0 ~ 3]
 * @param  moveCrdNo        立即插补坐标系号, 参数范围 [0, 4）
 * @param  pAxArray         轴号, 参数范围 [0, 最大支持轴数）
 * @param  axNum            轴数量, 参数范围 [2, 16]
 * @param  pMidPos          描述的圆弧中间任意一点位置
 *                          \n pMidPos[0]：圆弧任意点pAxArray[0]轴位置
 *                          \n pMidPos[1]：圆弧任意点pAxArray[1]轴位置
 *                          \n pMidPos[2]：圆弧任意点pAxArray[2]轴位置 (arcDimension=2时,pMidPos[2]无意义)
 * @param  pEndPos          各轴末点位置
 *                          \n pEndPos[0]：运动终点pAxArray[0]轴位置
 *                          \n pEndPos[1]：运动终点pAxArray[1]轴位置
 *                          \n ...
 * @param  turn             圈数,       参数范围 [0,1e8]
 * @param  trajVel          合成速度,   参数范围 [1e-6,1e9]
 * @param  trajAcc          合成加速度, 参数范围 [1e-6,1e9]
 * @param  trajDec          合成减速度, 参数范围 [1e-6,1e9]
 * @param  smoothCoef       平滑系数,单位:周期, 参数范围 [0,200]
 * @param  arcDimension     圆弧维数
 *                          \n 2：平面三点圆弧(前2轴)
 *                          \n 3：空间三点圆弧(前3轴)
 * @param  posType          位置类型：  0 绝对位置 1 相对位置
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x5501)
 *******************************************************************/
IMC_API IMC_MoveCrdArcThreePoint(short cardIndex, short moveCrdNo, short* pAxArray, short axNum, double* pMidPos, double* pEndPos, int turn,
                                 double trajVel, double trajAcc, double trajDec, short smoothCoef, short arcDimension = 2, short posType = 0);
/**
 * @brief  圆心末点圆弧立即插补
 * @details 前3个轴以给定速度/加速度做主插补运动, 其他轴按比例跟随主运动
 * @param  cardIndex        板卡卡号, 参数范围[0 ~ 3]
 * @param  moveCrdNo        立即插补坐标系号, 参数范围 [0, 4）
 * @param  pAxArray         轴号, 参数范围 [0, 最大支持轴数）
 * @param  axNum            轴数量, 参数范围 [2, 16]
 * @param  pCenterPos       圆弧圆心坐标(相对于起点的位置增量)
 *                          \n pCenterPos[0]：圆弧圆心pAxArray[0]轴位置
 *                          \n pCenterPos[1]：圆弧圆心pAxArray[1]轴位置
 * @param  pEndPos          各轴末点位置
 *                          \n pEndPos[0]：运动终点pAxArray[0]轴位置
 *                          \n pEndPos[1]：运动终点pAxArray[1]轴位置
 *                          \n ...
 * @param  dir              圆弧运动方向
 *                          \n 1：圆弧逆时针方向运动
 *                          \n -1 圆弧顺时针方向运动
 * @param  turn             圈数,       参数范围 [0,1e8]
 * @param  trajVel          合成速度,   参数范围 [1e-6,1e9]
 * @param  trajAcc          合成加速度, 参数范围 [1e-6,1e9]
 * @param  trajDec          合成减速度, 参数范围 [1e-6,1e9]
 * @param  smoothCoef       平滑系数,单位:周期, 参数范围 [0,200]
 * @param  posType          位置类型：  0 绝对位置 1 相对位置
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x5502)
 *******************************************************************/
IMC_API IMC_MoveCrdArcCenter(short cardIndex, short moveCrdNo, short* pAxArray, short axNum, double* pCenterPos, double* pEndPos, short dir, int turn, double trajVel, double trajAcc, double trajDec, short smoothCoef, short posType = 0);

/**
 * @brief  半径末点圆弧立即插补
 * @details 前3个轴以给定速度/加速度做主插补运动, 其他轴按比例跟随主运动
 * @param  cardIndex        板卡卡号, 参数范围[0 ~ 3]
 * @param  moveCrdNo        立即插补坐标系号, 参数范围 [0, 4）
 * @param  pAxArray         轴号, 参数范围 [0, 最大支持轴数）
 * @param  axNum            轴数量, 参数范围 [2, 16]
 * @param  radius           圆弧半径, 正号：优弧, 负号：劣弧
 * @param  pEndPos          各轴末点位置
 *                          \n pEndPos[0]：运动终点pAxArray[0]轴位置
 *                          \n pEndPos[1]：运动终点pAxArray[1]轴位置
 *                          \n ...
 * @param  dir              圆弧运动方向
 *                          \n 1：圆弧逆时针方向运动
 *                          \n -1 圆弧顺时针方向运动
 * @param  turn             圈数,       参数范围 [0,1e8]
 * @param  trajVel          合成速度,   参数范围 [1e-6,1e9]
 * @param  trajAcc          合成加速度, 参数范围 [1e-6,1e9]
 * @param  trajDec          合成减速度, 参数范围 [1e-6,1e9]
 * @param  smoothCoef       平滑系数,单位:周期, 参数范围 [0,200]
 * @param  posType          位置类型：  0 绝对位置 1 相对位置
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x5503)
 *******************************************************************/
IMC_API IMC_MoveCrdArcRadius(short cardIndex, short moveCrdNo, short* pAxArray, short axNum, double radius, double* pEndPos, short dir, int turn, double trajVel, double trajAcc, double trajDec, short smoothCoef, short posType = 0);

/**
 * @brief  圆心角圆弧立即插补
 * @details 平面圆心角圆弧立即插补支持2轴, 空间圆心角圆弧立即插补支持3轴, 圆心角圆弧立即插补不支持直线轴跟随
 * @param  cardIndex        板卡卡号, 参数范围[0 ~ 3]
 * @param  moveCrdNo        立即插补坐标系号, 参数范围 [0, 4）
 * @param  pAxArray         轴号, 参数范围 [0, 最大支持轴数）
 * @param  pCenterPos       圆弧圆心坐标(相对于起点的位置增量)
 *                          \n pCenterPos[0]：圆弧圆心pAxArray[0]轴位置
 *                          \n pCenterPos[1]：圆弧圆心pAxArray[1]轴位置
 * @param  angle            圆心角(单位: 弧度), 正号：优弧, 负号：劣弧,参数范围 [-1e9,1e9]
 * @param  trajVel          合成速度,   参数范围 [1e-6,1e9]
 * @param  trajAcc          合成加速度, 参数范围 [1e-6,1e9]
 * @param  trajDec          合成减速度, 参数范围 [1e-6,1e9]
 * @param  smoothCoef       平滑系数,单位:周期, 参数范围 [0,200]
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x5504)
 *******************************************************************/
IMC_API IMC_MoveCrdArcAngle(short cardIndex, short moveCrdNo, short* pAxArray, double* pCenterPos, double angle, double trajVel, double trajAcc, double trajDec, short smoothCoef);

/**
 * @brief  停止立即插补
 * @param  cardIndex        板卡卡号, 参数范围[0 ~ 3]
 * @param  moveCrdNo        立即插补坐标系号, 参数范围 [0, 4）
 * @param  stopType         停止类型
 *                          \n 0：平滑停止
 *                          \n 1：急停
 * @par 指令码              (0x550a)
 *******************************************************************/
IMC_API IMC_MoveCrdStop(short cardIndex, short moveCrdNo, short stopType);

/**
 * @brief  获取立即插补速度
 * @param  cardIndex        板卡卡号, 参数范围[0 ~ 3]
 * @param  moveCrdNo        立即插补坐标系号, 参数范围 [0, 4）
 * @param  pTrajVel         插补合成速度, 单位:unit/s
 * @par 指令码              (0x550b)
 *******************************************************************/
IMC_API IMC_MoveCrdGetVel(short cardIndex, short moveCrdNo, double* pTrajVel);

/**
 * @brief  获取立即插补位置
 * @param  cardIndex        板卡卡号, 参数范围[0 ~ 3]
 * @param  moveCrdNo        立即插补坐标系号, 参数范围 [0, 4）
 * @param  pTrajVel         立即插补位置
 * @par 指令码              (0x550c)
 *******************************************************************/
IMC_API IMC_MoveCrdGetPos(short cardIndex, short moveCrdNo, double* pTrajPos);

/**
 * @brief  获取立即插补到位状态
 * @param  cardIndex        板卡卡号, 参数范围[0 ~ 3]
 * @param  moveCrdNo        立即插补坐标系号, 参数范围 [0, 4）
 * @param  pSts             立即插补到位状态  0:运动中  1:到位、已完成
 * @par 指令码              (0x550d)
 *******************************************************************/
IMC_API IMC_MoveCrdGetArrivalSts(short cardIndex, short moveCrdNo, short* pSts);

/// @}

/// @}

/// @defgroup Compensate 板卡补偿功能
/// @brief 板卡补偿功能
/// @{
/// @defgroup Backlash 板卡反向间隙补偿功能
/// @brief 板卡反向间隙补偿功能
/// @{

/**
 * @brief  设置轴反向间隙补偿
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  wholeCmpVal      反向总补偿量, 参数范围：[0, intMax]单位(unit)
 * @param  cmpVel           补偿的变化速度, 参数范围：[0, intMax]单位(unit/ms)
 * @param  cmpDir           补偿的方向
 *                          \n 0正向运动补偿
 *                          \n 1负向运动补偿
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x3011)
 *******************************************************************/
IMC_API IMC_SetAxBacklash(short cardIndex, short axNo, int wholeCmpVal, int cmpVel, short cmpDir);

/**
 * @brief  获取轴反向间隙补偿
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  pWholeCmpVal     反向总补偿量, 参数范围：[0, intMax]单位(unit)
 * @param  pCmpVel          补偿的变化速度, 参数范围：[0, intMax]单位(unit/ms)
 * @param  pCmpDir          补偿的方向
 *                          \n 0正向运动补偿
 *                          \n 1负向运动补偿
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x3012)
 *******************************************************************/
IMC_API IMC_GetAxBacklash(short cardIndex, short axNo, int* pWholeCmpVal, int* pCmpVel, short* pCmpDir);

/**
 * @brief  获取轴反向间隙差值
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  pCmpVal          获取轴的反向间隙差值
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x3111)
 *******************************************************************/
IMC_API IMC_GetAxBacklashCmpVal(short cardIndex, short axNo, int* pCmpVal);

/// @}
/// @defgroup CompensateScrew 板卡螺距误差补偿功能
/// @brief 板卡螺距误差补偿功能
/// @{

/**
 * @brief  设置螺距误差补偿表
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  tableId          操作表索引, 参数范围：[0,63]
 * @param  cnt              补偿点个数,参数范围：[2,256]
 * @param  pPosCompArray    正向补偿表
 * @param  pNegCompArray    负向补偿表
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x9000)
 *******************************************************************/
IMC_API IMC_SetAxScrewCompTable(short cardIndex, short tableId, short cnt, int* pPosCompArray, int* pNegCompArray);

/**
 * @brief  获取螺距误差补偿表
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  tableId          操作轴号, 参数范围：[0,63]
 * @param  pCnt             补偿点个数,参数范围：[2,256]
 * @param  pPosCompArray    正向补偿表
 * @param  pNegCompArray    负向补偿表
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x9001)
 *******************************************************************/
IMC_API IMC_GetAxScrewCompTable(short cardIndex, short tableId, short* pCnt, int* pPosCompArray, int* pNegCompArray);

/**
 * @brief  设置螺距误差补偿参数
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  pointCnt         补偿点个数,参数范围：[2,256]
 * @param  startPos         补偿起始位置,参数范围：[intMin,intMax]
 * @param  len              补偿总长度,参数范围：(0,intMax]
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x9002)
 *******************************************************************/
IMC_API IMC_SetAxScrewCompParam(short cardIndex, short axNo, short pointCnt, int startPos, int len);

/**
 * @brief  获取螺距误差补偿参数
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  pPointCnt        补偿点个数,参数范围：[2,256]
 * @param  pStartPos        补偿起始位置,参数范围：[intMin,intMax]
 * @param  pLen             补偿总长度,参数范围：(0,intMax]
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x9003)
 *******************************************************************/
IMC_API IMC_GetAxScrewCompParam(short cardIndex, short axNo, short* pPointCnt, int* pStartPos, int* pLen);

/**
 * @brief  使能螺距误差补偿
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  enable           1 启动补偿 0 停止补偿
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x9004)
 *******************************************************************/
IMC_API IMC_EnableAxScrewComp(short cardIndex, short axNo, short enable);

/**
 * @brief  获取螺距误差补偿状态
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  pEnable          1 启动补偿 0 停止补偿
 * @param  pCompVal         螺距误差补偿的当前补偿值
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x9005)
 *******************************************************************/
IMC_API IMC_GetAxScrewCompSts(short cardIndex, short axNo, short* pEnable, double* pCompVal);
/// @}

/// @defgroup CompensateTable 板卡表补偿功能
/// @brief 板卡表误差补偿功能
/// @{
/**
 * @brief  设置表补偿的数据表
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  tableId          操作表索引, 参数范围：[0,0]
 * @param  cnt              补偿点个数,参数范围：[2,40000]
 * @param  pDataArray       补偿表数据(多维补偿时,补偿数据按srcAxNo[0],srcAxNo[1],srcAxNo[2]方向顺序增加,具体请查阅说明书)
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x9020)
 *******************************************************************/
IMC_API IMC_TableCompSetTable(short cardIndex, short tableId, int cnt, int* pDataArray);

/**
 * @brief  获取表补偿的数据表
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  tableId          操作表索引, 参数范围：[0,当前表补偿的表个数(默认为1) - 1]
 * @param  pCnt             补偿点个数,参数范围：[2,40000]
 * @param  pDataArray       补偿表数据(多维补偿时,补偿数据按srcAxNo[0],srcAxNo[1],srcAxNo[2]方向顺序增加,具体请查阅说明书)
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x9021)
 *******************************************************************/
IMC_API IMC_TableCompGetTable(short cardIndex, short tableId, int* pCnt, int* pDataArray);

/**
 * @brief  设置表补偿参数
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  pTTableCompParam 表补偿参数, 详细信息请参考\ref TTableCompParam "表补偿参数结构体"
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x9022)
 *******************************************************************/
IMC_API IMC_TableCompSetParam(short cardIndex, short axNo, TTableCompParam* pTTableCompParam);

/**
 * @brief  获取表补偿参数
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  pTTableCompParam 表补偿参数, 详细信息请参考\ref TTableCompParam "表补偿参数结构体"
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x9023)
 *******************************************************************/
IMC_API IMC_TableCompGetParam(short cardIndex, short axNo, TTableCompParam* pTTableCompParam);

/**
 * @brief  使能表补偿
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  enable           1 启动补偿 0 停止补偿
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x9024)
 *******************************************************************/
IMC_API IMC_TableCompEnable(short cardIndex, short axNo, short enable);

/**
 * @brief  获取表补偿状态
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  pEnable          1 启动补偿 0 停止补偿
 * @param  pCompVal         表补偿的当前补偿值
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x9025)
 *******************************************************************/
IMC_API IMC_TableCompGetSts(short cardIndex, short axNo, short* pEnable, double* pCompVal);

/**
 * @brief  重新分配表补偿的表数量和大小(总大小为40000)
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  tableNum         补偿表个数, 参数范围：[1,4]
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x9026)
 *******************************************************************/
IMC_API IMC_TableCompTableResize(short cardIndex, short tableNum);

/**
 * @brief  获取表补偿的表数量和大小信息
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  pTableNum        补偿表个数, 参数范围：[1,4]
 * @param  pTableSize       补偿表大小
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x9027)
 *******************************************************************/
IMC_API IMC_TableCompTableGetInfo(short cardIndex, short* pTableNum, int* pTableSize);

/// @}

/// @defgroup AxPrfComp 板卡轴补偿功能
/// @brief 板卡轴补偿功能
/// @{

/**
 * @brief  设置轴规划补偿
 * @param  cardIndex        板卡卡号, 参数范围[0 ~ 3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  compPos          补偿位置
 * @param  compTime         补偿时间, 单位ms, 小于1时修正为1
 * @param  posType          补偿位置类型, 0 绝对值  1 相对值
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x9040)
 *******************************************************************/
IMC_API IMC_SetAxCompPos(short cardIndex, short axNo, double compPos, double compTime, short posType);

/**
 * @brief  获取轴规划补偿状态
 * @param  cardIndex        板卡卡号, 参数范围[0 ~ 3]
 * @param  axNo             操作轴号, 参数范围：[0,63]
 * @param  pSts             补偿状态 0 为空闲 1为补偿进行中
 * @param  pCompPos         轴补偿功能的当前补偿值
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x9041)
 *******************************************************************/
IMC_API IMC_GetAxCompSts(short cardIndex, short axNo, short* pSts, double* pCompPos);

/// @}
/// @}

/// @defgroup Virtual 板卡虚拟资源
/// @brief 板卡虚拟资源
/// @{
/// @defgroup UserVal 板卡全局变量功能
/// @brief 板卡全局变量功能
/// @{

/**
 * @brief  设置全局变量
 * @param  cardIndex        板卡卡号, 参数范围[0 ~ 3]
 * @param  index            全局变量索引, 参数范围[0,15]
 * @param  value            设置值, 参数范围[intMin,intMax]
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x8000)
 *******************************************************************/
IMC_API IMC_SetUserVal(short cardIndex, short index, int value);

/**
 * @brief  读取全局变量
 * @param  cardIndex        板卡卡号, 参数范围[0 ~ 3]
 * @param  index            全局变量索引, 参数范围[0,15]
 * @param  pValue           获取的全局变量值
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x8001)
 *******************************************************************/
IMC_API IMC_GetUserVal(short cardIndex, short index, int* pValue);

/// @}
/// @}

/// @defgroup Sample 板卡采样功能
/// @brief 板卡采样功能
/// @{
/// @defgroup SampleTypeDef 板卡采集数据类型宏定义

/**
 * @brief  配置数据采集的参数
 * @param  cardIndex        板卡卡号, 参数范围[0 ~ 3]
 * @param  pSamplePara      数据采集参数, 详细请参考\ref TSamplePara "数据采集配置参数结构体"
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0xa000)
 *******************************************************************/
IMC_API IMC_ConfigSamplePara(short cardIndex, TSamplePara* pSamplePara);

/**
 * @brief  配置数据采集的采集对象
 * @param  cardIndex        板卡卡号, 参数范围[0 ~ 3]
 * @param  count            采集变量的个数, 参数范围[1,32]
 * @param  pDataTypeArray   采样数据的类型
 * @param  pDataIndexArray  采样数据的序号
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0xa001)
 *******************************************************************/
IMC_API IMC_ConfigSampleData(short cardIndex, short count, short* pDataTypeArray, int* pDataIndexArray);

/**
 * @brief  使能数据采集
 * @param  cardIndex        板卡卡号, 参数范围[0 ~ 3]
 * @param  enable           1 启动采样 0 停止采样
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0xa002)
 *******************************************************************/
IMC_API IMC_ConfigSampleEnable(short cardIndex, short enable);

/**
 * @brief  获取数据采集状态
 * @param  cardIndex        板卡卡号, 参数范围[0 ~ 3]
 * @param  pStatus          1 表示在采集 0 表示未启动采集
 * @param  pLen             采集的数据长度：单位：short
 * @param  pLeakageCount    漏采数据的包数
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0xa003)
 *******************************************************************/
IMC_API IMC_GetSampleStatus(short cardIndex, short* pStatus, int* pLen, int* pLeakageCount);

/**
 * @brief  获取采集的数据
 * @param  cardIndex        板卡卡号, 参数范围[0 ~ 3]
 * @param  pPackNum         反馈的数据包个数
 * @param  pDataArray       采样的数据
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0xa004)
 *******************************************************************/
IMC_API IMC_GetSampleData(short cardIndex, short* pPackNum, short* pDataArray);
/// @}

/// @defgroup SegmentLimit 分段限位功能
/// @brief 分段限位功能
/// @{
/**
 * @brief  分段限位功能参数设置
 * @param  cardIndex        板卡卡号, 参数范围[0 ~ 3]
 * @param  pAxNoArray       轴号
 * @param  sourceType       轴位置来源
 * @param  pPointXArray     X轴坐标
 * @param  pPointYArray     Y轴坐标
 * @param  pointCount       点数
 * @param  yLmtDir          Y限位方向
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0xa004)
 *******************************************************************/
IMC_API IMC_SegLmtSetParam(short cardIndex, short* pAxNoArray, short sourceType, int* pPointXArray, int* pPointYArray, short pointCount, short yLmtDir);

/**
 * @brief  分段限位功能参数获取
 * @param  cardIndex        板卡卡号, 参数范围[0 ~ 3]
 * @param  pAxNoArray       轴号
 * @param  pSourceType      轴位置来源
 * @param  pPointXArray     X轴坐标
 * @param  pPointYArray     Y轴坐标
 * @param  pPointCount      点数
 * @param  pYLmtDir         Y限位方向
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0xa004)
 *******************************************************************/
IMC_API IMC_SegLmtGetParam(short cardIndex, short* pAxNoArray, short* pSourceType, int* pPointXArray, int* pPointYArray, short* pPointCount, short* pYLmtDir);

/**
 * @brief  分段限位功能参数开启
 * @param  cardIndex        板卡卡号, 参数范围[0 ~ 3]
 * @param  enable           使能标志
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0xa004)
 *******************************************************************/
IMC_API IMC_SegLmtEnable(short cardIndex, short enable);
/// @}

/// @defgroup ArcZoneLimit 圆形软限位
/// @brief 圆形软限位
/// @{
/**
 * @brief  获取圆形软限位参数
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  pAxNoArray       轴号列表, 参数范围：[0,63]
 * @param  pCenterArray     圆心位置，
 * @param  radius           限位半径, 参数范围：[intMin,intMax],输入为负值时,内部转化为正数
 * @param  sourceType       圆形软限位数据类型。 0：规划位置;1：编码器
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x3030)
 *******************************************************************/
IMC_API IMC_ArcZoneLmtSetParam(short cardIndex, short* pAxNoArray, int* pCenterArray, int radius, short sourceType);

/**
 * @brief  获取圆形软限位参数
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  pAxNoArray       轴号列表, 参数范围：[0,63]
 * @param  pCenterArray     圆心位置,
 * @param  pRadius          限位半径, 参数范围：[intMin,intMax],输入为负值时,内部转化为正数
 * @param  pSourceType      圆形软限位数据类型。 0：规划位置;1：编码器
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x3031)
 *******************************************************************/
IMC_API IMC_ArcZoneLmtGetParam(short cardIndex, short* pAxNoArray, int* pCenterArray, int* pRadius, short* pSourceType);

/**
 * @brief  使能圆形软限位
 * @param  cardIndex        板卡卡号, 参数范围：[0,3]
 * @param  enable           1 启动限位检测 0 停止限位检测
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x3032)
 *******************************************************************/
IMC_API IMC_ArcZoneLmtEnable(short cardIndex, short enable);
/// @}

/// @defgroup MultiAxCmp 多轴比较功能
/// @brief 板卡多轴比较功能
/// @{
/**
 * @brief  配置多轴比较输入数据源
 * @param  cardIndex        板卡卡号, 参数范围[0 ~ 3]
 * @param  groupNo          组号, 最大支持4组位置比较输出
 * @param  cmpDimNum        比较维数, 最大支持6轴位置比较输出
 * @param  cmpSrcType       比较位置源类型：0 Ecat轴反馈 1 Ecat轴规划 2 端子板编码器 3 Ecat编码器
 * @param  pCmpSrcArray     比较位置源数组，按照设定维数依次设置对应索引
 * @param  errorLmt         比较误差带，大于0
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x5300)
 *******************************************************************/
IMC_API IMC_MultiAxCmpSrcCfg(short cardIndex, short groupNo, short cmpDimNum, short cmpSrcType, short* pCmpSrcArray, int errorLmt);

/**
 * @brief  设置多维位置比较输出类型
 * @param  cardIndex        板卡卡号, 参数范围[0 ~ 3]
 * @param  groupNo          组号, 最大支持4组位置比较输出
 * @param  outType          输出类型 0-脉冲DO通道输出 1-端子板CMP口输出
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x5301)
 *******************************************************************/
IMC_API IMC_MultiAxCmpOutputCfg(short cardIndex, short groupNo, short outType);

/**
 * @brief  使能位置比较
 * @param  cardIndex        板卡卡号, 参数范围[0 ~ 3]
 * @param  groupNo          组号, 最大支持4组位置比较输出
 * @param  enable           1 启动比较 0 停止比较
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x5304)
 *******************************************************************/
IMC_API IMC_MultiAxCmpEnable(short cardIndex, short groupNo, short enable);

/**
 * @brief  下发位置比较数据
 * @param  cardIndex        板卡卡号, 参数范围[0 ~ 3]
 * @param  groupNo          组号, 最大支持4组位置比较输出
 * @param  cmpDimNum        比较维数, 最大支持6轴位置比较输出
 * @param  eventIndex       比较输出事件索引, 参数范围[0 ~ 15]
 * @param  pCmpPosArray     比较位置值
 * @param  pSpace           比较数据buffer剩余空间
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x5302)
 *******************************************************************/
IMC_API IMC_MultiAxCmpPushData(short cardIndex, short groupNo, short cmpDimNum, short eventIndex, int* pCmpPosArray, short* pSpace);

/**
 * @brief  清除位置比较数据
 * @param  cardIndex        板卡卡号, 参数范围[0 ~ 3]
 * @param  groupNo          组号, 最大支持4组位置比较输出
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x5303)
 *******************************************************************/
IMC_API IMC_MultiAxCmpClrData(short cardIndex, short groupNo);

/**
 * @brief  获取位置比较数据buffer剩余空间
 * @param  cardIndex        板卡卡号, 参数范围[0 ~ 3]
 * @param  groupNo          组号, 最大支持4组位置比较输出
 * @param  pSpace           比较数据buffer剩余空间
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x5305)
 *******************************************************************/
IMC_API IMC_MultiAxCmpGetSpace(short cardIndex, short groupNo, short* pSpace);

/**
 * @brief  获取位置比较输出状态
 * @param  cardIndex        板卡卡号, 参数范围[0 ~ 3]
 * @param  groupNo          组号, 最大支持4组位置比较输出
 * @param  pStatus          比较输出状态：0 未开始 1 比较进行中 2 比较完成
 * @param  pCmpCnt          比较输出个数
 * @param  pCmpIndex        比较输出数据索引
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x5306)
 *******************************************************************/
IMC_API IMC_MultiAxCmpGetSts(short cardIndex, short groupNo, short* pStatus, short* pCmpCnt, short* pCmpIndex);

/**
 * @brief  下发位置比较数据
 * @param  cardIndex        板卡卡号, 参数范围[0 ~ 3]
 * @param  groupNo          组号, 最大支持4组位置比较输出
 * @param  cmpDimNum        比较维数, 最大支持6轴位置比较输出
 * @param  eventIndex       比较输出事件索引, 参数范围[0 ~ 15]
 * @param  pCmpPosArray     比较位置值
 * @param  cmpErrorLmt      比较误差带
 * @param  pSpace           比较数据buffer剩余空间
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x5307)
 *******************************************************************/
IMC_API IMC_MultiAxCmpPushDataEx(short cardIndex, short groupNo, short cmpDimNum, short eventIndex, int* pCmpPosArray, int cmpErrorLmt, short* pSpace);

/**
 * @brief  获取位置比较触发时位置数据
 * @param  cardIndex        板卡卡号, 参数范围[0 ~ 3]
 * @param  groupNo          组号, 最大支持4组位置比较输出
 * @param  pCmpPosArray     比较记录位置值, 最大支持记录5组位置
 * @param  pCmpCnt          比较输出次数
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x5308)
 *******************************************************************/
IMC_API IMC_MultiAxCmpGetOutputPos(short cardIndex, short groupNo, int* pCmpPosArray, short* pCmpCnt);

/**
 * @brief  设置位置比较数据缓存区空间
 * @param  cardIndex        板卡卡号, 参数范围[0 ~ 3]
 * @param  pDataLen         比较数据空间值，4个值分别对应4组比较系统对应的空间，总空间4096
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x5309)
 *******************************************************************/
IMC_API IMC_MultiAxCmpSetDataBufSpace(short cardIndex, short* pDataLen);

/// @}

/// @defgroup 运动程序 运动程序功能
/// @brief 板卡运动程序功能
/// @{
/**
 * @brief  编译运动程序
 * @param  pFileName        运动程序源文件路径
 * @param  pWrongInfo       编译结果结构体
 * @retval #EXE_SUCCESS     指令成功
 * @par    指令码           (0xb000)
 *******************************************************************/
IMC_API IMC_Compile(char* pFileName, TCompileInfo* pWrongInfo);

/**
 * @brief  下载运动程序
 * @param  cardIndex        板卡卡号, 参数范围[0 ~ 3]
 * @param  pFileName        运动程序编译文件路径
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0xb001)
 *******************************************************************/
IMC_API IMC_Download(short cardIndex, char* pFileName);

/**
 * @brief  获取程序中的函数id
 * @param  cardIndex        板卡卡号, 参数范围[0 ~ 3]
 * @param  pFunName         程序函数名
 * @param  pFunId           函数ID
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0xb002)
 *******************************************************************/
IMC_API IMC_GetFunId(short cardIndex, char* pFunName, short* pFunId);

/**
 * @brief  获取程序中的变量信息
 * @param  cardIndex        板卡卡号, 参数范围[0 ~ 3]
 * @param  pFunName         程序函数名:null为全局变量
 * @param  pVarName         变量名
 * @param  pVarInfo         获取的变量信息
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0xb003)
 *******************************************************************/
IMC_API IMC_GetVarId(short cardIndex, char* pFunName, char* pVarName, TVarInfo* pVarInfo);

/**
 * @brief  运行线程绑定函数
 * @param  cardIndex        板卡卡号, 参数范围[0 ~ 3]
 * @param  thread           线程号
 * @param  funId            函数ID
 * @param  page             绑定的数据页
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0xb004)
 *******************************************************************/
IMC_API IMC_Bind(short cardIndex, short thread, short funId, short page);

/**
 * @brief  启动运动线程执行
 * @param  cardIndex        板卡卡号, 参数范围[0 ~ 3]
 * @param  thread           线程号
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0xb005)
 *******************************************************************/
IMC_API IMC_RunThread(short cardIndex, short thread);

/**
 * @brief  运动线程周期执行
 * @param  cardIndex        板卡卡号, 参数范围[0 ~ 3]
 * @param  thread           线程号
 * @param  period           设定周期
 * @param  priority         执行步数
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0xb006)
 *******************************************************************/
IMC_API IMC_RunThreadPeriod(short cardIndex, short thread, short period, short priority = 4);

/**
 * @brief  运动线程执行断点
 * @param  cardIndex        板卡卡号, 参数范围[0 ~ 3]
 * @param  thread           线程号
 * @param  line             执行行号
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0xb007)
 *******************************************************************/
IMC_API IMC_RunThreadToBreakpoint(short cardIndex, short thread, short line);

/**
 * @brief  运动线程单步执行
 * @param  cardIndex        板卡卡号, 参数范围[0 ~ 3]
 * @param  thread           线程号
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0xb008)
 *******************************************************************/
IMC_API IMC_StepThread(short cardIndex, short thread);

/**
 * @brief  运动线程停止执行
 * @param  cardIndex        板卡卡号, 参数范围[0 ~ 3]
 * @param  thread           线程号
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0xb009)
 *******************************************************************/
IMC_API IMC_StopThread(short cardIndex, short thread);

/**
 * @brief  运动线程停止执行
 * @param  cardIndex        板卡卡号, 参数范围[0 ~ 3]
 * @param  thread           线程号
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0xb00a)
 *******************************************************************/
IMC_API IMC_PauseThread(short cardIndex, short thread);

/**
 * @brief  获取运动线程执行状态
 * @param  cardIndex        板卡卡号, 参数范围[0 ~ 3]
 * @param  thread           线程号
 * @param  pThreadSts       线程运行状态
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0xb00b)
 *******************************************************************/
IMC_API IMC_GetThreadSts(short cardIndex, short thread, TThreadSts* pThreadSts);

/**
 * @brief  获取运动线程执行周期
 * @param  cardIndex        板卡卡号, 参数范围[0 ~ 3]
 * @param  thread           线程号
 * @param  pPeriod          周期
 * @param  pExecuteTime     执行时间
 * @param  pExecuteTimeMax  执行时间最大值
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0xb00c)
 *******************************************************************/
IMC_API IMC_GetThreadTime(short cardIndex, short thread, short* pPeriod, unsigned int* pExecuteTime, unsigned int* pExecuteTimeMax);

/**
 * @brief  获取线程运动程序执行时间
 * @param  cardIndex        板卡卡号, 参数范围[0 ~ 3]
 * @param  thread           线程号
 * @param  pExecuteTimeTotal     代码执行时间
 * @param  pExecuteTimeTotalMax  代码执行时间最大值
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0xb00f)
 *******************************************************************/
IMC_API IMC_GetThreadRunTime(short cardIndex, short thread, unsigned int* pExecuteTimeTotal, unsigned int* pExecuteTimeTotalMax);

/**
 * @brief  设置变量值
 * @param  cardIndex        板卡卡号, 参数范围[0 ~ 3]
 * @param  page             线程号
 * @param  pVarInfo         变量信息
 * @param  pValue           变量数据
 * @param  count            数据个数
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0xb00d)
 *******************************************************************/
IMC_API IMC_SetVarValue(short cardIndex, short page, TVarInfo* pVarInfo, double* pValue, short count = 1);

/**
 * @brief  获取变量值
 * @param  cardIndex        板卡卡号, 参数范围[0 ~ 3]
 * @param  page             线程号
 * @param  pVarInfo         变量信息
 * @param  pValue           变量数据
 * @param  count            数据个数
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0xb00e)
 *******************************************************************/
IMC_API IMC_GetVarValue(short cardIndex, short page, TVarInfo* pVarInfo, double* pValue, short count = 1);

IMC_API IMC_UnbindVar(short cardIndex, short thread);
IMC_API IMC_BindDi(short cardIndex, short thread, TVarInfo* pVarInfo, TBindDi* pBindDi);
IMC_API IMC_BindDo(short cardIndex, short thread, TVarInfo* pVarInfo, TBindDo* pBindDo);
IMC_API IMC_BindTimer(short cardIndex, short thread, TVarInfo* pVarInfo, TBindTimer* pBindTimer);
IMC_API IMC_BindCounter(short cardIndex, short thread, TVarInfo* pVarInfo, TBindCounter* pBindCounter);
IMC_API IMC_BindFlank(short cardIndex, short thread, TVarInfo* pVarInfo, TBindFlank* pBindFlank);
IMC_API IMC_BindSrff(short cardIndex, short thread, TVarInfo* pVarInfo, TBindSrff* pBindSrff);

IMC_API IMC_GetBindDi(short cardIndex, TVarInfo* pVarInfo, TBindDi* pBindDi);
IMC_API IMC_GetBindDo(short cardIndex, TVarInfo* pVarInfo, TBindDo* pBindDo);
IMC_API IMC_GetBindTimer(short cardIndex, TVarInfo* pVarInfo, TBindTimer* pBindTimer, int* pCount);
IMC_API IMC_GetBindCounter(short cardIndex, TVarInfo* pVarInfo, TBindCounter* pBindCounter, int* pUnitCount, int* pCount);
IMC_API IMC_GetBindFlank(short cardIndex, TVarInfo* pVarInfo, TBindFlank* pBindFlank);
IMC_API IMC_GetBindSrff(short cardIndex, TVarInfo* pVarInfo, TBindSrff* pBindSrff);

IMC_API IMC_GetBindDiCount(short cardIndex, short thread, short* pCount);
IMC_API IMC_GetBindDoCount(short cardIndex, short thread, short* pCount);
IMC_API IMC_GetBindTimerCount(short cardIndex, short thread, short* pCount);
IMC_API IMC_GetBindCounterCount(short cardIndex, short thread, short* pCount);
IMC_API IMC_GetBindFlankCount(short cardIndex, short thread, short* pCount);
IMC_API IMC_GetBindSrffCount(short cardIndex, short thread, short* pCount);

IMC_API IMC_GetBindDiInfo(short cardIndex, short thread, short index, short* pVar, TBindDi* pBindDi);
IMC_API IMC_GetBindDoInfo(short cardIndex, short thread, short index, short* pVar, TBindDo* pBindDo);
IMC_API IMC_GetBindTimerInfo(short cardIndex, short thread, short index, short* pVar, TBindTimer* pBindTimer);
IMC_API IMC_GetBindCounterInfo(short cardIndex, short thread, short index, short* pVar, TBindCounter* pBindCounter);
IMC_API IMC_GetBindFlankInfo(short cardIndex, short thread, short index, short* pVar, TBindFlank* pBindFlank);
IMC_API IMC_GetBindSrffInfo(short cardIndex, short thread, short index, short* pVar, TBindSrff* pBindSrff);

/// @}

/// @defgroup 脉冲DO输出功能 脉冲DO输出功能
/// @brief 板卡脉冲DO输出功能
/// @{
/**
 * @brief  设置脉冲do输出映射
 * @param  cardIndex        板卡卡号, 参数范围[0 ~ 3]
 * @param  index            任务索引
 * @param  doIndex          do索引
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x5200)
 *******************************************************************/
IMC_API IMC_SetPulseDoMap(short cardIndex, short index, short doIndex);

/**
 * @brief  获取脉冲do输出映射
 * @param  cardIndex        板卡卡号, 参数范围[0 ~ 3]
 * @param  index            任务索引
 * @param  pDoIndex         do索引
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x5201)
 *******************************************************************/
IMC_API IMC_GetPulseDoMap(short cardIndex, short index, short* pDoIndex);

/**
 * @brief  设置脉冲do输出参数
 * @param  cardIndex        板卡卡号, 参数范围[0 ~ 3]
 * @param  index            任务索引
 * @param  highLevTime      输出高电平时间
 * @param  lowLevTime       输出低电平时间
 * @param  firstLevel       起始输出电平
 * @param  pulseNum         输出次数
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x5206)
 *******************************************************************/
IMC_API IMC_SetPulseDoOutputParam(short cardIndex, short index, unsigned short highLevTime, unsigned short lowLevTime, short firstLevel, short pulseNum);

/**
 * @brief  获取脉冲do输出参数
 * @param  cardIndex        板卡卡号, 参数范围[0 ~ 3]
 * @param  index            任务索引
 * @param  pHighLevTime     输出高电平时间
 * @param  pLowLevTime      输出低电平时间
 * @param  pFirstLevel      起始输出电平
 * @param  pPulseNum        输出次数
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x5207)
 *******************************************************************/
IMC_API IMC_GetPulseDoOutputParam(short cardIndex, short index, unsigned short* pHighLevTime, unsigned short* pLowLevTime, short* pFirstLevel, short* pPulseNum);

/**
 * @brief  设置脉冲do输出参数
 * @param  cardIndex        板卡卡号, 参数范围[0 ~ 3]
 * @param  index            任务索引
 * @param  highLevTime      输出高电平时间
 * @param  lowLevTime       输出低电平时间
 * @param  firstLevel       起始输出电平
 * @param  endLevel         输出结束电平 0-恢复到起始电平的反电平 1-恢复到起始电平
 * @param  pulseNum         输出次数
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x5208)
 *******************************************************************/
IMC_API IMC_SetPulseDoOutputParamEx(short cardIndex, short index, unsigned short highLevTime, unsigned short lowLevTime, short firstLevel, short endLevel, short pulseNum);

/**
 * @brief  获取脉冲do输出参数
 * @param  cardIndex        板卡卡号, 参数范围[0 ~ 3]
 * @param  index            任务索引
 * @param  pHighLevTime     输出高电平时间
 * @param  pLowLevTime      输出低电平时间
 * @param  pFirstLevel      起始输出电平
 * @param  pEndLevel        输出结束电平 0-恢复到起始电平的反电平 1-恢复到起始电平
 * @param  pPulseNum        输出次数
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x5209)
 *******************************************************************/
IMC_API IMC_GetPulseDoOutputParamEx(short cardIndex, short index, unsigned short* pHighLevTime, unsigned short* pLowLevTime, short* pFirstLevel, short* pEndLevel, short* pPulseNum);

/**
 * @brief  使能脉冲do输出
 * @param  cardIndex        板卡卡号, 参数范围[0 ~ 3]
 * @param  index            任务索引
 * @param  highLevTime      输出高电平时间
 * @param  lowLevTime       输出低电平时间
 * @param  firstLevel       起始输出电平
 * @param  pulseNum         输出次数
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x5202)
 *******************************************************************/
IMC_API IMC_EnablePulseDo(short cardIndex, short index, unsigned short highLevTime, unsigned short lowLevTime, short firstLevel, short pulseNum);

/**
 * @brief  停止脉冲do输出
 * @param  cardIndex        板卡卡号, 参数范围[0 ~ 3]
 * @param  index            任务索引
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x5203)
 *******************************************************************/
IMC_API IMC_DisablePulseDo(short cardIndex, short index);

/**
 * @brief  获取脉冲do输出状态
 * @param  cardIndex        板卡卡号, 参数范围[0 ~ 3]
 * @param  index            任务索引
 * @param  pStatus          输出状态
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x5205)
 *******************************************************************/
IMC_API IMC_GetPulseDoStatus(short cardIndex, short index, short* pStatus);
/// @}

/// @defgroup Event 事件功能
/// @brief 事件功能
/// @{
/// @defgroup EventDef 事件功能相关宏定义
/// @{
/// @defgroup EventTypeDef 事件功能相关类型定义
/// @defgroup EventConditionDef 事件功能相关条件宏定义
/// @defgroup TaskTypeDef 事件功能相关任务宏定义
/// @}
/// @defgroup EventStruct 事件功能相关结构体

/// @defgroup EventFunc 事件功能接口
/// @brief 事件功能接口
/// @{
/**
 * @brief  设置事件
 * @param  cardIndex        板卡卡号, 参数范围[0 ~ 3]
 * @param  eventIndex       事件索引
 * @param  pEvent           事件参数
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x5100)
 *******************************************************************/
IMC_API IMC_SetEvent(short cardIndex, short eventIndex, TEvent* pEvent);
/**
 * @brief  设置任务
 * @param  cardIndex        板卡卡号, 参数范围[0 ~ 3]
 * @param  taskIndex        任务索引
 * @param  taskType         任务类型
 * @param  pTask            任务数据
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x5101)
 *******************************************************************/
IMC_API IMC_SetTask(short cardIndex, short taskIndex, short taskType, void* pTask);
/**
 * @brief  设置事件与任务连接
 * @param  cardIndex        板卡卡号, 参数范围[0 ~ 3]
 * @param  eventIndex       事件索引
 * @param  taskIndexArray   任务数组
 * @param  count           任务个数
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x5102)
 *******************************************************************/
IMC_API IMC_SetEventTaskLink(short cardIndex, short eventIndex, short* taskIndexArray, short count);
/**
 * @brief  事件开启
 * @param  cardIndex        板卡卡号, 参数范围[0 ~ 3]
 * @param  eventIndex       事件索引
 * @param  count            事件个数
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x5103)
 *******************************************************************/
IMC_API IMC_EventOn(short cardIndex, short eventIndex, short count = 1);
/**
 * @brief  事件关闭
 * @param  cardIndex        板卡卡号, 参数范围[0 ~ 3]
 * @param  eventIndex       事件索引
 * @param  count            事件个数
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x5104)
 *******************************************************************/
IMC_API IMC_EventOff(short cardIndex, short eventIndex, short count = 1);
/**
 * @brief  事件强制触发
 * @param  cardIndex        板卡卡号, 参数范围[0 ~ 3]
 * @param  eventIndex       事件索引
 * @param  count            事件个数
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x5105)
 *******************************************************************/
IMC_API IMC_EventForceTrigger(short cardIndex, short eventIndex, short count = 1);
/**
 * @brief  清除事件
 * @param  cardIndex        板卡卡号, 参数范围[0 ~ 3]
 * @param  eventIndex       事件索引
 * @param  count           事件个数
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x5106)
 *******************************************************************/
IMC_API IMC_ClearEvent(short cardIndex, short eventIndex, short count = 1);
/**
 * @brief  清除任务
 * @param  cardIndex        板卡卡号, 参数范围[0 ~ 3]
 * @param  taskIndex        任务索引
 * @param  count            任务个数
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x5107)
 *******************************************************************/
IMC_API IMC_ClearTask(short cardIndex, short taskIndex, short count = 1);
/**
 * @brief  清除事件与任务的连接
 * @param  cardIndex        板卡卡号, 参数范围[0 ~ 3]
 * @param  eventIndex       事件索引
 * @param  count            事件个数
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x5108)
 *******************************************************************/
IMC_API IMC_ClearEventTaskLink(short cardIndex, short eventIndex, short count = 1);
/**
 * @brief  获取设置的事件数量，不生效，默认执行32个事件
 * @param  cardIndex        板卡卡号, 参数范围[0 ~ 3]
 * @param  pCount           设置的事件数量
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x5109)
 *******************************************************************/
IMC_API IMC_GetEventCount(short cardIndex, short* pCount);
/**
 * @brief  获取设置的事件
 * @param  cardIndex        板卡卡号, 参数范围[0 ~ 3]
 * @param  eventIndex       事件索引
 * @param  pEvent           事件参数
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x510a)
 *******************************************************************/
IMC_API IMC_GetEvent(short cardIndex, short eventIndex, TEvent* pEvent);
/**
 * @brief  获取事件的触发次数
 * @param  cardIndex        板卡卡号, 参数范围[0 ~ 3]
 * @param  eventIndex       事件索引
 * @param  pTriggerCount    设置的任务数量
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x510b)
 *******************************************************************/
IMC_API IMC_GetEventTriggerCount(short cardIndex, short eventIndex, short* pTriggerCount);
/**
 * @brief  获取设置的任务数量
 * @param  cardIndex        板卡卡号, 参数范围[0 ~ 3]
 * @param  pCount           设置的任务数量
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x510c)
 *******************************************************************/
IMC_API IMC_GetTaskCount(short cardIndex, short* pCount);
/**
 * @brief  获取任务
 * @param  cardIndex        板卡卡号, 参数范围[0 ~ 3]
 * @param  taskIndex        任务索引
 * @param  pTaskType        任务类型
 * @param  pTaskData        任务数据
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x510d)
 *******************************************************************/
IMC_API IMC_GetTask(short cardIndex, short taskIndex, short* pTaskType, void* pTaskData);
/**
 * @brief  获取设置的事件与任务连接数量
 * @param  cardIndex        板卡卡号, 参数范围[0 ~ 3]
 * @param  eventIndex       事件索引
 * @param  pCount           连接数量
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x510e)
 *******************************************************************/
IMC_API IMC_GetEventTaskLink(short cardIndex, short eventIndex, short* pTaskIndexArray, short* pCount);
/**
 * @brief  获取任务执行结果
 * @param  cardIndex        板卡卡号, 参数范围[0 ~ 3]
 * @param  taskIndex        任务索引
 * @param  pResult          任务执行结果
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x510f)
 *******************************************************************/
IMC_API IMC_GetTaskResult(short cardIndex, short taskIndex, short* pResult);
/// @}

/// @defgroup 指令缓存功能 指令缓存功能
/// @brief 指令缓存功能
/// @{

/**
 * @brief  启动缓存区指令执行
 * @param  cardIndex        板卡卡号, 参数范围[0 ~ 3]
 * @param  index            缓存区索引，参数范围：[0,7]
 * @param  type             缓存区启动方式：0-立即运行 1-重新运行（实现循环模式重新开始）
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x5400)
 *******************************************************************/
IMC_API IMC_CmdListStart(short cardIndex, short index, short type);

/**
 * @brief  停止指令缓存区执行
 * @param  cardIndex        板卡卡号, 参数范围[0 ~ 3]
 * @param  index            缓存区索引，参数范围：[0,7]
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x5401)
 *******************************************************************/
IMC_API IMC_CmdListStop(short cardIndex, short index);

/**
 * @brief  设置指令缓存区运行模式
 * @param  cardIndex        板卡卡号, 参数范围[0 ~ 3]
 * @param  index            缓存区索引，参数范围：[0,7]
 * @param  mode             缓存区运行方式：0-动态模式 1-静态模式（循环模式）
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x5402)
 *******************************************************************/
IMC_API IMC_CmdListSetMode(short cardIndex, short index, short mode);

/**
 * @brief  获取指令缓存区运行模式
 * @param  cardIndex        板卡卡号, 参数范围[0 ~ 3]
 * @param  index            缓存区索引，参数范围：[0,7]
 * @param  pMode            获取的缓存区运行方式：0-动态模式 1-静态模式（循环模式）
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x5403)
 *******************************************************************/
IMC_API IMC_CmdListGetMode(short cardIndex, short index, short* pMode);

/**
 * @brief  获取指令缓存区运行状态、错误码和当前执行段号
 * @param  cardIndex        板卡卡号, 参数范围[0 ~ 3]
 * @param  index            缓存区索引，参数范围：[0,7]
 * @param  pStatus          缓存区运行状态：0-停止中 1-运行中
 * @param  pErrorCode       缓存区运行错误码：
 * @param  pCurrentId       缓存区当前运行指令ID
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x5404)
 *******************************************************************/
IMC_API IMC_CmdListGetStatus(short cardIndex, short index, short* pStatus, short* pErrorCode, int* pCurrentId);

/**
 * @brief  获取指令缓存区剩余空间
 * @param  cardIndex        板卡卡号, 参数范围[0 ~ 3]
 * @param  index            缓存区索引，参数范围：[0,7]
 * @param  pSpace           缓存区剩余空间
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x5405)
 *******************************************************************/
IMC_API IMC_CmdListGetSpace(short cardIndex, short index, int* pSpace);

/**
 * @brief  清除指令缓存区数据
 * @param  cardIndex        板卡卡号, 参数范围[0 ~ 3]
 * @param  index            缓存区索引，参数范围：[0,7]
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x5406)
 *******************************************************************/
IMC_API IMC_CmdListClearData(short cardIndex, short index);

/**
 * @brief  清除指令缓存区错误
 * @param  cardIndex        板卡卡号, 参数范围[0 ~ 3]
 * @param  index            缓存区索引，参数范围：[0,7]
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x5407)
 *******************************************************************/
IMC_API IMC_CmdListClearError(short cardIndex, short index);

/**
 * @brief  设置指令缓存区循环执行次数，仅静态模式下可设置
 * @param  cardIndex        板卡卡号, 参数范围[0 ~ 3]
 * @param  index            缓存区索引，参数范围：[0,7]
 * @param  count            缓存区循环执行次数
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x5408)
 *******************************************************************/
IMC_API IMC_CmdListSetLoop(short cardIndex, short index, int count);

/**
 * @brief  获取指令缓存区循环执行次数
 * @param  cardIndex        板卡卡号, 参数范围[0 ~ 3]
 * @param  index            缓存区索引，参数范围：[0,7]
 * @param  pSetCount        缓存区循环设置次数
 * @param  pRunCount        缓存区循环已执行次数
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x5409)
 *******************************************************************/
IMC_API IMC_CmdListGetLoop(short cardIndex, short index, int* pSetCount, int* pRunCount);

/**
 * @brief  设置指令缓存区执行优先级
 * @param  cardIndex        板卡卡号, 参数范围[0 ~ 3]
 * @param  index            缓存区索引，参数范围：[0,7]
 * @param  priority         缓存区执行优先级
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x5408)
 *******************************************************************/
IMC_API IMC_CmdListSetPriority(short cardIndex, short index, short priority);

/**
 * @brief  获取指令缓存区执行优先级
 * @param  cardIndex        板卡卡号, 参数范围[0 ~ 3]
 * @param  index            缓存区索引，参数范围：[0,7]
 * @param  pPriority        缓存区执行优先级
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x5409)
 *******************************************************************/
IMC_API IMC_CmdListGetPriority(short cardIndex, short index, short* pPriority);

/**
 * @brief  缓存区运动指令，执行轴PTP运动
 * @param  cardIndex        板卡卡号, 参数范围[0 ~ 3]
 * @param  index            缓存区索引，参数范围：[0,7]
 * @param  axNo             操作轴号，参数范围：[0，最大支持数）
 * @param  tgtPos           目标位置（绝对位置）
 * @param  userID           缓存指令ID
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x5411)
 *******************************************************************/
IMC_API IMC_CmdListAxMove(short cardIndex, short index, short axNo, double tgtPos, int userID);

/**
 * @brief  缓存区运动指令，在线更新ptp运动目标位置
 * @param  cardIndex        板卡卡号, 参数范围[0 ~ 3]
 * @param  index            缓存区索引，参数范围：[0,7]
 * @param  axNo             操作轴号，参数范围：[0，最大支持数）
 * @param  tgtPos           更新的目标位置（绝对位置）
 * @param  userID           缓存指令ID
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x5412)
 *******************************************************************/
IMC_API IMC_CmdListUpdateAxMovePos(short cardIndex, short index, short axNo, double tgtPos, int userID);

/**
 * @brief  缓存区运动指令，在线更新轴运动速度参数
 * @param  cardIndex        板卡卡号, 参数范围[0 ~ 3]
 * @param  index            缓存区索引，参数范围：[0,7]
 * @param  axNo             操作轴号，参数范围：[0，最大支持数）
 * @param  tgtVel           更新的目标速度
 * @param  tgtAcc           更新的目标加速度
 * @param  tgtDec           更新的目标减速度
 * @param  userID           缓存指令ID
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x5413)
 *******************************************************************/
IMC_API IMC_CmdListUpdateAxMovePara(short cardIndex, short index, short axNo, double tgtVel, double tgtAcc, double tgtDec, int userID);

/**
 * @brief  缓存区运动指令，执行轴ptp运动，目标位置来源为全局变量值
 * @param  cardIndex        板卡卡号, 参数范围[0 ~ 3]
 * @param  index            缓存区索引，参数范围：[0,7]
 * @param  axNo             操作轴号，参数范围：[0，最大支持数）
 * @param  varIndex         全局变量索引，参数范围：[0，最大支持数）
 * @param  userID           缓存指令ID
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x5414)
 *******************************************************************/
IMC_API IMC_CmdListAxMoveUserVal(short cardIndex, short index, short axNo, short varIndex, int userID);

/**
 * @brief  缓存区运动指令，执行轴运动停止
 * @param  cardIndex        板卡卡号, 参数范围[0 ~ 3]
 * @param  index            缓存区索引，参数范围：[0,7]
 * @param  axNo             操作轴号，参数范围：[0，最大支持数）
 * @param  stopType         停止类型，0-平滑停止 1-紧急停止
 * @param  userID           缓存指令ID
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x5415)
 *******************************************************************/
IMC_API IMC_CmdListStopMove(short cardIndex, short index, short axNo, short stopType, int userID);

/**
 * @brief  缓存区等待指令，延时等待一段时间
 * @param  cardIndex        板卡卡号, 参数范围[0 ~ 3]
 * @param  index            缓存区索引，参数范围：[0,7]
 * @param  waitTime         等待时间，单位ms
 * @param  userID           缓存指令ID
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x5421)
 *******************************************************************/
IMC_API IMC_CmdListWaitTime(short cardIndex, short index, short waitTime, int userID);

/**
 * @brief  缓存区等待指令，等待DI触发
 * @param  cardIndex        板卡卡号, 参数范围[0 ~ 3]
 * @param  index            缓存区索引，参数范围：[0,7]
 * @param  diNo             操作DI索引，参数范围：[0，最大支持数）
 * @param  diLevel          DI输入状态
 * @param  diType           DI类型： 0-ECAT 1-端子板
 * @param  userID           缓存指令ID
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x5422)
 *******************************************************************/
IMC_API IMC_CmdListWaitDi(short cardIndex, short index, short diNo, short diLevel, short diType, int userID);

/**
 * @brief  缓存区等待指令，等待轴运动到位
 * @param  cardIndex        板卡卡号, 参数范围[0 ~ 3]
 * @param  index            缓存区索引，参数范围：[0,7]
 * @param  axNo             操作轴号，参数范围：[0，最大支持数）
 * @param  arrvType         到位类型： 0-实际到位 1-规划到位
 * @param  userID           缓存指令ID
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x5423)
 *******************************************************************/
IMC_API IMC_CmdListWaitAxMoveDone(short cardIndex, short index, short axNo, short arrvType, int userID);

/**
 * @brief  缓存区等待指令，等待轴位置穿越位置
 * @param  cardIndex        板卡卡号, 参数范围[0 ~ 3]
 * @param  index            缓存区索引，参数范围：[0,7]
 * @param  axNo             操作轴号，参数范围：[0，最大支持数）
 * @param  posType          位置类型 0-编码器位置 1-规划位置
 * @param  Pos              目标位置值
 * @param  crossType        穿越条件：
 *                          \n 0-等于
 *                          \n 1-大于
 *                          \n 2-小于
 *                          \n 3-大于等于
 *                          \n 4-小于等于
 *                          \n 5-穿越
 * @param  userID           缓存指令ID
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x5424)
 *******************************************************************/
IMC_API IMC_CmdListWaitAxPosCross(short cardIndex, short index, short axNo, short posType, int Pos, short crossType, int userID);

/**
 * @brief  缓存区等待指令，等待全局变量值
 * @param  cardIndex        板卡卡号, 参数范围[0 ~ 3]
 * @param  index            缓存区索引，参数范围：[0,7]
 * @param  varIndex         操作全局变量索引，参数范围：[0，最大支持数）
 * @param  varValue         全局变量值
 * @param  userID           缓存指令ID
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x5425)
 *******************************************************************/
IMC_API IMC_CmdListWaitUserVal(short cardIndex, short index, short varIndex, int varValue, int userID);

/**
 * @brief  缓存区等待指令，等待事件执行完成
 * @param  cardIndex        板卡卡号, 参数范围[0 ~ 3]
 * @param  index            缓存区索引，参数范围：[0,7]
 * @param  varIndex         操作事件索引，参数范围：[0，最大支持数）
 * @param  userID           缓存指令ID
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x5426)
 *******************************************************************/
IMC_API IMC_CmdListWaitEventDone(short cardIndex, short index, short eventIndex, int userID);

/**
 * @brief  缓存区等待指令，等待端子板编码器数据
 * @param  cardIndex        板卡卡号, 参数范围[0 ~ 3]
 * @param  index            缓存区索引，参数范围：[0,7]
 * @param  encIndex         操作编码器通道号，参数范围：[0，3）
 * @param  waitType         等待条件：
 *                          \n 0-等于
 *                          \n 1-大于
 *                          \n 2-小于
 *                          \n 3-大于等于
 *                          \n 4-小于等于
 *                          \n 5-穿越
 * @param  waitValue         目标位置值
 * @param  userID           缓存指令ID
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x5427)
 *******************************************************************/
IMC_API IMC_CmdListWaitLocalEnc(short cardIndex, short index, short encIndex, short waitType, int waitValue, int userID);

/**
 * @brief  缓存区等待指令，等待从站TxPDO对象数据
 * @param  cardIndex        板卡卡号, 参数范围[0 ~ 3]
 * @param  index            缓存区索引，参数范围：[0,7]
 * @param  pdoOffset        操作TxPDO字节偏移值；
 * @param  pdoLen           操作的PDO字节大小，取值范围为 2 或 4
 * @param  waitType         等待条件：
 *                          \n 0-等于
 *                          \n 1-大于
 *                          \n 2-小于
 *                          \n 3-大于等于
 *                          \n 4-小于等于
 * @param  waitValue        等待数据值
 * @param  userID           缓存指令ID
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x5428)
 *******************************************************************/
IMC_API IMC_CmdListWaitTxPDO(short cardIndex, short index, short pdoOffset, short pdoLen, short waitType, int waitValue, int userID);

/**
 * @brief  缓存区等待指令，等待Ecat从站Ad数据
 * @param  cardIndex        板卡卡号, 参数范围[0 ~ 3]
 * @param  index            缓存区索引，参数范围：[0,7]
 * @param  adIndex          操作EcatAD通道索引；
 * @param  waitType         等待条件：
 *                          \n 0-等于
 *                          \n 1-大于
 *                          \n 2-小于
 *                          \n 3-大于等于
 *                          \n 4-小于等于
 * @param  waitValue        等待数据值
 * @param  userID           缓存指令ID
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x5429)
 *******************************************************************/
IMC_API IMC_CmdListWaitEcatAD(short cardIndex, short index, short adIndex, short waitType, int waitValue, int userID);

/**
 * @brief  缓存区设置指令，设置DO输出
 * @param  cardIndex        板卡卡号, 参数范围[0 ~ 3]
 * @param  index            缓存区索引，参数范围：[0,7]
 * @param  doNo             操作DO索引，参数范围：[0，最大支持数）
 * @param  doLevel          操作DO输出状态
 * @param  doType           操作DO类型：0-ECAT 1-端子板
 * @param  userID           缓存指令ID
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x5431)
 *******************************************************************/
IMC_API IMC_CmdListSetDo(short cardIndex, short index, short doNo, short doLevel, short doType, int userID);

/**
 * @brief  缓存区设置指令，设置脉冲DO输出
 * @param  cardIndex        板卡卡号, 参数范围[0 ~ 3]
 * @param  index            缓存区索引，参数范围：[0,7]
 * @param  outIndex         操作脉冲DO输出索引，参数范围：[0，最大支持数）
 * @param  userID           缓存指令ID
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x5432)
 *******************************************************************/
IMC_API IMC_CmdListSetPulseDo(short cardIndex, short index, short outIndex, int userID);

/**
 * @brief  缓存区设置指令，设置DO输出
 * @param  cardIndex        板卡卡号, 参数范围[0 ~ 3]
 * @param  index            缓存区索引，参数范围：[0,7]
 * @param  varIndex         操作全局变量索引，参数范围：[0，最大支持数）
 * @param  varValue         设置全局变量值
 * @param  userID           缓存指令ID
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x5433)
 *******************************************************************/
IMC_API IMC_CmdListSetUserVal(short cardIndex, short index, short varIndex, int varValue, int userID);

/**
 * @brief  缓存区设置指令，设置事件执行
 * @param  cardIndex        板卡卡号, 参数范围[0 ~ 3]
 * @param  index            缓存区索引，参数范围：[0,7]
 * @param  eventIndex       操作事件索引，参数范围：[0，最大支持数）
 * @param  userID           缓存指令ID
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x5434)
 *******************************************************************/
IMC_API IMC_CmdListSetEventTrigger(short cardIndex, short index, short eventIndex, int userID);

/**
 * @brief  缓存区设置指令，设置从站RxPDO对象值
 * @param  cardIndex        板卡卡号, 参数范围[0 ~ 3]
 * @param  index            缓存区索引，参数范围：[0,7]
 * @param  pdoOffset        操作TxPDO字节偏移值；
 * @param  pdoLen           操作的PDO字节大小，取值范围为 2 或 4
 * @param  setValue         设置数据值
 * @param  userID           缓存指令ID
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x5435)
 *******************************************************************/
IMC_API IMC_CmdListSetRxPDO(short cardIndex, short index, short pdoOffset, short pdoLen, int setValue, int userID);

/**
 * @brief  缓存区设置指令，设置Ecat从站DA通道值
 * @param  cardIndex        板卡卡号, 参数范围[0 ~ 3]
 * @param  index            缓存区索引，参数范围：[0,7]
 * @param  daIndex          操作DA通道索引；
 * @param  setValue         设置数据值
 * @param  userID           缓存指令ID
 * @retval #EXE_SUCCESS     指令成功
 * @par 指令码              (0x5436)
 *******************************************************************/
IMC_API IMC_CmdListSetEcatDA(short cardIndex, short index, short daIndex, int setValue, int userID);

/// @}