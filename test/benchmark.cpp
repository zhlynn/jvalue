#include "json.h"
#include <chrono>
#include <cstdio>
#include <cstring>

using namespace std;
using namespace std::chrono;

// Build a large, realistic jvalue tree programmatically
static jvalue build_large_object(int num_users) {
    jvalue root;
    root["version"] = "2.1.0";
    root["generated"] = "2026-04-12T00:00:00Z";
    root["total_count"] = (int64_t)num_users;
    root["metadata"]["server"] = "api-prod-03.example.com";
    root["metadata"]["region"] = "us-west-2";
    root["metadata"]["response_time_ms"] = 42.5;
    root["metadata"]["cache_hit"] = false;
    root["metadata"]["tags"][0] = "production";
    root["metadata"]["tags"][1] = "v2";
    root["metadata"]["tags"][2] = "authenticated";

    for (int i = 0; i < num_users; i++) {
        char idx[16];
        snprintf(idx, sizeof(idx), "%d", i);

        jvalue user;
        char buf[128];
        snprintf(buf, sizeof(buf), "user_%d", i);
        user["id"] = (int64_t)(100000 + i);
        user["username"] = buf;
        snprintf(buf, sizeof(buf), "user_%d@example.com", i);
        user["email"] = buf;
        user["active"] = (i % 3 != 0);
        user["score"] = (double)(i * 1.7 + 0.3);
        user["level"] = (i % 50) + 1;

        // nested profile
        snprintf(buf, sizeof(buf), "User Number %d", i);
        user["profile"]["display_name"] = buf;
        user["profile"]["bio"] = "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Sed do eiusmod tempor incididunt ut labore.";
        user["profile"]["avatar_url"] = "https://cdn.example.com/avatars/default_256x256.png";
        user["profile"]["settings"]["theme"] = (i % 2 == 0) ? "dark" : "light";
        user["profile"]["settings"]["notifications"] = true;
        user["profile"]["settings"]["language"] = "en-US";
        user["profile"]["settings"]["timezone"] = "America/Los_Angeles";

        // friends list (array of ints)
        int num_friends = (i % 10) + 1;
        for (int f = 0; f < num_friends; f++) {
            user["friends"].push_back((int64_t)(100000 + (i + f + 1) % num_users));
        }

        // recent activity (array of objects)
        for (int a = 0; a < 3; a++) {
            jvalue activity;
            activity["type"] = (a % 3 == 0) ? "login" : (a % 3 == 1) ? "purchase" : "comment";
            activity["timestamp"] = (int64_t)(1700000000 + i * 1000 + a * 100);
            snprintf(buf, sizeof(buf), "Activity %d for user %d", a, i);
            activity["description"] = buf;
            activity["amount"] = (a % 3 == 1) ? (double)(a * 9.99 + i * 0.01) : 0.0;
            user["recent_activity"].push_back(activity);
        }

        root["users"].push_back(user);
    }

    return root;
}

// Build a large flat array with mixed types
static jvalue build_large_array(int size) {
    jvalue arr;
    for (int i = 0; i < size; i++) {
        jvalue item;
        item["index"] = i;
        char buf[64];
        snprintf(buf, sizeof(buf), "item_%05d", i);
        item["name"] = buf;
        item["value"] = (double)(i * 3.14159);
        item["enabled"] = (i % 2 == 0);
        item["category"] = (i % 5 == 0) ? "special" : "normal";

        // Small nested array
        for (int t = 0; t < 5; t++) {
            snprintf(buf, sizeof(buf), "tag_%d_%d", i, t);
            item["tags"].push_back(buf);
        }

        arr.push_back(item);
    }
    return arr;
}

// Build a plist-like structure with date and data types
static jvalue build_plist_data(int num_entries) {
    jvalue root;
    root["CFBundleIdentifier"] = "com.example.benchmark";
    root["CFBundleName"] = "BenchmarkApp";
    root["CFBundleVersion"] = "1.0.0";
    root["CFBundleShortVersionString"] = "1.0";
    root["MinimumOSVersion"] = "14.0";
    root["CFBundleExecutable"] = "BenchmarkApp";
    root["CFBundlePackageType"] = "APPL";
    root["LSRequiresIPhoneOS"] = true;
    root["UIRequiredDeviceCapabilities"][0] = "arm64";

    // Entitlements-like nested dict
    root["Entitlements"]["application-identifier"] = "TEAM123456.com.example.benchmark";
    root["Entitlements"]["com.apple.developer.team-identifier"] = "TEAM123456";
    root["Entitlements"]["get-task-allow"] = true;
    root["Entitlements"]["keychain-access-groups"][0] = "TEAM123456.com.example.benchmark";

    // Simulate provisioning profile entries
    for (int i = 0; i < num_entries; i++) {
        jvalue device;
        char buf[128];
        snprintf(buf, sizeof(buf), "Device_%d", i);
        device["name"] = buf;
        snprintf(buf, sizeof(buf), "00000000-0000-0000-0000-%012d", i);
        device["udid"] = buf;
        device["model"] = (i % 3 == 0) ? "iPhone15,2" : (i % 3 == 1) ? "iPhone14,5" : "iPad13,4";
        device["enabled"] = (i % 7 != 0);
        device["os_version"] = "17.4.1";
        device["capacity"] = (int64_t)(64 + (i % 4) * 64); // 64, 128, 192, 256

        // Simulate binary data (embedded cert snippet)
        uint8_t fake_cert[64];
        for (int b = 0; b < 64; b++) {
            fake_cert[b] = (uint8_t)((i * 7 + b * 13) & 0xFF);
        }
        device["cert_data"].assign_data(fake_cert, 64);

        // Simulate a date
        device["registered_date"].assign_date((time_t)(1700000000 + i * 86400));

        root["devices"].push_back(device);
    }

    // Larger binary data block
    {
        uint8_t big_data[4096];
        for (int b = 0; b < 4096; b++) {
            big_data[b] = (uint8_t)(b & 0xFF);
        }
        root["embedded_profile"].assign_data(big_data, 4096);
    }

    return root;
}

