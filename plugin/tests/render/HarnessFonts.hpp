// Loads the plugin fonts into a nanovg context the way the plugin does in Rack:
// the five Atlas fonts, each with Rack's DejaVuSans as the fallback for symbols (→ ✓ ✗ ≈).
#pragma once
#include <nanovg.h>

#include <cstdlib>
#include <string>

#include "text/NvgText.hpp"

inline std::string harnessRoot() {
	const char* r = std::getenv("ATLAS_PLUGIN_DIR");
	return r ? r : "rack/plugin";
}

inline std::string rackSystemDir() {
	const char* r = std::getenv("RACK_SYSTEM_DIR");
	if (r)
		return r;
	const char* home = std::getenv("HOME");
	return std::string(home ? home : "") + "/Applications/VCV Rack 2 Free.app/Contents/Resources";
}

inline atlas::text::Fonts harnessFonts(NVGcontext* vg) {
	static const char* files[] = {"PTSerif-Bold.ttf", "PTSans-Regular.ttf", "PTSans-Bold.ttf", "PTSans-Italic.ttf", "JetBrainsMono-Regular.ttf"};
	atlas::text::Fonts f;
	int fallback = nvgCreateFont(vg, "DejaVuSans", (rackSystemDir() + "/res/fonts/DejaVuSans.ttf").c_str());
	for (int i = 0; i < atlas::text::F_COUNT; i++) {
		f.handle[i] = nvgCreateFont(vg, files[i], (harnessRoot() + "/res/fonts/" + files[i]).c_str());
		if (f.handle[i] >= 0 && fallback >= 0)
			nvgAddFallbackFontId(vg, f.handle[i], fallback);
	}
	return f;
}
