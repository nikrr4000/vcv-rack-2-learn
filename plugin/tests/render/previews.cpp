#include <map>
// Module previews from a pack, drawn by the plugin's own views (visual QA without Rack).
//
//   render previews <pack.json> <outdir> [all | <lesson> …] [--dark] [--sheet]
//
// For each lesson: every item of every block in its module (page, note, figure, quiz in three
// states, task brief and result, bench), plus the Navigator (home, lesson, map), the docked
// panel (open, collapsed, map) and the Tutor module with a sample conversation. Writes PNGs,
// a per-lesson contact sheet of the rows (rows.png: the blocks laid out as in the patch, Atlas
// modules only) and `report.json` (pages and overflow of every text area).
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <set>
#include <string>
#include <vector>

#include "HarnessFonts.hpp"
#include "SoftNvg.hpp"
#include "check/Checker.hpp"
#include "core/Pack.hpp"
#include "core/Progress.hpp"
#include "core/Report.hpp"
#include "views/NavView.hpp"
#include "views/PageView.hpp"
#include "views/QuizView.hpp"
#include "views/SmallViews.hpp"
#include "views/TaskView.hpp"
#include "views/TutorView.hpp"

using namespace atlas;

namespace {

const float H = 380.f;
std::map<std::string, int> truncatedFigures;  // figure item → labels cut with «…»
std::string memoPath;  // res/memo/notes-voltage.md next to the pack's res/course

struct Canvas {
	NVGcontext* vg;
	Style st;
	Canvas(float w, float h, bool dark, int part, float dpr = 2.f) {
		vg = softCreate((int) w, (int) h, dpr, 2);
		st = makeStyle(dark, part, harnessFonts(vg));
		softClear(vg, dark ? nvgRGB(0x10, 0x10, 0x12) : nvgRGB(0xC9, 0xC2, 0xB6));
		softBegin(vg);
	}
	bool write(const std::string& path) {
		softEnd(vg);
		bool ok = softWritePng(vg, path);
		softDelete(vg);
		return ok;
	}
};

float hpOf(const Item& it, float def) { return (it.width > 0 ? it.width : def) * 15.f; }

/** Draws one Atlas item as its module at (x, 0); returns its width. */
float drawItem(NVGcontext* vg, const Style& st, const Pack& pk, const Lesson& L, const Block& B, const Item& it, float x, int variant, int noteNumber) {
	Pointer p;
	Hits hits;
	nvgSave(vg);
	nvgTranslate(vg, x, 0);
	float w = 0.f;
	std::string header = blockHeader(&L, &B);
	std::string report = L.id + "/" + B.id + "/" + it.id;
	if (it.kind == "page") {
		w = hpOf(it, 24);
		PageView v;
		v.header = it.title ? partHeader(pk, &L) : header;
		v.text.md = it.text;
		v.text.title = it.title;
		v.text.reportId = report;
		v.draw(vg, st, w, H, p, hits);
	}
	else if (it.kind == "note") {
		w = hpOf(it, 8);
		NoteView v;
		v.header = header;
		v.number = noteNumber;
		v.hasTargets = !it.targets.empty();
		v.text.md = it.text;
		v.text.reportId = report;
		v.draw(vg, st, w, H, p, hits);
	}
	else if (it.kind == "figure") {
		w = hpOf(it, 16);
		FigureView v;
		v.header = header;
		v.figure = it.figure;
		v.caption = it.caption;
		v.draw(vg, st, w, H);
	}
	else if (it.kind == "quiz") {
		w = hpOf(it, 14);
		QuizView v;
		QuizState s;
		s.fit(it.questions.size());
		if (variant == 1 && !it.questions.empty())
			s.answers[0].chosen = it.questions[0].options.size() > 1 ? (it.questions[0].options[0].correct ? 1 : 0) : 0;
		if (variant == 2 && !it.questions.empty())
			for (size_t k = 0; k < it.questions[0].options.size(); k++)
				if (it.questions[0].options[k].correct)
					s.answers[0].chosen = (int) k;
		if (variant == 3)
			s.freeMode = true;
		v.header = header;
		v.questions = &it.questions;
		v.state = &s;
		if (variant == 3) {
			v.input = "Частота стала больше, поэтому звук выше";
			v.cursor = v.selection = (int) v.input.size();
			v.focused = true;
		}
		v.draw(vg, st, w, H, p, hits);
		reportText(report + (variant ? "#" + std::to_string(variant) : ""), TextReport{1, v.scroll.active(), false, w});
	}
	else if (it.kind == "task") {
		w = hpOf(it, 14);
		TaskView v;
		TaskState s;
		if (variant >= 1 && it.check) {
			check::Patch empty;
			s.result = check::run(it.check, empty, nullptr);
			s.hasResult = true;
			s.page = 1;
			if (variant == 2)
				s.result.passed = true, s.result.items.clear(), s.result.items.push_back({"module:x", true, "Всё на месте", ""});
		}
		v.header = header;
		v.item = &it;
		v.title = B.title;
		v.state = &s;
		v.draw(vg, st, w, H, p, hits);
		reportText(report + (variant ? "#" + std::to_string(variant) : ""), TextReport{1, v.scroll.active(), false, w});
	}
	else if (it.kind == "bench") {
		w = 30.f;
		BenchView v;
		v.left = it.side != "right";
		v.lessonNum = L.num;
		v.draw(vg, st, w, H);
	}
	nvgRestore(vg);
	(void) pk;
	return w;
}

std::vector<TutorMessage> sampleConversation() {
	std::vector<TutorMessage> m;
	TutorMessage a;
	a.role = "learner";
	a.text = "Почему пила звучит ярче синуса?";
	m.push_back(a);
	TutorMessage b;
	b.role = "tutor";
	b.local = true;
	b.text = "В пиле много **гармоник** — призвуков, кратных основной частоте. В синусе только одна. Больше гармоник — ярче тембр.";
	m.push_back(b);
	TutorMessage c;
	c.role = "learner";
	c.text = "А если поставить фильтр после пилы?";
	m.push_back(c);
	TutorMessage d;
	d.role = "tutor";
	d.streaming = true;
	d.text = "Фильтр уберёт верхние гармоники, и пила станет мягче — ближе к синусу. Покрути:";
	m.push_back(d);
	return m;
}

int renderLesson(const Pack& pk, const Lesson& L, const std::string& out, bool dark) {
	int n = 0;
	std::string sfx = dark ? "-dark" : "";
	// Every item on its own.
	for (const Block& B : L.blocks) {
		int note = 0;
		for (const Item& it : B.items) {
			if (it.kind == "note")
				note++;
			std::vector<int> variants = {0};
			if (it.kind == "quiz")
				variants = {0, 1, 2, 3};
			if (it.kind == "task" && it.check)
				variants = {0, 1, 2};
			for (int v : variants) {
				float w = it.kind == "bench" ? 30.f : hpOf(it, it.kind == "note" ? 8 : it.kind == "page" ? 24 : it.kind == "figure" ? 16 : 14);
				Canvas c(w, H, dark, L.part);
				int before = ellipsizedCount() + hiddenEdgeCount();
				drawItem(c.vg, c.st, pk, L, B, it, 0, v, note);
				if (it.kind == "figure" && ellipsizedCount() + hiddenEdgeCount() > before)
					truncatedFigures[L.id + "/" + B.id + "/" + it.id] = ellipsizedCount() + hiddenEdgeCount() - before;
				std::string name = out + "/" + L.id + "-" + B.id + "-" + it.id + (v ? "-v" + std::to_string(v) : "") + sfx + ".png";
				c.write(name);
				n++;
			}
		}
	}
	// Rows: Atlas modules of each block as laid out (standard modules shown as gaps).
	float rowsW = 0.f;
	for (const Block& B : L.blocks) {
		float x = 0.f;
		for (const Item& it : B.items)
			x += (it.kind == "bench" ? 30.f : hpOf(it, it.kind == "note" ? 8 : it.kind == "page" ? 24 : it.kind == "figure" ? 16 : 14)) + 15.f;
		rowsW = std::max(rowsW, x);
	}
	{
		Canvas c(std::max(rowsW, 200.f), H * L.blocks.size(), dark, L.part, 0.75f);
		for (size_t r = 0; r < L.blocks.size(); r++) {
			const Block& B = L.blocks[r];
			nvgSave(c.vg);
			nvgTranslate(c.vg, 0, H * r);
			float x = 0.f;
			int note = 0;
			for (const Item& it : B.items) {
				if (it.kind == "note")
					note++;
				x += drawItem(c.vg, c.st, pk, L, B, it, x, 0, note) + 15.f;
			}
			nvgRestore(c.vg);
		}
		c.write(out + "/" + L.id + "-rows" + sfx + ".png");
		n++;
	}
	return n;
}

int renderShell(const Pack& pk, const std::string& lesson, const std::string& out, bool dark) {
	const Lesson* L = pk.lesson(lesson);
	int part = L ? L->part : 0;
	std::string sfx = dark ? "-dark" : "";
	Progress pr;
	std::vector<std::string> order = pk.order();
	for (size_t i = 0; i < order.size() && i < 3; i++)
		pr.lessons[order[i]].status = "learned";
	if (L) {
		pr.reach(L->id, L->blocks.size() > 1 ? L->blocks[1].id : "", 1);
		pr.reach(L->id, L->blocks.empty() ? "" : L->blocks[0].id, 1);
	}
	NavState s;
	s.pack = &pk;
	s.progress = &pr;
	s.lesson = lesson;
	s.block = 1;
	s.connected = true;
	s.panelExists = true;
	int n = 0;
	const char* modes[] = {"home", "lesson", "map"};
	for (int m = 0; m < 3; m++) {
		Canvas c(150.f, H, dark, part);
		NavigatorView v;
		v.mode = (NavMode) m;
		Pointer p;
		Hits hits;
		NavState ss = s;
		if (m == 0)
			ss.lesson = "";
		v.draw(c.vg, c.st, 150.f, H, ss, p, hits);
		c.write(out + "/navigator-" + modes[m] + sfx + ".png");
		n++;
	}
	std::vector<TutorMessage> conv = sampleConversation();
	for (int m = 0; m < 3; m++) {
		float w = m == 1 ? PanelView::STRIP : PanelView::WIDTH;
		Canvas c(w, 820.f, dark, part);
		PanelView v;
		v.map = m == 2;
		v.tutor.messages = &conv;
		const Block* B = L && L->blocks.size() > 1 ? &L->blocks[1] : nullptr;
		v.tutor.faq = B ? &B->faq : nullptr;
		v.tutor.time = 0.3f;
		Pointer p;
		Hits hits;
		v.draw(c.vg, c.st, w, 820.f, s, m == 1, p, hits);
		c.write(out + std::string("/panel-") + (m == 0 ? "open" : m == 1 ? "collapsed" : "map") + sfx + ".png");
		n++;
	}
	{
		float w = 18 * 15.f;
		Canvas c(w, H, dark, part);
		Rect r = drawPanel(c.vg, c.st, w, H, L ? L->num + " · Репетитор" : "Репетитор");
		TutorView v;
		v.messages = &conv;
		const Block* B = L && L->blocks.size() > 1 ? &L->blocks[1] : nullptr;
		v.faq = B ? &B->faq : nullptr;
		v.input = "";
		v.time = 0.3f;
		Pointer p;
		Hits hits;
		v.draw(c.vg, c.st, r, p, hits);
		c.write(out + "/tutor-module" + sfx + ".png");
		n++;
	}
	// Памятка: every page at its default width (the same PageView as the Memo module).
	std::string md;
	if (FILE* f = std::fopen(memoPath.c_str(), "rb")) {
		char buf[4096];
		size_t k;
		while ((k = std::fread(buf, 1, sizeof(buf), f)) > 0)
			md.append(buf, k);
		std::fclose(f);
	}
	for (int pg = 0; !md.empty() && pg < 40; pg++) {
		float w = 20 * 15.f;
		Canvas c(w, H, dark, 0);
		PageView v;
		v.header = "Памятка · звук и вольты";
		v.text.md = md;
		v.text.title = true;
		v.text.page = pg;
		v.text.reportId = "memo/notes-voltage";
		Pointer p;
		Hits hits;
		v.draw(c.vg, c.st, w, H, p, hits);
		c.write(out + "/memo-p" + std::to_string(pg + 1) + sfx + ".png");
		n++;
		if (pg + 1 >= v.text.count())
			break;
	}
	return n;
}

} // namespace

