# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build

- `make` — builds the `jvalue` demo binary into the repo root via `g++ -Wall -O2 -std=c++11`. Object files go into `.build/`.
- `make test` — builds the test suite binary (`./test`, 59 test cases).
- `make benchmark` — builds the performance benchmark binary (`./benchmark`).
- `make clean` — removes `.build/`, `jvalue`, `test`, and `benchmark` binaries.

## Architecture

This is a single-library, dependency-free C++11 project providing a dynamically-typed value container (`jvalue`) with serialization to/from JSON and Apple Property List formats (both XML plist and binary bplist). Despite the repo name, the library is not JSON-only — plist support is first-class and shares the same in-memory representation.

### Core type: `jvalue` (src/json.h, src/json.cpp)

`jvalue` is a tagged union wrapping one of: null, int64, bool, double, string, array, object, date (`time_t`), or data (raw bytes). Key points future edits must preserve:

- **Discriminant is `m_type` (enum `jtype`)**; payload is `union _hold m_value` holding either a scalar or a heap pointer (`p_string`, `p_array`, `p_object`, `p_data`). `_free()` + `_copy_value()` are the only places that know the type-to-slot mapping; extending the type enum requires updating both, plus the move constructor/assignment.
- **Move semantics**: `jvalue(jvalue&&)` and `operator=(jvalue&&)` transfer the union payload and reset the source to `E_NULL`. Zero-cost ownership transfer.
- **`E_DATE` and `E_DATA` are plist-only concepts** — JSON writers emit them as strings (ISO-ish date / base64). `is_date_string()` / `is_data_string()` heuristically detect these on read.
- **Implicit construction + `operator[]`** auto-vivifies objects/arrays (see `main.cpp` usage: `jv["answer"]["everything"] = 66`). Indexing a non-container mutates its type. Reads through `at()` return the static `jvalue::null` sentinel on miss.
- `json.h` declares `using namespace std;` at file scope — propagated to every translation unit that includes it. Do not add symbols that would collide.

### Object container: `jvalue::flat_map` (src/json.h, defined after jvalue)

The object type uses a custom `flat_map` instead of `std::map` or `std::unordered_map`. It is defined as a nested class of `jvalue` but placed after `jvalue`'s closing brace (to avoid incomplete-type issues).

- **Dense `entry[]` array** — stores `{string key, jvalue value}` pairs in insertion order via `malloc` + placement new. Preserves key insertion order in JSON/plist output.
- **Adaptive hash index** — when size reaches 32 keys, an `int32_t[]` open-addressing hash table (FNV-1a, linear probing) is allocated automatically. Below 32 keys, lookups use linear scan (cache-friendly for small objects).
- **Manual memory management** — uses `malloc`/`free` + placement `new` / explicit destructor calls. Copy/move constructors and assignment operators are fully implemented.

### Reader/writer classes

All four live in `json.h` and are implemented in the monolithic `src/json.cpp`. They are separate classes rather than methods on `jvalue`; `jvalue::read` / `write` / `style_write` / `*_plist` are thin facades over them.

- **`jreader`** — recursive-descent JSON parser. Tracks position via `m_pcursor` and enforces `MAX_DEPTH = 256`. Uses bulk `append()` for string decoding and lookup-table `_skip_spaces()`.
- **`jwriter`** — JSON serializer. Has a compact `write` path (with `reserve(4096)`) and a pretty-printing `style_write` path that uses `_is_multiline_array` to decide inline vs. indented array layout.
- **`jpreader`** — Apple plist reader. Handles **both** XML plist and binary bplist (`parse` auto-detects, `parse_binary` is the bplist-specific path). The `bplist_object_type` enum mirrors Apple's binary plist type markers (`NS_NUMBER_INT = 0x10`, `NS_DATE = 0x30`, etc.); keep those constants in sync with the bplist spec if touched. Uses `_bp_in_bounds` / `_get_uint_val_safe` for bounds-checked reads — all binary parsing must go through these helpers.
- **`jpwriter`** — plist writer, including binary bplist output. `_swap` overloads perform endian conversion (bplist is big-endian on disk); `_write_value_to_binary` builds a pool of `bplist_object`s first, then emits the offset table and trailer.

### Base64 (src/base64.{h,cpp})

`jbase64` provides encode/decode used by `jvalue::assign_data` / `as_data` and the plist data-tag path. It caches allocated buffers in `m_array_encodes` / `m_array_decodes` and frees them in the destructor — callers hold pointers only for the lifetime of the `jbase64` instance.

### Test suite and benchmark

- `src/test.cpp` — 59 test cases covering construction, copy/move, JSON parse/write, plist roundtrip, object/array operations, type conversions, edge cases. Run via `make test && ./test`.
- `src/benchmark.cpp` — large-scale performance benchmark (200-user nested JSON, 2000-element arrays, 100/500-device plists with binary data and dates). Run via `make benchmark && ./benchmark`.
