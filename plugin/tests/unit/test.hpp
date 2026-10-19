// Minimal test helpers for the Rack-free unit tests.
#pragma once
#include <cstdio>
#include <functional>
#include <string>
#include <vector>

namespace t {
struct Case {
	const char* name;
	std::function<void()> fn;
};
inline std::vector<Case>& cases() {
	static std::vector<Case> c;
	return c;
}
inline int& failures() {
	static int f = 0;
	return f;
}
struct Reg {
	Reg(const char* n, std::function<void()> f) { cases().push_back({n, f}); }
};
} // namespace t

#define TEST(name) \
	static void test_##name(); \
	static t::Reg reg_##name(#name, test_##name); \
	static void test_##name()

#define CHECK(cond) \
	do { \
		if (!(cond)) { \
			std::fprintf(stderr, "  FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
			t::failures()++; \
		} \
	} while (0)

#define CHECK_EQ(a, b) \
	do { \
		auto _a = (a); \
		auto _b = (b); \
		if (!(_a == _b)) { \
			std::fprintf(stderr, "  FAIL %s:%d: %s == %s\n    got: %s\n    exp: %s\n", __FILE__, __LINE__, #a, #b, t::str(_a).c_str(), t::str(_b).c_str()); \
			t::failures()++; \
		} \
	} while (0)

namespace t {
inline std::string str(const std::string& s) { return "«" + s + "»"; }
inline std::string str(const char* s) { return str(std::string(s)); }
template <typename T>
std::string str(const T& v) { return std::to_string(v); }
} // namespace t
