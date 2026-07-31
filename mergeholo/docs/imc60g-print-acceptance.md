# IMC60G 打印硬件分阶段验收记录

本文档是 Task10 的操作步骤和验收总表，不代表硬件已经通过。创建本文件和验收脚本时没有开卡、使能伺服、回零、移动轴或切换 DO1。每个阶段的真实结论必须引用 `runs/imc60g-acceptance/<时间戳>/` 中由当次运行生成的证据。

操作前同时遵守 [IMC60G 9030 打印操作手册](imc60g-print-operator.md)。软件停止不能替代物理急停。

## 当次现场安全门禁

每次、每个阶段开始前，现场操作员都必须重新确认：

- 物理急停可立即触及且功能已验证；
- X/Y 负限位接线、触发方向和机械端位已核对；
- X/Y 全行程已清空，人员、工件、工具和线缆均不会进入危险区域；
- DO1+/DO1- 仅接示波器、隔离输入或经批准的安全负载；
- 第二屏配置不会影响现场其他安全系统；
- 本次只执行命令中指定的一个阶段，不自动继续下一阶段。

缺少以上任一项时不要运行脚本。`-OperatorConfirmedSafe` 只记录当次操作员确认，不能代替现场检查，也不能沿用上一次运行的确认。

## 脚本安全边界

`scripts/run_imc60g_acceptance.ps1`：

- `-Stage` 必选，只接受 `discover`、`home`、`xy-small`、`display`、`do1`、`one-row`、`serpentine`、`cancel`、`end-to-end`；
- 每次只准备并记录一个阶段，绝不自动推进；
- 所有阶段都必须带 `-OperatorConfirmedSafe`；
- 不 P/Invoke、不加载 IMC SDK、不调用任何 IMC/SV660N API；
- 不发送连接、回零、运动或 DO1 命令；
- 只有显式增加 `-LaunchUi` 才启动 `mergeholo.exe --ui`，随后所有硬件按钮仍由操作员手动点击；
- `-DryRun` 不启动应用、不提示输入，始终生成 `blocked` 记录；
- 命令行 `-Result` 只允许预填 `fail` 或 `blocked`；`pass` 必须在本次脚本显式启动 UI、操作员完成阶段后回到控制台交互输入，不能用参数预填；
- `pass` 还要求：交互输入至少 40 个字符的实测说明、通过 `-EvidenceFiles` 提供至少一个本次新建/更新且可哈希的外部证据文件、输入阶段专用确认短语，并满足该阶段的历史前置 Pass 证据；任一项缺失均降为 `blocked`；
- 退出码：`0=pass`、`2=fail`、`3=blocked`。脚本错误使用其他非零退出码。

每次运行创建：

```text
runs/imc60g-acceptance/<时间戳>/acceptance.json
runs/imc60g-acceptance/<时间戳>/print_flow.log
```

JSON 记录绝对路径和 SHA-256（可执行文件、IMC DLL、部署配置和源配置）、锁定 profile 关键值、阶段、开始/结束时间、退出码、操作员、观察记录、pass/fail/blocked、安全确认、脚本是否启动 UI，以及脚本没有调用 SDK/硬件命令的声明。

开始阶段前，脚本还会拒绝非 x64 PE，逐字校验全部有效硬件/打印配置值，并要求部署 IMC DLL 与 vendored SDK、部署配置与源配置的 SHA-256 分别一致。任一门禁失败只能记录 `blocked`，不能启动 UI 或记为通过。

## 通用运行方式

先看帮助：

```powershell
Get-Help .\scripts\run_imc60g_acceptance.ps1 -Full
```

安全 dry-run 示例（会记录 blocked，不启动程序）：

```powershell
powershell -ExecutionPolicy Bypass -File scripts\run_imc60g_acceptance.ps1 `
  -Stage discover -OperatorConfirmedSafe -DryRun
```

人工 UI 阶段示例：

```powershell
powershell -ExecutionPolicy Bypass -File scripts\run_imc60g_acceptance.ps1 `
  -Stage home -OperatorConfirmedSafe -LaunchUi `
  -EvidenceFiles "D:\imc60g-evidence\home-measurement.csv"
