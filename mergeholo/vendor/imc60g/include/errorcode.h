
#define EXE_SUCCESS                                    (0x0000) // 指令成功

// 指令接收错误
#define ERR_TRANSMIT                                   (0x0001) // 指令传输错误
#define ERR_UNKNOWN                                    (0x0002) // 不支持的指令
#define ERR_PARSE                                      (0x0003) // 指令解析错误
#define ERR_CMD_EXECUTE                                (0x0006) // 执行命令错误
#define ERR_INPUT                                      (0x0009) // 指针参数错误

// 索引参数错误
#define ERR_INDEX_OUTRANG                              (0x0020) // 索引超出范围
#define ERR_CARD_INDEX_OUTRANG                         (0x0021) // 卡号索引超出范围
#define ERR_SLAVE_INDEX_OUTRANG                        (0x0022) // 从站索引超出范围
#define ERR_AX_INDEX_OUTRANG                           (0x0023) // 轴号索引超出范围
#define ERR_AXCHN_INDEX_OUTRANG                        (0x0024) // 轴通道索引超出范围
#define ERR_CRD_INDEX_OUTRANG                          (0x0025) // 坐标系号超出范围

#define ERR_GRP_INDEX_OUTRANG                          (0x0026) // 组号索引超出范围
#define ERR_CHN_INDEX_OUTRANG                          (0x0027) // 通道号索引超出范围

#define ERR_DIO_INDEX_OUTRANG                          (0x0028) // DIO索引号超出范围
#define ERR_AIO_INDEX_OUTRANG                          (0x0029) // AIO索引号超出范围
#define ERR_REG_INDEX_OUTRANG                          (0x002a) // Reg索引号超出范围
#define ERR_ENC_INDEX_OUTRANG                          (0x002b) // Enc索引号超出范围
#define ERR_TABLE_INDEX_OUTRANG                        (0x002c) // 数据表索引号超出范围

// 数量参数错误
#define ERR_PARAM_OUTRANG                              (0x0040) // 参数超出范围
#define ERR_COUNT_OUTRANG                              (0x0041) // 数量参数超出范围
#define ERR_LENGTH_OUTRANG                             (0x0042) // 长度参数超出范围
#define ERR_METHOD_OUTRANG                             (0x0043) // 方法参数超出范围
#define ERR_TYPE_OUTRANG                               (0x0044) // 类型参数超出范围
#define ERR_OFFSET_OUTRANG                             (0x0045) // 偏置参数超出范围
#define ERR_MODE_OUTRANG                               (0x0046) // 模式参数超出范围
#define ERR_BIT_OUTRANG                                (0x0047) // bit位参数超出范围
#define ERR_CONDITION_OUTRANG                          (0x0048) // 条件参数超出范围

// 布尔类型参数错误
#define ERR_ONOFF_PARA                                 (0x0060) // 开关参数非0或1
#define ERR_FLAG_PARA                                  (0x0061) // 标志参数非0或1
#define ERR_LEVEL_PARA                                 (0x0062) // 电平参数非0或1
#define ERR_TRIG_PARA                                  (0x0063) // 触发沿参数非0或1
#define ERR_DIR_PARA                                   (0x0064) // 方向参数非0或1
#define ERR_INVERSE_PARA                               (0x0065) // 取反参数非0或1
#define ERR_ENABLE_PARA                                (0x0066) // 使能参数非0或1

// 时间参数错误
#define ERR_TIME_PARA_OUTRANG                          (0x0080) // 时间参数超出范围
#define ERR_FLTTIME_OUTRANG                            (0x0081) // 滤波时间参数超出范围

// 缓冲区相关错误
#define ERR_FIFO_FULL                                  (0x00f0) // 缓冲区数据满
#define ERR_FIFO_EMPTY                                 (0x00f1) // 缓冲区数据空
#define ERR_FIFO_NOT_EMPTY                             (0x00f2) // 缓冲区数据不为空

// 数据表相关错误
#define ERR_TABLE_NUM                                  (0x00fc) // 数据表个数超限
#define ERR_TABLE_FULL                                 (0x00fd) // 数据表满
#define ERR_TABLE_EMPTY                                (0x00fe) // 数据表空
#define ERR_TABLE_BUSY                                 (0x00ff) // 数据表忙

// 运动参数错误
#define ERR_VEL_OUTRANG                                (0x0100) // 速度参数超出范围
#define ERR_ACC_OUTRANG                                (0x0101) // 加速度参数超出范围
#define ERR_DEC_OUTRANG                                (0x0102) // 减速度参数超出范围
#define ERR_TGTPOS_OUTRANG                             (0x0103) // 目标位置参数超出范围
#define ERR_ENDVEL_OUTRANG                             (0x0104) // 结束速度参数超出范围
#define ERR_RATIO_OUTRANG                              (0x0105) // 倍率参数超出范围
#define ERR_EQV_OUTRANG                                (0x0106) // 当量参数超出范围
#define ERR_TOL_OUTRANG                                (0x0107) // 精度参数超出范围
#define ERR_STVEL_OUTRANG                              (0x0104) // 起始速度参数超出范围
#define ERR_JERK_OUTRANG                               (0x0109) // 跃度参数超出范围

#define ERR_BGVEL_OUTRANG                              (0x0120) // 起始速度超出范围
#define ERR_MAXVEL_OUTRANG                             (0x0121) // 最大速度超出范围
#define ERR_MAXACC_OUTRANG                             (0x0122) // 最大加速度超出范围
#define ERR_MAXDEC_OUTRANG                             (0x0123) // 最大减速度设置超出范围
#define ERR_MAXJERK_OUTRANG                            (0x0124) // 最大加加速度超出范围
#define ERR_STOPDEC_OUTRANG                            (0x0125) // 平滑停止速度超出范围
#define ERR_ESTOPDEC_OUTRANG                           (0x0126) // 紧急停止速度超出范围
#define ERR_ARRIVEDBAND_OUTRANG                        (0x0127) // 到位误差超出范围
#define ERR_ERRLMT_OUTRANG                             (0x0128) // 跟随误差超出范围

// 资源超限错误
#define ERR_SLAVE_CNT_OUTRANG                          (0x0160) // 从站数量超出范围
#define ERR_ECAT_AX_CNT_OUTRANG                        (0x0161) // ECAT轴数量超出范围
#define ERR_DIO_MOD_CNT_OUTRANG                        (0x0162) // ECAT DIO模块数量超出范围
#define ERR_AIO_MOD_CNT_OUTRANG                        (0x0163) // ECAT AIO模块数量超出范围
#define ERR_REG_MOD_CNT_OUTRANG                        (0x0164) // ECAT REG模块数量超出范围

#define ERR_DI_CNT_OUTRANG                             (0x0180) // ECAT DI数量超出范围
#define ERR_DO_CNT_OUTRANG                             (0x0181) // ECAT DO数量超出范围
#define ERR_AI_CNT_OUTRANG                             (0x0182) // ECAT AI数量超出范围
#define ERR_AO_CNT_OUTRANG                             (0x0183) // ECAT AO数量超出范围
#define ERR_REGIN_CNT_OUTRANG                          (0x0184) // ECAT RegIn数量超过范围
#define ERR_REGOUT_CNT_OUTRANG                         (0x0185) // ECAT RegOut数量超过范围

#define ERR_NO_ECAT_DO                                 (0x01a0) // ECAT DO模块未配置
#define ERR_NO_ECAT_DI                                 (0x01a1) // ECAT DI模块未配置
#define ERR_NO_ECAT_AO                                 (0x01a2) // ECAT AO模块未配置
#define ERR_NO_ECAT_AI                                 (0x01a3) // ECAT AI模块未配置
#define ERR_NO_ECAT_REGIN                              (0x01a4) // ECAT RegIn模块未配置
#define ERR_NO_ECAT_REGOUT                             (0x01a5) // ECAT RegOut模块未配置

// 板卡ECAT相关错误
#define ERR_ECAT_MASTER_LINK                           (0x0200) // ECAT主站连接错误，条件不满足
#define ERR_ECAT_SLAVE_LINK                            (0x0201) // ECAT从站连接错误

#define ERR_ECAT_PDO_LEN_OUTRANG                       (0x0210) // ECAT的Pdo长度超过范围
#define ERR_ECAT_PDO_OFS_LEN                           (0x0211) // ECAT的Pdo偏移或长度错误
#define ERR_ECAT_PDO_OFS_BIT_LEN                       (0x0212) // ECAT的Pdo对象字bit偏移非0长度跨字节

