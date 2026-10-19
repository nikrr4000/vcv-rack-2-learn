// What the text areas drew since the last reset: pages per area and any overflow.
// Filled from draw() (UI thread), read by the dev bridge snapshot and the render harness.
#pragma once
#include <map>
#include <string>

namespace atlas {

struct TextReport {
	int pages = 0;
	bool overflow = false;   // something did not fit even after pagination
	bool splitWords = false; // a word longer than the line was hyphenated
	float width = 0.f;
};

std::map<std::string, TextReport>& textReports();
inline void reportText(const std::string& where, const TextReport& r) { textReports()[where] = r; }
inline void resetTextReports() { textReports().clear(); }

} // namespace atlas
