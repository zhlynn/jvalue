# jvalue — 高性能 C++ JSON 与 Apple Plist（bplist）库

[English](README.md) | [中文](README.zh-CN.md)

> **高性能 C++11 JSON 解析/序列化库，原生支持 Apple Property List（XML plist + 二进制 bplist）。** 源码直接拖入项目即可使用，零依赖，MIT 许可证。

**关键词：** C++ JSON 库 · C++ plist 解析器 · bplist 读取 · 二进制 plist · Apple 属性列表 · C++11 JSON 序列化 · 轻量级 JSON · 移动语义 · 单文件 JSON 库。

一个单库式 C++11 值容器，支持 **JSON** 与 Apple **Property List**（XML `plist` 与二进制 `bplist`）的读写。统一的内存表示（`jvalue`）、四个读写器、零外部依赖。

## 为什么选择 jvalue？

- 📦 **即插即用**：只需将 4 个文件（`json.{h,cpp}`、`base64.{h,cpp}`）拷入任意 C++11 项目
- 🍎 **完整的 Apple plist 支持**：XML `.plist` 与二进制 `.bplist` 读写，自动识别格式
- 🧩 **统一数据模型**：JSON 与 plist 共用 `jvalue` 树，可以解析 JSON 后直接保存为 bplist，反之亦然
- 🪶 **零依赖**，标准 C++11，可使用 GCC / Clang / MSVC 构建
- 🔒 **边界检查**的 bplist 解析器，递归深度保护 `MAX_DEPTH = 256`

## 功能特性

- 动态类型值：`null` / `bool` / `int64` / `double` / `string` / `array` / `object` / `date` / `data`
- JSON 读取 / 紧凑写入 / 美化输出（`style_write`）
- Apple plist 读写 —— XML 与二进制 `bplist` 共享同一棵 `jvalue` 树
- 自动生成的 `operator[]` —— 构造对象/数组更顺手
- 移动语义（`jvalue&&`）—— 零成本所有权转移
- 内置 Base64 工具（`jbase64`），方便处理二进制字段
- C++11，仅依赖标准库，支持 `g++` / `clang++` / MSVC

## 编译

```sh
make           # 生成 ./jvalue 演示程序
make test      # 编译测试套件
make clean
```

或者直接将 `src/json.{h,cpp}` 和 `src/base64.{h,cpp}` 拷入你自己的项目 —— 不需要其他文件。

编译要求：C++11 编译器。Makefile 使用 `g++ -Wall -O2 -std=c++11`。

## 快速上手

```cpp
#include "json.h"

int main() {
    jvalue jv;
    jv["happy"] = true;
    jv["name"]  = "Niels";
    jv["answer"]["everything"] = 66;

    printf("%s\n", jv.style_write().c_str());
    return 0;
}
```

输出（键按插入顺序）：

```json
{
    "happy": true,
    "name": "Niels",
    "answer": {
        "everything": 66
    }
}
```

## 使用

### 读取 JSON

```cpp
jvalue jv;
string err;
if (jv.read(text, &err)) {
    int    n    = jv["count"];            // 隐式类型转换
    string name = jv["user"]["name"];
    for (size_t i = 0; i < jv["items"].size(); i++) {
        const jvalue& item = jv["items"].at(i);
        // ...
    }
}
```

### 写入 JSON

```cpp
string compact = jv.write();         // 紧凑
string pretty  = jv.style_write();   // 缩进美化
jv.write_to_file("out.json");
jv.style_write_to_file("pretty.json");
```

### 构造值

```cpp
jvalue arr(jvalue::E_ARRAY);
arr.push_back(1);
arr.push_back("two");
arr.push_back(3.14);

jvalue obj;
obj["list"] = arr;
obj["flag"] = true;
```

对非容器值做下标操作会自动把它提升为对象：`jv["a"]["b"] = 1` 会将 `jv` 与 `jv["a"]` 自动变成 object。

### 移动语义

```cpp
jvalue a;
a["data"] = build_large_tree();

jvalue b = std::move(a);   // 零成本转移，a 变为 null
```

### Apple Property List

```cpp
jvalue pv;
pv.read_plist_from_file("Info.plist");   // 自动识别 XML / 二进制

string xml;    pv.write_plist(xml);        // XML plist
string bin;    pv.write_bplist(bin);       // 二进制 bplist
pv.style_write_plist_to_file("Info.plist");
pv.write_bplist_to_file("Info.bplist");
```

### 二进制数据与日期

```cpp
jv["icon"].assign_data(bytes, size);       // 存储原始字节，序列化时输出 base64
string raw = jv["icon"].as_data();

jv["created"].assign_date(time(NULL));
time_t t = jv["created"].as_date();
```

在 JSON 输出中，`data` 与 `date` 会被序列化为字符串（base64 / 类 ISO 时间）；而在 plist 中使用原生的 `<data>` 与 `<date>` 标签。

## API 概览

| 领域              | 类 / 方法                                                                        |
|-------------------|----------------------------------------------------------------------------------|
| 值容器            | `jvalue` —— 类型化访问器 `as_int` / `as_bool` / `as_string` / `as_data` / ...    |
| JSON I/O          | `jvalue::read`、`write`、`style_write`、`*_to_file`                              |
| Plist I/O（XML）  | `jvalue::read_plist`、`write_plist`、`style_write_plist`                         |
| Plist I/O（二进制）| `jvalue::read_plist`（自动识别）、`write_bplist`、`write_bplist_to_file`        |
| Base64            | `jbase64::encode` / `decode`，或使用 `jvalue::assign_data` / `as_data`           |

完整接口请参阅 `src/json.h`。


## 仓库结构

```
src/
  json.h / json.cpp     # jvalue + flat_map + jreader/jwriter + jpreader/jpwriter
  base64.h / base64.cpp # jbase64
  main.cpp              # 使用示例
  test.cpp              # 59 个测试用例
Makefile
```

## 许可证

基于 [MIT License](LICENSE) 发布。
