<div align="center">

<img src="ScreenShot/main_ui.png" alt="QuickView Hero" width="100%" style="border-radius: 10px; box-shadow: 0 0 20px rgba(0,0,0,0.5);">

<br><br>

# ⚡ QuickView

### Windows 平台的高性能图像查看器。
**为速度而生。为极客打造。**

<p>
    <strong>Direct2D Native</strong> • 
    <strong>Modern C++23</strong> • 
    <strong>动态调度架构 (Dynamic Scheduling)</strong> • 
    <strong>绿色便携</strong>
</p>

<p>
    <a href="LICENSE">
        <img src="https://img.shields.io/badge/license-GPL--3.0-blue.svg?style=flat-square&logo=github" alt="License">
    </a>
    <a href="#">
        <img src="https://img.shields.io/badge/platform-Windows%2010%20%7C%2011%20(x64)-0078D6.svg?style=flat-square&logo=windows" alt="Platform">
    </a>
    <a href="https://github.com/justnullname/QuickView/releases/latest">
        <img src="https://img.shields.io/github/v/release/justnullname/QuickView?style=flat-square&label=latest&color=2ea44f&logo=rocket" alt="Latest Release">
    </a>
    <a href="#">
         <img src="https://img.shields.io/badge/arch-ARM64%20%7C%20x64-darkred?style=flat-square&logo=cpu" alt="Architecture Support">
    </a>
    <a href="#">
         <img src="https://img.shields.io/badge/simd-Highway%20SSE4%2B-blue?style=flat-square&logo=google" alt="Highway SIMD">
    </a>
</p>

<h3>
    <a href="https://github.com/justnullname/QuickView/releases/latest">📥 下载最新版</a>
    <span> • </span>
    <a href="https://github.com/justnullname/QuickView/tree/main/ScreenShot">📸 截图预览</a>
    <span> • </span>
    <a href="https://github.com/justnullname/QuickView/issues">🐛 报告 Bug</a>
</h3>

</div>

---

## 🚀 简介

**QuickView** 是目前 Windows 平台上最快的图像查看器之一。我们专注于提供极致的 **浏览体验**——把繁重的编辑工作留给 Photoshop 这样的专业工具。

使用 **Direct2D** 和 **C++23** 从头重写，QuickView 摒弃了传统的 GDI 渲染，采用了游戏级的视觉架构。它的启动速度和渲染性能足以媲美甚至超越闭源商业软件，旨在以零延迟处理从微小图标到巨型 8K RAW 照片的所有内容。

🌐 **多语言支持:** English, 简体中文, 繁體中文, 日本語, Deutsch, Español, Русский

### 📂 支持格式
QuickView 支持几乎所有现代和专业图像格式：

* **经典格式：** `JPG`, `JPEG`, `PNG`, `BMP`, `GIF`, `TIF`, `TIFF`, `ICO`
* **现代/Web格式：** `WEBP`, `AVIF`, `HEIC`, `HEIF`, `SVG`, `SVGZ`, `JXL`
* **专业/HDR：** `EXR`, `HDR`, `PIC`, `PSD`, `TGA`, `PCX`, `QOI`, `WBMP`, `PAM`, `PBM`, `PGM`, `PPM`, `WDP`, `HDP`
* **RAW (LibRaw)：** `ARW`, `CR2`, `CR3`, `DNG`, `NEF`, `ORF`, `RAF`, `RW2`, `SRW`, `X3F`, `MRW`, `MOS`, `KDC`, `DCR`, `SR2`, `PEF`, `ERF`, `3FR`, `MEF`, `NRW`, `RAW`

---

# QuickView v6.2.10 - 高性能 HDR、漫画格式进阶与目录监控
**发布日期**: 2026-06-01

*💡 小贴士：可随时按 **F1** 键呼出交互式快捷键与帮助面板，查看最新的操作方法与说明。*

### 🔍 目录极速自动监控 (#192)
- **后台文件监控线程**：集成 Windows 原生 `FindFirstChangeNotificationW` 对活动文件夹进行实时变更监控，支持文件增删、重命名的自动捕捉。
- **300毫秒防抖过滤**：加入了高效的事件防抖机制，能完美合并系统密集的 I/O 变更，确保极致平滑 of 浏览和极速切图体验。
- **主线程安全同步**：利用系统级事件消息异步 reconcile 列表索引，实现 100% 线程安全的主线程数据热重载，避免任何 UI 锁死或画面闪烁。

