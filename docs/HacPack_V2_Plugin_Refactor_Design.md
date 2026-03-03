# HacPack V2 重构设计（编号存储 + 元数据插件）

## 1. 目标

本设计用于把当前“文件名内建在核心格式”的 HacPack 迁移为“核心只管编号与字节流”的架构：

- 所有文件按 `file_id` 编号存储，不再由核心格式直接承载文件名。
- `file_id=0` 作为“元数据文件（meta file）”，写在包末尾。
- 文件名列表、HASH、压缩映射等都通过插件记录在 `id=0` 元数据文件中。
- 核心格式尽量稳定，后续扩展（加密、压缩、签名、索引优化）通过插件演进。

---

## 2. 设计原则

1. **核心最小化**：核心仅定义“文件编号 + 数据偏移/大小 + 容器边界”。
2. **插件可选**：文件名、哈希、压缩信息均为可选插件，不耦合核心。
3. **向前扩展**：未知插件可跳过，不影响已知插件读取。
4. **写入单通道**：先写 `id=1..N` 数据，再写 `id=0` 元数据，最后写核心索引。
5. **兼容演进**：通过 `version` 区分 V1 / V2，先实现双读，再逐步切换写入默认。

---

## 3. V2 文件结构（提案）

## 3.1 顶层布局

```text
[Magic(8)]
[Version(u32)]
[CoreHeaderSize(u32)]
[CoreHeader(variable)]
[DataArea(variable)]   # 按 file_id 顺序写入，要求 id=0 最后写
[CoreIndex(variable)]  # 每个 file_id 的 offset/size/flags
```

说明：

- `Magic` 仍使用 `HacPack`。
- `Version=2` 代表新格式。
- 核心不解释业务语义；它只知道“某个 id 对应一段字节”。

## 3.2 CoreHeader 字段

建议字段：

- `entry_count (u32)`：总条目数（含 `id=0`）。
- `data_offset (u64)`：数据区起始偏移。
- `data_size (u64)`：数据区总大小。
- `index_offset (u64)`：索引区起始偏移。
- `index_size (u64)`：索引区大小。
- `core_flags (u32)`：核心保留标志位。

## 3.3 CoreIndex 条目

每条记录（固定长度）建议：

- `file_id (u32)`
- `data_offset_rel (u64)`：相对 `data_offset`。
- `data_size (u64)`
- `storage_flags (u32)`：如“是否压缩/加密”，仅标记，不承载算法细节。
- `reserved (u32)`：保留。

约束：

- `file_id` 唯一。
- `file_id=0` 必须存在，且位于数据区最后一段（便于流式写入元数据）。

---

## 4. 元数据文件（file_id=0）与插件容器

`id=0` 的内容定义为插件容器（Plugin Container），核心只把它当普通字节流。

## 4.1 插件容器布局

```text
[PluginContainerVersion(u16)]
[PluginRecordCount(u16)]
repeat PluginRecordCount times:
  [PluginType(u16)]
  [PluginVersion(u16)]
  [RecordFlags(u32)]
  [PayloadSize(u32)]
  [Payload(bytes)]
```

读取策略：

- 识别的插件按版本解析。
- 不识别的插件直接按 `PayloadSize` 跳过。

## 4.2 插件类型预留

- `0x0001`：文件名映射插件（NameMap）
- `0x0002`：HASH 插件（HashMap）
- `0x0003`：压缩映射插件（CompressionMap）
- `0x0004`：加密映射插件（EncryptionMap）
- `0x7FFF` 以上：实验/私有插件

---

## 5. 首批插件定义

## 5.1 NameMap 插件（首发必做）

作用：建立 `file_id -> UTF-8 相对路径` 映射。

Payload 建议：

```text
[Count(u32)]
repeat Count times:
  [FileId(u32)]
  [PathLen(u16)]
  [PathUtf8(bytes, no NUL)]
```

约束：

- 不包含 `id=0`。
- 路径使用 `/` 分隔。
- 禁止绝对路径、禁止 `..`。

