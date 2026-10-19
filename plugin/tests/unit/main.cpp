#include "test.hpp"

int main(int argc, char** argv) {
	std::string only = argc > 1 ? argv[1] : "";
	int run = 0;
	for (auto& c : t::cases()) {
		if (!only.empty() && std::string(c.name).find(only) == std::string::npos)
			continue;
		int before = t::failures();
		c.fn();
		run++;
		std::printf("%s %s\n", t::failures() == before ? "ok  " : "FAIL", c.name);
	}
	std::printf("%d tests, %d failed checks\n", run, t::failures());
	return t::failures() ? 1 : 0;
}