#define ERR_ECAT_PDO_NOT_EXIST                         (0x0218) // ECAT的Pdo对象未配置
#define ERR_ECAT_PDO_NOT_SUPPORT                       (0x0219) // ECAT的Pdo对象不支持

#define ERR_PDO_CTRLWORD_NO_CFG                        (0x0220) // ECAT轴的Pdo对象字控制字0x6040未配置
#define ERR_PDO_STSWORD_NO_CFG                         (0x0221) // ECAT轴的Pdo对象字状态字0x6041未配置
#define ERR_PDO_TGTPOS_NO_CFG                          (0x0222) // ECAT轴的Pdo对象字目标位置0x607a未配置
#define ERR_PDO_ATLPOS_NO_CFG                          (0x0223) // ECAT轴的Pdo对象字实际位置0x6064未配置
#define ERR_PDO_TGTVEL_NO_CFG                          (0x0224) // ECAT轴的Pdo对象字目标速度0x60ff未配置
#define ERR_PDO_ATLVEL_NO_CFG                          (0x0225) // ECAT轴的Pdo对象字实际速度0x606c未配置
#define ERR_PDO_TORQ_SLP_NO_CFG                        (0x0226) // ECAT轴Pdo对象字转矩斜坡0x6087未配置
#define ERR_PDO_TGT_TORQ_NO_CFG                        (0x0227) // ECAT轴Pdo对象字目标转矩0x6071未配置
#define ERR_PDO_ATLTRQ_NO_CFG                          (0x0228) // ECAT轴的Pdo对象字实际转矩0x6077未配置
#define ERR_PDO_ATLFERR_NO_CFG                         (0x0229) // ECAT轴的Pdo对象字实际跟随误差0x60f4未配置
#define ERR_PDO_ERRCODE_NO_CFG                         (0x022a) // ECAT轴的Pdo对象字错误码0x603f未配置
#define ERR_PDO_AX_DI_CFG                              (0x022b) // ECAT轴Pdo对象字Di输入0x60fd未配置
#define ERR_PDO_AX_DO_CFG                              (0x022c) // ECAT轴Pdo对象字Do输出0x60fe未配置
#define ERR_PDO_OP_MODE_NO_CFG                         (0x022d) // ECAT轴Pdo对象字控制模式0x6060未配置

#define ERR_PDO_PROBE_FUN_NO_CFG                       (0x0230) // ECAT轴Pdo对象字探针功能字0x60b8未配置
#define ERR_PDO_PROBE_STS_NO_CFG                       (0x0231) // ECAT轴Pdo对象字探针状态字0x60b9未配置
#define ERR_PDO_PROBE1_RPOS_NO_CFG                     (0x0232) // ECAT轴Pdo对象字探针1上升沿位置0x60ba未配置
#define ERR_PDO_PROBE1_FPOS_NO_CFG                     (0x0233) // ECAT轴Pdo对象字探针1下降沿位置0x60bb未配置
#define ERR_PDO_PROBE2_RPOS_NO_CFG                     (0x0234) // ECAT轴Pdo对象字探针2上升沿位置0x60bc未配置
#define ERR_PDO_PROBE2_FPOS_NO_CFG                     (0x0235) // ECAT轴Pdo对象字探针2下降沿位置0x60bd未配置

#define ERR_PDO_MAXVEL_NO_CFG                          (0x0240) // ECAT轴Pdo对象字最大速度限制0x607f未配置
#define ERR_PDO_POS_TORQ_LMT_NO_CFG                    (0x0241) // ECAT轴Pdo对象字正向转矩限制0x60e0未配置
#define ERR_PDO_NEG_TORQ_LMT_NO_CFG                    (0x0242) // ECAT轴Pdo对象字负向转矩限制0x60e1未配置
#define ERR_PDO_MAX_TORQ_LMT_NO_CFG                    (0x0243) // ECAT轴Pdo对象字最大转矩限制0x6072未配置

#define ERR_PDO_OFSPOS_NO_CFG                          (0x0248) // ECAT轴的Pdo对象字位置偏移0x60b0未配置
#define ERR_PDO_OFSVEL_NO_CFG                          (0x0249) // ECAT轴的Pdo对象字速度偏移0x60b1未配置
#define ERR_PDO_OFSTRQ_NO_CFG                          (0x024a) // ECAT轴的Pdo对象字力矩偏移0x60b2未配置

#define ERR_CTRL_MODE                                  (0x0250) // 错误控制模式
#define ERR_MODE_NOT_CSP                               (0x0251) // 控制模式非CSP模式
#define ERR_MODE_NOT_CSV                               (0x0252) // 控制模式非CSV模式
#define ERR_MODE_NOT_CST                               (0x0253) // 控制模式非CST模式
#define ERR_MODE_NOT_HOME                              (0x0254) // 控制模式非Home模式

#define ERR_MODE_NOT_HOME_CSP                          (0x0258) // 控制模式非CSP或Home模式

#define ERR_CSV_PRF_NOT_START                          (0x0260) // CSV规划没有启动
#define ERR_CSV_IN_PRFING                              (0x0261) // CSV在规划中，不允许切换轴模式

#define ERR_TGTTRQ_OUTRANG                             (0x0270) // 目标转矩设置超出范围

#define ERR_ECAT_CAPT_DISEN                            (0x0280) // ECAT捕获未使能
#define ERR_ECAT_CAPT_TABLE_UNBIND                     (0x0281) // ECAT连续捕获未绑定缓存表

// 轴安全参数错误
#define ERR_ALRM_ENABLE                                (0x0300) // 报警使能参数非0或1
#define ERR_SOFTLMT_ENABLE                             (0x0301) // 软限位使能参数非0或1
#define ERR_HWLMT_ENABLE                               (0x0302) // 硬限位使能参数非0或1
#define ERR_ERRLMT_ENABLE                              (0x0303) // 跟随误差报警使能参数非0或1

#define ERR_HW_ESTP_IS_TRIG                            (0x0310) // 硬件急停信号触发
#define ERR_AX_MV_DIR_LIMT_TRIG                        (0x0311) // 轴运动方向的限位触发
#define ERR_AX_FOLLOW_ERROR                            (0x0312) // 轴跟随误差报警

#define ERR_TGTPOS_OVER_SOFTLMT                        (0x0318) // 目标位置超出了软限位范围
#define ERR_SOFTLMT_POS_LESS_NEG                       (0x0319) // 软正限位设置小于软负限位

// 轴状态相关错误
#define ERR_PRF_VEL_NOT_ZERO                           (0x0330) // 规划速度不为零或未到位
#define ERR_ENC_VEL_NOT_ZERO                           (0x0331) // 反馈速度不为零

#define ERR_AX_SVON                                    (0x0338) // 轴使能状态
#define ERR_AX_ALARM                                   (0x0339) // 伺服报警
#define ERR_AX_SVOFF                                   (0x033a) // 掉使能报警
#define ERR_AX_BUSY                                    (0x033c) // 轴正处于规划中
#define ERR_AX_ABNOR                                   (0x033d) // 轴异常报警

#define ERR_AX_BOND_SAME_CHN                           (0x0350) // 轴绑定了相同的一个物理轴
#define ERR_AX_NOT_ECAT                                (0x0351) // 非ECAT伺服轴

#define ERR_AX_MAPPED_CRD                              (0x0360) // 轴已映射至插补坐标系
#define ERR_AX_MAPPED_MULTI                            (0x0361) // 轴已映射至多轴模式
#define ERR_AX_MAPPED_PT                               (0x0362) // 轴已映射至PT模式

// 轴模式相关错误
#define ERR_PRF_MODE                                   (0x0380) // 错误的规划模式
#define ERR_PRF_MODE_NOT_PTP                           (0x0381) // 非PTP规划模式
#define ERR_PRF_MODE_NOT_JOG                           (0x0382) // 非JOG规划模式

#define ERR_HOMING_MODE                                (0x0390) // 轴正处于回零中
#define ERR_HOMING_CSP                                 (0x0391) // 轴正处于CSP回零中
#define ERR_HOMING_CIA                                 (0x0392) // 轴正处于CIA402回零中
#define ERR_HOMING_NOT_CSP                             (0x0393) // 非CSP回零
#define ERR_HOMING_NOT_CIA                             (0x0394) // 非CIA402回零

