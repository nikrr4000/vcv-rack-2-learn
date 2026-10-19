// Faces of the small modules: Note (caption with a numbered pointer), Figure, Bench.
#pragma once
#include "PagedText.hpp"
#include "core/Pack.hpp"

namespace atlas {

struct NoteView {
	std::string header;
	int number = 0;          // badge shared with the pointer ring
	bool hasTargets = false;
	bool missingTarget = false;
	PagedText text;
	void draw(NVGcontext* vg, const Style& st, float w, float h, const Pointer& p, Hits& hits);
	bool action(int a, int arg);
	std::string termAt(float x, float y) const;
	Rect content;
};

struct FigureView {
	std::string header;
	json_t* figure = nullptr;
	std::string caption;
	void draw(NVGcontext* vg, const Style& st, float w, float h);
};

struct BenchView {
	bool left = true;
	std::string lessonNum;
	void draw(NVGcontext* vg, const Style& st, float w, float h);
};

/** The numbered badge used by notes and pointer rings. */
void drawBadge(NVGcontext* vg, const Style& st, float cx, float cy, int n);

} // namespace atlas