### 🛠️ 工具链迁移与编译极限优化 (#177)
- **Clang-cl + Ninja 矩阵**：全面向 CMake 驱动的 **Clang-cl** 工具链及 Ninja 编译后端迁移，正式弃用旧版 Visual Studio `.sln` 和 `.vcxproj` 工程。
- **全链接时优化 (LTO)**：使用 Full LTO (`-flto=full`)、极致函数内联 (`-inline-threshold=250`) 及无异常机制 (`/EHs-c-`)，使单文件绿色体积大幅压缩，性能飞跃。
- **x64-windows-static-clang 三元组**：通过定制 triplets 实现百分之百纯静态编译，完全脱离外部动态链接库 (DLL) 依赖。
- **GeekGlass 菜单渲染开销缩减**：对 DWM 亚克力毛玻璃右键菜单的 D2D 滤镜初始化逻辑进行重构，跳过无用的特效实例化，从而彻底规避大容量 WARP JIT 内存爆表与切换卡顿。
- **stb_image 库升级 v2.30**：同步更新 `stb_image.h` 至最新 v2.30 版本，并成功消除 CodeQL 针对 `ImageLoader.cpp` 的所有潜在整数溢出安全警告。

### 📚 漫画模式与零开销压缩包直接查看 (#186)
- **零盘写极速引擎**：支持直接通过内存流读取 `.zip`、`.rar`、`.cbz`、`.cbr` 压缩文件，采用零开销数据导向设计 (DOD) 虚拟文件系统 (VFS)，不占用临时磁盘空间。
- **漫画双页拼接模式**：自动实现临近页面左右拼合显示，配合智能动态缩放，完美还原跨页画作的连贯感。配有全语言工具栏提示。
- **Unrar 提取加速**：集成定制轻量级单线程 `unrar-mini` 引擎，实现无延迟的漫画页面快速翻阅与画廊缩略图加载。

### 🌈 高精度 FP16 HDR 管线重构 (#131)
- **HLG inverse OETF 与 CPU system gamma 1.2 纠正**：修正了 HLG 格式的反 OETF 解码公式，并引入了 CPU 端标准的 OOTF 系统 Gamma 1.2 曲线缩放，极大提升了未经标记和原生 HLG 视频帧的亮度还原精度。
- **分支消除 GPU HLG 加速**：在 Shader 中全面重构为无分支的 lerp-step 矢量化数学函数，大幅加速 GPU 端的 HLG-to-linear 实时色调映射转换。
- **微软高级色彩 scRGB 亮度增益标准对齐**：完全对齐微软 Windows Advanced Color 的 scRGB Semantics 亮度映射策略（绝对亮度 HDR 保持 1.0 倍率增益，相对亮度 SDR 及 scene-linear 内容按 `SdrWhite/80` 动态线性缩放）。
- **智能样条曲线色调映射直通 (Bypass)**：引入了智能判断机制。当 HDR 图像的最高亮度峰值完全处于显示器当前极限硬件亮度内时，自动跳过复杂的 Spline 映射计算，直通执行高效的色调裁剪，极大降低 GPU 渲染开销。
- **64bpp FP16 线性色域**：全渲染管线升级至 64 位半精度浮点处理，结合 GPU 硬件矩阵转换，杜绝高动态范围下的色彩断层与溢出。
- **libplacebo PQ 样条色调映射**：移植科学级 Spline 色调映射算法到 PQ 亮度空间，完美释放 HDR 显示器物理性能。
- **高光去饱和与 BT.2408**：引入参考 BT.2408 的亮度增益路由和高光色域压缩，在保留 HDR 余量的同时减少“发白”和过曝观感。
- **JXL 与 AVIF 进阶支持**：JXL 解码输出提升至 FP16 半精度；AVIF 格式改进 EOTF 处理，以提升 HDR 色彩和亮度一致性。
- **Reinhard Extended 可调参数**：升级高动态感知渲染映射，设置中新增 Reinhard anchor 配置滑动条以支持颗粒化自定义调节。