// 单轴运动Ptp，Jog
#define ERR_PTP_STOPPED                                (0x0400) // PTP运动已停止，不能调用Pause指令

// 单轴运动Ptpc
#define ERR_PTPC_MOVE_PARAM                            (0x0420) // PTP缓存模式运动参数有误
#define ERR_PTPC_STPOS_DIFF_CURPOS                     (0x0421) // 队列起始位置与当前位置不一致

// 单轴运动PtpS
#define ERR_PTPS_SLOW_POS							   (0x0430) // PTPS低速位置错误

// 单轴运动Gear
#define ERR_GEAR_MASTER_SCALE                          (0x0450) // 主轴齿数超出范围
#define ERR_GEAR_SLAVE_SCALE                           (0x0451) // 从轴齿数超出范围
#define ERR_GEAR_MASTER_INDEX                          (0x0452) // 主轴索引超限
#define ERR_GEAR_DIR_PARA                              (0x0453) // 跟随方向类型超出范围
#define ERR_GEAR_MASTER_SLOPE                          (0x0454) // 主轴离合区超出范围
#define ERR_GEAR_MASTER_CFG                            (0x0455) // 主轴未配置
#define ERR_GEAR_UPDATE_EVEN                           (0x0456) // 更新齿轮比不在同步阶段
#define ERR_GEAR_REPEAT                                (0x0457) // 主轴和从轴的轴号重复

// 单轴运动Pvt
#define ERR_PVT_TABLE_TIME                             (0x0470) // 数据表时间错误，运动时间少于插补周期
#define ERR_PVT_LOOP                                   (0x0471) // 循环次数错误
#define ERR_PVT_PERCENT                                (0x0472) // 百分比参数错误

// 单轴运动CAM
#define ERR_CAM_MASTER_CFG                             (0x0480) // 主轴未配置
#define ERR_CAM_MASTER_INDEX                           (0x0481) // 主轴索引超限
#define ERR_CAM_MASTER_REPEAT                          (0x0482) // 主轴索引与从轴重复
#define ERR_CAM_DIR_MODE                               (0x0483) // 跟随方向类型超出范围
#define ERR_CAM_PERCENT_OUTRANGE                       (0x0484) // 百分比参数超限
#define ERR_CAM_CURVE_TYPE                             (0x0485) // 曲线类型错误
#define ERR_CAM_PHASING_FORBID                         (0x0486) // 当前状态不允许执行主轴相位偏移
#define ERR_CAM_PHASING_BUSY                           (0x0487) // 主轴相位偏移忙
#define ERR_CAM_MASTER_DIST                            (0x0488) // 主轴位移增量为零或反向
#define ERR_CAM_INPUT_DIST                             (0x0489) // 输入的位移超出范围

// 单轴运动Gear，Pvt，follow

// 多轴运动Crd
#define ERR_CRD_DIMEN_OUTRANG                          (0x0500) // 坐标系维数超出范围
#define ERR_CRD_MASK_AXNO_OUTRANG                      (0x0501) // 坐标系输出轴映射号超出范围
#define ERR_CRD_MASK_SAME_AXNO                         (0x0502) // 坐标系输出轴映射到相同端口错误
#define ERR_CRD_MSYNC_AX_SAME                          (0x0503) // 坐标系多轴同步轴号重复

#define ERR_CRD_NOT_EXIST                              (0x0510) // 坐标系不存在
#define ERR_CRD_REPEATED_CREATED                       (0x0511) // 坐标系重复创建
#define ERR_CRD_CREATE_FAILED                          (0x0512) // 坐标系创建失败
#define ERR_CRD_BUSY                                   (0x0513) // 坐标系正处于规划中
#define ERR_CRD_RUN_ERR                                (0x0514) // 坐标系运行报警错误，需要清除才能再次启动或者压入数据
#define ERR_CRD_PAUSE_POS_CHANGE                       (0x0515) // 坐标系暂停后，位置被改变
#define ERR_CRD_BUFFERMODE_NOT_STATIC                  (0x0516) // 坐标系缓冲区不为静态模式

#define ERR_CRD_BUF_FULL                               (0x0520) // 坐标系数据缓冲区满
#define ERR_CRD_BUF_DATA_PARA                          (0x0521) // 坐标系缓冲区指令参数错误
#define ERR_CRD_BUF_NOT_EMPTY                          (0x0522) // 坐标系参数设置未满足缓冲区数据为空要求
#define ERR_CRD_DOWNLOAD_CNT                           (0x0523) // 坐标系缓冲区指令下发个数超限

#define ERR_CRD_ARC_DIR_PARA                           (0x0530) // 坐标系中圆弧运动的方向参数错误
#define ERR_CRD_INLINE                                 (0x0531) // 圆弧三点共线
#define ERR_CRD_NORMAL_TOO_SHORT                       (0x0532) // 圆弧输入的法向量太短
#define ERR_CRD_RADIUS_TOO_SHORT                       (0x0533) // 圆弧输入的半径太短
#define ERR_CRD_NORMAL_ANGLE                           (0x0534) // 圆弧输入的法向量角度错误
#define ERR_CRD_RADIUS_ERR                             (0x0535) // 特指圆弧半径输入模式下，半径小于首点到末点长度的一半
#define ERR_CRD_DATA_SEG_ERR                           (0x0536) // 插补两段衔接位置错误，上一段的终点与当前段起点偏差过大
#define ERR_CRD_DIS_ERR                                (0x0537) // 坐标系输入线段弧长过短

#define ERR_CRD_COPLAN_PARA                            (0x0540) // 坐标系异面过渡参数设置错误
#define ERR_CRD_TURN_PARA                              (0x0541) // 坐标系拐弯系数设置错误
#define ERR_CRD_TRANSMODE_PARA                         (0x0542) // 坐标系过渡模式设置错误
#define ERR_CRD_NODATA_PROTECT_PARA                    (0x0543) // 坐标系数据断流保护参数错误
#define ERR_CRD_ARC_ACC_PARA                           (0x0544) // 坐标系圆弧变加速参数设置错误
#define ERR_CRD_TRAJ_TYPE_PARA                         (0x0545) // 坐标系速度规划类型参数设置错误
#define ERR_CRD_FATAL_DATA_ERR                         (0x0546) // 坐标系压入数据致命错误
#define ERR_CRD_DO_TYPE                                (0x0547) // 坐标系压入的DO类型错误
#define ERR_CRD_SEG_TYPE                               (0x0548) // 坐标系段类型参数错误
#define ERR_CRD_WAIT_TYPE                              (0x0549) // 坐标系等待段类型参数错误
#define ERR_CRD_OTHER_TYPE                             (0x054a) // 坐标系其他段类型参数错误
#define ERR_CRD_SMOOTH_COEF                            (0x054b) // 坐标系平滑系数设置错误
#define ERR_CRD_SMOOTH_TOL                             (0x054c) // 坐标系平滑精度设置错误
#define ERR_CRD_MODE_UNMACHED                          (0x054d) // 坐标系不匹配的插补模式，如：非用户规划模式
#define ERR_CRD_CRDNO_REPEATED                         (0x054e) // 坐标系号有重复

// 多轴同步运动0x560
#define ERR_SYNCMOVE_NOT_EXIST                         (0x0560) // 多轴系统未建立
#define ERR_SYNCMOVE_HAVE_EXISTED                      (0x0561) // 多轴系统重复创建
#define ERR_SYNCMOVE_COUNT                             (0x0562) // 多轴系统轴映射总数量不在范围内
#define ERR_SYNCMOVE_AXNO_REPEATED                     (0x0563) // 多轴系统的映射轴有重复
#define ERR_SYNCMOVE_AXNO_OUTRANG                      (0x0564) // 多轴系统的映射轴不在系统范围

#define ERR_SYNCMOVE_PAUSE_POS_CHANGE                  (0x0570) // 多轴系统暂停后，位置被改变
#define ERR_SYNCMOVE_ALARM                             (0x0571) // 多轴系统有报警，不允许启动

#define ERR_SYNCMOVE_SEG_DIS                           (0x0580) // 多轴系统当前段位移量太短

// 绑定PT运动0x590
#define ERR_BANDPT_COUNT                               (0x0590) // 绑定PT轴映射总数量不在范围内
#define ERR_BANDPT_AXNO_OUTRANG                        (0x0591) // 绑定PT轴超过范围
#define ERR_BANDPT_AXNO_REPEATED                       (0x0592) // 绑定PT轴重复

