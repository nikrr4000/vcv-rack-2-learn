// Plugin smoke test without a Rack window: links libRack from the SDK, loads plugin.dylib,
// runs init(), creates every module and round-trips its data through dataToJson/dataFromJson.
#include <dlfcn.h>

#include <cstdio>
#include <rack.hpp>

using namespace rack;

int main(int argc, char** argv) {
	setvbuf(stdout, nullptr, _IONBF, 0);
	const char* path = argc > 1 ? argv[1] : "rack/plugin/plugin.dylib";
	void* h = dlopen(path, RTLD_NOW | RTLD_LOCAL);
	if (!h) {
		std::printf("FAIL dlopen: %s\n", dlerror());
		return 1;
	}
	typedef void (*InitFn)(plugin::Plugin*);
	InitFn init = (InitFn) dlsym(h, "init");
	if (!init) {
		std::printf("FAIL no init symbol\n");
		return 1;
	}
	plugin::Plugin* p = new plugin::Plugin;
	p->slug = "Atlas";
	p->path = "rack/plugin";
	init(p);
	int fails = 0;
	for (plugin::Model* m : p->models) {
		engine::Module* mod = m->createModule();
		if (!mod) {
			std::printf("FAIL %s: createModule\n", m->slug.c_str());
			fails++;
			continue;
		}
		json_t* d = json_object();
		json_object_set_new(d, "lesson", json_string("p1-01-sound-wave"));
		json_object_set_new(d, "block", json_string("b2"));
		json_object_set_new(d, "item", json_string("page-1"));
		json_object_set_new(d, "width", json_integer(20));
		mod->dataFromJson(d);
		json_t* out = mod->dataToJson();
		bool ok = out && std::string(json_string_value(json_object_get(out, "lesson"))) == "p1-01-sound-wave" && json_integer_value(json_object_get(out, "width")) == 20;
		std::printf("%s %s (%s): params %zu, data round-trip\n", ok ? "ok  " : "FAIL", m->slug.c_str(), m->name.c_str(), mod->params.size());
		fails += !ok;
		json_decref(d);
		if (out)
			json_decref(out);
		// The widget with its module and the preview widget (module == nullptr).
		app::ModuleWidget* mw = m->createModuleWidget(mod);
		app::ModuleWidget* pw = m->createModuleWidget(nullptr);
		bool wok = mw && pw && mw->box.size.y == RACK_GRID_HEIGHT && pw->box.size.x >= RACK_GRID_WIDTH;
		std::printf("%s %s widget %.0f×%.0f, preview %.0f HP\n", wok ? "ok  " : "FAIL", m->slug.c_str(), mw ? mw->box.size.x : 0.f, mw ? mw->box.size.y : 0.f, pw ? pw->box.size.x / RACK_GRID_WIDTH : 0.f);
		fails += !wok;
		if (mw)
			mw->module = nullptr;  // no engine here: the widget must not remove the module from it
		// Widgets are leaked on purpose: deleting them needs APP->event, which a windowless test has not.
		delete mod;
	}
	std::printf("%zu models, %d failures\n", p->models.size(), fails);
	return fails ? 1 : 0;
}
