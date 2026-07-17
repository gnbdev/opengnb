# 自动化测试框架

当前目录只提供 Unity 测试框架的最小构建与运行入口，用于验证框架在 Linux GCC 和 Windows MSYS2 MINGW64 环境下可以编译、链接和启动。当前运行器注册零个生产测试，不链接 `opengnb` 的任何生产源码，也不提供模块测试或覆盖率统计。

## 运行方式

所有命令都必须从仓库根目录执行，并显式指定平台。

Linux：

```sh
make -f tests/Makefile PLATFORM=linux CC=gcc test-framework
make -f tests/Makefile PLATFORM=linux test-clean
```

Windows MSYS2 MINGW64：

```sh
make -f tests/Makefile PLATFORM=mingw CC=gcc test-framework
make -f tests/Makefile PLATFORM=mingw test-clean
```

`test-framework` 会先根据 `libs/Unity/SHA256SUMS` 校验归档字节，再编译并执行空运行器。运行时会明确输出“框架冒烟，零个生产测试”，随后由 Unity 报告零个测试、零个失败。构建产物分别位于：

- `build/tests/linux/unity_empty_runner`
- `build/tests/mingw/unity_empty_runner.exe`

`test-clean` 只清理当前 `PLATFORM` 对应的构建目录。

## 当前边界

- 构建入口独立于平台主 Makefile，不修改 `Makefile.linux`、`Makefile.mingw_x86_64` 或其他平台构建文件。
- 构建图只包含 `libs/Unity/src/unity.c` 和零用例运行器 `tests/framework/unity_empty_runner.c`。
- 运行器显式依赖 Unity 头文件和 `tests/Makefile`，相关内容变化时会重新链接，避免增量冒烟复用旧二进制。
- 当前不包含真实业务测试，不链接 `src/` 下的生产源码，不提供通用 `test` 目标。
- 在 PR #54、#55、#56、#57 合并后，再基于稳定的生产代码基线增加核心模块测试、平台 Makefile 集成和完整构建门禁。

如果后续测试暴露生产缺陷，必须先停止扩展测试并完成缺陷修复，不得绕过、屏蔽或弱化测试。