#define ERR_BANDPT_NOT_EXIST                           (0x05a0) // 绑定PT系统轴没有建立
#define ERR_BANDPT_BEYOND_MAXCNT                       (0x05a1) // 绑定PT输入数据量超过单次最大限制
#define ERR_BANDPT_ERR                                 (0x05a2) // 绑定PT处于错误状态
#define ERR_BANDPT_HAVE_EXISTED                        (0x05a3) // 绑定PT系统重复创建

// 立即插补
#define ERR_MOVECRD_AXNO_REPEATED                      (0x05b0) // 立即插补轴重复
#define ERR_MOVECRD_SMOOTH_COEF                        (0x05b1) // 立即插补平滑系数设置错误
#define ERR_MOVECRD_GEOM_ERR                           (0x05b2) // 立即插补几何错误
#define ERR_MOVECRD_DIMENSION                          (0x05b3) // 立即插补维数错误

// 端子板相关错误
#define ERR_LOCAL_LINK_FAIL                            (0x0600) // 端子板通讯失败
#define ERR_LOCAL_NO_CFG                               (0x0601) // 端子板没有配置，不允许访问

#define ERR_ENC_FILTER_DEPTH                           (0x0608) // 编码器滤波深度参数错误
#define ERR_ENC_FILTER_COEF                            (0x0609) // 编码器滤波系数参数错误

#define ERR_DO_PIN_TYPE                                (0x060a) // 端子板DO输出口类型错误
// 位置比较
#define ERR_COMP_MODE                                  (0x0610) // 位置比较输出：位置比较模式设置错误
#define ERR_COMP_OUTTYPE                               (0x0611) // 位置比较输出：位置比较手动多脉冲模式下输出类型错误
#define ERR_COMP_DIMENS                                (0x0612) // 位置比较的维数错误
#define ERR_COMP_SRC_NO                                (0x0613) // 位置比较的位置源端口号错误
#define ERR_COMP_SRC_TYPE                              (0x0614) // 位置比较的源类型错误
#define ERR_COMP_PULSE_WIDTH                           (0x0615) // 位置比较的脉冲输出宽度
#define ERR_COMP_OUTVAL                                (0x0616) // 位置比较的立即输出值不在范围内
#define ERR_COMP_CTRLMODE                              (0x0617) // 位置比较输出的控制模式参数设置错误
#define ERR_COMP_ERR_BAND                              (0x0618) // 位置比较误差带参数不在范围

#define ERR_COMP_INTERVAL                              (0x0619) // 位置等距比较输出的间距值小于1个pulse
#define ERR_COMP_REPEATTIME                            (0x061a) // 位置等距比较输出的次数小于1次

#define ERR_COMP_BUSY                                  (0x0620) // 位置比较输出忙
#define ERR_COMP_BUF_EMPTY                             (0x0621) // 位置比较缓冲区数据为空
#define ERR_COMP_DATA_INVERSE                          (0x0622) // 位置比较设置的比较数据出现反向
#define ERR_COMP_POS_TYPE                              (0x0623) // 等距比较数据不允许在绝对位置比较类型下压入
#define ERR_COMP_DATA_TYPE                             (0x0624) // 等距比较数据不允许在动态数据类型下压入
#define ERR_COMP_PORT_TYPE                             (0x0625) // 位置比较输出端口类型错误
// PSO
#define ERR_PSO_DIMENS                                 (0x0630) // PSO的维数参数错误
#define ERR_PSO_PULSE_WIDTH                            (0x0631) // PSO的脉冲输出宽度错误
#define ERR_PSO_SRC_NO                                 (0x0632) // PSO的位置源序号错误

#define ERR_PSO_SYN_POS                                (0x0640) // PSO的同步比较输出间距错误
#define ERR_PSO_BASE_FRQ                               (0x0641) // PSO的同步比较输出基频错误
#define ERR_PSO_RUN_BUSY                               (0x0642) // PSO的同步比较输出忙

// PWM
#define ERR_PWM_FRQ_OUTRANG                            (0x0650) // PWM设置的频率错误
#define ERR_PWM_DUTY_OUTRANG                           (0x0651) // PWM设置的占空比超限
#define ERR_PWM_PORT_TYPE                              (0x0652) // PWM输出端口类型错误

// 多轴位置比较
#define ERR_MULTI_AX_CMP_BUSY                          (0x0680) // 多轴位置比较进行中
#define ERR_MULTI_AX_CMP_BUF_FULL                      (0x0681) // 多轴位置比较缓冲区空间已满
#define ERR_MULTI_AX_CMP_AX_CNT                        (0x0682) // 多轴位置比较轴数小于1
#define ERR_MULTI_AX_CMP_REPEATED                      (0x0683) // 多轴位置比较相邻点重复
#define ERR_MULTI_AX_CMP_BUF_EMPTY                     (0x0684) // 多轴位置比较缓冲区为空

// 其他功能模块错误
#define ERR_CYCTIME                                    (0x0700) // 设置规划周期参数不在规定范围
#define ERR_RESET_IS_PERMIT                            (0x0701) // 系统复位条件不足，检查是否还在使能
#define ERR_NO_SYS_INT_SIGNAL                          (0x0702) // 系统中断没有起来，请检查总线是否处于OP状态

#define ERR_WATCHDOG_NOT_OPENED                        (0x070a) // 看门狗未开启

#define ERR_BACKLASH_CMPVAL                            (0x0710) // 反向间隙总补偿量设置小于0
#define ERR_BACKLASH_CMPVEL                            (0x0711) // 反向间隙补偿量速度小于0
#define ERR_BACKLASH_CMPDIR                            (0x0712) // 反向间隙补偿方向错误

// 螺距误差补偿
#define ERR_SCREWCMP_RUNNING                           (0x0720) // 螺距补偿正在运行，无法修改表或参数
#define ERR_SCREWCMP_TABLE_COUNT                       (0x0721) // 螺距误差补偿参数数据点个数大于表的数据个数

// 表补偿
#define ERR_TABLECOMP_BUSY                             (0x0730) // 表补偿正在运行,无法修改参数
#define ERR_TABLECOMP_DIMENSION                        (0x0731) // 表补偿维数错误,维数不为1~3
#define ERR_TABLECOMP_SRCAX_REPEAT                     (0x0732) // 表补偿参考轴号重复
#define ERR_TABLECOMP_TABLE_COUNT                      (0x0733) // 表补偿count错误:count小于2;count乘积超过表容量或不等于表数据个数
#define ERR_TABLECOMP_TABLE_STEP                       (0x0734) // 表补偿间隔小于等于0

// 分段限位功能
#define ERR_SEGMENT_ISON                               (0x0740) // 分段限功能已经处于on状态
#define ERR_SEGMENT_XPOINT_NOT_SEQUENCE                (0x0741) // X轴的位置不是从小到大输入

// 轴规划补偿
#define ERR_AX_COMPEN_BUSY                             (0x0750) // 轴补偿正忙,无法再设置补偿,需等待当前补偿完成

// 采样
#define ERR_SAMPLE_COUNT                               (0x0780) // 采样数据的个数不对
#define ERR_SAMPLE_BUSY                                (0x0781) // 采样正忙，不能配置参数
#define ERR_SAMPLE_DATATYPE                            (0x0782) // 采样数据的类型不存在
#define ERR_SAMPLE_INDEX                               (0x0783) // 采样数据的下标越界
#define ERR_SAMPLE_DRAW_DATA                           (0x0784) // 读取采样数据失败
#define ERR_SAMPLE_INTERVAL                            (0x0785) // 数据采样配置间隔参数错误
#define ERR_SAMPLE_TRIG_TYPE                           (0x0786) // 数据采样配置触发采样类型参数错误
#define ERR_SAMPLE_DELAY_PARA                          (0x0787) // 数据采样配置启动延时参数错误
#define ERR_SAMPLE_DI_NO                               (0x0788) // 数据采样配置DI触发端口错误
#define ERR_SAMPLE_DI_LEVEL                            (0x0789) // 数据采样配置DI触发电平错误

// 事件
#define ERR_EVENT_TYPE                                 (0x07a0) // 错误的事件类型
#define ERR_EVENT_TASK_TYPE                            (0x07a1) // 错误的任务类型
#define ERR_EVENT_FORMAT                               (0x07a2) // 错误的事件数据类型
#define ERR_EVENT_VAR_NOADDRESS                        (0x07a3) // 事件变量没有地址
#define ERR_EVENT_TASK_OUTRANGE_TYPE                   (0x07a4) // 超过任务类型范围
#define ERR_EVENT_ENABLED                              (0x07a5) // 当前事件已使能
#define ERR_EVENT_CONDITION_OUTRANG                    (0x07a6) // 事件条件设置超过范围
#define ERR_EVENT_ISON                                 (0x07a7) // 该事件已处于on状态
#define ERR_EVENT_TASK_NOT_CONFIG                      (0x07a8) // 任务没配置

