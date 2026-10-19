// A wrapping multi-line text input drawn in the «Атлас» style. The Rack TextField that owns
// the keyboard sits on top of it (modules/Field.hpp); layout and hit-testing live here so the
// render harness draws the same thing.
#pragma once
#include <string>
#include <vector>

#include "ui/Draw.hpp"

namespace atlas {

struct InputRow {
	size_t start = 0, end = 0;  // byte offsets
	float y = 0.f;              // top, relative to the text origin
};

struct InputLayout {
	std::vector<InputRow> rows;
	float lineH = 16.f;
	float size = 11.5f;
};

const float INPUT_PAD = 6.f;
const int INPUT_MAX_ROWS = 5;

InputLayout layoutInput(NVGcontext* vg, const Style& st, const std::string& text, float width);

/** Height of an input box for `text` (1–5 rows). */
float inputHeight(NVGcontext* vg, const Style& st, const std::string& text, float width);

/** Draws the box, text (or placeholder), selection and caret. `blink` toggles the caret. */
void drawInput(NVGcontext* vg, const Style& st, const Rect& r, const std::string& text, const std::string& placeholder, int cursor, int selection, bool focused, bool blink, bool disabled = false);

/** Byte index under (x, y), in box coordinates. */
int inputIndexAt(NVGcontext* vg, const Style& st, const Rect& r, const std::string& text, float x, float y);

} // namespace atlas