```

脚本打印本阶段清单后，操作员只执行该阶段；完成后在控制台输入 `pass`、`fail` 或 `blocked`，再填写测量值。若准备通过，启动命令必须带 `-LaunchUi -EvidenceFiles <本次证据文件路径>`；证据文件可以是控制器导出日志、示波器采集、现场照片或测量结果，必须在本次阶段期间新建或更新。Pass 不能预先通过 `-Result` 或 `-Observations` 传入。Fail/blocked 可用参数非交互记录。

脚本按当前 DLL 和两份部署配置的 SHA-256 查找兼容的历史 Pass 记录并执行前置门禁：`xy-small` 需要 `home`；`do1` 需要 `home + xy-small`；`one-row` 需要 `home + xy-small + display + do1`；`serpentine` 需要 `one-row`；`cancel` 需要 `serpentine`；`end-to-end` 需要全部上述硬件阶段。`display` 可独立执行；`home` 使用脚本自身的部署/profile 门禁代替当前无法完成的只读 discover 硬件结论。

## 阶段 1：discover（只发现，不运动）

目标：验证统一 x64 部署、DLL/配置哈希、启动诊断、对话框打开不动作；目标硬件事实还包括一张卡、EtherCAT 主站 OP、X=Axis1、Y=Axis0。

限制：当前程序没有独立的“只发现、不连接/不回零”硬件 CLI；UI 的 **连接并回零** 会把开卡和回零合并。因此本脚本绝不通过私自 P/Invoke 或点击该按钮来满足 discover。部署信息可以记录，但 Card 数和 EtherCAT OP 不能在本阶段安全自动验证，阶段必须明确记为 `blocked`，直至提供经审查的只读发现接口或外部受控证据。

操作：

1. 可用 `-LaunchUi` 仅打开 UI；不要点击 **连接并回零**。
2. 确认打开对话框没有卡、Servo、轴和 DO1 动作。
3. 核对 `runs/latest/imc60g_startup.log` 中 x64、DLL 绝对路径/SHA-256、profile 版本、Card0/X1/Y0、SV660N 后端和“卡保持关闭”。
4. 在观察记录中写明无动作证据和缺少只读卡/EtherCAT 发现接口的阻塞原因。

预期：部署发现有证据；硬件发现保持 blocked；没有任何运动命令。

## 阶段 2：home（显式连接并回零）

前置：discover 的部署部分通过人工复核，现场安全门禁当次确认。

1. 使用 `-Stage home -OperatorConfirmedSafe -LaunchUi`。
2. 只点击一次 **连接并回零**。
3. 观察并记录：Card0、EtherCAT OP、两轴 Servo On；先 Y/Axis0 向负限位、回退 92000 pulse、同步/置零；再 X/Axis1 向负限位、回退 28000 pulse、同步/置零。
4. 记录限位触发、停止原因、计划/编码器零点和最终 UI 状态。
5. 方向、距离、限位或状态异常时立即使用物理急停并记录 fail。

## 阶段 3：xy-small（低速小距离 X/Y）

前置：home 有证据地通过。

1. 由机械/控制负责人审核一个低速和小距离；不要首次使用生产速度或大步距。
2. 在 UI 中分别执行 X-、X+、Y-、Y+，记录物理轴号、方向、命令距离、实际位移、计划/编码器位置和停止状态。
3. 验证 **停止** 同时作用于映射后的 X/Axis1、Y/Axis0。
4. 验证回到批准位置；任何方向或距离不符立即急停并 fail。

## 阶段 4：display（只显示）

1. 保持运动未连接或已停止，不执行连接、回零、运动或 DO1。
2. 在非主显示器显示带编号的测试帧。
3. 记录输出设备、实时刷新率、十帧可见顺序、每次 Present 结果、物理 `WaitForVBlank` 和 DXGI frame statistics。
4. 切屏、刷新率缺失、VBlank 或 Present 失败均记录 fail；不得回退到软件计时伪装通过。

## 阶段 5：do1（SV660N 安全负载）

前置：示波器/隔离安全负载和低速 Y 运动方案均经现场批准。

1. 核对固定后端：逻辑 Y/Axis0、DO 功能 25、比较点 1、模式 0、`width=1000` 用户单位、属性 129/130、DO1+/DO1-。
2. 以低速分别执行一次正向和负向位置穿越。
3. 记录 H04/H18/H19 的 SDO 写入及读回、abort code、比较起止位置、触发实际位置、DO1 极性、实测宽度/波形。
4. 每次结束都验证 compare enable 已关闭、DO1 为安全状态；任一清理未验证均 fail。
5. `width=1000` 来自锁定硬件 profile，不是旧 `exposureSeconds` 字段。

## 阶段 6：one-row（低风险单行）

前置：home、xy-small、display、do1 均有证据地通过。

1. 使用经审核的小型单行图像集，数量严格等于 `1 × 列数`。
2. 记录正向帧序 `0,1,2...`、Y 目标、刷新率、每帧 VBlank、DO1 窗口、计划/编码器位置。
3. 记录终止清理：解除曝光、停止并验证 X/Y、返回逻辑零点并验证、关闭 presenter。

## 阶段 7：serpentine（小型两行蛇形）

前置：one-row 通过。

1. 使用小型两行任务。
2. 记录第一行正向 `0,1,2...`，X 行步进，第二行反向 `...2,1,0`。
3. 记录两行 Y 目标、X 步进、VBlank、DO1 触发和返回行为。
4. 帧重复/跳过、方向错误、提前曝光或清理未验证均 fail。

## 阶段 8：cancel（暂停/继续/取消/故障清理）

前置：serpentine 通过，并有经审核的安全故障注入方法。

1. 请求暂停，验证只在当前行完成后进入 Paused；进入前曝光已解除、X/Y 已停止。
2. 继续，验证重新预检且不重复已完成帧。
3. 取消，记录“解除曝光 → 停 X/Y → 验证停止 → 条件式返回零点并验证 → presenter shutdown”的顺序。
4. 使用受控方式分别注入显示或硬件失败，验证不启动下一运动、所有清理错误均追加报告、未验证安全时保持 Fault。

## 阶段 9：end-to-end（内存和文件夹）

前置：前述硬件阶段均有证据地通过。

1. 使用同一组经审核的小型任务先从 elemental 内存源打印，再从文件夹源打印。
2. 文件夹使用等宽零填充文件名，确保字典序明确。
3. 比较两次的帧数/顺序、运动调用、DO1 事件、输出时序、清理和返回结果。
4. 任一来源结果不一致或未安全收尾均 fail。

## 验收证据总表

不要预填通过。每完成一个阶段，只把对应证据目录和实测值写入此表；机器特定运行日志保留在 `runs/`，不提交到仓库，除非仓库策略明确要求。

| 阶段 | 状态 | 证据目录 | 操作员/时间 | 关键实测值或阻塞原因 |
|---|---|---|---|---|
| discover | Not run / expected blocked without read-only interface | — | — | — |
| home | Not run | — | — | — |
| xy-small | Not run | — | — | — |
| display | Not run | — | — | — |
| do1 | Not run | — | — | — |
| one-row | Not run | — | — | — |
| serpentine | Not run | — | — | — |
| cancel | Not run | — | — | — |
| end-to-end | Not run | — | — | — |

## 最终判定

当前结论：**硬件验收尚未执行，不能声明通过。**

最终通过需要每个适用阶段都有可追溯证据，并确认：

- 全链路 x64，无 DFJZH/x86 打印依赖；
- 打开对话框无硬件动作，只有显式 **连接并回零** 才初始化；
- Card0、X→Axis1、Y→Axis0、Y 后 X 回零准确；
- SV660N 内部位置比较 DO1+/DO1- 的正反向波形和安全关闭准确；
- 第二屏帧序、实时刷新率和物理 VBlank 准确；
- 单行、蛇形、暂停/继续/取消/故障和两种图像源均安全完成；
- 所有终止路径均能证明曝光安全、轴停止和所需返回位置；任何无法验证的清理结果都必须判为 Fault/Fail，而不是 Pass。