//指令缓存功能相关错误码
#define ERR_CMD_LIST_IS_BUSAY                          (0x07b0) // 循环模式运行中, 不允许操作
#define ERR_CMD_LIST_IS_LOOP_MODE                      (0x07b1) // 非循环模式运行中, 不允许操作
#define ERR_CMD_LIST_NOT_LOOP_MODE                     (0x07b2) // 指令缓存区处于运行中

// 脉冲do输出   0x07c0
#define ERR_PULSE_DO_BUSY                              (0x07c0) // 脉冲DO输出忙

// PLC运动程序相关错误
#define ERR_PLC_BUSY                                   (0x07d0) // 运动程序执行中
#define ERR_PLC_COMPILE                                (0x07d1) // 运动程序编译错误
#define ERR_PLC_FILE_PATH_LENGTH                       (0x07d2) // 运动程序文件路径错误：路径字符串长度超过256字节
#define ERR_PLC_INI_FILE                               (0x07d3) // 运动程序变量表文件错误：ini文件未下载
#define ERR_PLC_THREAD_ID                              (0x07d4) // 运动程序绑定线程ID错误
#define ERR_PLC_FUNC_ID                                (0x07d5) // 运动程序绑定函数ID错误
#define ERR_PLC_PAGE_ID                                (0x07d6) // 运动程序绑定数据页ID错误
#define ERR_PLC_INI_FILE_READ                          (0x07d7) // 运动程序读取变量表文件错误
#define ERR_PLC_OPEN_FILE                              (0x07d8) // 运动程序文件打开错误
#define ERR_PLC_CMD_PTR                                (0x07d9) // 运动程序执行指令指针错误
#define ERR_PLC_DATA_ID                                (0x07da) // 运动程序绑定数据ID错误
#define ERR_PLC_PAGE_REPEAT                            (0x07db) // 运动程序绑定数据页重复
#define ERR_PLC_NO_LINK                                (0x07dc) // 运动程序线程没有绑定
#define ERR_PLC_VAR_BAND                               (0x07dd) // 运动程序变量绑定错误
#define ERR_PLC_CMD_DEAL                               (0x07df) // 运动程序执行指令错误

// (0x8000)API相关错误
#define ERR_CARD_OPEN                                  (0x8000) // 未开卡
#define ERR_CARD_INDEX_READ                            (0x8001) // 卡号读取失败
#define ERR_CARD_INDEX_REPEAT                          (0x8002) // 卡号重复
#define ERR_CARD_CNT_OUTRANG                           (0x8003) // 扫描板卡数量超限
#define ERR_CARD_NOT_FOUND                             (0x8004) // 未找到该卡号的板卡
#define ERR_CARD_OPEN_OPTION                           (0x8005) // 开卡选项错误,不支持当前方式或选项超限

#define ERR_PCI_OPENDEV                                (0x8006) // 打开PCI设备错误
#define ERR_PCI_DEV_ATTR                               (0x8007) // 获取PCI设备属性错误
#define ERR_PCI_WR_OFFSET                              (0x8008) // PCI读写偏移错误
#define ERR_PCI_READ_CHKSUM                            (0x8009) // PCI读取校验和错误
#define ERR_PCI_READ_DATA                              (0x800a) // PCI读取数据失败
#define ERR_PCI_WRITE_DATA                             (0x800b) // PCI写入数据失败
#define ERR_PCI_SHM_READ                               (0x800c) // PCI读取共享数据失败
#define ERR_PCI_SHM_WRITE                              (0x800d) // PCI写入共享数据失败
#define ERR_PCI_BAR_RES                                (0x800e) // PCI资源获取失败

#define ERR_LOCK_ARM_CMD                               (0x8010) // 获取锁资源失败(ARM CMD)
#define ERR_LOCK_ARM_FIFO                              (0x8011) // 获取锁资源失败(ARM FIFO)
#define ERR_LOCK_DSP_CMD                               (0x8012) // 获取锁资源失败(DSP CMD)
#define ERR_LOCK_DSP_BLK                               (0x8013) // 获取锁资源失败(DSP BLOCK)
#define ERR_LOCK_DSPH_CMD                              (0x8014) // 获取锁资源失败(DSPH CMD)
#define ERR_LOCK_DSPH_BLK                              (0x8015) // 获取锁资源失败(DSPH BLOCK)

#define ERR_FREE_LOCK_ARM_CMD                          (0x8018) // 释放锁资源失败(ARM CMD)
#define ERR_FREE_LOCK_ARM_FIFO                         (0x8019) // 释放锁资源失败(ARM FIFO)
#define ERR_FREE_LOCK_DSP_CMD                          (0x801a) // 释放锁资源失败(DSP CMD)
#define ERR_FREE_LOCK_DSP_BLK                          (0x801b) // 释放锁资源失败(DSP BLOCK)
#define ERR_FREE_LOCK_DSPH_CMD                         (0x801c) // 释放锁资源失败(DSPH CMD)
#define ERR_FREE_LOCK_DSPH_BLK                         (0x801d) // 释放锁资源失败(DSPH BLOCK)

#define ERR_PCI2DSP_BUSY                               (0x8020) // DSP一直处于忙状态
#define ERR_PCI2DSP_WAIT                               (0x8021) // PCI到DSP等待数据超时
#define ERR_PCI2DSP_CHN                                (0x8022) // PCI到DSP通道异常
#define ERR_PCI2DSP_NORESP                             (0x8023) // PCI到DSP指令无响应

#define ERR_PCI2ARM_EXEC                               (0x8028) // ARM指令执行错误
#define ERR_PCI2ARM_CHN                                (0x8029) // PCI到ARM通道异常
#define ERR_PCI2ARM_NORESP                             (0x802a) // PCI到ARM指令无响应
#define ERR_PCI2ARM_WAIT                               (0x802b) // PCI到ARM等待数据超时

#define ERR_FILE_OPEN                                  (0x8100) // 创建或打开文件失败
#define ERR_MALLOC_MEM                                 (0x8101) // 分配文件内存失败
#define ERR_ENI_MASTER_ENABLE                          (0x8102) // ENI文件XML主站未开启
#define ERR_PARSER_XML                                 (0x8103) // 解析系统参数配置错误
#define ERR_FILE_SIZE_MAX                              (0x8104) // 文件大小超过超过最大限制
#define ERR_MALLOC_FAILED                              (0x8105) // 申请内存失败
#define ERR_FILE_NO_EXIST                              (0x8106) // 文件不存在

#define ERR_CHECK_USER_CODE                            (0x8120) // 用户密码校验错误

#define ERR_LOCAL_LINK_WAIT                            (0x8130) // 端子板连接配置成功等待超时

#define ERR_ECAT_MASTER_STS                            (0x8200) // ECAT主站总线错误
#define ERR_ECAT_MASTER_NOT_OP_STS                     (0x8201) // ECAT主站未处于op状态
#define ERR_ECAT_NOT_ALIAS_MODE                        (0x8202) // ECAT主站未处于别名模式

#define ERR_ECAT_NOT_FIND_AXCHN                        (0x8220) // 未找到对应的轴通道
#define ERR_ECAT_NOT_FIND_SLAVE                        (0x8221) // 未找到对应的从站

#define ERR_ECAT_MASTER_OP_WAIT                        (0x8240) // ECAT主站进入OP等待超时
#define ERR_ECAT_ENTER_HOME_WAIT                       (0x8241) // ECAT轴进入回零模式等待超时
#define ERR_ECAT_EXIT_HOME_WAIT                        (0x8242) // ECAT轴退出回零模式等待超时

#define ERR_ECAT_SDO_DOWNLOAD_6060                     (0x8280) // 下载回零模式Sdo失败
#define ERR_ECAT_SDO_DOWNLOAD_6098                     (0x8281) // 下载回零方法Sdo失败
#define ERR_ECAT_SDO_DOWNLOAD_6099H                    (0x8282) // 下载回零高速Sdo失败
#define ERR_ECAT_SDO_DOWNLOAD_6099L                    (0x8283) // 下载回零低速Sdo失败
#define ERR_ECAT_SDO_DOWNLOAD_609A                     (0x8284) // 下载回零加速度Sdo失败
#define ERR_ECAT_SDO_DOWNLOAD_607C                     (0x8285) // 下载回零偏移Sdo失败