template<typename Func>
static double measure(Func fn, int iterations) {
    auto start = high_resolution_clock::now();
    for (int i = 0; i < iterations; i++) {
        fn();
    }
    auto end = high_resolution_clock::now();
    return duration_cast<microseconds>(end - start).count() / 1000.0;
}

static void run_json_benchmark(const char* label, const jvalue& data, int iters) {
    printf("\n========== %s ==========\n", label);

    // Write to JSON string
    string json_str = data.write();
    string json_style = data.style_write();
    printf("  compact JSON size : %zu bytes\n", json_str.size());
    printf("  pretty JSON size  : %zu bytes\n", json_style.size());

    printf("\n  %-35s %10.3f ms\n", "JSON parse (compact):",
        measure([&]() { jvalue v; v.read(json_str); }, iters));

    printf("  %-35s %10.3f ms\n", "JSON parse (pretty):",
        measure([&]() { jvalue v; v.read(json_style); }, iters));

    printf("  %-35s %10.3f ms\n", "JSON write (compact):",
        measure([&]() { string out = data.write(); }, iters));

    printf("  %-35s %10.3f ms\n", "JSON write (pretty):",
        measure([&]() { string out = data.style_write(); }, iters));

    printf("  %-35s %10.3f ms\n", "Deep copy:",
        measure([&]() { jvalue c = data; }, iters));

    printf("  %-35s %10.3f ms\n", "Move (copy+move):",
        measure([&]() { jvalue c = data; jvalue m = std::move(c); }, iters));

    // Roundtrip verification (parse -> write -> parse -> compare key-by-key)
    {
        jvalue v1;
        v1.read(json_str);
        string re_json = v1.write();
        jvalue v2;
        v2.read(re_json);
        // Compare sizes and a sampling of values
        bool pass = (v1.size() == v2.size()) && (v1.type() == v2.type());
        if (pass && v1.is_object()) {
            vector<string> k1;
            v1.get_keys(k1);
            for (size_t i = 0; i < k1.size() && pass; i++) {
                pass = v2.has(k1[i].c_str());
            }
        }
        printf("  %-35s %s\n", "Roundtrip check:", pass ? "PASS" : "FAIL");
    }
}

static void run_plist_benchmark(const char* label, const jvalue& data, int iters) {
    printf("\n========== %s ==========\n", label);

    // Write to plist formats
    string xml_plist = data.style_write_plist();
    string compact_plist = data.write_plist();
    string bplist;
    data.write_bplist(bplist);

    printf("  XML plist size     : %zu bytes\n", xml_plist.size());
    printf("  compact plist size : %zu bytes\n", compact_plist.size());
    printf("  binary plist size  : %zu bytes\n", bplist.size());

    printf("\n  %-35s %10.3f ms\n", "XML plist parse:",
        measure([&]() { jvalue v; v.read_plist(xml_plist); }, iters));

    printf("  %-35s %10.3f ms\n", "Binary plist parse:",
        measure([&]() { jvalue v; v.read_plist(bplist.data(), bplist.size()); }, iters));

    printf("  %-35s %10.3f ms\n", "XML plist write (pretty):",
        measure([&]() { string out = data.style_write_plist(); }, iters));

    printf("  %-35s %10.3f ms\n", "XML plist write (compact):",
        measure([&]() { string out = data.write_plist(); }, iters));

    printf("  %-35s %10.3f ms\n", "Binary plist write:",
        measure([&]() { string out; data.write_bplist(out); }, iters));

    printf("  %-35s %10.3f ms\n", "Deep copy:",
        measure([&]() { jvalue c = data; }, iters));

    printf("  %-35s %10.3f ms\n", "Move (copy+move):",
        measure([&]() { jvalue c = data; jvalue m = std::move(c); }, iters));

    // XML plist roundtrip
    {
        jvalue v1;
        v1.read_plist(xml_plist);
        string re_plist = v1.style_write_plist();
        jvalue v2;
        v2.read_plist(re_plist);
        bool pass = (v1.size() == v2.size()) && (v1.type() == v2.type());
        if (pass && v1.is_object()) {
            vector<string> k1;
            v1.get_keys(k1);
            for (size_t i = 0; i < k1.size() && pass; i++) {
                pass = v2.has(k1[i].c_str());
            }
        }
        printf("  %-35s %s\n", "XML plist roundtrip:", pass ? "PASS" : "FAIL");
    }

    // Binary plist roundtrip
    {
        jvalue v1;
        bool is_bin = false;
        v1.read_plist(bplist.data(), bplist.size(), NULL, &is_bin);
        string re_bplist;
        v1.write_bplist(re_bplist);
        jvalue v2;
        v2.read_plist(re_bplist.data(), re_bplist.size());
        bool pass = (v1.size() == v2.size()) && (v1.type() == v2.type());
        if (pass && v1.is_object()) {
            vector<string> k1;
            v1.get_keys(k1);
            for (size_t i = 0; i < k1.size() && pass; i++) {
                pass = v2.has(k1[i].c_str());
            }
        }
        printf("  %-35s %s (is_binary=%s)\n", "Binary plist roundtrip:",
            pass ? "PASS" : "FAIL",
            is_bin ? "true" : "false");
    }
}

