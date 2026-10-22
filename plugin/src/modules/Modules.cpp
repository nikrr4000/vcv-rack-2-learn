#include "Modules.hpp"

#include "host/Host.hpp"
#if defined(__has_include)
#if __has_include("BuildInfo.hpp")
#include "BuildInfo.hpp"
#endif
#endif
#ifndef ATLAS_BUILD
#define ATLAS_BUILD "dev"
#endif

namespace atlas {

void NavigatorModule::saveState(json_t* s) {
	json_object_set_new(s, "layoutHash", json_string(layoutHash.c_str()));
	json_object_set_new(s, "blockIndex", json_integer(blockIndex));
}

void NavigatorModule::loadState(json_t* s) {
	json_t* h = json_object_get(s, "layoutHash");
	if (h && json_is_string(h))
		layoutHash = json_string_value(h);
	json_t* b = json_object_get(s, "blockIndex");
	if (b && json_is_number(b))
		blockIndex = (int) json_number_value(b);
}

void QuizModule::saveState(json_t* s) {
	json_object_set_new(s, "current", json_integer(state.current));
	json_t* a = json_array();
	for (const QuizAnswer& x : state.answers) {
		json_t* o = json_object();
		json_object_set_new(o, "chosen", json_integer(x.chosen));
		if (!x.freeText.empty())
			json_object_set_new(o, "free", json_string(x.freeText.c_str()));
		if (!x.verdict.empty())
			json_object_set_new(o, "verdict", json_string(x.verdict.c_str()));
		json_array_append_new(a, o);
	}
	json_object_set_new(s, "answers", a);
}

void QuizModule::loadState(json_t* s) {
	json_t* c = json_object_get(s, "current");
	if (c && json_is_number(c))
		state.current = (int) json_number_value(c);
	state.answers.clear();
	size_t i;
	json_t* o;
	json_array_foreach(json_object_get(s, "answers"), i, o) {
		QuizAnswer x;
		json_t* v = json_object_get(o, "chosen");
		if (v && json_is_number(v))
			x.chosen = (int) json_number_value(v);
		if ((v = json_object_get(o, "free")) && json_is_string(v))
			x.freeText = json_string_value(v);
		if ((v = json_object_get(o, "verdict")) && json_is_string(v))
			x.verdict = json_string_value(v);
		state.answers.push_back(x);
	}
}

void TaskModule::saveState(json_t* s) {
	json_t* steps = json_array();
	for (bool b : state.stepsDone)
		json_array_append_new(steps, json_boolean(b));
	json_object_set_new(s, "steps", steps);
	if (state.hasResult) {
		json_object_set_new(s, "passed", json_boolean(state.result.passed));
	}
	json_t* hl = json_object();
	for (const auto& kv : state.hintLevel)
		json_object_set_new(hl, kv.first.c_str(), json_integer(kv.second));
	json_object_set_new(s, "hintLevel", hl);
}

void TaskModule::loadState(json_t* s) {
	state.stepsDone.clear();
	size_t i;
	json_t* v;
	json_array_foreach(json_object_get(s, "steps"), i, v) state.stepsDone.push_back(json_is_true(v));
	const char* k;
	json_object_foreach(json_object_get(s, "hintLevel"), k, v) if (json_is_number(v)) state.hintLevel[k] = (int) json_number_value(v);
}

void AtlasField::onSelectKey(const SelectKeyEvent& e) {
	if (e.action == GLFW_PRESS || e.action == GLFW_REPEAT) {
		if (e.key == GLFW_KEY_ENTER || e.key == GLFW_KEY_KP_ENTER) {
			if (e.mods & GLFW_MOD_SHIFT)
				insertText("\n");
			else if (onSubmit && !text.empty()) {
				std::string t = text;
				setText("");
				onSubmit(t);
			}
			e.consume(this);
			return;
		}
	}
	ui::TextField::onSelectKey(e);
}

void AtlasField::onSelectText(const SelectTextEvent& e) {
	ui::TextField::onSelectText(e);
}

/** «Адрес Atlas» in the context menus of the Navigator and the Tutor. */
struct UrlField : ui::TextField {
	UrlField() {
		box.size.x = 240;
		text = settings().atlasUrl;
	}
	void onChange(const ChangeEvent& e) override {
		settings().atlasUrl = text;
		settings().save();
	}
};

void appendAtlasMenu(ui::Menu* menu) {
	menu->addChild(new ui::MenuSeparator);
	// Which build is loaded (Rack reads plugin.dylib only at start): version · commit · build time.
	menu->addChild(createMenuLabel("Atlas " + pluginInstance->version + " · сборка " + ATLAS_BUILD));
	menu->addChild(createMenuLabel("Адрес Atlas (ATLAS_URL в окружении важнее)"));
	menu->addChild(new UrlField);
	menu->addChild(createBoolMenuItem("Боковая панель", "", []() { return settings().panelOpen; }, [](bool v) {
		if (v != settings().panelOpen && host())
			host()->togglePanel();
	}));
}

} // namespace atlas
