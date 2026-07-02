# Holo 整合项目

`Holo` 是新的全链路入口，用来把现有三个项目串成一个程序：

1. `point_cloud`：深度 TIFF + JPG 生成点云、网格、贴图 OBJ。
2. `multiview`：OBJ 生成多视角 JPG。
3. `Holo elemental`：把多视角图重排成目标小图。

## 目标尺寸

要生成 `150 * 150` 张、每张 `270 * 270` 像素的目标图，配置关系是：

```ini
multiview_angle=30
multiview_per=9
multiview_resolution=150
view_rows=270
view_cols=270
target_rows=150
target_cols=150
```

含义：

- `multiview_angle * multiview_per = 270`，所以多视角阶段输出 `270 * 270 = 72900` 张视角图。
- `multiview_resolution=150`，所以每张视角图是 `150 * 150`。
- `target_rows * target_cols = 150 * 150`，所以最终输出 `22500` 张图。
- `view_rows * view_cols = 270 * 270`，所以每张最终图是 `270 * 270` 像素。

## 使用

可以直接打开 `Holo.sln`，只加载 Holo 项目；也可以从上级 `point_cloud.sln` 一起打开。

复制示例配置：

```bat
copy holo_config.example.ini holo_config.ini
```

干跑查看命令和配置检查：

```bat
target\Holo.exe --config holo_config.ini --dry-run
```

只跑第三段图像重排：

```bat
target\Holo.exe --config holo_config.ini --stage elemental
```

跑完整链路：

```bat
target\Holo.exe --config holo_config.ini --stage all
```

## 当前边界

- `point_cloud` 阶段现在先通过现有 `point_cloud.exe` 适配，后续可以把 `depthImage` 和 `ConverPointCloud` 抽成库直接调用。
- `multiview` 阶段现在仍会启动现有窗口程序，当前旧逻辑需要右键开始采图；下一步建议给 `multiview` 加 `-autostart` 或 headless 模式。
- `Generateimages` 的 MFC GUI 不再作为目标入口。Holo 已经内置命令行 elemental 生成器，采用分块输出，避免一次性加载 72900 张图和 22500 张目标图。