static void run_operation_benchmark(int iters) {
    printf("\n========== Operation Benchmarks ==========\n");

    // Key access: unordered_map O(1) vs old map O(log n)
    jvalue obj;
    vector<string> keys;
    for (int i = 0; i < 1000; i++) {
        char key[32];
        snprintf(key, sizeof(key), "key_%04d", i);
        obj[(const char*)key] = i;
        keys.push_back(key);
    }

    printf("\n  %-35s %10.3f ms\n", "Key access (1000 keys):",
        measure([&]() {
            volatile int64_t sink = 0;
            for (size_t k = 0; k < keys.size(); k++) {
                sink += obj[keys[k]].as_int64();
            }
        }, iters));

    // Push back
    printf("  %-35s %10.3f ms\n", "Push back (5000 ints):",
        measure([&]() {
            jvalue arr;
            for (int j = 0; j < 5000; j++) arr.push_back(j);
        }, iters));

    printf("  %-35s %10.3f ms\n", "Push back (1000 strings):",
        measure([&]() {
            jvalue arr;
            char buf[64];
            for (int j = 0; j < 1000; j++) {
                snprintf(buf, sizeof(buf), "string_value_%d_with_some_padding", j);
                arr.push_back(buf);
            }
        }, iters));

    printf("  %-35s %10.3f ms\n", "Push back (1000 objects):",
        measure([&]() {
            jvalue arr;
            for (int j = 0; j < 1000; j++) {
                jvalue item;
                item["id"] = j;
                item["name"] = "test";
                arr.push_back(item);
            }
        }, iters));

    // Erase from middle of array
    printf("  %-35s %10.3f ms\n", "Array erase (500 from 1000):",
        measure([&]() {
            jvalue arr;
            for (int j = 0; j < 1000; j++) arr.push_back(j);
            for (int j = 0; j < 500; j++) arr.erase(0);
        }, iters));

    // Object construction + deep nesting access
    printf("  %-35s %10.3f ms\n", "Deep nesting build (20 levels):",
        measure([&]() {
            jvalue v;
            jvalue* cur = &v;
            for (int j = 0; j < 20; j++) {
                (*cur)["level"] = j;
                cur = &((*cur)["child"]);
            }
            *cur = 42;
        }, iters));
}

int main(int argc, char* argv[]) {
    printf("=== jvalue Large-Scale Performance Benchmark ===\n");
    printf("All times are for the given number of iterations.\n");

    // --- Large JSON benchmarks ---
    {
        const int ITERS = 100;
        printf("\n[Iterations: %d]\n", ITERS);

        // 200 users ~ realistic API response
        jvalue large_obj = build_large_object(200);
        run_json_benchmark("Large JSON Object (200 users, nested)", large_obj, ITERS);

        // 2000-element array
        jvalue large_arr = build_large_array(2000);
        run_json_benchmark("Large JSON Array (2000 items)", large_arr, ITERS);
    }

    // --- Large Plist benchmarks (with binary data + dates) ---
    {
        const int ITERS = 100;

        // 100 devices with cert data + dates
        jvalue plist_data = build_plist_data(100);
        run_plist_benchmark("Plist (100 devices, certs, dates)", plist_data, ITERS);

        // 500 devices
        jvalue plist_large = build_plist_data(500);
        run_plist_benchmark("Plist (500 devices, certs, dates)", plist_large, ITERS);
    }

    // --- Operation benchmarks ---
    {
        const int ITERS = 500;
        run_operation_benchmark(ITERS);
    }

    printf("\n=== Benchmark Complete ===\n");
    return 0;
}
