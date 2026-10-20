#include "Figures.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>

namespace atlas {
namespace figures {

using text::F_BODY;
using text::F_BOLD;
using text::F_ITALIC;
using text::F_MONO;

NVGcolor alphaColor(NVGcolor c, float a);

namespace {

const float LABEL = 10.f;   // label size
const float LINE = 1.6f;    // main line width

double num(json_t* o, const char* k, double def) {
	json_t* v = json_object_get(o, k);
	return v && json_is_number(v) ? json_number_value(v) : def;
}

std::string str(json_t* o, const char* k, const std::string& def = "") {
	json_t* v = json_object_get(o, k);
	return v && json_is_string(v) ? json_string_value(v) : def;
}

std::string valueText(json_t* v) {
	if (!v)
		return "";
	if (json_is_string(v))
		return json_string_value(v);
	if (json_is_integer(v))
		return std::to_string((long long) json_integer_value(v));
	if (json_is_real(v)) {
		char b[32];
		std::snprintf(b, sizeof(b), "%g", json_real_value(v));
		return b;
	}
	return "";
}

int font(const Style& st, text::FontId f) { return st.fonts.handle[f]; }

void label(NVGcontext* vg, const Style& st, float x, float y, const std::string& s, int align = NVG_ALIGN_LEFT | NVG_ALIGN_BASELINE, NVGcolor* c = nullptr, text::FontId f = F_BODY, float size = LABEL) {
	text::drawText(vg, font(st, f), size, c ? *c : st.pal.ink2, x, y, s, align);
}

void hline(NVGcontext* vg, NVGcolor c, float x0, float y, float x1, float w = 1.f) {
	nvgBeginPath(vg);
	nvgMoveTo(vg, x0, y);
	nvgLineTo(vg, x1, y);
	nvgStrokeColor(vg, c);
	nvgStrokeWidth(vg, w);
	nvgStroke(vg);
}

void dashed(NVGcontext* vg, NVGcolor c, float x0, float y0, float x1, float y1, float dash, float gap, float w) {
	float dx = x1 - x0, dy = y1 - y0;
	float len = std::sqrt(dx * dx + dy * dy);
	if (len < 0.1f)
		return;
	float ux = dx / len, uy = dy / len;
	nvgBeginPath(vg);
	for (float t = 0; t < len; t += dash + gap) {
		float e = std::min(len, t + dash);
		nvgMoveTo(vg, x0 + ux * t, y0 + uy * t);
		nvgLineTo(vg, x0 + ux * e, y0 + uy * e);
	}
	nvgStrokeColor(vg, c);
	nvgStrokeWidth(vg, w);
	nvgLineCap(vg, NVG_ROUND);
	nvgStroke(vg);
}

/** One period of a waveform at phase p ∈ [0,1): −1…1. */
float wave(const std::string& shape, float p) {
	p = p - std::floor(p);
	if (shape == "sine")
		return std::sin(2.f * (float) M_PI * p);
	if (shape == "triangle")
		return p < 0.25f ? 4 * p : p < 0.75f ? 2 - 4 * p : 4 * p - 4;
	if (shape == "saw")
		return p < 0.5f ? 2 * p : 2 * p - 2;  // rising ramp, centered
	if (shape == "square")
		return p < 0.5f ? 1.f : -1.f;
	if (shape == "pulse")
		return p < 0.25f ? 1.f : -1.f;
	return 0.f;
}

std::string shapeName(const std::string& s) {
	if (s == "sine")
		return "синус";
	if (s == "triangle")
		return "треугольник";
	if (s == "saw")
		return "пила";
	if (s == "square")
		return "квадрат";
	if (s == "pulse")
		return "импульс";
	return s;
}

void drawWaveforms(NVGcontext* vg, const Style& st, const Rect& r, json_t* j) {
	json_t* shapes = json_object_get(j, "shapes");
	size_t n = json_array_size(shapes);
	if (!n)
		return;
	json_t* periods = json_object_get(j, "periods");
	json_t* labels = json_object_get(j, "labels");
	float rowH = r.h / n;
	float labelW = 0.f;
	for (size_t i = 0; i < n; i++) {
		std::string l = json_array_get(labels, i) ? valueText(json_array_get(labels, i)) : shapeName(json_string_value(json_array_get(shapes, i)));
		labelW = std::max(labelW, text::textWidth(vg, font(st, F_BODY), LABEL, l));
	}
	bool side = labelW < r.w * 0.3f;
	for (size_t i = 0; i < n; i++) {
		std::string shape = json_string_value(json_array_get(shapes, i));
		std::string l = json_array_get(labels, i) ? valueText(json_array_get(labels, i)) : shapeName(shape);
		float per = 2.f;
		if (periods && json_is_number(periods))
			per = (float) json_number_value(periods);
		else if (periods && json_is_array(periods) && json_array_get(periods, i))
			per = (float) json_number_value(json_array_get(periods, i));
		float y0 = r.y + rowH * i;
		float x0 = r.x + (side ? labelW + 8.f : 0.f);
		float top = y0 + (side ? 4.f : 14.f), bottom = y0 + rowH - 6.f;
		float mid = (top + bottom) / 2.f, amp = (bottom - top) / 2.f * 0.9f;
		if (side)
			label(vg, st, r.x, mid + 3.5f, l);
		else
			label(vg, st, r.x, y0 + 10.f, l);
		dashed(vg, st.pal.hairline, x0, mid, r.right(), mid, 2.f, 3.f, 1.f);
		nvgBeginPath(vg);
		int steps = (int) std::max(80.f, (r.right() - x0) * 1.5f);
		bool vertical = shape == "square" || shape == "pulse" || shape == "saw";
		float prev = 0.f;
		for (int k = 0; k <= steps; k++) {
			float t = (float) k / steps;
			float v = wave(shape, t * per);
			float x = x0 + (r.right() - x0) * t, y = mid - v * amp;
			if (k == 0)
				nvgMoveTo(vg, x, y);
			else {
				if (vertical && std::fabs(v - prev) > 1.f)
					nvgLineTo(vg, x, mid - prev * amp);
				nvgLineTo(vg, x, y);
			}
			prev = v;
		}
		nvgStrokeColor(vg, i == 0 && n > 1 && json_is_true(json_object_get(j, "highlightFirst")) ? st.accent : st.pal.ink);
		nvgStrokeWidth(vg, LINE);
		nvgLineJoin(vg, NVG_ROUND);
		nvgStroke(vg);
	}
}

void drawHarmonics(NVGcontext* vg, const Style& st, const Rect& r, json_t* j) {
	std::string shape = str(j, "shape", "saw");
	int count = (int) std::max(1.0, std::min(16.0, num(j, "count", 8)));
	std::string law = str(j, "law", shape == "triangle" ? "odd2" : shape == "square" ? "odd" : shape == "sine" ? "sine" : "1/n");
	float base = r.bottom() - 14.f, top = r.y + 20.f;
	hline(vg, st.pal.hairline, r.x, base + 0.5f, r.right());
	float slot = r.w / count;
	float bw = std::min(slot * 0.55f, 14.f);
	for (int n = 1; n <= count; n++) {
		float a = 0.f;
		bool odd = n % 2 == 1;
		if (law == "sine")
			a = n == 1 ? 1.f : 0.f;
		else if (law == "1/n")
			a = 1.f / n;
		else if (law == "1/n2")
			a = 1.f / (n * n);
		else if (law == "odd")
			a = odd ? 1.f / n : 0.f;
		else if (law == "odd2")
			a = odd ? 1.f / (n * n) : 0.f;
		float cx = r.x + slot * (n - 0.5f);
		if (a > 0.f) {
			float h = (base - top) * a;
			nvgBeginPath(vg);
			nvgRect(vg, cx - bw / 2, base - h, bw, h);
			nvgFillColor(vg, n == 1 ? st.accent : st.pal.ink);
			nvgFill(vg);
		}
		else {
			nvgBeginPath(vg);
			nvgCircle(vg, cx, base - 2.f, 1.2f);
			nvgFillColor(vg, st.pal.hairline);
			nvgFill(vg);
		}
		if (count <= 10 || n % 2 == 1)
			label(vg, st, cx, base + 11.f, std::to_string(n), NVG_ALIGN_CENTER | NVG_ALIGN_BASELINE, nullptr, F_MONO, 9.f);
	}
	std::string title = str(j, "title", shapeName(shape) + " · гармоники");
	label(vg, st, r.x, r.y + 9.f, title);
}

void drawAdsr(NVGcontext* vg, const Style& st, const Rect& r, json_t* j) {
	float a = (float) num(j, "a", 0.05), d = (float) num(j, "d", 0.3), s = (float) num(j, "s", 0.5), rel = (float) num(j, "r", 0.5);
	if (s > 1.f)
		s /= 100.f;
	float gate = (float) num(j, "gate", std::max(1.0, (double) (a + d) * 2.0));
	float total = gate + rel;
	float total2 = total * 1.08f;
	float gateH = 12.f;
	float envTop = r.y + 6.f, envBottom = r.bottom() - gateH - 34.f;
	float gy = r.bottom() - 4.f;
	auto X = [&](float t) { return r.x + r.w * (t / total2); };
	auto Y = [&](float v) { return envBottom - (envBottom - envTop) * v; };
	hline(vg, st.pal.hairline, r.x, envBottom + 0.5f, r.right());
	// The envelope, attack and decay reaching their targets before the gate ends.
	float ta = std::min(a, gate), td = std::min(a + d, gate);
	float peak = a > 0 ? std::min(1.f, gate / a) : 1.f;
	float atGateEnd = gate >= a + d ? s : gate >= a ? 1.f - (1.f - s) * ((gate - a) / std::max(d, 1e-4f)) : peak;
	nvgBeginPath(vg);
	nvgMoveTo(vg, X(0), Y(0));
	nvgLineTo(vg, X(ta), Y(peak));
	if (gate > a) {
		nvgLineTo(vg, X(td), Y(gate >= a + d ? s : atGateEnd));
		nvgLineTo(vg, X(gate), Y(atGateEnd));
	}
	nvgLineTo(vg, X(gate + rel), Y(0));
	nvgStrokeColor(vg, st.pal.ink);
	nvgStrokeWidth(vg, LINE);
	nvgLineJoin(vg, NVG_ROUND);
	nvgStroke(vg);
	// Gate lane.
	nvgBeginPath(vg);
	nvgMoveTo(vg, r.x, gy);
	nvgLineTo(vg, X(0), gy);
	nvgLineTo(vg, X(0), gy - gateH);
	nvgLineTo(vg, X(gate), gy - gateH);
	nvgLineTo(vg, X(gate), gy);
	nvgLineTo(vg, r.right(), gy);
	nvgStrokeColor(vg, st.accent);
	nvgStrokeWidth(vg, 1.4f);
	nvgStroke(vg);
	label(vg, st, r.right(), gy - 3.f, "гейт", NVG_ALIGN_RIGHT | NVG_ALIGN_BASELINE);
	if (!json_is_false(json_object_get(j, "markers"))) {
		NVGcolor c = st.pal.ink2;
		float ly = envBottom + 12.f;
		// Segment letters at segment centers, pushed apart so they never collide.
		std::vector<std::pair<std::string, float>> marks;
		marks.push_back({"A", (X(0) + X(ta)) / 2});
		if (gate > a) {
			marks.push_back({"D", (X(ta) + X(td)) / 2});
			marks.push_back({"S", (X(td) + X(gate)) / 2});
		}
		marks.push_back({"R", (X(gate) + X(gate + rel)) / 2});
		for (size_t k = 1; k < marks.size(); k++)
			marks[k].second = std::max(marks[k].second, marks[k - 1].second + 11.f);
		for (auto& m : marks)
			label(vg, st, m.second, ly, m.first, NVG_ALIGN_CENTER | NVG_ALIGN_BASELINE, &c, F_MONO);
		dashed(vg, st.pal.hairline, X(gate), envTop, X(gate), envBottom, 2.f, 2.f, 1.f);
	}
}

void drawVoltages(NVGcontext* vg, const Style& st, const Rect& r, json_t* j) {
	json_t* ranges = json_object_get(j, "ranges");
	size_t n = json_array_size(ranges);
	if (!n)
		return;
	float lo = -10.f, hi = 10.f;
	size_t i;
	json_t* v;
	json_array_foreach(ranges, i, v) {
		lo = std::min(lo, (float) num(v, "min", 0));
		hi = std::max(hi, (float) num(v, "max", 0));
	}
	float axisW = 26.f;
	float top = r.y + 6.f, bottom = r.bottom() - 24.f;
	auto Y = [&](float volts) { return bottom - (bottom - top) * ((volts - lo) / (hi - lo)); };
	for (float t = lo; t <= hi + 0.01f; t += 5.f) {
		hline(vg, t == 0.f ? st.pal.ink2 : st.pal.hairline, r.x + axisW, Y(t) + 0.5f, r.right());
		char b[16];
		std::snprintf(b, sizeof(b), "%+g", t);
		label(vg, st, r.x + axisW - 4.f, Y(t) + 3.5f, t == 0.f ? "0" : b, NVG_ALIGN_RIGHT | NVG_ALIGN_BASELINE, nullptr, F_MONO, 9.f);
	}
	float slot = (r.w - axisW) / n;
	json_array_foreach(ranges, i, v) {
		float a = (float) num(v, "min", 0), b = (float) num(v, "max", 0);
		float cx = r.x + axisW + slot * (i + 0.5f);
		float bw = std::min(slot * 0.42f, 22.f);
		nvgBeginPath(vg);
		nvgRect(vg, cx - bw / 2, Y(b), bw, std::max(2.f, Y(a) - Y(b)));
		nvgFillColor(vg, i % 2 == 0 ? st.accent : st.pal.ink);
		nvgFill(vg);
		std::string l = str(v, "label");
		std::string fitted = ellipsize(vg, font(st, F_BODY), 9.5f, l, slot - 2.f);
		label(vg, st, cx, r.bottom() - 10.f, fitted, NVG_ALIGN_CENTER | NVG_ALIGN_BASELINE, nullptr, F_BODY, 9.5f);
	}
}

int noteIndex(const std::string& note) {
	// C4 → 60
	static const char* names[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
	if (note.empty())
		return 60;
	size_t k = 1;
	std::string name = note.substr(0, 1);
	if (note.size() > 1 && (note[1] == '#' || note[1] == 'b')) {
		name = note.substr(0, 2);
		k = 2;
	}
	int oct = std::atoi(note.substr(k).c_str());
	int idx = 0;
	for (int i = 0; i < 12; i++)
		if (name == names[i])
			idx = i;
	if (name.size() == 2 && name[1] == 'b') {
		for (int i = 0; i < 12; i++)
			if (std::string(1, name[0]) == names[i])
				idx = (i + 11) % 12;
	}
	return (oct + 1) * 12 + idx;
}

void drawKeyboard(NVGcontext* vg, const Style& st, const Rect& r, json_t* j) {
	int from = noteIndex(str(j, "from", "C3")), to = noteIndex(str(j, "to", "C5"));
	if (to <= from)
		to = from + 24;
	auto isBlack = [](int n) {
		int k = n % 12;
		return k == 1 || k == 3 || k == 6 || k == 8 || k == 10;
	};
	int whites = 0;
	for (int n = from; n <= to; n++)
		if (!isBlack(n))
			whites++;
	float kw = r.w / std::max(1, whites);
	float top = r.y + 18.f, kh = std::min(r.h - 34.f, kw * 4.2f);
	std::map<int, float> whiteX;
	int w = 0;
	for (int n = from; n <= to; n++) {
		if (isBlack(n))
			continue;
		float x = r.x + kw * w++;
		whiteX[n] = x;
		nvgBeginPath(vg);
		nvgRect(vg, x + 0.5f, top + 0.5f, kw - 1.f, kh);
		nvgFillColor(vg, st.dark ? nvgRGB(0xE8, 0xE2, 0xD6) : nvgRGB(0xFF, 0xFD, 0xF8));
		nvgFill(vg);
		nvgStrokeColor(vg, st.pal.ink2);
		nvgStrokeWidth(vg, 1.f);
		nvgStroke(vg);
		if (n % 12 == 0)
			label(vg, st, x + kw / 2, top + kh - 4.f, "C" + std::to_string(n / 12 - 1), NVG_ALIGN_CENTER | NVG_ALIGN_BASELINE, nullptr, F_MONO, 8.f);
	}
	for (int n = from; n <= to; n++) {
		if (!isBlack(n) || !whiteX.count(n - 1))
			continue;
		float x = whiteX[n - 1] + kw * 0.68f;
		nvgBeginPath(vg);
		nvgRect(vg, x, top, kw * 0.64f, kh * 0.6f);
		nvgFillColor(vg, nvgRGB(0x2A, 0x27, 0x23));
		nvgFill(vg);
	}
	size_t i;
	json_t* m;
	json_array_foreach(json_object_get(j, "marks"), i, m) {
		int n = noteIndex(str(m, "note"));
		float cx;
		if (isBlack(n) && whiteX.count(n - 1))
			cx = whiteX[n - 1] + kw;
		else if (whiteX.count(n))
			cx = whiteX[n] + kw / 2;
		else
			continue;
		nvgBeginPath(vg);
		nvgCircle(vg, cx, top - 6.f, 3.f);
		nvgFillColor(vg, st.accent);
		nvgFill(vg);
		std::string v = valueText(json_object_get(m, "volts"));
		if (!v.empty()) {
			NVGcolor c = st.accent;
			label(vg, st, cx, top + kh + 12.f, v + " В", NVG_ALIGN_CENTER | NVG_ALIGN_BASELINE, &c, F_MONO, 9.f);
		}
	}
}

void arrow(NVGcontext* vg, float x0, float y0, float x1, float y1, NVGcolor c, float w, int style) {
	// style: 0 solid (audio), 1 dashed (cv), 2 dotted (gate)
	if (style == 0) {
		nvgBeginPath(vg);
		nvgMoveTo(vg, x0, y0);
		nvgLineTo(vg, x1, y1);
		nvgStrokeColor(vg, c);
		nvgStrokeWidth(vg, w);
		nvgStroke(vg);
	}
	else
		dashed(vg, c, x0, y0, x1, y1, style == 1 ? 4.f : 0.6f, style == 1 ? 3.f : 3.f, w);
	float dx = x1 - x0, dy = y1 - y0, len = std::sqrt(dx * dx + dy * dy);
	if (len < 1.f)
		return;
	float ux = dx / len, uy = dy / len, s = 5.f + w;
	nvgBeginPath(vg);
	nvgMoveTo(vg, x1, y1);
	nvgLineTo(vg, x1 - ux * s - uy * s * 0.55f, y1 - uy * s + ux * s * 0.55f);
	nvgLineTo(vg, x1 - ux * s + uy * s * 0.55f, y1 - uy * s - ux * s * 0.55f);
	nvgClosePath(vg);
	nvgFillColor(vg, c);
	nvgFill(vg);
}

void drawFlow(NVGcontext* vg, const Style& st, const Rect& r, json_t* j) {
	json_t* nodes = json_object_get(j, "nodes");
	size_t n = json_array_size(nodes);
	if (!n)
		return;
	bool two = str(j, "layout", "row") == "two-rows";
	struct Box {
		std::string id, label, kind;
		Rect r;
	};
	std::vector<Box> boxes;
	std::vector<size_t> top, bottom;
	size_t i;
	json_t* v;
	json_array_foreach(nodes, i, v) {
		Box b;
		b.id = str(v, "id");
		b.label = str(v, "label", b.id);
		b.kind = str(v, "kind", "audio");
		boxes.push_back(b);
		// Row: explicit `row: 0|1`, else sound on top and control below.
		json_t* rv = json_object_get(v, "row");
		bool low = rv && json_is_integer(rv) ? json_integer_value(rv) == 1 : b.kind != "audio";
		(two && low ? bottom : top).push_back(i);
	}
	// Two rows asked for but one is empty (all control, or all sound): split in node order.
	if (two && (top.empty() || bottom.empty())) {
		std::vector<size_t> all = top.empty() ? bottom : top;
		top.assign(all.begin(), all.begin() + (all.size() + 1) / 2);
		bottom.assign(all.begin() + (all.size() + 1) / 2, all.end());
	}
	float bh = 22.f, gapX = 14.f;
	auto place = [&](std::vector<size_t>& row, float cy) {
		if (row.empty())
			return;
		float avail = r.w - gapX * (row.size() - 1);
		float bw = avail / row.size();
		for (size_t k = 0; k < row.size(); k++)
			boxes[row[k]].r = Rect(r.x + (bw + gapX) * k, cy - bh / 2, bw, bh);
	};
	if (two && !bottom.empty()) {
		place(top, r.y + r.h * 0.3f);
		place(bottom, r.y + r.h * 0.75f);
	}
	else
		place(top, r.y + r.h / 2);
	auto find = [&](const std::string& id) -> Box* {
		for (Box& b : boxes)
			if (b.id == id)
				return &b;
		return nullptr;
	};
	int lanes = 0;  // edges routed over their row so far (each one a little higher)
	json_array_foreach(json_object_get(j, "edges"), i, v) {
		Box* a = find(str(v, "from"));
		Box* b = find(str(v, "to"));
		if (!a || !b)
			continue;
		std::string kind = str(v, "kind", "audio");
		int style = kind == "audio" ? 0 : kind == "cv" ? 1 : 2;
		NVGcolor c = kind == "audio" ? st.pal.ink : st.accent;
		float w = kind == "audio" ? 2.f : 1.4f;
		float x0, y0, x1, y1;
		if (std::fabs(a->r.y - b->r.y) < 1.f) {
			bool fwd = b->r.x > a->r.x;
			x0 = fwd ? a->r.right() : a->r.x;
			x1 = fwd ? b->r.x : b->r.right();
			y0 = y1 = a->r.y + bh / 2;
			bool through = false;
			for (const Box& o : boxes)
				if (&o != a && &o != b && std::fabs(o.r.y - a->r.y) < 1.f && o.r.x > std::min(x0, x1) - 1.f && o.r.right() < std::max(x0, x1) + 1.f)
					through = true;
			if (through) {
				// Around the boxes in between: up from the source, across above the row, down into the target.
				float yr = std::max(r.y + 2.f, a->r.y - 7.f - 4.f * lanes++);
				float xa = a->r.x + a->r.w * (fwd ? 0.7f : 0.3f), xb = b->r.x + b->r.w * (fwd ? 0.3f : 0.7f);
				auto seg = [&](float sx, float sy, float ex, float ey) {
					if (style == 0) {
						nvgBeginPath(vg);
						nvgMoveTo(vg, sx, sy);
						nvgLineTo(vg, ex, ey);
						nvgStrokeColor(vg, c);
						nvgStrokeWidth(vg, w);
						nvgStroke(vg);
					}
					else
						dashed(vg, c, sx, sy, ex, ey, style == 1 ? 4.f : 0.6f, 3.f, w);
				};
				seg(xa, a->r.y, xa, yr);
				seg(xa, yr, xb, yr);
				arrow(vg, xb, yr, xb, b->r.y, c, w, style);
				continue;
			}
		}
		else {
			bool down = b->r.y > a->r.y;
			x0 = a->r.x + a->r.w / 2;
			x1 = b->r.x + b->r.w / 2;
			y0 = down ? a->r.bottom() : a->r.y;
			y1 = down ? b->r.y : b->r.bottom();
		}
		arrow(vg, x0, y0, x1, y1, c, w, style);
	}
	for (Box& b : boxes) {
		nvgBeginPath(vg);
		nvgRect(vg, b.r.x + 0.5f, b.r.y + 0.5f, b.r.w - 1.f, b.r.h - 1.f);
		nvgFillColor(vg, st.pal.paper2);
		nvgFill(vg);
		nvgStrokeColor(vg, b.kind == "audio" ? st.pal.ink : st.accent);
		nvgStrokeWidth(vg, 1.f);
		nvgStroke(vg);
		std::string l = ellipsize(vg, font(st, F_BOLD), 9.5f, b.label, b.r.w - 4.f);
		NVGcolor c = st.pal.ink;
		label(vg, st, b.r.x + b.r.w / 2, b.r.y + bh / 2 + 3.4f, l, NVG_ALIGN_CENTER | NVG_ALIGN_BASELINE, &c, F_BOLD, 9.5f);
	}
	// Legend.
	float ly = r.bottom() - 3.f, lx = r.x;
	struct L {
		const char* t;
		int style;
	} legend[] = {{"звук", 0}, {"CV", 1}, {"гейт", 2}};
	for (const L& l : legend) {
		arrow(vg, lx, ly - 3.f, lx + 16.f, ly - 3.f, l.style == 0 ? st.pal.ink : st.accent, l.style == 0 ? 2.f : 1.4f, l.style);
		label(vg, st, lx + 22.f, ly, l.t, NVG_ALIGN_LEFT | NVG_ALIGN_BASELINE, nullptr, F_BODY, 9.f);
		lx += 26.f + text::textWidth(vg, font(st, F_BODY), 9.f, l.t) + 8.f;
	}
}

void drawFilter(NVGcontext* vg, const Style& st, const Rect& r, json_t* j) {
	bool hp = str(j, "type", "lp") == "hp";
	float cutoff = (float) num(j, "cutoff", 1000);
	float res = (float) num(j, "resonance", 0.2);
	if (res > 1.f)
		res /= 100.f;
	float fmin = 20.f, fmax = 20000.f;
	float top = r.y + 8.f, bottom = r.bottom() - 18.f;
	auto X = [&](float f) { return r.x + r.w * (std::log(f / fmin) / std::log(fmax / fmin)); };
	float dbTop = 12.f, dbBottom = -36.f;
	auto Y = [&](float db) { return top + (bottom - top) * ((dbTop - db) / (dbTop - dbBottom)); };
	hline(vg, st.pal.hairline, r.x, Y(0) + 0.5f, r.right());
	nvgBeginPath(vg);
	for (int k = 0; k <= 200; k++) {
		float f = fmin * std::pow(fmax / fmin, k / 200.f);
		float x = f / cutoff;
		if (hp)
			x = 1.f / x;
		// 2-pole response with resonance peak
		float q = 0.707f + res * 6.f;
		float re = 1.f - x * x, im = x / q;
		float mag = 1.f / std::sqrt(re * re + im * im);
		float db = std::max(dbBottom, std::min(dbTop, 20.f * std::log10(mag)));
		if (k == 0)
			nvgMoveTo(vg, X(f), Y(db));
		else
			nvgLineTo(vg, X(f), Y(db));
	}
	nvgStrokeColor(vg, st.pal.ink);
	nvgStrokeWidth(vg, LINE);
	nvgStroke(vg);
	dashed(vg, st.accent, X(cutoff), top, X(cutoff), bottom, 3.f, 2.f, 1.2f);
	NVGcolor c = st.accent;
	char b[32];
	if (cutoff >= 1000)
		std::snprintf(b, sizeof(b), "%g кГц", cutoff / 1000.f);
	else
		std::snprintf(b, sizeof(b), "%g Гц", cutoff);
	float lx = X(cutoff);
	bool rightSide = lx < r.x + r.w * 0.7f;
	label(vg, st, lx + (rightSide ? 4.f : -4.f), top + 9.f, std::string("срез ") + b, (rightSide ? NVG_ALIGN_LEFT : NVG_ALIGN_RIGHT) | NVG_ALIGN_BASELINE, &c);
	label(vg, st, r.x, r.bottom() - 4.f, "20 Гц", NVG_ALIGN_LEFT | NVG_ALIGN_BASELINE, nullptr, F_MONO, 9.f);
	label(vg, st, r.right(), r.bottom() - 4.f, "20 кГц", NVG_ALIGN_RIGHT | NVG_ALIGN_BASELINE, nullptr, F_MONO, 9.f);
	label(vg, st, r.x + r.w / 2, r.bottom() - 4.f, hp ? "фильтр верхних частот" : "фильтр нижних частот", NVG_ALIGN_CENTER | NVG_ALIGN_BASELINE, nullptr, F_BODY, 9.f);
}

void drawClock(NVGcontext* vg, const Style& st, const Rect& r, json_t* j) {
	json_t* divs = json_object_get(j, "divisions");
	std::vector<int> d;
	size_t i;
	json_t* v;
	json_array_foreach(divs, i, v) d.push_back((int) json_number_value(v));
	if (d.empty())
		d = {1, 2, 4};
	int bars = (int) std::max(1.0, num(j, "bars", 2));
	int beats = bars * 4;
	float labelW = 30.f;
	float laneH = std::min(26.f, (r.h - 18.f) / d.size());
	float x0 = r.x + labelW, w = r.w - labelW;
	for (int b = 0; b <= bars; b++)
		dashed(vg, st.pal.hairline, x0 + w * b / bars, r.y, x0 + w * b / bars, r.y + laneH * d.size(), 2.f, 2.f, 1.f);
	for (size_t k = 0; k < d.size(); k++) {
		float y0 = r.y + laneH * k + 4.f, y1 = y0 + laneH - 8.f;
		label(vg, st, r.x, (y0 + y1) / 2 + 3.5f, d[k] == 1 ? "клок" : "÷" + std::to_string(d[k]), NVG_ALIGN_LEFT | NVG_ALIGN_BASELINE, nullptr, F_MONO, 9.5f);
		nvgBeginPath(vg);
		nvgMoveTo(vg, x0, y1);
		float step = w / beats;
		for (int t = 0; t < beats; t += d[k]) {
			float xa = x0 + step * t, xb = xa + step * 0.5f;
			nvgLineTo(vg, xa, y1);
			nvgLineTo(vg, xa, y0);
			nvgLineTo(vg, xb, y0);
			nvgLineTo(vg, xb, y1);
		}
		nvgLineTo(vg, x0 + w, y1);
		nvgStrokeColor(vg, k == 0 ? st.accent : st.pal.ink);
		nvgStrokeWidth(vg, 1.4f);
		nvgStroke(vg);
	}
	std::string bpm = valueText(json_object_get(j, "bpm"));
	if (!bpm.empty())
		label(vg, st, r.right(), r.bottom() - 4.f, bpm + " BPM", NVG_ALIGN_RIGHT | NVG_ALIGN_BASELINE, nullptr, F_MONO, 9.f);
}

void drawSteps(NVGcontext* vg, const Style& st, const Rect& r, json_t* j) {
	json_t* values = json_object_get(j, "values");
	json_t* gates = json_object_get(j, "gates");
	size_t n = json_array_size(values);
	if (!n)
		return;
	float lo = 0, hi = 0;
	for (size_t i = 0; i < n; i++) {
		float v = (float) json_number_value(json_array_get(values, i));
		lo = std::min(lo, v);
		hi = std::max(hi, v);
	}
	if (hi - lo < 1e-3f)
		hi = lo + 1.f;
	float gateH = gates ? 14.f : 0.f;
	float top = r.y + 4.f, bottom = r.bottom() - gateH - 16.f;
	float slot = r.w / n;
	for (size_t i = 0; i < n; i++) {
		float v = (float) json_number_value(json_array_get(values, i));
		float cx = r.x + slot * (i + 0.5f);
		float h = (bottom - top) * ((v - lo) / (hi - lo));
		nvgBeginPath(vg);
		nvgRect(vg, cx - slot * 0.32f, bottom - std::max(2.f, h), slot * 0.64f, std::max(2.f, h));
		nvgFillColor(vg, st.pal.ink);
		nvgFill(vg);
		label(vg, st, cx, bottom + 11.f, std::to_string(i + 1), NVG_ALIGN_CENTER | NVG_ALIGN_BASELINE, nullptr, F_MONO, 9.f);
		if (gates) {
			bool on = json_is_true(json_array_get(gates, i)) || (json_is_number(json_array_get(gates, i)) && json_number_value(json_array_get(gates, i)) > 0);
			nvgBeginPath(vg);
			nvgRect(vg, cx - slot * 0.32f, r.bottom() - gateH + 2.f, slot * 0.64f, gateH - 4.f);
			if (on) {
				nvgFillColor(vg, st.accent);
				nvgFill(vg);
			}
			else {
				nvgStrokeColor(vg, st.pal.hairline);
				nvgStrokeWidth(vg, 1.f);
				nvgStroke(vg);
			}
		}
	}
}

void drawMixer(NVGcontext* vg, const Style& st, const Rect& r, json_t* j) {
	json_t* ch = json_object_get(j, "channels");
	size_t n = json_array_size(ch);
	if (!n)
		return;
	float head = (float) num(j, "headroom", 10);
	float scaleMax = head * 1.25f;
	size_t i;
	json_t* v;
	json_array_foreach(ch, i, v) scaleMax = std::max(scaleMax, (float) num(v, "level", 0) * 1.1f);
	float top = r.y + 6.f, bottom = r.bottom() - 18.f;
	auto Y = [&](float x) { return bottom - (bottom - top) * (x / scaleMax); };
	float slot = r.w / n;
	json_array_foreach(ch, i, v) {
		float lv = (float) num(v, "level", 0);
		float cx = r.x + slot * (i + 0.5f);
		float bw = std::min(slot * 0.5f, 20.f);
		nvgBeginPath(vg);
		nvgRect(vg, cx - bw / 2, top, bw, bottom - top);
		nvgFillColor(vg, st.pal.paper2);
		nvgFill(vg);
		nvgBeginPath(vg);
		nvgRect(vg, cx - bw / 2, Y(lv), bw, bottom - Y(lv));
		nvgFillColor(vg, lv > head ? failColor(st) : st.pal.ink);
		nvgFill(vg);
		label(vg, st, cx, r.bottom() - 4.f, ellipsize(vg, font(st, F_BODY), 9.5f, str(v, "label"), slot - 2.f), NVG_ALIGN_CENTER | NVG_ALIGN_BASELINE, nullptr, F_BODY, 9.5f);
	}
	dashed(vg, st.accent, r.x, Y(head), r.right(), Y(head), 4.f, 3.f, 1.2f);
	NVGcolor c = st.accent;
	char b[32];
	std::snprintf(b, sizeof(b), "предел %g В", head);
	float tw = text::textWidth(vg, font(st, F_BODY), 9.5f, b);
	nvgBeginPath(vg);
	nvgRect(vg, r.x, Y(head) - 13.f, tw + 6.f, 11.f);
	nvgFillColor(vg, st.pal.panel);
	nvgFill(vg);
	label(vg, st, r.x + 2.f, Y(head) - 4.f, b, NVG_ALIGN_LEFT | NVG_ALIGN_BASELINE, &c, F_BODY, 9.5f);
}

void drawStereo(NVGcontext* vg, const Style& st, const Rect& r, json_t* j) {
	json_t* src = json_object_get(j, "sources");
	size_t n = json_array_size(src);
	float x0 = r.x + 10.f, x1 = r.right() - 10.f;
	float axisY = r.bottom() - 14.f;
	hline(vg, st.pal.ink2, x0, axisY, x1);
	NVGcolor c = st.pal.ink2;
	label(vg, st, x0, r.bottom() - 2.f, "L", NVG_ALIGN_CENTER | NVG_ALIGN_BASELINE, &c, F_MONO);
	label(vg, st, (x0 + x1) / 2, r.bottom() - 2.f, "C", NVG_ALIGN_CENTER | NVG_ALIGN_BASELINE, &c, F_MONO);
	label(vg, st, x1, r.bottom() - 2.f, "R", NVG_ALIGN_CENTER | NVG_ALIGN_BASELINE, &c, F_MONO);
	if (!n)
		return;
	float rowH = std::min(30.f, (axisY - r.y - 4.f) / n);
	size_t i;
	json_t* v;
	json_array_foreach(src, i, v) {
		float pan = (float) num(v, "pan", 0), width = (float) num(v, "width", 0);
		float cx = x0 + (x1 - x0) * (pan + 1.f) / 2.f;
		float half = (x1 - x0) / 2.f * width;
		float y = r.y + rowH * (i + 0.5f) + 6.f;
		if (half > 1.f) {
			nvgBeginPath(vg);
			nvgRect(vg, std::max(x0, cx - half), y - 3.f, std::min(x1, cx + half) - std::max(x0, cx - half), 6.f);
			nvgFillColor(vg, alphaColor(st.accent, 0.35f));
			nvgFill(vg);
		}
		nvgBeginPath(vg);
		nvgCircle(vg, cx, y, 4.f);
		nvgFillColor(vg, st.accent);
		nvgFill(vg);
		std::string l = str(v, "label");
		float lw = text::textWidth(vg, font(st, F_BODY), 9.5f, l);
		float lx = std::max(r.x + lw / 2, std::min(r.right() - lw / 2, cx));
		label(vg, st, lx, y - 7.f, l, NVG_ALIGN_CENTER | NVG_ALIGN_BASELINE, nullptr, F_BODY, 9.5f);
	}
}

void drawTable(NVGcontext* vg, const Style& st, const Rect& r, json_t* j) {
	json_t* rows = json_object_get(j, "rows");
	size_t nr = json_array_size(rows);
	if (!nr)
		return;
	size_t nc = 0;
	for (size_t i = 0; i < nr; i++)
		nc = std::max(nc, json_array_size(json_array_get(rows, i)));
	float rowH = std::min(26.f, r.h / nr);
	// Columns as wide as their longest cell (+6 px padding), scaled down together when the table is wider than the figure.
	std::vector<float> need(nc, 0.f), colX(nc + 1, r.x);
	for (size_t i = 0; i < nr; i++)
		for (size_t k = 0; k < nc; k++)
			need[k] = std::max(need[k], text::textWidth(vg, font(st, i == 0 ? F_BOLD : F_BODY), 10.f, valueText(json_array_get(json_array_get(rows, i), k))) + 6.f);
	float total = 0.f;
	for (float x : need)
		total += x;
	for (size_t k = 0; k < nc; k++)
		colX[k + 1] = colX[k] + (total > 0.f ? need[k] * r.w / total : r.w / nc);
	for (size_t i = 0; i < nr; i++) {
		float y = r.y + rowH * i;
		if (i > 0)
			hline(vg, st.pal.hairline, r.x, y + 0.5f, r.right());
		for (size_t k = 0; k < nc; k++) {
			std::string t = valueText(json_array_get(json_array_get(rows, i), k));
			text::FontId f = i == 0 ? F_BOLD : F_BODY;
			NVGcolor c = st.pal.ink;
			label(vg, st, colX[k] + 3.f, y + rowH / 2 + 3.6f, ellipsize(vg, font(st, f), 10.f, t, colX[k + 1] - colX[k] - 6.f), NVG_ALIGN_LEFT | NVG_ALIGN_BASELINE, &c, f, 10.f);
		}
	}
	hline(vg, st.pal.ink2, r.x, r.y + rowH + 0.5f, r.right());
}

} // namespace

const std::vector<std::string>& templates() {
	static const std::vector<std::string> t = {"waveforms", "harmonics", "adsr", "voltages", "keyboard", "flow", "filter", "clock", "steps", "mixer", "stereo", "table"};
	return t;
}

std::string validate(json_t* j) {
	if (!j || !json_is_object(j))
		return "figure must be an object";
	std::string t = str(j, "template");
	if (std::find(templates().begin(), templates().end(), t) == templates().end())
		return "unknown figure template «" + t + "»";
	auto needArray = [&](const char* k) -> std::string {
		json_t* v = json_object_get(j, k);
		return v && json_is_array(v) && json_array_size(v) > 0 ? "" : std::string(t) + ": «" + k + "» must be a non-empty array";
	};
	if (t == "waveforms") {
		std::string e = needArray("shapes");
		if (!e.empty())
			return e;
		size_t i;
		json_t* v;
		json_array_foreach(json_object_get(j, "shapes"), i, v) {
			std::string s = json_is_string(v) ? json_string_value(v) : "";
			if (s != "sine" && s != "triangle" && s != "saw" && s != "square" && s != "pulse")
				return "waveforms: unknown shape «" + s + "»";
		}
		return "";
	}
	if (t == "harmonics") {
		double c = num(j, "count", 8);
		if (c < 1 || c > 16)
			return "harmonics: count must be 1–16";
		std::string law = str(j, "law", "1/n");
		if (law != "1/n" && law != "1/n2" && law != "odd" && law != "odd2" && law != "sine")
			return "harmonics: unknown law «" + law + "»";
		return "";
	}
	if (t == "voltages")
		return needArray("ranges");
	if (t == "flow")
		return needArray("nodes");
	if (t == "steps")
		return needArray("values");
	if (t == "mixer")
		return needArray("channels");
	if (t == "stereo")
		return needArray("sources");
	if (t == "table") {
		std::string e = needArray("rows");
		if (!e.empty())
			return e;
		if (json_array_size(json_object_get(j, "rows")) > 5)
			return "table: at most 5 rows";
		size_t i;
		json_t* v;
		json_array_foreach(json_object_get(j, "rows"), i, v) if (json_array_size(v) > 4) return "table: at most 4 columns";
		return "";
	}
	return "";
}

float aspect(json_t* j) {
	std::string t = str(j, "template");
	if (t == "waveforms")
		return 0.28f * std::max<size_t>(1, json_array_size(json_object_get(j, "shapes"))) + 0.1f;
	if (t == "keyboard")
		return 0.42f;
	if (t == "flow")
		return str(j, "layout", "row") == "two-rows" ? 0.62f : 0.42f;
	if (t == "table")
		return 0.12f * std::max<size_t>(1, json_array_size(json_object_get(j, "rows"))) + 0.05f;
	if (t == "stereo")
		return 0.18f * std::max<size_t>(1, json_array_size(json_object_get(j, "sources"))) + 0.2f;
	if (t == "clock")
		return 0.16f * std::max<size_t>(1, json_array_size(json_object_get(j, "divisions"))) + 0.12f;
	return 0.62f;
}

void draw(NVGcontext* vg, const Style& st, const Rect& r, json_t* j) {
	nvgSave(vg);
	nvgScissor(vg, r.x - 2.f, r.y - 2.f, r.w + 4.f, r.h + 4.f);
	std::string t = str(j, "template");
	if (!validate(j).empty()) {
		NVGcolor c = st.pal.ink2;
		label(vg, st, r.x + r.w / 2, r.y + r.h / 2, "(схема)", NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE, &c, F_ITALIC);
	}
	else if (t == "waveforms")
		drawWaveforms(vg, st, r, j);
	else if (t == "harmonics")
		drawHarmonics(vg, st, r, j);
	else if (t == "adsr")
		drawAdsr(vg, st, r, j);
	else if (t == "voltages")
		drawVoltages(vg, st, r, j);
	else if (t == "keyboard")
		drawKeyboard(vg, st, r, j);
	else if (t == "flow")
		drawFlow(vg, st, r, j);
	else if (t == "filter")
		drawFilter(vg, st, r, j);
	else if (t == "clock")
		drawClock(vg, st, r, j);
	else if (t == "steps")
		drawSteps(vg, st, r, j);
	else if (t == "mixer")
		drawMixer(vg, st, r, j);
	else if (t == "stereo")
		drawStereo(vg, st, r, j);
	else if (t == "table")
		drawTable(vg, st, r, j);
	nvgRestore(vg);
}

NVGcolor alphaColor(NVGcolor c, float a) {
	c.a = a;
	return c;
}

} // namespace figures
} // namespace atlas
