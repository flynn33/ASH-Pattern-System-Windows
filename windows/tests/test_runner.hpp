// test_runner.hpp
//
// Minimal single-header test harness for the ASH Pattern System
// Windows C++ branch.
//
// Design goals:
//   - No external dependencies beyond the C++ standard library
//   - No Visual Studio project-type binding
//   - Builds as a plain C++ executable with cl.exe
//   - Deterministic: one TU per test executable, tests registered
//     via a static-initializer in TEST_CASE macro
//   - Reports [PASS] or [FAIL] per test case, exit code = failure count
//
// Usage:
//   #include "test_runner.hpp"
//
//   TEST_CASE(my_module_does_the_thing) {
//       ASSERT_EQ(1 + 1, 2);
//       ASSERT_TRUE(true);
//   }
//
//   int main() { return ash::test::run_all(); }

#pragma once

#include <cstdio>
#include <cstdlib>
#include <exception>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace ash::test {

struct AssertionError : std::exception {
    std::string message;
    explicit AssertionError(std::string m) : message(std::move(m)) {}
    const char* what() const noexcept override { return message.c_str(); }
};

using TestFn = void (*)();

struct TestCase {
    std::string_view name;
    TestFn fn;
};

inline std::vector<TestCase>& registry() {
    static std::vector<TestCase> r;
    return r;
}

struct Register {
    Register(std::string_view name, TestFn fn) {
        registry().push_back(TestCase{name, fn});
    }
};

inline int run_all() {
    int failures = 0;
    std::size_t total = registry().size();
    std::printf("Running %zu test cases...\n", total);
    for (const auto& tc : registry()) {
        try {
            tc.fn();
            std::printf("[PASS] %.*s\n",
                        static_cast<int>(tc.name.size()), tc.name.data());
        } catch (const AssertionError& e) {
            ++failures;
            std::printf("[FAIL] %.*s :: %s\n",
                        static_cast<int>(tc.name.size()), tc.name.data(),
                        e.what());
        } catch (const std::exception& e) {
            ++failures;
            std::printf("[FAIL] %.*s :: unexpected exception: %s\n",
                        static_cast<int>(tc.name.size()), tc.name.data(),
                        e.what());
        } catch (...) {
            ++failures;
            std::printf("[FAIL] %.*s :: unknown exception\n",
                        static_cast<int>(tc.name.size()), tc.name.data());
        }
    }
    std::printf("----------------------------------------\n");
    std::printf("Total: %zu  Failures: %d\n", total, failures);
    return failures;
}

}  // namespace ash::test

// Concatenation helpers for unique identifiers
#define ASH_TEST_CAT_INNER(a, b) a##b
#define ASH_TEST_CAT(a, b) ASH_TEST_CAT_INNER(a, b)

// Register a test case. Body is the test code.
#define TEST_CASE(name)                                                      \
    static void ASH_TEST_CAT(ash_test_fn_, name)();                          \
    static const ::ash::test::Register ASH_TEST_CAT(ash_test_reg_, name){    \
        #name, &ASH_TEST_CAT(ash_test_fn_, name)};                           \
    static void ASH_TEST_CAT(ash_test_fn_, name)()

// Assertion macros. Each throws AssertionError with file:line on failure.
#define ASSERT_TRUE(expr)                                                    \
    do {                                                                     \
        if (!(expr)) {                                                       \
            std::ostringstream _oss;                                         \
            _oss << __FILE__ << ":" << __LINE__                              \
                 << " ASSERT_TRUE(" #expr ")";                               \
            throw ::ash::test::AssertionError(_oss.str());                   \
        }                                                                    \
    } while (false)

#define ASSERT_FALSE(expr)                                                   \
    do {                                                                     \
        if ((expr)) {                                                        \
            std::ostringstream _oss;                                         \
            _oss << __FILE__ << ":" << __LINE__                              \
                 << " ASSERT_FALSE(" #expr ")";                              \
            throw ::ash::test::AssertionError(_oss.str());                   \
        }                                                                    \
    } while (false)

#define ASSERT_EQ(a, b)                                                      \
    do {                                                                     \
        auto _lhs = (a);                                                     \
        auto _rhs = (b);                                                     \
        if (!(_lhs == _rhs)) {                                               \
            std::ostringstream _oss;                                         \
            _oss << __FILE__ << ":" << __LINE__                              \
                 << " ASSERT_EQ(" #a ", " #b ")";                            \
            throw ::ash::test::AssertionError(_oss.str());                   \
        }                                                                    \
    } while (false)

#define ASSERT_NE(a, b)                                                      \
    do {                                                                     \
        auto _lhs = (a);                                                     \
        auto _rhs = (b);                                                     \
        if ((_lhs == _rhs)) {                                                \
            std::ostringstream _oss;                                         \
            _oss << __FILE__ << ":" << __LINE__                              \
                 << " ASSERT_NE(" #a ", " #b ")";                            \
            throw ::ash::test::AssertionError(_oss.str());                   \
        }                                                                    \
    } while (false)