### 🎨 色域警告与软打样同步对比
- **65x65x65 3D LUT**：提供极高精度的 GPU 硬件加速色域溢出检测。
- **软打样自动对比**：开启色彩软打样时，若按 **C** 键进入对比模式，QuickView 将自动为您并排呈现“打样前（原图）”与“打样后（应用 ICC 打样效果后）”的图像，便于直观比对印刷/色域转换效果。
- **1秒防抖缓冲**：增加了计算去抖处理，确保急速切图时不会有任何系统延迟。

### 📐 可拖拽 EXIF 面板与临摹/鼠标穿透模式
- **可拖拽 EXIF 面板 (#179)**：EXIF 元数据面板全面支持鼠标自由拖拽 reposition，具备视口边缘强限制保护，且会自动记忆与保存面板位置。
- **临摹/鼠标穿透模式 (Overlay Mode)**：利用 DirectComposition 渲染出高度自定义的半透明浮动视口。
  - **鼠标穿透**：开启后，鼠标的所有点击直接透传至下方软件（`WM_EX_TRANSPARENT`），是数字画师、UI 设计师临摹和描红的绝佳利器。
  - **快捷键**：`Ctrl + Shift + O` 开启/关闭临摹模式；`Alt + 键盘上下键` 调整图层透明度；`Shift + Esc` 退出鼠标穿透状态。

---

# QuickView v5.0.0 - 高级色彩与架构升级
**发布日期**: 2026-04-05

### 🚀 Google Highway & ARM64 支持
# QuickView v5.2.1 - 动画与个性化更新
**发布日期**: 2026-04-15

### 🎬 全格式动画支持 (#92)
- **通用播放**: 高性能支持 `.gif`, `.webp`, `.apng` 以及 `.avifs` 动画格式。
- **帧查看器**: 动画播放时可通过 `Alt + 左右方向键` 进行逐帧步进，精确分析运动轨迹或提取特定帧。

### 🛠️ 专业工具：脏矩形 (Dirty Rect)
- **视觉刷新调试**: 为设计师和开发者引入了 **脏矩形** 可视化工具。它可以实时标注 GPU 当前正在解码和刷新渲染的精确区域。
- **配置路径**: 在 `设置 -> 界面 -> 专业工具` 中开启“显示脏矩形按钮”。
<br><img src="ScreenShot/Dirty_Rect.gif" alt="脏矩形调试演示" width="100%" style="border-radius: 6px; margin-top: 10px;">

### 🎨 深度个性化与主题定制 (#129)
- **深色/浅色模式**: 完美支持手动切换或随系统自动同步的深色/浅色 UI。
- **强调色控制**: 自定义界面强调色，使其与您的 Windows 配色或个人喜好完美融合。
- **环境遮罩 (Ambient Dimmer)**: 新增沉浸式遮罩，在呼出菜单或画廊时自动调暗背景，让视觉重心瞬间聚焦。

### 🌈 HDR 与高级色彩 (#131)
- **16-bit scRGB 管线**: 通过全面迁移至原生 scRGB 浮点管线，彻底解决了 HDR 显示器下图片发灰、色彩暗淡的顽疾。
- **自动化 CMS**: 智能识别 ICC 配置文件，驱动 GPU 进行毫秒级色彩空间转换。

### 🧭 导航与交互优化 (#132)
- **右键拖动缩放**: 引入专业级的交互方式——按住鼠标右键并上下拖动即可实现平滑缩放。
- **智能初始缩放 (#127)**: 优化了在 1080p 显示器上的像素对齐逻辑，确保图片初始打开即极致清晰。

---

## ✨ 核心功能

> *"速度即功能。"*

QuickView 利用 **多线程解码** 技术处理 **JXL** 和 **AVIF** 等现代格式，在 8 核 CPU 上相比标准查看器加载速度提升高达 **6倍**。
* **零延迟预览：** 针对巨型 RAW (ARW, CR2) 和 PSD 文件的智能提取技术。
* **调试 HUD：** 按 `F12` 查看实时性能指标（解码时间、渲染时间、内存使用）。*(首次使用请在 **设置 > 高级** 中开启调试模式)*
  <br><img src="ScreenShot/DebugHUD.png" alt="调试模式" width="100%" style="border-radius: 6px; margin-top: 10px;">

### 2. 🎛️ 可视化控制中心
> *告别手动编辑 .ini 文件。*

<img src="ScreenShot/settings_ui.png" alt="Settings UI" width="100%" style="border-radius: 6px;">

完全硬件加速的 **设置仪表板**。
* **精细控制：** 调整鼠标行为（平移 vs 拖拽）、缩放灵敏度和循环规则。
* **视觉个性化：** 实时调整 UI 透明度和背景网格。
* **便携模式：** 一键切换配置存储位置（AppData/系统 还是 程序文件夹/USB）。

### 3. 🔄 智能更新
> *让软件时刻保持最新。*

QuickView 会自动检测新版本，并支持一键静默更新。无需打开浏览器，即刻体验最新功能。

### 4. 📊 极客可视化
> *不只是看图；更要洞察数据。*

<div align="center">
  <img src="ScreenShot/geek_info.png" alt="Geek Info" width="48%">
  <img src="ScreenShot/photo_wall.png" alt="Photo Wall" width="48%">
</div>

* **实时 RGB 直方图：** 半透明波形叠加。
* **重构的元数据架构：** 更快、更准确的 EXIF/元数据解析。
* **HUD 照片墙：** 按 `T` 召唤高性能画廊叠加层，能够以 60fps 虚拟化滚动 10,000+ 张图片。
* **智能后缀修正：** 自动检测并修复错误的扩展名（如将 PNG 误存为 JPG）。
* **一键 RAW 渲染：** 极速切换 RAW 文件的“内嵌预览”与“完整解码”模式。
* **专业色彩分析：** 实时显示 **色彩空间** (sRGB/P3/Rec.2020)、**色彩模式** (YCC/RGB/CMYK) 和 **压缩质量** (Q-Factor)。

### 5. 🔍 极致视觉对比
> *"以前所未有的精度并行比对。"*

QuickView 提供了专为深度视觉分析打造的 **双图比对模式 (Compare Mode)**。
* **双路同步：** 两个窗格之间的缩放、平移和旋转完全同步，支持毫米级的精细核对。
* **极客 HUD：** 实时显示 **RGB 包络直方图** 和图像质量指标（熵、锐度），帮助您快速识别更优质的样张。
* **智能分割线：** 带有智能透明度的分割线，自动标注每个对比维度的“优胜者”。
  <br>![基础比对演示](ScreenShot/compare_mode.gif)
  <br>![HUD分析演示](ScreenShot/compare_mode2.gif)

### 6. 🌈 HDR 与色彩之巅
> *"像光本身一样观察。"*

QuickView 5.0 引入了工业级色彩工具链。
*   **真实 HDR 面板**：基于 SIMD 加速的实时峰值亮度估算与 "HDR Pro" 元数据（MaxCLL/FALL）分析。
*   **全局软打样**：瞬时模拟印刷效果或特殊色彩空间，支持自定义 ICC 映射。
*   **全线性工作流**：内部 32 位浮点管线，确保无带宽阶梯感与绝对色准。

---

## ⚙️ 引擎室

我们不使用通用编解码器。我们为每种格式选用 **最先进 (State-of-the-Art)** 的库。

| 格式 | 后端引擎 | 为什么它很棒 (架构) |
| :--- | :--- | :--- |
| **JPEG** | **libjpeg-turbo v3** | **AVX2 SIMD**。解压速度之王。 |
| **PNG / QOI** | **Google Wuffs** | **内存安全**。超越 libpng，轻松处理超大尺寸。 |
| **JXL** | **libjxl + threads** | **并行化**。高分辨率 JPEG XL 即时解码。 |
| **AVIF** | **dav1d + threads** | **汇编优化** 的 AV1 解码。 |
| **SVG** | **Direct2D Native** | **硬件加速**。无限无损缩放。 |
| **RAW** | **LibRaw** | 针对“即时预览”提取进行了优化。 |
| **EXR** | **TinyEXR** | 轻量级、工业级 OpenEXR 支持。 |
| **HEIC / TIFF**| **Windows WIC** | 硬件加速（需要系统扩展）。 |

---

## ⌨️ 快捷键与帮助

> *随时按 `F1` 呼出交互式快捷键指南。*

<div align="center">
  <img src="ScreenShot/help_ui.png" alt="帮助窗口" width="100%" style="border-radius: 10px; box-shadow: 0 0 20px rgba(0,0,0,0.5);">
</div>

🗺️ 开发计划🗺️ Roadmap
我们因为持续进化而卓越。以下是当前正在开发的功能：

- **临摹模式 (Tracing Mode)**: 半透明的薄膜覆盖模式，适用于设计师参考图及临摹描绘。
- **视频墙分屏 (Video Wall Port)**: 支持多显示器视觉阵列输出。
- **插件系统 (Plugin System)**: 开放式架构，支持社区开发编解码器与滤镜插件。

---

## 💻 系统要求

| 组件 | 最低要求 | 备注 |
| :--- | :--- | :--- |
| **操作系统** | Windows 10 (1511+) | 需要 DirectComposition Visual3 支持 |
| **CPU** | 支持 SSE4 指令集的处理器 | **大幅扩展硬件覆盖** (Intel 2008+ / AMD 2011+) |
| **架构** | x64 或 ARM64 | Windows on ARM 原生 NEON 优化 |

> ⚠️ **重要提示:** QuickView 现已集成 **Google Highway**，通过动态 SIMD 调度显著提升了老旧硬件的适应性。虽然要求降至 **SSE4**，但现代 CPU 用户仍可享受全速 NEON 或 AVX 带来的性能收益。

---

## 📥 安装

QuickView 的核心设计理念是 **绿色与便携**。

### 🍃 推荐方案：绿色便携模式
1. 从 [**Releases**](https://github.com/justnullname/QuickView/releases) 下载 `QuickView_*.zip` 压缩包。
2. 解压到任意位置并运行 `QuickView.exe`。
3. 在应用内 **设置 > 便携模式** 中管理配置文件存储位置。

### 📦 安装版与 WinGet
如果您需要标准的 Windows 安装程序（内置深度卸载工具）：
*   **手动安装**：从 [**Releases**](https://github.com/justnullname/QuickView/releases) 下载 `QuickView_Installer_*.exe`。
*   **命令行安装**：
    ```powershell
    winget install QuickView
    ```

---

## 🛠️ 从源码编译

> ⚠️ **架构说明**：本项目已全面迁移至 `CMake + Ninja + vcpkg` 构建系统，采用 **Clang-cl** 编译器和 **Full LTO** 链接时优化以实现极限体积压缩。旧版的 `.sln` 和 `.vcxproj` 已被弃用。

### 前置要求
1. **Visual Studio 2022** (安装 "使用 C++ 的桌面开发" 工作负载)。
2. **LLVM / Clang 工具链** (确保 `clang-cl.exe` 和 `lld-link.exe` 已添加至系统环境变量 `PATH` 中)。
3. **CMake** 和 **Ninja** (可由 VS 自带或单独安装)。
4. **Git**。

### 一键编译指令
克隆项目后，在根目录下依次执行以下两条咒语：

```powershell
# 1. 自动调用 vcpkg 拉取依赖并配置 Release-LTO 构建矩阵
cmake --preset Release-LTO

# 2. 启动 Ninja 后端进行多核极限编译与 LTO 链接
cmake --build out/build/Release-LTO
```

编译产物将位于 `out/build/Release-LTO/` 目录下。

---

## ⚖️ 致谢鸣谢

> [!NOTE]
> **开发者寄语**
>
> 我利用业余时间维护 QuickView，只因我相信 Windows 值得拥有一个更快、更纯粹的看图工具。
> 我没有推广预算，也没有团队。如果 QuickView 对您有所帮助，在 GitHub 上点一颗星或分享给朋友，就是对我最大的支持。

**QuickView** 站在巨人的肩膀上。
基于 **GPL-3.0** 许可。
特别感谢 **David Kleiner** (JPEGView 原作者) 以及 **LibRaw, Google Wuffs, dav1d, 和 libjxl** 的维护者们。
特别感谢 **@Dimmitrius** 对俄罗斯语翻译进行的深度优化。
