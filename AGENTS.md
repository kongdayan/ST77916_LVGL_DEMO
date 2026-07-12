# Agent Instructions — ST77916 LVGL Demo

ESP32-S3 嵌入式固件项目，使用 **PlatformIO + Arduino + LVGL 8.3**。
目标硬件：ST77916 QSPI 圆形屏（360×360）+ CST816S 电容触摸 + TF 卡 + I2S 音频/麦克风。

---

## 一、快速上手

### 环境依赖

| 工具 | 版本要求 | 安装方式 |
|------|---------|---------|
| Python | ≥ 3.10 | [python.org](https://python.org) |
| PlatformIO CLI | 最新 | `pip install platformio` |
| esptool.py | 随 PlatformIO 自动安装 | — |

> VS Code 用户安装 PlatformIO IDE 插件可直接使用图形界面。

---

## 二、编译链命令

### 常用命令速查

| 操作 | 命令 |
|------|------|
| 编译 | `pio run` |
| 编译 + 烧录 | `pio run --target upload` |
| 串口监视器（115200） | `pio device monitor` |
| 编译 + 烧录 + 监视器 | `pio run --target upload && pio device monitor` |
| 清除构建缓存 | `pio run --target clean` |
| 查看连接设备 | `pio device list` |
| 详细编译日志 | `pio run -v` |
| 查看固件大小 | `pio run --target size` |

> **规则**：修改任何 `.c` / `.cpp` / `.h` 文件后，必须用 `pio run` 验证编译通过，再提交。

### 手动烧录（esptool）

仅烧录应用分区：
```bash
esptool.py --chip esp32s3 --port /dev/ttyUSB0 --baud 921600 \
  write_flash 0x10000 .pio/build/esp32s3/firmware.bin
```

完整烧录（bootloader + 分区表 + 应用）：
```bash
esptool.py --chip esp32s3 --port /dev/ttyUSB0 --baud 921600 \
  write_flash \
  0x0000  .pio/build/esp32s3/bootloader.bin \
  0x8000  .pio/build/esp32s3/partitions.bin \
  0xe000  ~/.platformio/packages/framework-arduinoespressif32/tools/partitions/boot_app0.bin \
  0x10000 .pio/build/esp32s3/firmware.bin
```

> 若设备不能自动进入下载模式，按住 **BOOT** 键再插 USB，进入下载模式后松开。

### 固件产物

```
.pio/build/esp32s3/
  firmware.bin      ← 应用分区（烧录到 0x10000）
  firmware.elf      ← 含调试符号，用于 GDB / 崩溃地址反解
  bootloader.bin    ← Bootloader
  partitions.bin    ← 分区表
```

---

## 三、视频素材准备

`screen_video.c` 从 TF 卡读取原始 RGB565 视频文件：

```bash
# 用 ffmpeg 将任意视频转为 360×360 RGB565 原始格式
ffmpeg -i input.mp4 \
  -vf "scale=360:360:force_original_aspect_ratio=increase,crop=360:360" \
  -vcodec rawvideo -pix_fmt rgb565be \
  -f rawvideo video.rgb
```

将 `video.rgb` 复制到 TF 卡根目录，文件名固定为 `video.rgb`。
每帧大小：`360 × 360 × 2 = 259200 bytes`，帧率约 24fps（42ms/帧）。

---

## 四、项目目录结构

```
ST77916_LVGL_DEMO/
├── main.cpp                  ← Arduino 入口：setup() / loop()
├── platformio.ini            ← 构建配置（平台、库依赖、编译 filter）
├── lv_conf.h                 ← LVGL 功能开关（只开启项目用到的控件）
├── ST77916_LVGL_DEMO.ino     ← Arduino IDE 兼容入口（不用于 PlatformIO 构建）
│
├── hal/                      ← 硬件抽象层（Hardware Abstraction Layer）
│   ├── pincfg.h              ← 全部 GPIO 宏定义（屏幕/触摸/SD/音频/麦克风）
│   ├── display.cpp/.h        ← 显示 + 触摸 + LVGL driver 初始化；屏幕省电逻辑
│   └── sd_card.cpp/.h        ← SD 卡（SDMMC 4-bit）挂载，挂载点 /sdcard
│
├── screens/                  ← 各功能屏幕，每屏一个 .c/.h 文件对
│   ├── screen_dashboard.c/h  ← 首页：3 层同心弧度盘 + +/- 按钮
│   ├── screen_info.c/h       ← TabView：网络/日历/设置三个 Tab
│   ├── screen_image.c/h      ← 全屏图片展示
│   ├── screen_video.c        ← TF 卡 RGB565 视频回放（无 .h，内部逻辑）
│   ├── screen_about.c/h      ← 关于页，设计师信息
│   ├── screen_agent.c/h      ← Hex-Ball 小游戏（物理、自定义绘制、触摸旋转圆环）
│   ├── screen_3dmodel.c/h    ← 3D 模型渲染演示
│   └── screen_codex_usage.c/h← Codex 使用量 Watch Face 界面
│
├── ui/                       ← LVGL 基础层（部分由 SquareLine Studio 生成）
│   ├── ui.c / ui.h           ← 主题初始化、懒加载屏幕注册、全局 screen 声明
│   ├── ui_helpers.c / .h     ← _ui_screen_change()、_ui_arc_increment() 工具函数
│   └── ui_img_1539399133.c   ← 嵌入式图片资源（LV_IMG_DECLARE）
│
├── assets/                   ← 设计原图（PNG），不编译进固件
├── example/                  ← 效果截图（dark/light 主题对比图）
├── test/                     ← 测试用真机照片
├── backup/                   ← SquareLine Studio 项目备份（.zip）
├── cache/                    ← SquareLine Studio 缩略图缓存（可忽略）
│
└── .github/
    ├── PULL_REQUEST_TEMPLATE.md  ← PR 中文模版（自动填充）
    └── workflows/
        ├── build.yml         ← push/PR 自动编译，产物保留 30 天
        ├── pr-check.yml      ← PR 专属检查：lint + 密钥扫描 + 固件大小评论
        └── release.yml       ← 发布 Release 时编译固件并上传 zip + sha256
```

---

## 五、硬件平台参数

| 参数 | 值 |
|------|----|
| SoC | ESP32-S3（Xtensa LX7，240 MHz 双核） |
| Flash | 16 MB（QIO） |
| PSRAM | 8 MB（OPI） |
| 显示 | ST77916，QSPI 4-bit，360×360，圆形 |
| 触摸 | CST816S，I2C |
| 存储 | TF 卡（SDMMC 4-bit，最高 40 MHz） |
| 音频输出 | I2S DAC |
| 音频输入 | I2S MEMS 麦克风 |
| PlatformIO platform | pioarduino `53.03.11`（Arduino ESP32 3.1.1 / IDF 5.3） |
| 上传波特率 | 921600 |
| 串口波特率 | 115200 |

### GPIO 引脚一览

| 功能 | GPIO |
|------|------|
| 背光 PWM（LEDC） | 15 |
| 屏幕 RST | 47 |
| 屏幕 CS | 10 |
| 屏幕 SCK | 9 |
| 屏幕 DATA0–3（QSPI） | 11、12、13、14 |
| 触摸 SCL | 8 |
| 触摸 SDA | 7 |
| 触摸 INT | 41 |
| 触摸 RST | 40 |
| 按钮（BOOT） | 0 |
| SD D0–D3 | 2、1、6、5 |
| SD CLK | 3 |
| SD CMD | 4 |
| I2S BCK | 18 |
| I2S WS（LCK） | 16 |
| I2S DO（DIN） | 17 |
| 音频静音（低有效） | 48 |
| 麦克风 WS | 45 |
| 麦克风 SD | 46 |
| 麦克风 SCK | 42 |

---

## 六、屏幕导航架构

所有屏幕采用**懒加载**模式，`lv_obj_t *` 首次访问前为 `NULL`。

```
ui_init()
  └─ 注册首屏 screen_dashboard
        ↓ 用户滑动 / 点击
  _ui_screen_change(get_ptr_fn, init_fn, dir)
        ↓
  若目标屏为 NULL → 调用 init_fn() 创建
        ↓
  lv_scr_load_anim() 切换
```

每个屏幕文件对外暴露两个函数：

```c
void       screen_xxx_init(void);      // 创建所有 LVGL 控件
lv_obj_t **screen_xxx_get_ptr(void);   // 返回指向内部 scr 指针的地址
```

切换示例：
```c
_ui_screen_change(screen_info_get_ptr, screen_info_init, LV_SCR_LOAD_ANIM_MOVE_LEFT);
```

---

## 七、添加新屏幕（标准流程）

1. 在 `screens/` 创建 `screen_foo.c` 和 `screen_foo.h`。
2. `.h` 中声明：
   ```c
   void       screen_foo_init(void);
   lv_obj_t **screen_foo_get_ptr(void);
   ```
3. `.c` 中实现，内部维护 `static lv_obj_t *scr = NULL`。
4. 在 `ui/ui.h` 中引入声明（`#include` 或直接复制函数签名）。
5. 在 `platformio.ini` 的 `build_src_filter` 中追加：
   ```ini
   +<screens/screen_foo.c>
   ```
6. 在触发切换的地方调用 `_ui_screen_change()`。
7. 运行 `pio run` 验证编译。

---

## 八、CI / CD 工作流

| Workflow | 触发条件 | 做什么 |
|----------|---------|--------|
| `build.yml` | push 到 main / PR 到 main | 编译，artifact 保留 30 天 |
| `pr-check.yml` | PR 到 main | ① cppcheck lint ② Gitleaks 密钥扫描 ③ 编译并将固件大小评论到 PR |
| `release.yml` | 发布 Release | 编译，打 zip，生成 sha256，上传到 Release 附件 |

### 分支保护规则（main）

- 禁止直接 push，所有改动必须通过 PR
- 唯一合并方式：**Squash and merge**
- PR 合并后自动删除 feature branch

---

## 九、编码约定

1. **每屏一文件**：所有静态状态变量放在该 `.c` 文件内部，不跨文件共享。
2. **跨屏引用**：通过 `screen_xxx_get_ptr()` 获取指针，不使用全局变量。
3. **LVGL 绘制回调**：在进入实际绘制逻辑前先检查 `lv_event_get_code(e) == LV_EVENT_DRAW_POST_BEGIN`。
4. **定时器生命周期**：在 `LV_EVENT_SCREEN_LOADED` 中创建，在 `LV_EVENT_SCREEN_UNLOADED` 中删除。
5. **新增源文件**：必须在 `platformio.ini` 的 `build_src_filter` 中手动注册，否则不会被编译。
6. **指针类型转换**：使用 `static_cast<>` / `reinterpret_cast<>`，禁止 C 风格 `(Type *)` 转换（会被 cppcheck lint 拦截）。
7. **`ui/` 目录**：部分文件由 SquareLine Studio 生成，不纳入 cppcheck 检查，尽量不手动修改。

---

## 十、常见问题排查

| 现象 | 原因 | 解决 |
|------|------|------|
| `quad_mode` 编译报错 | IDF 版本不匹配 | 确认使用 pioarduino platform，不使用官方 espressif32 |
| `setup()` 未定义 | `main.cpp` 未加入 `build_src_filter` | 在 `platformio.ini` 中追加 `+<main.cpp>` |
| 屏幕切换后黑屏 | `init_fn` 未被调用或 `get_ptr` 返回 NULL | 检查 `_ui_screen_change` 的第二个参数是否传入了 `init` 函数 |
| 触摸无响应 | 控件未设置 `LV_OBJ_FLAG_CLICKABLE` | 在 `lv_obj_add_flag(obj, LV_OBJ_FLAG_CLICKABLE)` |
| 触摸被上层透明控件拦截 | 透明层默认可点击 | 对透明层添加 `LV_OBJ_FLAG_EVENT_BUBBLE` 或 `clear_flag(CLICKABLE)` |
| SD 卡未挂载 | 卡未插入或格式非 FAT32 | 检查 `sd_card_is_mounted()` 返回值，确认 TF 卡格式化为 FAT32 |
| 视频卡顿/花屏 | `video.rgb` 格式不正确 | 用 ffmpeg 重新转换，确认 `-pix_fmt rgb565be` 和分辨率 360×360 |
| cppcheck lint 失败 | C 风格指针转换 | 改用 `static_cast<>` / `reinterpret_cast<>` |
| Gitleaks 密钥扫描失败 | 代码中含硬编码 token/密码 | 移除敏感字符串，改用配置文件或编译宏注入 |