#define ERR_ECAT_SDO_UPLOAD_6060                       (0x8290) // 上传回零模式Sdo失败

#define ERR_ARM_EXE_FUN                                (0x9003) // 执行调用函数异常
#define ERR_ARM_SLAVE_OUTRANG                          (0x9004) // 从站索引号超限
#define ERR_ARM_INUPUT_PARA                            (0x9005) // 输入参数错误
#define ERR_ARM_INUPT_TYPE							(0x9006) // 输入参数类型操作

#define ERR_ARM_ECAT_SHM                               (0x9020) // 共享内存错误
#define ERR_ARM_ECAT_SHM_ADDR                          (0x9021) // 共享内存地址非法
#define ERR_ARM_FILE_BEYOND_MAX                        (0x9022) // 文件太大，超出buffer长度
#define ERR_ARM_FILE_OPEN                              (0x9023) // 打开文件失败
#define ERR_ARM_FILE_MMAP                              (0x9024) // 文件映射失败
#define ERR_ARM_FILE_MUNMMAP                           (0x9025) // 文件反映射失败
#define ERR_ARM_SET_SYSTIME                            (0x9026) // 设置系统时间错误

// ENI相关
#define ERR_ARM_ENI_NONE_ENABLE                        (0x9030) // 主ENI文件中所有端口都未启用
#define ERR_ARM_ENI_ILLEGAL                            (0x9031) // 主ENI非法
#define ERR_ARM_ENI_CHECK                              (0x9032) // ENI校验异常

#define ERR_ARM_ECAT_LINK                              (0x9043) // ECAT总线连接错误
#define ERR_ARM_ECAT_INIT_STEP0                        (0x9044) // ECAT总线初始化错误Step0
#define ERR_ARM_SCAN_MODULE                            (0x9045) // ECAT总线初始化扫描Module错误
#define ERR_ARM_ECAT_INIT_STEP2                        (0x9046) // ECAT总线初始化错误Step2
#define ERR_ARM_DEL_MASTER                             (0x9047) // ECAT总线释放主站错误
#define ERR_ARM_SCAN_SLAVES                            (0x9048) // ECAT总线扫描从站设备错误

#define ERR_ARM_SDO_UPLOAD                             (0x904a) // ECAT总线上传Sdo失败
#define ERR_ARM_SDO_DOWNLOAD                           (0x904b) // ECAT总线下载Sdo失败
#define ERR_ARM_SDO_NOOP                               (0x904c) // SDO操作时主站未处于OP状态

#define ERR_ARM_READ_SLAVE_E2P                         (0x9050) // 读取从站EEPROM 失败
#define ERR_ARM_WRITE_SLAVE_E2P                        (0x9051) // 写从站EEPROM 失败

#define ERR_ARM_NOT_EXIST_STATION                      (0x9060) // 不存在该站点
#define ERR_ARM_INIT_OP_TIMEOUT                        (0x9061) // ECAT总线初始化进OP超时
#define ERR_ARM_OFFSET_ADDR                            (0x9062) // ECAT PDO偏移地址错误

#define ERR_ARM_ECAT_STEP3                             (0x9070) // ECAT总线初始化失败Step3
#define ERR_ARM_ECAT_STEP3_LINK_READ                   (0x9071) // ECAT总线初始化失败Step3，link状态读取失败
#define ERR_ARM_ECAT_STEP3_LINK_LOST                   (0x9072) // ECAT总线初始化失败Step3，link状态丢失
#define ERR_ARM_ECAT_STEP3_AX_ATTR                     (0x9073) // ECAT总线初始化失败Step3，轴类型错误
#define ERR_ARM_ECAT_STEP3_AX_CNT                      (0x9074) // ECAT总线初始化失败Step3，轴数超限
#define ERR_ARM_ECAT_STEP3_PDO_CNT                     (0x9075) // ECAT总线初始化失败Step3，Pdo超限
#define ERR_ARM_ECAT_STEP3_TOP_DIFF                    (0x9076) // ECAT总线初始化失败Step3，拓扑不匹配
#define ERR_ARM_ECAT_STEP3_OP_FAIL                     (0x9077) // ECAT总线初始化失败Step3，进OP失败
#define ERR_ARM_ECAT_STEP3_SLAVE_ATTR                  (0x9078) // ECAT总线初始化失败Step3，从站类型错误

#define ERR_ARM_CMP_ENCRY_CHIP                         (0x90e0) // 板卡权限验证不通过
#define ERR_ARM_NO_ENCRY_CHIP                          (0x90e1) // 板卡没有加密芯片
#define ERR_ARM_ENCRY_READ_ID                          (0x90e3) // 读取加密芯片ID错误
#define ERR_ARM_ENCRY_SET_SECRET                       (0x90e4) // 设置加密秘钥CRC校验失败
#define ERR_ARM_ENCRY_SECRET_HADSET                    (0x90e5) // 芯片已经被被加密
#define ERR_ARM_ENCRY_SET_WP                           (0x90e6) // 设置写保护失败
#define ERR_ARM_ENCRY_READ_STS                         (0x90e7) // 读取加密芯片状态失败
#define ERR_ARM_ENCRY_RD_SCRATCHPAD                    (0x90e8) // 读加密芯片刮擦板失败
#define ERR_ARM_ENCRY_READ_MEM                         (0x90e9) // 读取加密MEM CRC校验失败
#define ERR_ARM_ENCRY_AUTHER_WR                        (0x90ea) // 认证写加密内存 校验失败

#define ERR_ARM_BOARD_NONSUPPORT_CFG_1G		(0x90f0) // 板卡不支持配置
#define ERR_ARM_BOARD_CFG_AX_CNT                       (0x90f1) // 板卡配置超过最大轴数
#define ERR_ARM_BOARD_CFG_SAVE                        (0x90f2) // 板卡配置保存失败


