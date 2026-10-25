// tests/p3/test_p3.cpp — YOUR P3 TEST SUITE.
//
// The five tests below are worked examples. Keep them, and add your own
// until you have AT LEAST 15 (spec §6 lists what must be covered).
// Name every test <area>_<what>: reg_ for the registry, ws_ for the
// workspace, calc_ for the calculator.
//
// Build & run:   cmake --build build && ./build/test_p3
// One area:      ./build/test_p3 reg_        (names starting with "reg_")
// One test:      ./build/test_p3 =reg_add_and_find

#include "../mini_test.h"
#include "../test_helpers.h"     // TempDir, write_text, args1, ...

#include "tools/calculator.h"
#include "tools/tool_registry.h"
#include "tools/workspace.h"

#include <algorithm>
#include <cstdio>
#include <iterator>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace fs = std::filesystem;

namespace {

// A tool whose name you choose — use it to fill the registry in tests.
class NamedTool : public Tool {
public:
    explicit NamedTool(std::string n) : n_(std::move(n)), schema_(JsonValue::object()) {}
    std::string name() const override { return n_; }
    std::string description() const override { return "test tool"; }
    const JsonValue& schema() const override { return schema_; }
    ToolResult invoke(const JsonValue&, ToolContext&) override { return ToolResult::success(n_); }
private:
    std::string n_;
    JsonValue   schema_;   // an empty JSON object: {}
};

// Tokenize with the provided tokenizer, then run YOUR calc_evaluate.
bool eval(const std::string& expr, double* value, std::string* error) {
    std::vector<CalcToken> tokens;
    if (!calc_tokenize(expr, &tokens, error)) return false;
    return calc_evaluate(tokens, value, error);
}

}  // namespace

// ---------------------------------------------------------------- examples

TEST(reg_add_and_find) {
    ToolRegistry r;
    auto t = std::make_unique<NamedTool>("alpha");
    Tool* raw = t.get();                        // remember the address before moving
    REQUIRE(r.add(std::move(t)));
    CHECK(r.size() == 1);
    CHECK(r.find("alpha") == raw);              // same object, not a copy
    CHECK(r.find("beta") == nullptr);
    CHECK(r.size() == 1);
}

TEST(reg_duplicate_keeps_original) {
    ToolRegistry r;
    auto first = std::make_unique<NamedTool>("alpha");
    Tool* original = first.get();
    REQUIRE(r.add(std::move(first)));
    CHECK(!r.add(std::make_unique<NamedTool>("alpha")));
    CHECK(r.find("alpha") == original);
    CHECK(r.size() == 1);
}

TEST(reg_handles_collisions) {
    ToolRegistry r;
    const std::string first_name = "collision_0";
    const std::size_t first_bucket =
        ToolRegistry::hash(first_name) % r.bucket_count();

    std::string second_name;
    for (int i = 1; i < 1000; ++i) {
        const std::string candidate = "collision_" + std::to_string(i);
        if (ToolRegistry::hash(candidate) % r.bucket_count() == first_bucket) {
            second_name = candidate;
            break;
        }
    }
    REQUIRE(!second_name.empty());

    REQUIRE(r.add(std::make_unique<NamedTool>(first_name)));
    REQUIRE(r.add(std::make_unique<NamedTool>(second_name)));
    CHECK(r.find(first_name) != nullptr);
    CHECK(r.find(second_name) != nullptr);
    CHECK(r.size() == 2);
    CHECK(r.max_probe_length() == 2);
}

TEST(reg_rejects_null_tool) {
    ToolRegistry r;
    CHECK(!r.add(nullptr));
    CHECK(r.size() == 0);
    CHECK(r.find("alpha") == nullptr);
}

TEST(reg_for_each_visits_each_tool_once) {
    ToolRegistry r;
    REQUIRE(r.add(std::make_unique<NamedTool>("alpha")));
    REQUIRE(r.add(std::make_unique<NamedTool>("beta")));
    REQUIRE(r.add(std::make_unique<NamedTool>("gamma")));

    std::vector<std::string> names;
    r.for_each([&](const Tool& tool) { names.push_back(tool.name()); });
    CHECK(names.size() == 3);
    CHECK(r.size() == 3);
    CHECK(std::count(names.begin(), names.end(), "alpha") == 1);
    CHECK(std::count(names.begin(), names.end(), "beta") == 1);
    CHECK(std::count(names.begin(), names.end(), "gamma") == 1);
}

