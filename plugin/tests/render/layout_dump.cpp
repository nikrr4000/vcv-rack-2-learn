// `render layout <corpus.json> <out.json>`: lays out each corpus entry ({md, width, height, title})
// with the plugin's engine and real nanovg measuring; writes the line texts and page counts so the
// compiler's text-fit tool can be checked against it (parity test).
#include <jansson.h>

#include <cstdio>
#include <string>

#include "HarnessFonts.hpp"
#include "SoftNvg.hpp"
#include "ui/Style.hpp"

using namespace atlas;

int layoutMain(int argc, char** argv) {
	json_error_t err;
	json_t* corpus = json_load_file(argv[2], 0, &err);
	if (!corpus) {
		std::fprintf(stderr, "%s\n", err.text);
		return 1;
	}
	NVGcontext* vg = softCreate(64, 64);
	Style st = makeStyle(false, 0, harnessFonts(vg));
	softBegin(vg);
	text::NvgMeasurer m(vg, st.fonts);
	json_t* out = json_array();
	size_t i;
	json_t* e;
	json_array_foreach(corpus, i, e) {
		std::string md = json_string_value(json_object_get(e, "md"));
		float w = (float) json_number_value(json_object_get(e, "width"));
		float h = (float) json_number_value(json_object_get(e, "height"));
		bool title = json_is_true(json_object_get(e, "title"));
		text::TextStyle ts = st.textStyle(title);
		text::Document d = text::layout(text::parse(md), w, m, ts);
		json_t* lines = json_array();
		for (const text::LaidBlock& b : d.blocks)
			for (const text::Line& l : b.lines) {
				std::string t;
				for (const text::Run& r : l.runs)
					t += r.text;
				json_array_append_new(lines, json_string(t.c_str()));
			}
		json_t* r = json_object();
		json_object_set_new(r, "lines", lines);
		json_object_set_new(r, "pages", json_integer((int) text::paginate(d, h, ts).size()));
		json_array_append_new(out, r);
	}
	softEnd(vg);
	json_dump_file(out, argv[3], JSON_INDENT(1));
	std::printf("%zu entries\n", json_array_size(out));
	return 0;
}