int previewsMain(int argc, char** argv) {
	std::string packPath = argv[2], out = argv[3];
	{
		std::string dir = packPath.substr(0, packPath.find_last_of('/'));  // …/res/course
		memoPath = dir.substr(0, dir.find_last_of('/')) + "/memo/notes-voltage.md";
	}
	bool dark = false;
	std::vector<std::string> lessons;
	for (int i = 4; i < argc; i++) {
		if (std::strcmp(argv[i], "--dark") == 0)
			dark = true;
		else if (std::strcmp(argv[i], "all") != 0)
			lessons.push_back(argv[i]);
	}
	Pack pk;
	if (!pk.loadFile(packPath)) {
		std::fprintf(stderr, "%s\n", pk.error.c_str());
		return 1;
	}
	if (lessons.empty())
		lessons = pk.order();
	resetTextReports();
	int n = 0;
	for (const std::string& id : lessons) {
		const Lesson* L = pk.lesson(id);
		if (!L) {
			std::fprintf(stderr, "no lesson %s\n", id.c_str());
			continue;
		}
		n += renderLesson(pk, *L, out, dark);
	}
	n += renderShell(pk, lessons.empty() ? "" : lessons[0], out, dark);
	// Report of every text area: pages, overflow, hyphenated words.
	json_t* r = json_object();
	int over = 0, multi = 0;
	for (const auto& kv : textReports()) {
		json_t* e = json_object();
		json_object_set_new(e, "pages", json_integer(kv.second.pages));
		json_object_set_new(e, "overflow", json_boolean(kv.second.overflow));
		json_object_set_new(e, "splitWords", json_boolean(kv.second.splitWords));
		json_object_set_new(e, "width", json_real(kv.second.width));
		json_object_set_new(r, kv.first.c_str(), e);
		over += kv.second.overflow;
		multi += kv.second.pages > 1;
	}
	for (const auto& kv : truncatedFigures) {
		json_t* e = json_object();
		json_object_set_new(e, "overflow", json_false());
		json_object_set_new(e, "truncated", json_integer(kv.second));
		json_object_set_new(r, kv.first.c_str(), e);
	}
	json_dump_file(r, (out + (dark ? "/report-dark.json" : "/report.json")).c_str(), JSON_INDENT(1) | JSON_SORT_KEYS);
	json_decref(r);
	std::printf("%d images; %zu text areas, %d with several pages, %d overflowing or scrolling; %zu figures with cut labels or hidden edges\n", n, textReports().size(), multi, over, truncatedFigures.size());
	return 0;
}