TEST(reg_initial_bucket_statistics) {
    ToolRegistry r;
    CHECK(r.size() == 0);
    CHECK(r.bucket_count() >= 1);
    CHECK(r.bucket_count() <= 64);
    CHECK(r.load_factor() == 0.0);
    CHECK(r.max_probe_length() == 0);
}

TEST(reg_hash_uses_all_characters) {
    CHECK(ToolRegistry::hash("ab") != ToolRegistry::hash("ba"));

    std::vector<std::size_t> hashes;
    for (int i = 0; i < 10000; ++i) {
        hashes.push_back(ToolRegistry::hash("tool_" + std::to_string(i)));
    }
    std::sort(hashes.begin(), hashes.end());
    const auto unique_end = std::unique(hashes.begin(), hashes.end());
    CHECK(std::distance(hashes.begin(), unique_end) >= 9990);
}

TEST(reg_10000_tools_and_metrics) {
    ToolRegistry r;
    Tool* first_tool = nullptr;
    const int tool_count = 10000;

    for (int i = 0; i < tool_count; ++i) {
        const std::string name = "tool_" + std::to_string(i);
        auto tool = std::make_unique<NamedTool>(name);
        if (i == 0) {
            first_tool = tool.get();
        }
        REQUIRE(r.add(std::move(tool)));
        CHECK(r.load_factor() <= 1.0);
    }

    CHECK(r.size() == static_cast<std::size_t>(tool_count));
    CHECK(r.find("tool_0") == first_tool);
    for (int i = 0; i < tool_count; ++i) {
        CHECK(r.find("tool_" + std::to_string(i)) != nullptr);
    }

    const std::size_t real_max_chain = r.max_probe_length();
    CHECK(real_max_chain <= 64);
    std::printf("Registry: buckets=%zu load_factor=%.3f max_probe_length=%zu\n",
                r.bucket_count(), r.load_factor(), real_max_chain);

    std::vector<int> weak_hash_counts(r.bucket_count(), 0);
    for (int i = 0; i < tool_count; ++i) {
        const std::string name = "tool_" + std::to_string(i);
        const std::size_t weak_hash =
            static_cast<unsigned char>(name[0]) + name.size();
        ++weak_hash_counts[weak_hash % r.bucket_count()];
    }
    int weak_hash_max_chain = 0;
    for (int count : weak_hash_counts) {
        if (count > weak_hash_max_chain) {
            weak_hash_max_chain = count;
        }
    }
    std::printf("Weak hash max chain: %d\n", weak_hash_max_chain);
    CHECK(weak_hash_max_chain > static_cast<int>(real_max_chain));
}

TEST(ws_rejects_parent_escape) {
    TempDir d;
    Workspace ws(d.path());
    fs::path out;
    std::string reason;
    REQUIRE(ws.resolve("notes/todo.txt", &out, &reason));   // sanity: accepts *something*
    CHECK(out == ws.root() / "notes" / "todo.txt");
    CHECK(!ws.resolve("../etc/passwd", &out, &reason));
    CHECK(!reason.empty());
}

TEST(ws_accepts_normalized_paths) {
    TempDir d;
    Workspace ws(d.path());
    fs::path out;
    std::string reason;

    REQUIRE(ws.resolve("notes/todo.txt", &out, &reason));
    CHECK(out == ws.root() / "notes" / "todo.txt");
    REQUIRE(ws.resolve("./notes//todo.txt", &out, &reason));
    CHECK(out == ws.root() / "notes" / "todo.txt");
    REQUIRE(ws.resolve("a/../b", &out, &reason));
    CHECK(out == ws.root() / "b");
    REQUIRE(ws.resolve("a/..", &out, &reason));
    CHECK(out == ws.root());
    REQUIRE(ws.resolve(".", &out, &reason));
    CHECK(out == ws.root());
}