// 协议栈错误码
#define ERROR_ECFG_SET_DEBUG_LEVEL_WRONG_PARAM         (0x9100) // 打印等级超出0-3范围
#define ERROR_ECFG_GET_VERSION_NULL_PARAM              (0x9101) // 接口传入指针为空指针
#define ERROR_ECFG_GET_VERSION_WRONG_PARAM             (0x9102) // 版本号不在20-23的范围
#define ERROR_ECAT_MASTER_INIT_MASTER_ERROR            (0x9103) // 初始化主站失败
#define ERROR_ECFG_DSP_FUNC_WRONG_PARAM                (0x9104) // 设置DSP接口超出最大主站数2
#define ERROR_ECFG_SET_DSP_LINKSTS_WRONG_PARAM         (0x9105) // 设置Link状态超出最大主站数2
#define ERROR_ECFG_SET_DSP_LINKSTS_FUNC_NULL           (0x9106) // 设置Link状态接口为空
#define ERROR_ECFG_SET_DSP_LINKSTS_ERROR               (0x9107) // 设置Link状态执行失败
#define ERROR_ECFG_GET_SLAVEOFFSET_WRONG_PARAM         (0x9108) // 通用主站获取从站偏移接口，传入空指针或从站数超限
#define ERROR_ECFG_GET_AXSTATION_AX_ID_WRONG           (0x9109) // 通用主站获取轴站号接口轴号超过最大轴数
#define ERROR_ECFG_GET_AXSTATION_NULL_PARAM            (0x910A) // 通用主站获取轴站号接口参数空指针
#define ERROR_INIT_AXIS_ATTR_AXIS_EXCEED               (0x9110) // 通用主站构建轴资源轴超过最大轴数限制
#define ERROR_SLAVE_ATTR_BUILD_PDO_NOT_FOUND           (0x9111) // 通用主站构建从站PDO资源未找到PDO
#define ERROR_SLAVE_ATTR_BUILD_AXIS_SLOT_WRONG         (0x9112) // 通用主站构建从站PDO资源轴slot号计算错误
#define ERROR_SLAVE_ATTR_BUILD_AXIS_ID_WRONG           (0x9113) // 通用主站构建从站PDO资源轴ID计算超限
#define ERROR_SLAVE_ATTR_BUILD_DO_BLOCK_WRONG          (0x9114) // 通用主站构建从站PDO资源DO块ID超内部最大值
#define ERROR_SLAVE_ATTR_BUILD_DI_BLOCK_WRONG          (0x9115) // 通用主站构建从站PDO资源DI块ID超内部最大值
#define ERROR_SLAVE_ATTR_BUILD_DA_BLOCK_WRONG          (0x9116) // 通用主站构建从站PDO资源DA块ID超内部最大值
#define ERROR_SLAVE_ATTR_BUILD_AD_BLOCK_WRONG          (0x9117) // 通用主站构建从站PDO资源AD块ID超内部最大值
#define ERROR_SLAVE_ATTR_BUILD_REGOUT_BLOCK_WRONG      (0x9118) // 通用主站构建从站PDO资源REGOUT块ID超内部最大值
#define ERROR_SLAVE_ATTR_BUILD_REGIN_BLOCK_WRONG       (0x9119) // 通用主站构建从站PDO资源REGIN块ID超内部最大值
#define ERROR_SLAVE_ATTR_BUILD_ENC_BLOCK_WRONG         (0x911A) // 通用主站构建从站PDO资源ENC块ID超内部最大值
#define ERROR_SLAVE_ATTR_BUILD_PDO_DEV_TYPE_WRONG      (0x911B) // 通用主站构建从站PDO资源PDO类型错误
#define ERROR_ECFG_MASTER_SCAN_SLAVE_ERROR             (0x9120) // 通用主站扫描从站失败
#define ERROR_ECFG_MASTER_SCAN_SLAVE_GET_NUM_ERROR     (0x9121) // 通用主站获取扫描从站数失败
#define ERROR_ECFG_MASTER_SCAN_SLAVE_GET_INFO_ERROR    (0x9122) // 通用主站获取扫描从站信息失败
#define ERROR_ECFG_MASTER_SCAN_SLAVE_APP_ERROR         (0x9123) // 通用主站应用相关错误
#define ERROR_ECFG_GET_SLAVE_CURRENT_STATE_NULL_PARAM  (0x9124) // 通用主站获取从站状态参数为空指针
#define ERROR_ECFG_GET_SLAVE_CURRENT_STATE_ERROR       (0x9125) // 通用主站获取从站状态失败
#define ERROR_ECFG_GET_SLAVE_ERROR_COUNTER_NULL_PARAM  (0x9126) // 通用主站获取主站错误帧计数参数为空指针
#define ERROR_ECFG_GET_SLAVE_ERROR_COUNTER_ERROR       (0x9127) // 通用主站获取主站错误帧计数失败
#define ERROR_ECFG_CLEAR_SLAVE_ERROR_COUNTER_ERROR     (0x9128) // 通用主站清除主站错误帧计数失败
#define ERROR_STEP2_UPDATECONFIG_ERROR                 (0x9130) // 通用主站STEP2更新协议栈配置失败
#define ERROR_STEP3_LINK_READ_ERROR                    (0x9131) // 通用主站STEP3读Link失败
#define ERROR_STEP3_LINK_LOST_ERROR                    (0x9132) // 通用主站STEP3启动阶段Link断开
#define ERROR_STEP3_EXCEED_MAX_AXIS_ERROR              (0x9133) // 通用主站STEP3启动阶段轴超最大个数限制
#define ERROR_STEP3_PDO_EXCEED_MAX                     (0x9134) // 通用主站STEP3启动阶段PDO数据超1486
#define ERROR_STEP3_TOPOLOGY_DIFF                      (0x9135) // 通用主站STEP3启动阶段拓扑不匹配
#define ERROR_STEP3_OP_FAIL                            (0x9136) // 通用主站STEP3启动阶段进OP失败
#define ERROR_ECFG_CHECK_MASTER_LINK_NULL_PARAM        (0x9137) // 通用主站获取Link传入空指针
#define ERROR_ECFG_CHECK_MASTER_LINK_ERROR             (0x9138) // 通用主站获取Link失败
#define ERROR_SDO_UPLOAD_EXCEED_SLAVE_TOTAL            (0x9140) // 通用主站sdo上传超出最大从站数
#define ERROR_SDO_UPLOAD_FAIL                          (0x9141) // 通用主站sdo上传失败
#define ERROR_SDO_DOWNLOAD_EXCEED_SLAVE_TOTAL          (0x9142) // 通用主站sdo下载超出最大从站数
#define ERROR_SDO_DOWNLOAD_FAIL                        (0x9143) // 通用主站sdo下载失败
#define ERROR_SII_WRITE_EXCEED_SLAVE_TOTAL             (0x9144) // 通用主站eeprom写操作超出最大从站数
#define ERROR_SII_WRITE_FAIL                           (0x9145) // 通用主站eeprom写操作失败
#define ERROR_SII_READ_EXCEED_SLAVE_TOTAL              (0x9146) // 通用主站eeprom读操作超出最大从站数
#define ERROR_SII_READ_FAIL                            (0x9147) // 通用主站eeprom读操作失败
#define ERROR_REG_WRITE_EXCEED_SLAVE_TOTAL             (0x9148) // 通用主站寄存器写操作超出最大从站数
#define ERROR_REG_WRITE_NULL_PARAM                     (0x9149) // 通用主站寄存器写操作传入空指针
#define ERROR_REG_WRITE_FAIL                           (0x914A) // 通用主站寄存器写操作失败
#define ERROR_REG_READ_EXCEED_SLAVE_TOTAL              (0x912B) // 通用主站寄存器读操作超出最大从站数
#define ERROR_REG_READ_NULL_PARAM                      (0x913C) // 通用主站寄存器读操作传入空指针
#define ERROR_REG_READ_FAIL                            (0x913D) // 通用主站寄存器读操作失败
#define ERROR_GET_SV_ATTR_EXCEED_SLAVE_TOTAL           (0x9150) // 通用主站获取伺服属性超出最大从站数
#define ERROR_ECFG_GET_ECAT_CFG_MASTER_NULL_PARAM      (0x9151) // 通用主站获取主站配置信息传入空指针
#define ERROR_ECFG_GET_ECAT_CFG_SLAVE_NULL_PARAM       (0x9152) // 通用主站获取从站配置信息传入空指针
#define ERROR_ECFG_GET_ECAT_CFG_SLAVE_EXCEED           (0x9153) // 通用主站获取从站配置信息超出最大从站数
#define ERROR_ECFG_GET_SLAVEOFFSET1_WRONG_PARAM        (0x9160) // 高速主站获取从站偏移接口，传入空指针或从站数超限
#define ERROR_SLAVE_ATTR_BUILD1_PDO_NOT_FOUND          (0x9161) // 高速主站构建从站PDO资源未找到PDO
#define ERROR_SLAVE_ATTR_BUILD1_DO_BLOCK_WRONG         (0x9162) // 高速主站构建从站PDO资源DO块ID超内部最大值
#define ERROR_SLAVE_ATTR_BUILD1_DI_BLOCK_WRONG         (0x9163) // 高速主站构建从站PDO资源DI块ID超内部最大值
#define ERROR_SLAVE_ATTR_BUILD1_DA_BLOCK_WRONG         (0x9164) // 高速主站构建从站PDO资源DA块ID超内部最大值
#define ERROR_SLAVE_ATTR_BUILD1_AD_BLOCK_WRONG         (0x9165) // 高速主站构建从站PDO资源AD块ID超内部最大值
#define ERROR_SLAVE_ATTR_BUILD1_REGOUT_BLOCK_WRONG     (0x9166) // 高速主站构建从站PDO资源REGOUT块ID超内部最大值
#define ERROR_SLAVE_ATTR_BUILD1_REGIN_BLOCK_WRONG      (0x9167) // 高速主站构建从站PDO资源REGIN块ID超内部最大值
#define ERROR_SLAVE_ATTR_BUILD1_ENC_BLOCK_WRONG        (0x9168) // 高速主站构建从站PDO资源ENC块ID超内部最大值
#define ERROR_SLAVE_ATTR_BUILD1_PDO_DEV_TYPE_WRONG     (0x9169) // 高速主站构建从站PDO资源PDO类型错误
#define ERROR_ECFG_MASTER_SCAN_SLAVE1_ERROR            (0x9170) // 高速主站扫描从站失败
#define ERROR_ECFG_MASTER_SCAN_SLAVE1_GET_NUM_ERROR    (0x9171) // 高速主站获取扫描从站数失败
#define ERROR_ECFG_MASTER_SCAN_SLAVE1_GET_INFO_ERROR   (0x9172) // 高速主站获取扫描从站信息失败
#define ERROR_ECFG_MASTER_SCAN_SLAVE1_APP_ERROR        (0x9173) // 高速主站应用相关错误
#define ERROR_ECFG_GET_SLAVE_CURRENT_STATE1_NULL_PARAM (0x9174) // 高速主站获取从站状态参数为空指针
#define ERROR_ECFG_GET_SLAVE_CURRENT_STATE1_ERROR      (0x9175) // 高速主站获取从站状态失败
#define ERROR_ECFG_GET_SLAVE_ERROR_COUNTER1_NULL_PARAM (0x9176) // 高速主站获取主站错误帧计数参数为空指针
#define ERROR_ECFG_GET_SLAVE_ERROR_COUNTER1_ERROR      (0x9177) // 高速主站获取主站错误帧计数失败
#define ERROR_ECFG_CLEAR_SLAVE_ERROR_COUNTER1_ERROR    (0x9178) // 高速主站清除主站错误帧计数失败
#define ERROR_STEP21_UPDATECONFIG_ERROR                (0x9180) // 高速主站STEP2更新协议栈配置失败
#define ERROR_STEP31_LINK_READ_ERROR                   (0x9181) // 高速主站STEP3读Link失败
#define ERROR_STEP31_LINK_LOST_ERROR                   (0x9182) // 高速主站STEP3启动阶段Link断开
#define ERROR_STEP31_PDO_EXCEED_MAX                    (0x9183) // 高速主站STEP3启动阶段PDO数据超1486
#define ERROR_STEP31_TOPOLOGY_DIFF                     (0x9184) // 高速主站STEP3启动阶段拓扑不匹配
#define ERROR_STEP31_OP_FAIL                           (0x9185) // 高速主站STEP3启动阶段进OP失败
#define ERROR_ECFG_CHECK_MASTER_LINK1_NULL_PARAM       (0x9186) // 高速主站获取Link传入空指针
#define ERROR_ECFG_CHECK_MASTER_LINK1_ERROR            (0x9187) // 高速主站获取Link失败
#define ERROR_SDO_UPLOAD1_EXCEED_SLAVE_TOTAL           (0x9190) // 高速主站sdo上传超出最大从站数
#define ERROR_SDO_UPLOAD1_FAIL                         (0x9191) // 高速主站sdo上传失败
#define ERROR_SDO_DOWNLOAD1_EXCEED_SLAVE_TOTAL         (0x9192) // 高速主站sdo下载超出最大从站数
#define ERROR_SDO_DOWNLOAD1_FAIL                       (0x9193) // 高速主站sdo下载失败
#define ERROR_SII_WRITE1_EXCEED_SLAVE_TOTAL            (0x9194) // 高速主站eeprom写操作超出最大从站数
#define ERROR_SII_WRITE1_FAIL                          (0x9195) // 高速主站eeprom写操作失败
#define ERROR_SII_READ1_EXCEED_SLAVE_TOTAL             (0x9196) // 高速主站eeprom读操作超出最大从站数
#define ERROR_SII_READ1_FAIL                           (0x9197) // 高速主站eeprom读操作失败
#define ERROR_REG_WRITE1_EXCEED_SLAVE_TOTAL            (0x9198) // 高速主站寄存器写操作超出最大从站数
#define ERROR_REG_WRITE1_NULL_PARAM                    (0x9199) // 高速主站寄存器写操作传入空指针
#define ERROR_REG_WRITE1_FAIL                          (0x919A) // 高速主站寄存器写操作失败
#define ERROR_REG_READ1_EXCEED_SLAVE_TOTAL             (0x919B) // 高速主站寄存器读操作超出最大从站数
#define ERROR_REG_READ1_NULL_PARAM                     (0x919C) // 高速主站寄存器读操作传入空指针
#define ERROR_REG_READ1_FAIL                           (0x919D) // 高速主站寄存器读操作失败
#define ERROR_ECFG_GET_ECAT_CFG_MASTER1_NULL_PARAM     (0x91A0) // 高速主站获取主站配置传入空指针
#define ERROR_ECFG_GET_ECAT_CFG_SLAVE1_NULL_PARAM      (0x91A1) // 高速主站获取从站配置传入空指针
#define ERROR_ECFG_GET_ECAT_CFG_SLAVE1_EXCEED          (0x91A2) // 高速主站获取从站配置超出最大从站数

