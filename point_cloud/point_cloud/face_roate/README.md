# Face_Roate 处理说明

## 📁 文件结构

```
face_roate/
├── 0.tiff_re.tiff      # 深度图
├── 3.tiff_re.tiff      # 深度图
├── 25.tiff_re.tiff     # 深度图
├── f0.jpg              # RGB图像
├── f3.jpg              # RGB图像
├── f25.jpg             # RGB图像
├── depth_to_pointcloud_config.cfg  # 深度图转点云配置
├── mesh_config.cfg                 # 网格重建配置
├── 运行指南.md                      # 详细使用说明
└── 运行批处理.bat                   # 一键运行脚本
```

## 🚀 快速开始

### 方法1：使用批处理脚本（推荐）

1. 编辑 `运行批处理.bat`，修改 `PROGRAM_PATH` 变量为你的程序路径
2. 双击运行 `运行批处理.bat`

### 方法2：手动运行

#### 步骤1：深度图转点云
```bash
testreadpcd.exe -point "face_roate文件夹完整路径" -config "depth_to_pointcloud_config.cfg完整路径"
```

#### 步骤2：点云转网格
```bash
testreadpcd.exe -mesh "face_roate文件夹完整路径" -config "mesh_config.cfg完整路径"
```

#### 步骤3：网格贴图
```bash
testreadpcd.exe -model "face_roate文件夹完整路径" -config "mesh_config.cfg完整路径"
```

## 📝 输出文件

处理完成后会生成：
- `0_rgb.ply` 或 `0.tiff_rgb.ply` - 点云文件
- `0_mesh.ply` - 网格文件
- `0.obj` 等 - 最终3D模型文件（带纹理）

## ⚙️ 配置文件说明

### depth_to_pointcloud_config.cfg
用于深度图转点云的相机参数，需要根据实际相机参数调整：
- `focus`: 焦距（毫米）
- `step`: 视差步长
- `label`: 深度图最大标签值

### mesh_config.cfg
用于点云转网格的算法参数：
- `reconstruct`: 1=泊松重建，2=贪婪投影
- `leafsize`: 体素滤波大小（0表示不滤波）
- `holesize`: 补洞大小

## ⚠️ 注意事项

1. **文件命名**：确保深度图命名为 `X.tiff_re.tiff`，RGB图命名为 `fX.jpg`
2. **配置文件路径**：使用绝对路径更可靠
3. **处理顺序**：必须按顺序执行三个步骤
4. **参数调整**：根据实际数据质量调整配置文件中的参数

## 🔧 参数调优建议

如果结果不理想，可以尝试：

1. **点云太稀疏**：减小 `step` 值（depth_to_pointcloud_config.cfg）
2. **点云太密集**：增大 `leafsize` 值（mesh_config.cfg）
3. **网格有空洞**：增大 `holesize` 值
4. **网格质量差**：尝试切换 `reconstruct=2`（贪婪投影算法）

## 📚 更多信息

详细说明请查看 `运行指南.md`



