# jvalue

A single-header-style C++11 value container with serialization to **JSON** and Apple **Property List** (both XML `plist` and binary `bplist`). One in-memory representation (`jvalue`), four readers/writers, zero external dependencies.

## Features

- Dynamically-typed value: `null` / `bool` / `int64` / `double` / `string` / `array` / `object` / `date` / `data`
- JSON read / compact write / pretty-print (`style_write`)
- Apple plist read/write — XML and binary `bplist` share the same `jvalue` tree
- Auto-vivifying `operator[]` for ergonomic construction
- Move semantics (`jvalue&&`) — zero-cost ownership transfer
- Custom `flat_map` object container — insertion-order preserving, adaptive hash indexing
- Built-in Base64 helper (`jbase64`) for binary data fields
- C++11, standard library only, builds with `g++` / `clang++` / MSVC

## Build

```sh
make           # produces ./jvalue (demo binary)
make test      # builds and links test suite
make benchmark # builds and links performance benchmark
make clean
```

Or drop `src/json.{h,cpp}` and `src/base64.{h,cpp}` into your own project — no other files are required.

Requirements: C++11 compiler. The Makefile uses `g++ -Wall -O2 -std=c++11`.

## Quick start

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

Output (keys in insertion order):

```json
{
    "happy": true,
    "name": "Niels",
    "answer": {
        "everything": 66
    }
}
```

## Usage

### Reading JSON

```cpp
jvalue jv;
string err;
if (jv.read(text, &err)) {
    int    n    = jv["count"];            // implicit conversion
    string name = jv["user"]["name"];
    for (size_t i = 0; i < jv["items"].size(); i++) {
        const jvalue& item = jv["items"].at(i);
        // ...
    }
}
```

### Writing JSON

```cpp
string compact = jv.write();         // minified
string pretty  = jv.style_write();   // indented
jv.write_to_file("out.json");
jv.style_write_to_file("pretty.json");
```

### Building values

```cpp
jvalue arr(jvalue::E_ARRAY);
arr.push_back(1);
arr.push_back("two");
arr.push_back(3.14);

jvalue obj;
obj["list"] = arr;
obj["flag"] = true;
```

Indexing into a non-container promotes it: `jv["a"]["b"] = 1` will turn `jv` and `jv["a"]` into objects automatically.

### Move semantics

```cpp
jvalue a;
a["data"] = build_large_tree();

jvalue b = std::move(a);   // zero-cost, a becomes null
```

### Apple Property Lists

```cpp
jvalue pv;
pv.read_plist_from_file("Info.plist");   // auto-detects XML vs. binary

string xml;    pv.write_plist(xml);        // XML plist
string bin;    pv.write_bplist(bin);       // binary bplist
pv.style_write_plist_to_file("Info.plist");
pv.write_bplist_to_file("Info.bplist");
```

### Binary data & dates

```cpp
jv["icon"].assign_data(bytes, size);       // stored as raw bytes, emitted as base64
string raw = jv["icon"].as_data();

jv["created"].assign_date(time(NULL));
time_t t = jv["created"].as_date();
```

In JSON output, `data` and `date` are serialized as strings (base64 / ISO-ish). In plist output they use the native `<data>` and `<date>` tags.

## API overview

| Area              | Class / methods                                                                 |
|-------------------|---------------------------------------------------------------------------------|
| Value container   | `jvalue` — typed accessors `as_int` / `as_bool` / `as_string` / `as_data` / ... |
| JSON I/O          | `jvalue::read`, `write`, `style_write`, `*_to_file`                             |
| Plist I/O (XML)   | `jvalue::read_plist`, `write_plist`, `style_write_plist`                        |
| Plist I/O (bin)   | `jvalue::read_plist` (auto-detect), `write_bplist`, `write_bplist_to_file`      |
| Base64            | `jbase64::encode` / `decode`, or use `jvalue::assign_data` / `as_data`          |

See `src/json.h` for the full surface.

## Performance

Benchmarked on Apple M-series, `g++ -O2 -std=c++11`, 100 iterations. Comparison between the original C++98 `std::map`-based implementation and the current C++11 optimized version.

### JSON parse

| Test case | Before | After | Speedup |
|-----------|--------|-------|---------|
| Nested object (200 users, 155 KB) | 313.0 ms | 114.2 ms | **2.7x** |
| Array (2000 items, 314 KB) | 723.1 ms | 337.2 ms | **2.1x** |

### JSON write

| Test case | Before | After | Speedup |
|-----------|--------|-------|---------|
| Nested object compact | 165.2 ms | 150.0 ms | **1.1x** |
| Nested object pretty | 195.6 ms | 184.6 ms | **1.1x** |
| Array compact | 360.2 ms | 339.1 ms | **1.1x** |

### Plist (XML)

| Test case | Before | After | Speedup |
|-----------|--------|-------|---------|
| XML parse (100 devices) | 87.7 ms | 63.2 ms | **1.4x** |
| XML parse (500 devices) | 433.9 ms | 303.5 ms | **1.4x** |

### Plist (binary)

| Test case | Before | After | Speedup |
|-----------|--------|-------|---------|
| Binary parse (100 devices, certs + dates) | 47.6 ms | 22.1 ms | **2.2x** |
| Binary parse (500 devices) | 226.0 ms | 115.0 ms | **2.0x** |
| Binary write (100 devices) | 98.6 ms | 75.2 ms | **1.3x** |

### Object operations

| Test case | Before | After | Speedup |
|-----------|--------|-------|---------|
| Deep copy (200 users) | 66.5 ms | 35.9 ms | **1.9x** |
| Move (200 users) | 136.0 ms | 36.8 ms | **3.7x** |
| Key access (1000 keys) | 71.2 ms | 15.7 ms | **4.5x** |
| push_back (5000 ints) | 59.9 ms | 33.0 ms | **1.8x** |
| push_back (1000 objects) | 344.8 ms | 171.0 ms | **2.0x** |

### Memory (heap RSS)

| Test case | Before | After | Saved |
|-----------|--------|-------|-------|
| 500 users nested | 592 KB (6.1x JSON) | 360 KB (3.7x JSON) | **39%** |
| 2000 users nested | 1632 KB (4.2x JSON) | 1320 KB (3.4x JSON) | **19%** |

### What changed

- **Move semantics** — `jvalue(jvalue&&)` and `operator=(jvalue&&)` transfer ownership in O(1), eliminating deep copies for temporaries and returns
- **`flat_map`** — custom ordered map replaces `std::map`. Dense `entry[]` array preserves insertion order; open-addressing `int32_t[]` hash table activates automatically at 32+ keys. No per-node heap allocation, cache-friendly iteration
- **Bulk string copy** — JSON parser scans for the next escape character and copies chunks via `append()` instead of character-by-character `+=`
- **Lookup-table whitespace skip** — `_skip_spaces()` uses a 256-byte table instead of chained `if` comparisons
- **Writer `reserve()`** — pre-allocates output buffer to reduce `std::string` reallocations
- **Direct `push_back`** — `jvalue::push_back` calls `vector::push_back` directly instead of routing through `operator[]`
- **Simplified `_free()`** — removes redundant scalar zeroing and unnecessary NULL checks (C++ `delete` on nullptr is a no-op)

## Repository layout

```
src/
  json.h / json.cpp     # jvalue + flat_map + jreader/jwriter + jpreader/jpwriter
  base64.h / base64.cpp # jbase64
  main.cpp              # usage demo
  test.cpp              # 59-case test suite
  benchmark.cpp         # performance benchmark
Makefile
```

## License

Released under the [MIT License](LICENSE).