## 5.2 HashMap 插件（预留并尽快实现）

作用：记录每个文件内容摘要，供 `compare`、完整性校验、缓存命中使用。

Payload 建议：

```text
[Algo(u16)]          # 1=FNV1A64, 2=XXH64, 3=SHA256 ...
[DigestSize(u16)]
[Count(u32)]
repeat Count times:
  [FileId(u32)]
  [Digest(bytes)]
```

说明：

- 现有实现已在 compare 内部做 FNV1A64 计算；后续可落盘到该插件。

## 5.3 CompressionMap 插件（预留）

作用：标注每个 `file_id` 的压缩算法与参数。

Payload 建议：

```text
[Count(u32)]
repeat Count times:
  [FileId(u32)]
  [Codec(u16)]       # 0=none, 1=zstd, 2=lz4 ...
  [Level(i16)]
  [OriginalSize(u64)]
```

---

## 6. CLI 行为变更建议

现有命令保持：`pack / unpack / list / compare`，但内部改成基于插件。

## 6.1 pack

- 目录文件按顺序分配 `file_id=1..N`。
- 生成 NameMap 插件（必须）。
- 可选生成 HashMap 插件（建议默认开启）。
- 最后把插件容器写为 `id=0`。

## 6.2 list

- 优先读取 NameMap 展示名称；若缺失则仅展示 `file_id`。
- `--hash`：优先读取 HashMap；无 HashMap 时实时计算并标记 `(computed)`。

## 6.3 unpack

- 需要 NameMap 才能还原文件名。
- 若无 NameMap，可降级为 `id_<n>.bin` 输出。

## 6.4 compare

- 若 HashMap 存在，优先使用包内 digest 进行快速比较。
- 若 HashMap 缺失，回退到读取包内容现场计算。

---

## 7. 代码结构重构建议

建议新增模块：

- `hacpack_core_format.h/.cpp`
  - V2 CoreHeader/CoreIndex 编解码。
- `hacpack_plugin_container.h/.cpp`
  - `id=0` 插件容器读写。
- `plugins/name_map_plugin.h/.cpp`
- `plugins/hash_plugin.h/.cpp`
- `plugins/compression_plugin.h/.cpp`（先定义接口）

当前 `main.cpp` 中与“文件名/HASH”耦合的逻辑应迁移到插件层。

---

## 8. 兼容策略（强烈建议）

分三阶段：

1. **阶段A（双读）**：Reader 同时支持 V1/V2；Writer 仍默认写 V1。
2. **阶段B（双写可选）**：新增 `--format v2`，灰度验证。
3. **阶段C（默认V2）**：V2 默认写入，V1 仅保留读取。

兼容判定：

- 根据 `Version` 分派解析路径。
- V1 不强行映射成插件；通过适配层对外提供统一视图。

---

## 9. 测试计划

新增测试集：

1. `pack_compare_source`：V2 pack 后 compare 通过。
2. `pack_unpack_compare_source`：V2 roundtrip。
3. `pack_without_namemap`：验证 `unpack` 降级为 `id_<n>.bin`。
4. `pack_with_hash_plugin`：校验 compare 优先走插件摘要路径。
5. `unknown_plugin_skip`：插入未知插件，reader 可跳过。

---

## 10. 风险与决策点

1. **id 分配稳定性**：是否按路径排序保证 deterministic build（建议是）。
2. **hash 算法默认值**：FNV1A64 速度快但抗碰撞弱；建议后续支持 SHA-256。
3. **大文件支持**：V2 已建议 `u64` offset/size，避免 V1 的 32-bit 上限。
4. **插件可信性**：HashMap 仅是“包内声明”，可被篡改；如需安全性需签名插件。

---

## 11. 本次文档对应的首批落地范围（建议）

M1（最小可用）：

- 完成 V2 Core + NameMap 插件。
- CLI `pack/unpack/list/compare` 跑通 V2。
- 保留 V1 读取。

M2：

- 增加 HashMap 插件落盘与读取优先路径。

M3：

- 增加 CompressionMap 插件与压缩实现。
