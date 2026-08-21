#include "json.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <cmath>

using namespace std;

static int tests_run = 0;
static int tests_passed = 0;

#define TEST(name) \
    do { \
        tests_run++; \
        printf("  [TEST] %-50s", name); \
    } while(0)

#define PASS() \
    do { \
        tests_passed++; \
        printf("PASS\n"); \
    } while(0)

#define ASSERT(cond) \
    do { \
        if (!(cond)) { \
            printf("FAIL (line %d: %s)\n", __LINE__, #cond); \
            return; \
        } \
    } while(0)

#define ASSERT_EQ(a, b) \
    do { \
        if ((a) != (b)) { \
            printf("FAIL (line %d: %s != %s)\n", __LINE__, #a, #b); \
            return; \
        } \
    } while(0)

// === Basic Construction Tests ===
static void test_null_construction() {
    TEST("null construction");
    jvalue jv;
    ASSERT(jv.is_null());
    ASSERT(jv.is_empty());
    ASSERT_EQ(jv.type(), jvalue::E_NULL);
    PASS();
}

static void test_int_construction() {
    TEST("int construction");
    jvalue jv(42);
    ASSERT(jv.is_int());
    ASSERT_EQ(jv.as_int(), 42);
    ASSERT_EQ(jv.as_int64(), 42);
    PASS();
}

static void test_int64_construction() {
    TEST("int64 construction");
    jvalue jv((int64_t)1234567890123LL);
    ASSERT(jv.is_int());
    ASSERT_EQ(jv.as_int64(), 1234567890123LL);
    PASS();
}

static void test_bool_construction() {
    TEST("bool construction");
    jvalue t(true), f(false);
    ASSERT(t.is_bool());
    ASSERT(t.as_bool());
    ASSERT(!f.as_bool());
    PASS();
}

static void test_double_construction() {
    TEST("double construction");
    jvalue jv(3.14);
    ASSERT(jv.is_double());
    ASSERT(fabs(jv.as_double() - 3.14) < 1e-10);
    PASS();
}

static void test_string_construction() {
    TEST("string construction (const char*)");
    jvalue jv("hello");
    ASSERT(jv.is_string());
    ASSERT_EQ(string(jv.as_cstr()), string("hello"));
    PASS();
}

static void test_string_construction_std() {
    TEST("string construction (std::string)");
    string s = "world";
    jvalue jv(s);
    ASSERT(jv.is_string());
    ASSERT_EQ(string(jv.as_cstr()), string("world"));
    PASS();
}

static void test_data_construction() {
    TEST("data construction");
    const char raw[] = "\x00\x01\x02\x03";
    jvalue jv(raw, 4);
    ASSERT(jv.is_data());
    ASSERT_EQ(jv.size(), (size_t)4);
    PASS();
}

// === Copy Tests ===
static void test_copy_construction() {
    TEST("copy construction");
    jvalue orig;
    orig["name"] = "test";
    orig["value"] = 42;
    orig["nested"]["inner"] = true;

    jvalue copy(orig);
    ASSERT(copy.is_object());
    ASSERT_EQ(string(copy["name"].as_cstr()), string("test"));
    ASSERT_EQ(copy["value"].as_int(), 42);
    ASSERT(copy["nested"]["inner"].as_bool());

    orig["name"] = "modified";
    ASSERT_EQ(string(copy["name"].as_cstr()), string("test"));
    PASS();
}

static void test_copy_assignment() {
    TEST("copy assignment");
    jvalue a, b;
    a["x"] = 1;
    b = a;
    ASSERT_EQ(b["x"].as_int(), 1);
    a["x"] = 2;
    ASSERT_EQ(b["x"].as_int(), 1);
    PASS();
}

// === Move Tests ===
static void test_move_construction() {
    TEST("move construction");
    jvalue orig;
    orig["name"] = "test";
    orig["arr"][0] = 1;
    orig["arr"][1] = 2;

    jvalue moved(std::move(orig));
    ASSERT(moved.is_object());
    ASSERT_EQ(string(moved["name"].as_cstr()), string("test"));
    ASSERT_EQ(moved["arr"][0].as_int(), 1);
    ASSERT(orig.is_null());
    PASS();
}

static void test_move_assignment() {
    TEST("move assignment");
    jvalue a, b;
    a["key"] = "value";
    b = std::move(a);
    ASSERT(b.is_object());
    ASSERT_EQ(string(b["key"].as_cstr()), string("value"));
    ASSERT(a.is_null());
    PASS();
}

static void test_move_string() {
    TEST("move string value");
    jvalue a("hello world this is a long string for testing");
    jvalue b(std::move(a));
    ASSERT(b.is_string());
    ASSERT_EQ(string(b.as_cstr()), string("hello world this is a long string for testing"));
    ASSERT(a.is_null());
    PASS();
}

static void test_move_array() {
    TEST("move array");
    jvalue a;
    for (int i = 0; i < 100; i++) a.push_back(i);
    jvalue b(std::move(a));
    ASSERT(b.is_array());
    ASSERT_EQ(b.size(), (size_t)100);
    ASSERT_EQ(b[50].as_int(), 50);
    ASSERT(a.is_null());
    PASS();
}

// === JSON Parse Tests ===
static void test_parse_simple_object() {
    TEST("parse simple object");
    jvalue jv;
    bool ok = jv.read("{\"name\":\"Alice\",\"age\":30,\"active\":true}");
    ASSERT(ok);
    ASSERT(jv.is_object());
    ASSERT_EQ(string(jv["name"].as_cstr()), string("Alice"));
    ASSERT_EQ(jv["age"].as_int(), 30);
    ASSERT(jv["active"].as_bool());
    PASS();
}

static void test_parse_nested_object() {
    TEST("parse nested object");
    jvalue jv;
    bool ok = jv.read("{\"a\":{\"b\":{\"c\":42}}}");
    ASSERT(ok);
    ASSERT_EQ(jv["a"]["b"]["c"].as_int(), 42);
    PASS();
}

static void test_parse_array() {
    TEST("parse array");
    jvalue jv;
    bool ok = jv.read("[1,2,3,4,5]");
    ASSERT(ok);
    ASSERT(jv.is_array());
    ASSERT_EQ(jv.size(), (size_t)5);
    ASSERT_EQ(jv[0].as_int(), 1);
    ASSERT_EQ(jv[4].as_int(), 5);
    PASS();
}

static void test_parse_mixed_array() {
    TEST("parse mixed array");
    jvalue jv;
    bool ok = jv.read("[1,\"two\",3.0,true,null]");
    ASSERT(ok);
    ASSERT_EQ(jv[0].as_int(), 1);
    ASSERT_EQ(string(jv[1].as_cstr()), string("two"));
    ASSERT(fabs(jv[2].as_double() - 3.0) < 1e-10);
    ASSERT(jv[3].as_bool());
    ASSERT(jv[4].is_null());
    PASS();
}

static void test_parse_unicode_escape() {
    TEST("parse unicode escape");
    jvalue jv;
    bool ok = jv.read("[\"\\u0048\\u0065\\u006C\\u006C\\u006F\"]");
    ASSERT(ok);
    ASSERT_EQ(string(jv[0].as_cstr()), string("Hello"));
    PASS();
}

static void test_parse_surrogate_pair() {
    TEST("parse surrogate pair");
    jvalue jv;
    bool ok = jv.read("[\"\\uD83D\\uDE00\"]");
    ASSERT(ok);
    const char* s = jv[0].as_cstr();
    ASSERT_EQ((uint8_t)s[0], 0xF0);
    ASSERT_EQ((uint8_t)s[1], 0x9F);
    ASSERT_EQ((uint8_t)s[2], 0x98);
    ASSERT_EQ((uint8_t)s[3], 0x80);
    PASS();
}

static void test_parse_escape_sequences() {
    TEST("parse escape sequences");
    jvalue jv;
    bool ok = jv.read("[\"line1\\nline2\\ttab\\\\backslash\"]");
    ASSERT(ok);
    ASSERT_EQ(string(jv[0].as_cstr()), string("line1\nline2\ttab\\backslash"));
    PASS();
}

static void test_parse_negative_number() {
    TEST("parse negative number");
    jvalue jv;
    jv.read("-42");
    ASSERT_EQ(jv.as_int(), -42);
    PASS();
}

static void test_parse_float_number() {
    TEST("parse float number");
    jvalue jv;
    jv.read("3.14e2");
    ASSERT(fabs(jv.as_double() - 314.0) < 1e-10);
    PASS();
}

static void test_parse_large_int() {
    TEST("parse large int64");
    jvalue jv;
    jv.read("9223372036854775807");
    ASSERT_EQ(jv.as_int64(), 9223372036854775807LL);
    PASS();
}

static void test_parse_empty_object() {
    TEST("parse empty object");
    jvalue jv;
    bool ok = jv.read("{}");
    ASSERT(ok);
    ASSERT(jv.is_object());
    ASSERT_EQ(jv.size(), (size_t)0);
    PASS();
}

static void test_parse_empty_array() {
    TEST("parse empty array");
    jvalue jv;
    bool ok = jv.read("[]");
    ASSERT(ok);
    ASSERT(jv.is_array());
    ASSERT_EQ(jv.size(), (size_t)0);
    PASS();
}

static void test_parse_error() {
    TEST("parse error handling");
    jvalue jv;
    string err;
    bool ok = jv.read("{invalid}", &err);
    ASSERT(!ok);
    ASSERT(!err.empty());
    PASS();
}

// === JSON Write Tests ===
static void test_write_roundtrip() {
    TEST("write roundtrip");
    jvalue orig;
    orig["name"] = "test";
    orig["value"] = 42;
    orig["pi"] = 3.14;
    orig["flag"] = true;
    orig["nothing"] = jvalue();

    string json = orig.write();
    jvalue parsed;
    parsed.read(json);

    ASSERT_EQ(string(parsed["name"].as_cstr()), string("test"));
    ASSERT_EQ(parsed["value"].as_int(), 42);
    ASSERT(fabs(parsed["pi"].as_double() - 3.14) < 1e-10);
    ASSERT(parsed["flag"].as_bool());
    ASSERT(parsed["nothing"].is_null());
    PASS();
}

static void test_style_write_roundtrip() {
    TEST("style_write roundtrip");
    jvalue orig;
    orig["arr"][0] = 1;
    orig["arr"][1] = "two";
    orig["nested"]["x"] = true;

    string json = orig.style_write();
    jvalue parsed;
    parsed.read(json);

    ASSERT_EQ(parsed["arr"][0].as_int(), 1);
    ASSERT_EQ(string(parsed["arr"][1].as_cstr()), string("two"));
    ASSERT(parsed["nested"]["x"].as_bool());
    PASS();
}

static void test_write_special_chars() {
    TEST("write special characters");
    jvalue jv;
    jv["text"] = "line1\nline2\ttab\"quote\\backslash";
    string json = jv.write();
    jvalue parsed;
    parsed.read(json);
    ASSERT_EQ(string(parsed["text"].as_cstr()), string("line1\nline2\ttab\"quote\\backslash"));
    PASS();
}

// === Object Operations ===
static void test_object_access() {
    TEST("object access (operator[])");
    jvalue jv;
    jv["a"] = 1;
    jv["b"] = "hello";
    jv["c"] = 3.14;
    ASSERT_EQ(jv["a"].as_int(), 1);
    ASSERT_EQ(string(jv["b"].as_cstr()), string("hello"));
    ASSERT(jv.has("a"));
    ASSERT(!jv.has("z"));
    PASS();
}

static void test_object_const_access() {
    TEST("object const access (miss returns null)");
    jvalue jv;
    jv["x"] = 1;
    const jvalue& cjv = jv;
    ASSERT_EQ(cjv["x"].as_int(), 1);
    ASSERT(cjv["missing"].is_null());
    PASS();
}

static void test_object_erase() {
    TEST("object erase");
    jvalue jv;
    jv["a"] = 1;
    jv["b"] = 2;
    ASSERT(jv.erase("a"));
    ASSERT(!jv.has("a"));
    ASSERT(jv.has("b"));
    PASS();
}

static void test_object_get_keys() {
    TEST("object get_keys");
    jvalue jv;
    jv["c"] = 3;
    jv["a"] = 1;
    jv["b"] = 2;
    vector<string> keys;
    jv.get_keys(keys);
    ASSERT_EQ(keys.size(), (size_t)3);
    PASS();
}

static void test_object_append() {
    TEST("object append");
    jvalue a, b;
    a["x"] = 1;
    b["y"] = 2;
    ASSERT(a.append(b));
    ASSERT(a.has("x"));
    ASSERT(a.has("y"));
    PASS();
}

// === Array Operations ===
static void test_array_push_back() {
    TEST("array push_back");
    jvalue arr;
    arr.push_back(1);
    arr.push_back("two");
    arr.push_back(3.0);
    arr.push_back(true);
    ASSERT(arr.is_array());
    ASSERT_EQ(arr.size(), (size_t)4);
    ASSERT_EQ(arr[0].as_int(), 1);
    ASSERT_EQ(string(arr[1].as_cstr()), string("two"));
    PASS();
}

static void test_array_erase() {
    TEST("array erase");
    jvalue arr;
    arr.push_back(1);
    arr.push_back(2);
    arr.push_back(3);
    ASSERT(arr.erase(1));
    ASSERT_EQ(arr.size(), (size_t)2);
    ASSERT_EQ(arr[0].as_int(), 1);
    ASSERT_EQ(arr[1].as_int(), 3);
    PASS();
}

static void test_array_front_back() {
    TEST("array front/back");
    jvalue arr;
    arr.push_back(10);
    arr.push_back(20);
    arr.push_back(30);
    ASSERT_EQ(arr.front().as_int(), 10);
    ASSERT_EQ(arr.back().as_int(), 30);
    PASS();
}

static void test_array_index() {
    TEST("array index (string search)");
    jvalue arr;
    arr.push_back("apple");
    arr.push_back("banana");
    arr.push_back("cherry");
    ASSERT_EQ(arr.index("banana"), 1);
    ASSERT_EQ(arr.index("missing"), -1);
    PASS();
}

static void test_array_append() {
    TEST("array append");
    jvalue a, b;
    a.push_back(1);
    a.push_back(2);
    b.push_back(3);
    b.push_back(4);
    ASSERT(a.append(b));
    ASSERT_EQ(a.size(), (size_t)4);
    ASSERT_EQ(a[2].as_int(), 3);
    PASS();
}

// === Type Conversion Tests ===
static void test_type_conversions() {
    TEST("type conversions");
    jvalue iv(42);
    ASSERT_EQ((int)iv, 42);
    ASSERT_EQ((int64_t)iv, 42);
    ASSERT(fabs((double)iv - 42.0) < 1e-10);
    ASSERT_EQ((bool)iv, true);
    ASSERT_EQ(string((const char*)iv), string(""));

    jvalue sv("123");
    ASSERT_EQ(sv.as_int(), 123);
    ASSERT(fabs(sv.as_double() - 123.0) < 1e-10);
    PASS();
}

static void test_as_string() {
    TEST("as_string for all types");
    ASSERT_EQ(jvalue(42).as_string(), string("42"));
    ASSERT_EQ(jvalue(true).as_string(), string("true"));
    ASSERT_EQ(jvalue(false).as_string(), string("false"));
    ASSERT_EQ(jvalue("hello").as_string(), string("hello"));
    ASSERT_EQ(jvalue().as_string(), string(""));
    PASS();
}

// === Comparison Operators ===
static void test_comparison_operators() {
    TEST("comparison operators");
    jvalue jv("hello");
    ASSERT(jv == "hello");
    ASSERT("hello" == jv);
    ASSERT(jv != "world");
    ASSERT("world" != jv);

    jvalue null_val;
    ASSERT(null_val == (const char*)NULL);
    ASSERT((const char*)NULL == null_val);

    string s = "hello";
    ASSERT(jv == s);
    ASSERT(s == jv);
    ASSERT(jv != string("world"));
    PASS();
}

// === is_empty Tests ===
static void test_is_empty() {
    TEST("is_empty for all types");
    ASSERT(jvalue().is_empty());
    ASSERT(jvalue(0).is_empty());
    ASSERT(!jvalue(1).is_empty());
    ASSERT(jvalue(false).is_empty());
    ASSERT(!jvalue(true).is_empty());
    ASSERT(jvalue(0.0).is_empty());
    ASSERT(!jvalue(1.0).is_empty());
    ASSERT(jvalue("").is_empty());
    ASSERT(!jvalue("x").is_empty());

    jvalue empty_arr(jvalue::E_ARRAY);
    ASSERT(empty_arr.is_empty());
    jvalue empty_obj(jvalue::E_OBJECT);
    ASSERT(empty_obj.is_empty());
    PASS();
}

// === Auto-vivification Tests ===
static void test_auto_vivification() {
    TEST("auto-vivification");
    jvalue jv;
    jv["a"]["b"]["c"] = 42;
    ASSERT(jv.is_object());
    ASSERT(jv["a"].is_object());
    ASSERT(jv["a"]["b"].is_object());
    ASSERT_EQ(jv["a"]["b"]["c"].as_int(), 42);
    PASS();
}

static void test_array_auto_vivification() {
    TEST("array auto-vivification");
    jvalue jv;
    jv[0] = "first";
    jv[5] = "sixth";
    ASSERT(jv.is_array());
    ASSERT_EQ(jv.size(), (size_t)6);
    ASSERT_EQ(string(jv[0].as_cstr()), string("first"));
    ASSERT(jv[3].is_null());
    PASS();
}

// === Plist Tests ===
static void test_plist_roundtrip() {
    TEST("plist XML roundtrip");
    jvalue orig;
    orig["name"] = "test";
    orig["value"] = 42;
    orig["pi"] = 3.14;
    orig["flag"] = true;
    orig["arr"][0] = 1;
    orig["arr"][1] = "two";

    string plist = orig.style_write_plist();
    jvalue parsed;
    bool ok = parsed.read_plist(plist);
    ASSERT(ok);
    ASSERT_EQ(string(parsed["name"].as_cstr()), string("test"));
    ASSERT_EQ(parsed["value"].as_int(), 42);
    ASSERT(parsed["flag"].as_bool());
    ASSERT_EQ(parsed["arr"][0].as_int(), 1);
    PASS();
}

static void test_bplist_roundtrip() {
    TEST("bplist binary roundtrip");
    jvalue orig;
    orig["name"] = "binary_test";
    orig["count"] = 99;
    orig["items"][0] = "a";
    orig["items"][1] = "b";

    string bplist;
    orig.write_bplist(bplist);

    jvalue parsed;
    bool is_binary = false;
    bool ok = parsed.read_plist(bplist.data(), bplist.size(), NULL, &is_binary);
    ASSERT(ok);
    ASSERT(is_binary);
    ASSERT_EQ(string(parsed["name"].as_cstr()), string("binary_test"));
    ASSERT_EQ(parsed["count"].as_int(), 99);
    ASSERT_EQ(parsed["items"].size(), (size_t)2);
    PASS();
}

// === Date/Data Tests ===
static void test_date_operations() {
    TEST("date operations");
    jvalue jv;
    time_t now = 1000000000;
    jv.assign_date(now);
    ASSERT(jv.is_date());
    ASSERT_EQ(jv.as_date(), now);
    PASS();
}

static void test_data_operations() {
    TEST("data operations");
    jvalue jv;
    const uint8_t raw[] = {0x00, 0x01, 0x02, 0xFF};
    jv.assign_data(raw, 4);
    ASSERT(jv.is_data());
    string data = jv.as_data();
    ASSERT_EQ(data.size(), (size_t)4);
    ASSERT_EQ((uint8_t)data[0], 0x00);
    ASSERT_EQ((uint8_t)data[3], 0xFF);
    PASS();
}

static void test_date_string() {
    TEST("date string detection");
    jvalue jv("date:2024-01-15T10:30:00Z");
    ASSERT(jv.is_date_string());
    time_t t = jv.as_date();
    ASSERT(t > 0);
    PASS();
}

static void test_data_string() {
    TEST("data string detection");
    jvalue jv("data:SGVsbG8=");
    ASSERT(jv.is_data_string());
    PASS();
}

// === Self-assignment test ===
static void test_self_assignment() {
    TEST("self assignment");
    jvalue jv;
    jv["key"] = "value";
    jv = jv;
    ASSERT(jv.is_object());
    ASSERT_EQ(string(jv["key"].as_cstr()), string("value"));
    PASS();
}

// === Clear test ===
static void test_clear() {
    TEST("clear");
    jvalue jv;
    jv["a"] = 1;
    jv.clear();
    ASSERT(jv.is_null());
    PASS();
}

// === Large data test ===
static void test_large_json_parse() {
    TEST("large JSON parse (1000 keys)");
    string json = "{";
    for (int i = 0; i < 1000; i++) {
        char kv[128];
        snprintf(kv, sizeof(kv), "%s\"key_%d\":%d", i > 0 ? "," : "", i, i);
        json += kv;
    }
    json += "}";
    jvalue jv;
    bool ok = jv.read(json);
    ASSERT(ok);
    ASSERT_EQ(jv.size(), (size_t)1000);
    ASSERT_EQ(jv["key_0"].as_int(), 0);
    ASSERT_EQ(jv["key_999"].as_int(), 999);
    PASS();
}

// === Deeply nested test ===
static void test_deep_nesting() {
    TEST("deep nesting (50 levels)");
    string json;
    for (int i = 0; i < 50; i++) json += "{\"d\":";
    json += "42";
    for (int i = 0; i < 50; i++) json += "}";
    jvalue jv;
    bool ok = jv.read(json);
    ASSERT(ok);
    const jvalue* cur = &jv;
    for (int i = 0; i < 50; i++) cur = &((*cur)["d"]);
    ASSERT_EQ(cur->as_int(), 42);
    PASS();
}

// === String edge cases ===
static void test_empty_string() {
    TEST("empty string");
    jvalue jv("");
    ASSERT(jv.is_string());
    ASSERT_EQ(string(jv.as_cstr()), string(""));
    ASSERT(jv.is_empty());
    PASS();
}

static void test_has_string_key() {
    TEST("has with string key");
    jvalue jv;
    jv["hello"] = 1;
    ASSERT(jv.has(string("hello")));
    ASSERT(!jv.has(string("world")));
    PASS();
}

// === Full roundtrip all types ===
static void test_full_roundtrip_all_types() {
    TEST("full roundtrip all types");
    jvalue orig;
    orig["int_val"] = 42;
    orig["int64_val"] = (int64_t)9876543210LL;
    orig["bool_val"] = true;
    orig["double_val"] = 2.718281828;
    orig["string_val"] = "hello world";
    orig["null_val"] = jvalue();
    orig["array_val"][0] = 1;
    orig["array_val"][1] = "two";
    orig["nested"]["x"] = 99;

    string json = orig.write();
    jvalue parsed;
    parsed.read(json);

    ASSERT_EQ(parsed["int_val"].as_int(), 42);
    ASSERT_EQ(parsed["int64_val"].as_int64(), 9876543210LL);
    ASSERT(parsed["bool_val"].as_bool());
    ASSERT(fabs(parsed["double_val"].as_double() - 2.718281828) < 1e-4);
    ASSERT_EQ(string(parsed["string_val"].as_cstr()), string("hello world"));
    ASSERT(parsed["null_val"].is_null());
    ASSERT_EQ(parsed["array_val"].size(), (size_t)2);
    ASSERT_EQ(parsed["nested"]["x"].as_int(), 99);
    PASS();
}

int main(int argc, char* argv[]) {
    printf("=== jvalue Test Suite ===\n\n");

    printf("[Construction]\n");
    test_null_construction();
    test_int_construction();
    test_int64_construction();
    test_bool_construction();
    test_double_construction();
    test_string_construction();
    test_string_construction_std();
    test_data_construction();

    printf("\n[Copy/Move]\n");
    test_copy_construction();
    test_copy_assignment();
    test_move_construction();
    test_move_assignment();
    test_move_string();
    test_move_array();
    test_self_assignment();

    printf("\n[JSON Parse]\n");
    test_parse_simple_object();
    test_parse_nested_object();
    test_parse_array();
    test_parse_mixed_array();
    test_parse_unicode_escape();
    test_parse_surrogate_pair();
    test_parse_escape_sequences();
    test_parse_negative_number();
    test_parse_float_number();
    test_parse_large_int();
    test_parse_empty_object();
    test_parse_empty_array();
    test_parse_error();

    printf("\n[JSON Write]\n");
    test_write_roundtrip();
    test_style_write_roundtrip();
    test_write_special_chars();

    printf("\n[Object Operations]\n");
    test_object_access();
    test_object_const_access();
    test_object_erase();
    test_object_get_keys();
    test_object_append();
    test_auto_vivification();

    printf("\n[Array Operations]\n");
    test_array_push_back();
    test_array_erase();
    test_array_front_back();
    test_array_index();
    test_array_append();
    test_array_auto_vivification();

    printf("\n[Type Conversions]\n");
    test_type_conversions();
    test_as_string();
    test_is_empty();

    printf("\n[Comparison]\n");
    test_comparison_operators();

    printf("\n[Plist]\n");
    test_plist_roundtrip();
    test_bplist_roundtrip();

    printf("\n[Date/Data]\n");
    test_date_operations();
    test_data_operations();
    test_date_string();
    test_data_string();

    printf("\n[Edge Cases]\n");
    test_clear();
    test_empty_string();
    test_has_string_key();
    test_large_json_parse();
    test_deep_nesting();
    test_full_roundtrip_all_types();

    printf("\n=== Results: %d/%d tests passed ===\n", tests_passed, tests_run);
    return (tests_passed == tests_run) ? 0 : 1;
}