TEST(ws_accepts_new_path_and_double_dots_in_name) {
    TempDir d;
    Workspace ws(d.path());
    fs::path out;
    std::string reason;

    REQUIRE(ws.resolve("new/dir/file.txt", &out, &reason));
    CHECK(out == ws.root() / "new" / "dir" / "file.txt");
    REQUIRE(ws.resolve("my..notes.txt", &out, &reason));
    CHECK(out == ws.root() / "my..notes.txt");
}

TEST(ws_rejects_empty_nul_and_absolute_paths) {
    TempDir d;
    Workspace ws(d.path());
    fs::path out;
    std::string reason;

    CHECK(!ws.resolve("", &out, &reason));
    CHECK(!reason.empty());

    const std::string nul_path = std::string("notes") + '\0' + "/x";
    CHECK(!ws.resolve(nul_path, &out, &reason));
    CHECK(!reason.empty());

    CHECK(!ws.resolve("/etc/passwd", &out, &reason));
    CHECK(!reason.empty());
}

TEST(ws_rejects_all_parent_escapes) {
    TempDir d;
    Workspace ws(d.path());
    fs::path out;
    std::string reason;
    const std::vector<std::string> paths = {
        "..",
        "../etc/passwd",
        "a/../../b",
        "notes/../../ws/notes/todo.txt"
    };

    for (const std::string& path : paths) {
        CHECK(!ws.resolve(path, &out, &reason));
        CHECK(!reason.empty());
    }
}

TEST(calc_precedence) {
    double v = 0;
    std::string err;
    REQUIRE(eval("2 + 3 * 4", &v, &err));
    CHECK(v == 14);
}

TEST(calc_division_by_zero) {
    double v = 0;
    std::string err;
    CHECK(!eval("1 / 0", &v, &err));
    CHECK(err == "division by zero");

    CHECK(!eval("1 / (2 - 2)", &v, &err));
    CHECK(err == "division by zero");
}

TEST(calc_left_to_right) {
    double v = 0;
    std::string err;
    REQUIRE(eval("10 - 4 - 3", &v, &err));
    CHECK(v == 3);
}

TEST(calc_division_left_to_right) {
    double v = 0;
    std::string err;
    REQUIRE(eval("100 / 10 / 5", &v, &err));
    CHECK(v == 2);
}

TEST(calc_parentheses) {
    double v = 0;
    std::string err;
    REQUIRE(eval("(2 + 3) * 4", &v, &err));
    CHECK(v == 20);
}

TEST(calc_worked_expression) {
    double v = 0;
    std::string err;
    REQUIRE(eval("2 * (3 + 4) - 5", &v, &err));
    CHECK(v == 9);
}

TEST(calc_negative_numbers) {
    double v = 0;
    std::string err;
    REQUIRE(eval("-3 * -3", &v, &err));
    CHECK(v == 9);
}

TEST(calc_decimal_numbers) {
    double v = 0;
    std::string err;
    REQUIRE(eval("7 / 2", &v, &err));
    CHECK(v == 3.5);
}

TEST(calc_custom_expressions) {
    double v = 0;
    std::string err;
    REQUIRE(eval("2.5 * (4 - 1.5)", &v, &err));
    CHECK(v == 6.25);
    REQUIRE(eval("(8 + 4) / (3 * 2)", &v, &err));
    CHECK(v == 2);
}

TEST(calc_long_expression) {
    std::string expression;
    expression.reserve(40000);
    for (int i = 0; i < 20000; ++i) {
        if (i > 0) {
            expression += '+';
        }
        expression += '1';
    }

    double v = 0;
    std::string err;
    REQUIRE(eval(expression, &v, &err));
    CHECK(v == 20000);
}

TEST(calc_deep_parentheses) {
    std::string expression(5000, '(');
    expression += '7';
    expression.append(5000, ')');

    double v = 0;
    std::string err;
    REQUIRE(eval(expression, &v, &err));
    CHECK(v == 7);
}

TEST(calc_rejects_malformed_tokens) {
    double v = 0;
    std::string err;
    const std::vector<CalcToken> tokens = {
        {CalcToken::Kind::Plus, 0.0},
        {CalcToken::Kind::Number, 1.0}
    };
    CHECK(!calc_evaluate(tokens, &v, &err));
    CHECK(!err.empty());
}

int main(int argc, char** argv) { return mt::run_all(argc, argv); }
