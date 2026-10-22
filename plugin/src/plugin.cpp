#include "atlas.hpp"

#include <cstdlib>

Plugin* pluginInstance;

void init(Plugin* p) {
	pluginInstance = p;
	p->addModel(modelNavigator);
	p->addModel(modelPage);
	p->addModel(modelNote);
	p->addModel(modelFigure);
	p->addModel(modelQuiz);
	p->addModel(modelTask);
	p->addModel(modelBench);
	p->addModel(modelTutor);
	p->addModel(modelMemo);
	atlas::settings().load();
}

namespace atlas {

const Pack& pack() {
	static Pack* p = nullptr;
	if (!p) {
		p = new Pack;
		std::string path = asset::plugin(pluginInstance, "res/course/pack.json");
		if (const char* env = std::getenv("ATLAS_PACK"))
			path = env;
		if (!p->loadFile(path))
			WARN("Atlas: %s", p->error.c_str());
		else
			INFO("Atlas: course pack %s, %d lessons", path.c_str(), (int) p->lessons.size());
	}
	return *p;
}

Settings& settings() {
	static Settings s;
	return s;
}

void Settings::load() {
	std::string path = asset::user("Atlas/settings.json");
	json_error_t err;
	json_t* r = json_load_file(path.c_str(), 0, &err);
	if (!r)
		return;
	json_t* u = json_object_get(r, "atlasUrl");
	if (u && json_is_string(u))
		atlasUrl = json_string_value(u);
	json_t* po = json_object_get(r, "panelOpen");
	if (po)
		panelOpen = json_is_true(po);
	json_t* ps = json_object_get(r, "panelScale");
	if (ps && json_is_number(ps))
		panelScale = (float) json_number_value(ps);
	json_decref(r);
}

void Settings::save() {
	system::createDirectories(asset::user("Atlas"));
	json_t* r = json_object();
	json_object_set_new(r, "atlasUrl", json_string(atlasUrl.c_str()));
	json_object_set_new(r, "panelOpen", json_boolean(panelOpen));
	json_object_set_new(r, "panelScale", json_real(panelScale));
	json_dump_file(r, asset::user("Atlas/settings.json").c_str(), JSON_INDENT(2));
	json_decref(r);
}

std::string Settings::url() const {
	std::string u = atlasUrl;
	if (const char* env = std::getenv("ATLAS_URL"))
		u = env;
	while (!u.empty() && u.back() == '/')
		u.pop_back();
	return u;
}

std::string previewRef(const std::string& modelSlug) {
	const char* env = std::getenv("ATLAS_PREVIEW");
	if (!env)
		return "";
	std::string s = env;
	size_t pos = 0;
	while (pos < s.size()) {
		size_t end = s.find(';', pos);
		if (end == std::string::npos)
			end = s.size();
		std::string part = s.substr(pos, end - pos);
		size_t eq = part.find('=');
		if (eq != std::string::npos && part.substr(0, eq) == modelSlug)
			return part.substr(eq + 1);
		pos = end + 1;
	}
	return "";
}

std::vector<ItemRef> itemsOfKind(const std::string& kind) {
	std::vector<ItemRef> out;
	const Pack& p = pack();
	for (const std::string& id : p.order()) {
		const Lesson* l = p.lesson(id);
		for (const Block& b : l->blocks)
			for (const Item& it : b.items)
				if (it.kind == kind)
					out.push_back({id, b.id, it.id});
	}
	return out;
}

} // namespace atlas
