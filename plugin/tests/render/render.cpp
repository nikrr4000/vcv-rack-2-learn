// Render harness CLI (visual QA without Rack, see docs/rack/PROGRESS.md):
//   render gallery <outdir>                 every figure template, light and dark
//   render previews <pack.json> <outdir> [<spec>…]   module previews from the pack (see previews.cpp)
#include <cstdio>
#include <cstring>
#include <string>

#include "HarnessFonts.hpp"
#include "SoftNvg.hpp"
#include "figures/Figures.hpp"
#include "samples.hpp"
#include "ui/Draw.hpp"

using namespace atlas;

int previewsMain(int argc, char** argv);
int layoutMain(int argc, char** argv);

static int gallery(const std::string& out) {
	int n = 0;
	for (const char* spec : FIGURE_SAMPLES) {
		json_t* j = json_loads(spec, 0, nullptr);
		std::string name = json_string_value(json_object_get(j, "template"));
		for (int dark = 0; dark < 2; dark++) {
			float w = 240.f, h = std::max(90.f, w * figures::aspect(j)) + 40.f;
			NVGcontext* vg = softCreate((int) w, (int) h);
			text::Fonts f = harnessFonts(vg);
			Style st = makeStyle(dark, 3, f);
			softClear(vg, st.pal.panel);
			softBegin(vg);
			Rect c = drawPanel(vg, st, w, h, "Схема · " + name);
			figures::draw(vg, st, c, j);
			softEnd(vg);
			std::string path = out + "/figure-" + name + (dark ? "-dark" : "-light") + ".png";
			if (!softWritePng(vg, path))
				std::fprintf(stderr, "cannot write %s\n", path.c_str());
			softDelete(vg);
			n++;
		}
		json_decref(j);
	}
	// Contact sheets: all templates on one canvas per theme.
	for (int dark = 0; dark < 2; dark++) {
		const int cols = 4;
		const float cw = 240.f, ch = 200.f;
		int count = (int) (sizeof(FIGURE_SAMPLES) / sizeof(FIGURE_SAMPLES[0]));
		int rows = (count + cols - 1) / cols;
		NVGcontext* vg = softCreate((int) (cw * cols), (int) (ch * rows), 1.5f, 2);
		text::Fonts f = harnessFonts(vg);
		Style st = makeStyle(dark, 3, f);
		softClear(vg, st.pal.panel);
		softBegin(vg);
		for (int i = 0; i < count; i++) {
			json_t* j = json_loads(FIGURE_SAMPLES[i], 0, nullptr);
			nvgSave(vg);
			nvgTranslate(vg, cw * (i % cols), ch * (i / cols));
			Rect c = drawPanel(vg, st, cw, ch, std::string("Схема · ") + json_string_value(json_object_get(j, "template")));
			figures::draw(vg, st, c, j);
			nvgRestore(vg);
			json_decref(j);
		}
		softEnd(vg);
		softWritePng(vg, out + (dark ? "/gallery-dark.png" : "/gallery-light.png"));
		softDelete(vg);
	}
	std::printf("%d images + 2 contact sheets\n", n);
	return 0;
}

int main(int argc, char** argv) {
	if (argc >= 3 && std::strcmp(argv[1], "gallery") == 0)
		return gallery(argv[2]);
	if (argc >= 4 && std::strcmp(argv[1], "layout") == 0)
		return layoutMain(argc, argv);
	if (argc >= 4 && std::strcmp(argv[1], "previews") == 0)
		return previewsMain(argc, argv);
	std::fprintf(stderr, "usage: render gallery <outdir> | render previews <pack.json> <outdir> [spec…]\n");
	return 2;
}
