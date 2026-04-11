# jvalue

A single-header-style C++11 value container with serialization to **JSON** and Apple **Property List** (both XML `plist` and binary `bplist`). One in-memory representation (`jvalue`), four readers/writers, zero external dependencies.

## Features

- Dynamically-typed value: `null` / `bool` / `int64` / `double` / `string` / `array` / `object` / `date` / `data`
- JSON read / compact write / pretty-print (`style_write`)
- Apple plist read/write — XML and binary `bplist` share the same `jvalue` tree
- Auto-vivifying `operator[]` for ergonomic construction
- Built-in Base64 helper (`jbase64`) for binary data fields
- Pure C++11, standard library only, builds with `g++` / `clang++` / MSVC

## Build

```sh
make           # produces ./jvalue (demo binary)
make clean
```

Or drop `src/json.{h,cpp}` and `src/base64.{h,cpp}` into your own project — no other files are required.

Requirements: a C++11 compiler. The Makefile uses `g++ -Wall -O2 -std=c++11`.

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

Output:

```json
{
    "answer" : {
        "everything" : 66
    },
    "happy" : true,
    "name" : "Niels"
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
| Value container   | `jvalue` — typed accessors `as_int` / `as_bool` / `as_string` / `as_data` / … |
| JSON I/O          | `jvalue::read`, `write`, `style_write`, `*_to_file`                             |
| Plist I/O (XML)   | `jvalue::read_plist`, `write_plist`, `style_write_plist`                        |
| Plist I/O (bin)   | `jvalue::read_plist` (auto-detect), `write_bplist`, `write_bplist_to_file`      |
| Base64            | `jbase64::encode` / `decode`, or use `jvalue::assign_data` / `as_data`          |

See `src/json.h` for the full surface.

## Repository layout

```
src/
  json.h / json.cpp     # jvalue + jreader/jwriter + jpreader/jpwriter
  base64.h / base64.cpp # jbase64
  main.cpp              # 14-line usage demo
Makefile
```

## License

Released under the [MIT License](LICENSE).
