# HacPack

轻量级的文件打包工具和库（C++20）。

Lightweight file packaging tool and libraries (C++20).

---

## HacPack 提供：

## HacPack provides:

- 一个 V2 专用打包 CLI（`HacPack`）：`pack / unpack / list / compare`。

- A V2-only CLI (`HacPack`) with `pack / unpack / list / compare` commands.

- 核心存储改为“按编号 file_id 存储数据”，文件名与 HASH 通过插件元信息提供。

- Core storage is ID-based (`file_id`), while names and hashes are provided by metadata plugins.

- `file_id=0` 为特殊元信息文件（KV 列表），支持插件内联或独立文件两种存法。

- `file_id=0` is a special metadata file (KV list) storing plugin name -> file_id mappings.

---

## 主要特点

## Main features

- 除STL外不依赖任何第三方库

- No third-party dependencies except for the STL.

- 纯 C++20 实现，可移植到 Windows / POSIX 平台。

- Pure C++20 implementation, portable to Windows / POSIX platforms.

- 仅支持 V2 包格式（`version=2`），不支持 V1 文件读取。

- V2-only (`version=2`); V1 files are not supported.

- HASH 插件当前实现为 `CRC32`，并在 `list --hash` 与 `compare` 中使用。

- Hash plugin currently uses `CRC32`, used by `list --hash` and `compare`.

- 可选使用静态 C 运行时（CMake 选项 `USE_STATIC_CRT`）。

- Optional static C runtime via CMake option `USE_STATIC_CRT`.

---

## 快速开始（使用 CMake）

## Quick start (using CMake)

```bash
mkdir build && cd build
cmake ..
cmake --build . --config Release
# 可执行文件位于 build/bin/HacPack
```

```bash
mkdir build && cd build
cmake ..
cmake --build . --config Release
# The executable is located at build/bin/HacPack
```

---

- 说明：CMake 选项 `USE_STATIC_CRT` 默认为 `ON`，用于选择是否链接静态运行时（MSVC 使用 `/MT`，其它工具链尝试 `-static-libgcc -static-libstdc++`）。

- Note: The CMake option `USE_STATIC_CRT` defaults to `ON` and controls whether to link the static runtime (MSVC uses `/MT`, other toolchains attempt `-static-libgcc -static-libstdc++`).

---

## 用法示例

## Usage examples

- 打包目录：
  `HacPack pack path/to/directory output.hac`

- Pack a directory:
  `HacPack pack path/to/directory output.hac`

---

- 解包到目录：
  `HacPack unpack input.hac output_dir`

- Unpack to directory:
  `HacPack unpack input.hac output_dir`

---

- 显示包文件列表（含 HASH）：
  `HacPack list input.hac --hash`

- Show pack file list (with HASH):
  `HacPack list input.hac --hash`

---

- 比较包与目录是否一致：
  `HacPack compare input.hac path/to/directory`

- Compare pack and directory consistency:
  `HacPack compare input.hac path/to/directory`

---

## HacPack V2 文件结构（概要）

## HacPack V2 file layout (overview)

V2 采用“核心按编号 + 插件化元信息”的布局：

V2 uses a "core by id + plugin metadata" layout:

| 字段 | 偏移 / 大小 | 描述 |
|---|---:|---|
| Magic | 0 (8 bytes) | ASCII `"HacPack"`。 |
| Version | 8 (4 bytes, little-endian uint32) | 固定为 `2`（V2 only）。 |
| EntryCount | 12 (4 bytes, little-endian uint32) | 核心条目数（包含 `id=0`）。 |
| CoreIndex | 16 (EntryCount * 12 bytes) | 每条记录：`id(u32), offset(u32), size(u32)`。 |
| DataArea | 16 + EntryCount*12 | 按 CoreIndex 中 offset/size 存储数据。 |

| Field | Offset / Size | Description |
|---|---:|---|
| Magic | 0 (8 bytes) | ASCII `"HacPack"`. |
| Version | 8 (4 bytes, little-endian uint32) | Fixed to `2` (V2 only). |
| EntryCount | 12 (4 bytes, little-endian uint32) | Number of core entries (including `id=0`). |
| CoreIndex | 16 (EntryCount * 12 bytes) | Per record: `id(u32), offset(u32), size(u32)`. |
| DataArea | 16 + EntryCount*12 | Raw data blocks addressed by CoreIndex offset/size. |

---

## 元信息与插件（file_id=0）

## Metadata and plugins (file_id=0)

- `id=0` 是特殊元信息文件，内容是 KV 列表，每项是：`插件名字符串 + file_id(u32)`。
- `id=0` is a special metadata file storing KV entries of `plugin-name string + file_id(u32)`.