// 升级相关
#define ERR_UBIMKVOL_FAILURE                           (0x92ec) // bsp生成ubi vol失败
#define ERR_UBIUPDATEVOL_FAILURE                       (0x92ed) // bsp升级ubi vol失败
#define ERR_UBIFORMAT_FAILURE                          (0x92ee) // bsp格式化 ubi失败
#define ERR_DETACH_FAILURE                             (0x92ef) // bsp detach ubi 分区失败
#define ERR_ATTACH_FAILURE                             (0x92f0) // bsp attach ubi 分区失败
#define ERR_UNMOUNT_FAILURE                            (0x92f1) // bsp unmount 分区失败
#define ERR_MOUNT_FAILURE                              (0x92f2) // bsp mount 分区失败
#define ERR_HW_INFO_NOT_EXSIT                          (0x92f3) // bsp硬件信息不存在
#define ERR_HW_VER_MISMATCHING                         (0x92f4) // bsp硬件版本不匹配
#define ERR_PLATFORM_MISMATCHING                       (0x92f5) // bsp cpu信息匹配失败
#define ERR_MACHINE_MISMATCHING                        (0x92f6) // bsp设备不匹配
#define ERR_STORAGE_MISMATCHING                        (0x92f7) // bsp存储介质不匹配
#define ERR_CREATE_LOG_FILE                            (0x92f8) // bsp创建log文件失败
#define ERR_RB_CHECK_FAILURE                           (0x92f9) // bsp rb检查失败
#define ERR_UNKNOW_TYPE                                (0x92fa) // bsp存储介质非emmc非nand
#define ERR_NOT_SUPPORT                                (0x92fb) // bsp操作不支持
#define ERR_ZIP_PACKAGE                                (0x92fc) // bsp解压bsp压缩包失败
#define ERR_UNKNOW_MODE                                (0x92fd) // bsp模式错误
#define ERR_ERASE_FATAL                                (0x92fe) // bspmtd分区擦除错误
#define ERR_WRITE_FATAL                                (0x92ff) // bsp写入mtd分区错误

#define ERR_PD_CHECK_IMAGE_VALIDITY                    (0x930a) // 产品升级检查烧录文件非法
#define ERR_PD_CHECK_IMAGE_VERSION                     (0x9314) // 产品升级烧录文件版本校验失败
#define ERR_PD_CHECK_IMAGE_COMPATIBILITY               (0x931e) // 产品升级烧录文件不完整
#define ERR_PD_FS_UMOUNT                               (0x9328) // 产品升级umount分区失败
#define ERR_PD_FS_DETACH                               (0x9332) // 产品升级删除分区TACH失败
#define ERR_PD_FS_ATTACH                               (0x933c) // 产品升级新增分区TACH失败
#define ERR_PD_FS_MOUNT                                (0x9346) // 产品升级mount分区失败
#define ERR_PD_UBIFORMAT_FAIL                          (0x9350) // 产品升级格式化ubi分区失败
#define ERR_PD_UBIMKVOL_FAIL                           (0x935a) // 产品升级创建ubi卷失败
#define ERR_PD_TUNE_PART_FAIL                          (0x9364) // 产品升级设置ext分区参数失败
#define ERR_PD_FORMAT_PART_FAIL                        (0x936e) // 产品升级格式化mtd失败
#define ERR_PD_COPY_NEW_DATA                           (0x9378) // 产品升级固件拷贝失败
#define ERR_PD_DEL_FILES                               (0x9382) // 产品升级固件删除失败
#define ERR_PD_IMG_TOO_OLD                             (0x938c) // 产品升级固件版本过旧
#define ERR_PD_INVALID_ARGS                            (0x93e6) // 产品升级未找到mount配置
#define ERR_PD_INTERNAL                                (0x93f0) // 产品升级异常终止
#define ERR_PD_UNKNOWN                                 (0x93fa) // 产品升级非emmc或mtd
#define ERR_PD_NOT_SUPPORT                             (0x93ff) // 产品升级不支持的分区

// 非标错误码
#define ERR_NON_STD_UNLOCK_CUSTOM_SLAVE				   (0xa010) // 解锁定制从站失败