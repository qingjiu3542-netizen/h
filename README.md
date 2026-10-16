# SidebarLauncher (iOS 越狱侧边栏原型)

这是一个基于 **Theos + Logos** 的 iOS 越狱侧边栏原型，面向 **iPhone 14 Pro Max / iOS 16.4.1**，目标是提供类似安卓浮屏的系统级右侧边缘呼出侧边栏，用于启动用户配置的系统应用和第三方应用。

## 假设与适配范围

- 默认按 **Dopamine rootless** 方案设计与打包（`THEOS_PACKAGE_SCHEME = rootless`）
- 注入目标：`SpringBoard`（`Filter: com.apple.springboard`）
- rootless 配置文件路径：`/var/jb/Library/SidebarLauncher/config.plist`
- 也可尝试在 palera1n rootless 使用；若为 rootful，请按环境调整路径与打包参数

> 本原型不包含 DRM、付费验证、账号限制绕过或其他安全防护绕过功能。

## 功能说明

- 屏幕右侧轻量触发区域（可配置宽度）
- 从右向左滑动呼出侧边栏
- 在侧边栏内从左向右滑动、点击遮罩层、点击关闭按钮可隐藏
- 侧边栏样式：圆角、半透明、阴影
- 配置化应用列表（名称、URL Scheme、图标）
- 使用公开 `UIApplication openURL` 方式启动，不使用私有绕过
- 支持配置：
  - `enabled`（总开关）
  - `triggerEdgeWidth`（触发边缘宽度）
  - `sidebarWidth`（侧边栏宽度）
  - `opacity`（透明度）
  - `positionYRatio`（纵向位置比例）
  - `apps`（应用列表）

## 工程结构

- `Makefile`：Theos rootless tweak 构建
- `Tweak.xm`：SpringBoard 注入与侧边栏交互/UI
- `SidebarConfigManager.h/.m`：配置解析与参数兜底
- `SidebarLauncher.plist`：注入过滤
- `entitlements.plist`：签名 entitlements（最小示例）
- `control`：Debian 包元数据
- `layout/var/jb/Library/SidebarLauncher/config.plist`：默认配置
- `tests/validate_config.py`：配置格式校验脚本
- `scripts/build.sh`：可复现构建入口

## 配置格式（plist）

`/var/jb/Library/SidebarLauncher/config.plist` 示例：

```xml
<dict>
  <key>enabled</key><true/>
  <key>triggerEdgeWidth</key><real>18</real>
  <key>sidebarWidth</key><real>250</real>
  <key>opacity</key><real>0.85</real>
  <key>positionYRatio</key><real>0.5</real>
  <key>apps</key>
  <array>
    <dict>
      <key>bundleID</key><string>com.apple.Preferences</string>
      <key>displayName</key><string>Settings</string>
      <key>urlScheme</key><string>App-prefs:</string>
      <key>iconSystemName</key><string>gearshape.fill</string>
    </dict>
  </array>
</dict>
```

字段说明：

- `urlScheme` 必填；只有目标 App 支持对应 URL Scheme 时才能启动
- 对无法通过 URL Scheme 打开的应用，请在配置中替换为其可用 Scheme；若应用无公开 Scheme，则无法由本原型直接启动
- 图标支持：
  - `iconSystemName`（SF Symbols 名称）
  - `iconPath`（本地图片绝对路径）

## 依赖与编译

前置：

- 已安装 Theos 与 iOS toolchain
- 已正确设置 `THEOS`

构建：

```bash
python3 tests/validate_config.py layout/var/jb/Library/SidebarLauncher/config.plist
./scripts/build.sh
```

等价命令：

```bash
make clean package FINALPACKAGE=1
```

> 本仓库仅提供源码与可复现流程，不声称已生成 `.deb` 二进制产物。

## 安装与卸载

安装（设备端）：

1. 将生成的 `.deb` 传到设备
2. 使用 `dpkg -i <package>.deb`
3. `sbreload` 或重启 SpringBoard

卸载：

```bash
dpkg -r com.qingjiu3542.sidebarlauncher
sbreload
```

## 最小测试/验证

- `tests/validate_config.py`：检查配置键、类型、数值范围、应用项必填字段

## 已知限制

- 仅实现基础原型，未提供 PreferenceBundle 图形设置页
- App 启动能力取决于可用公开 URL Scheme
- 主要针对 iOS 16.4.1 + rootless 验证路径，其他越狱环境需二次适配

## 后续适配建议（Dopamine / palera1n）

- Dopamine rootless：保持 `/var/jb` 路径与 rootless 打包即可
- palera1n rootless：优先保持一致路径，重点验证注入稳定性与重载流程
- palera1n rootful：可增设 rootful 构建参数与配置路径映射（如 `/Library/...`）
