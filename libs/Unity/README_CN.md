# Unity 归档说明

本目录归档 Unity 测试框架的最小运行时源码，用于项目内自动化测试基础设施。第三方源码和许可证保持上游原始字节，不在本仓库中翻译、格式化或直接修改。

## 来源

- 上游仓库：https://github.com/ThrowTheSwitch/Unity
- 上游版本：`v2.6.1`
- 发布页面：https://github.com/ThrowTheSwitch/Unity/releases/tag/v2.6.1
- 固定提交：`cbcd08fa7de711053a3deec6339ee89cad5d2697`
- 提交页面：https://github.com/ThrowTheSwitch/Unity/commit/cbcd08fa7de711053a3deec6339ee89cad5d2697
- 获取日期：`2026-07-17`
- 许可证：MIT，许可证原文见 `LICENSE.txt`
- 校验清单：`SHA256SUMS`

`v2.6.1` tag 已核对为直接指向上述固定提交。归档文件均从该提交的 `raw.githubusercontent.com` 地址直接获取。

## 文件摘要

以下摘要使用 SHA-256 计算：

| 归档文件 | 上游路径 | SHA-256 |
| --- | --- | --- |
| `src/unity.c` | `src/unity.c` | `b90e735a54cf3b3765ab6caa955d11a1488ee73d9c6152cdc98576c2d17cb871` |
| `src/unity.h` | `src/unity.h` | `9db174d3c2c6424fd35c0980c5941d124c5ebb0f48e8172f997a2aa9554b64ea` |
| `src/unity_internals.h` | `src/unity_internals.h` | `fcd8b3f6b412ac0ab599547eb8a30b6d7f3f0af77aab31f7a1822a2a8fc9a2b2` |
| `LICENSE.txt` | `LICENSE.txt` | `907d9e859c6433703c0c183de3ddeaaf4baf3d517382f8f368b2c190fd2581d1` |

`SHA256SUMS` 是自动校验使用的机器可读清单，内容与上表一致。仓库通过 `.gitattributes` 禁止 Git 转换四个上游文件的行尾，确保 Windows 检出不会改变归档字节。

## 最小集成边界

- 当前只归档 Unity 核心运行时所需的一个 C 源文件、两个头文件和许可证。
- 不归档 Unity 的生成脚本、示例、测试、插件、CMock、Ceedling 或 Ruby 依赖。
- 项目测试只需编译 `src/unity.c` 并将 `src/` 加入头文件搜索路径，无需安装系统级测试包。
- 本目录不包含项目测试用例和运行器，也不链接 opengnb 的生产源码；这些内容由项目自己的 `tests/` 构建入口负责。

## 升级约束

1. 升级必须作为独立变更审查，不得在无关功能修改中顺带更新。
2. 选择有明确 tag 的上游版本，并记录 tag 实际指向的不可变完整提交。
3. 从固定提交重新获取所需文件，保持第三方文件原始字节，不直接修改归档源码或许可证。
4. 重新计算并同步更新 `SHA256SUMS` 与本说明中的全部 SHA-256，同时核对许可证和上游发布说明。
5. 升级后必须使用 Linux GCC 与 Windows MinGW GCC 编译并执行项目的框架冒烟入口。