- 主程序根据“插件名”在注册表里找插件，并把 `file_id` 交给插件初始化。
- The main program resolves plugins by plugin name in a registry and passes `file_id` for initialization.

- 当前已实现 HASH 插件算法名为 `CRC32`。
- Current hash plugin algorithm name is `CRC32`.

### 关键插件映射（当前实现）

### Key plugin mappings (current implementation)

- `NameList -> <file_id>`：文件名+长度信息文件。
- `NameList -> <file_id>`: filename + size metadata file.

- `CRC32 -> <file_id>`：CRC32 索引信息文件。
- `CRC32 -> <file_id>`: CRC32 index metadata file.

- 预留插件名（当前不实现逻辑）：`Compression`、`Encryption`。
- Reserved plugin names (logic not implemented yet): `Compression`, `Encryption`.

---

## pack 参数

## pack options

当前 `pack` 不需要额外元信息模式参数，默认输出插件分离文件并在 `id=0` 中登记。

Current `pack` requires no extra metadata mode options; plugin metadata is stored in separate files and registered in `id=0`.

---

## 库和源码用途说明

## Library and source purpose

- `hacpack_writer`（库）

- `hacpack_writer` (library)

  - 提供 `HacPackBuilder` 和写入器接口（`HacPackWriter`），负责在内存中构建条目并将最终的 header/info 与数据写入目标（例如文件或内存）。

  - Provides `HacPackBuilder` and writer interfaces (`HacPackWriter`) to build entries in memory and write the final header/info and data to a target (e.g., file or memory).

  - 包含 `hacpack_builder_file.cpp` 中的便利函数，用于从磁盘加载文件数据并把它们作为条目添加到 `HacPackBuilder`。

  - Includes helper functions in `hacpack_builder_file.cpp` to load file data from disk and add entries to `HacPackBuilder`.

---

- `hacpack_reader`（库）

- `hacpack_reader` (library)

  - 提供读取能力（历史实现为 V1 的 info/index 读取；当前 CLI 主流程已切换到 V2 自定义读取实现）。

  - Provides reading support (historically V1 info/index parsing; current CLI main path uses dedicated V2 parsing).

  - V2 下文件名不属于核心格式，文件名由 NameMap 插件提供。

  - Under V2, filenames are not part of the core format and are provided by the NameMap plugin.

---

- `hacpack_utf`（库）

- `hacpack_utf` (library)

  - 提供 UTF 编码/转换的实现（例如 UTF-8/16/32 的互转），以及与平台相关的编码抽象（Windows/Posix 实现）。

  - Provides UTF encoding/conversion utilities (e.g., UTF-8/16/32 conversions) and platform-specific encoding abstractions (Windows/Posix implementations).

  - 主要用于处理外部文本输入（如文件列表、平台代码页等），与包内名称读取解耦。

  - Primarily used for external text inputs (e.g., file lists, platform code pages), decoupled from reading names inside packs.

---

- 可执行文件相关源

- Executable-related sources

  - `main.cpp`：当前 V2 CLI 主逻辑（pack/unpack/list/compare + KV 插件加载）。

  - `main.cpp`: Current V2 CLI main logic (pack/unpack/list/compare + KV plugin loading).

  - `dir_scan.cpp`：实现递归扫描目录并产生 `(disk_path, stored_name)` 对，仅由可执行程序 `HacPack` 使用，不被静态库依赖。

  - `dir_scan.cpp`: Implements recursive directory scanning producing `(disk_path, stored_name)` pairs; used only by the `HacPack` executable and not by the static libraries.

  - `hacpack_writer_file.cpp` / `hacpack_writer_vector.cpp`：提供将最终包写入文件或内存缓冲区的具体 `HacPackWriter` 实现。

  - `hacpack_writer_file.cpp` / `hacpack_writer_vector.cpp`: Provide concrete `HacPackWriter` implementations to write the final pack to a file or an in-memory buffer.

---

## 开发与贡献

## Development & Contribution

欢迎提交 issue 或 pull request。请在提交前确保代码在主流平台上能通过构建。

Issues and pull requests are welcome. Please ensure the code builds on mainstream platforms before submitting.

---

## 许可证

## License

本项目采用 BSD 3-Clause 许可证发布，详见仓库根目录的 `LICENSE` 文件。

This project is released under the BSD 3-Clause License; see the `LICENSE` file in the repository root for the full text.

这个库过于简单，所以我们不关心您用它做什么，也不对任何使用该库的结果负责。您使用它产生的任何问题都与我们无关，请自行承担风险。

This library is very simple, so we do not care what you do with it and are not responsible for any results of using it. Any issues arising from your use of it are your own responsibility; use at your own risk.

---

## 联系方式

## Contact

如需帮助，请在仓库中打开 issue。

If you need help, please open an issue in the repository.