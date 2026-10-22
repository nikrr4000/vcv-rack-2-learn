#include "Host.hpp"

#include <algorithm>
#include <cmath>

#include "Panel.hpp"
#include "core/Report.hpp"
#include <patch.hpp>
#include "check/RackPatch.hpp"
#include "modules/Modules.hpp"
#include "text/Markdown.hpp"

namespace atlas {

AtlasHost* AtlasHost::instance = nullptr;

DevBridge* startDevBridge(AtlasHost* host);
void stopDevBridge(DevBridge* b);

void ensureHost() {
	if (AtlasHost::instance || !APP || !APP->scene)
		return;
	AtlasHost* h = new AtlasHost;
	if (APP->scene->menuBar)
		APP->scene->addChildBelow(h, APP->scene->menuBar);
	else
		APP->scene->addChild(h);
}

AtlasHost::AtlasHost() {
	instance = this;
	system::createDirectories(asset::user("Atlas/patches/old"));
	progress.load(asset::user("Atlas/progress.json"));
	client.setUrl(settings().url());
	client.requestProgress();
	panel = new DockPanel;
	addChild(panel);
	overlay = new PointerOverlay;
	if (const char* port = std::getenv("ATLAS_DEV_PORT")) {
		(void) port;
		bridge = startDevBridge(this);
	}
}

AtlasHost::~AtlasHost() {
	if (bridge)
		stopDevBridge(bridge);
	if (overlay && overlay->parent)
		overlay->parent->removeChild(overlay);
	delete overlay;
	if (instance == this)
		instance = nullptr;
}

void AtlasHost::post(std::function<void()> fn) {
	std::lock_guard<std::mutex> lock(queueMutex);
	queue.push_back(std::move(fn));
}

std::string AtlasHost::userPatchPath(const std::string& id) const {
	return asset::user("Atlas/patches/" + id + ".vcv");
}

const Block* AtlasHost::currentBlock() const {
	const Lesson* L = currentLesson();
	if (!L || L->blocks.empty())
		return nullptr;
	return &L->blocks[std::max(0, std::min((int) L->blocks.size() - 1, block))];
}

std::vector<TutorMessage>& AtlasHost::conversation(const std::string& id) {
	return conversations[id];
}

NavState AtlasHost::navState() const {
	NavState s;
	s.pack = &pack();
	s.progress = &progress;
	s.lesson = lesson;
	s.block = block;
	s.connected = const_cast<Client&>(client).connected.load();
	s.panelOpen = settings().panelOpen;
	s.panelExists = navigatorId >= 0;
	s.notice = notice;
	return s;
}

NavigatorModule* AtlasHost::findNavigator() {
	for (int64_t id : APP->engine->getModuleIds()) {
		NavigatorModule* n = dynamic_cast<NavigatorModule*>(APP->engine->getModule(id));
		if (n)
			return n;
	}
	return nullptr;
}

/** Follows the patch: which lesson is open, which block, the overlay in the rack. */
void AtlasHost::sync() {
	NavigatorModule* n = findNavigator();
	int64_t id = n ? n->id : -1;
	if (id != navigatorId) {
		navigatorId = id;
		std::string next = n ? n->lesson : "";
		if (next != lesson) {
			lesson = next;
			mapOpen = false;
		}
		if (n && pack().lesson(lesson)) {
			block = n->blockIndex;
			startGlide(block, false);
			applyMixer();
		}
	}
	if (n && n->lesson != lesson) {
		lesson = n->lesson;
		block = n->blockIndex;
	}
	if (APP->scene->rack && overlay->parent != APP->scene->rack) {
		if (overlay->parent)
			overlay->parent->removeChild(overlay);
		APP->scene->rack->addChild(overlay);
	}
}

void AtlasHost::step() {
	if (parent)
		box = parent->box.zeroPos();
	frame++;
	framesSinceGoto++;
	std::vector<std::function<void()>> run;
	{
		std::lock_guard<std::mutex> lock(queueMutex);
		run.swap(queue);
	}
	for (auto& fn : run)
		fn();
	if (frame % 6 == 1)
		sync();
	client.setUrl(settings().url());
	stepClient();
	stepGlide();
	stepChecks();
	if (frame % 10 == 0)
		publishSnapshot();
	double now = system::getTime();
	if (now - lastProgressPull > 60.0) {
		lastProgressPull = now;
		client.requestProgress();
	}
	Widget::step();
}

void AtlasHost::draw(const DrawArgs& args) {
	Widget::draw(args);
}

// ── lessons ──

void AtlasHost::openLesson(const std::string& id) {
	post([this, id]() { doOpenLesson(id); });
}

/** Top of a lesson row in rack coordinates: Rack loads `pos` as pos * RACK_GRID_SIZE + RACK_OFFSET. */
static float rowTop(int row) {
	return RACK_OFFSET.y + row * RACK_GRID_HEIGHT;
}

static json_t* audioData() {
	for (int64_t id : APP->engine->getModuleIds()) {
		engine::Module* m = APP->engine->getModule(id);
		if (m && m->model && m->model->plugin && m->model->plugin->slug == "Core" && m->model->slug == "AudioInterface2")
			return m->dataToJson();
	}
	return nullptr;
}

void AtlasHost::doOpenLesson(const std::string& id) {
	const Lesson* L = pack().lesson(id);
	if (!L) {
		notice = "Урок «" + id + "» не найден в курсе.";
		return;
	}
	// 1. Keep the current patch: a course patch goes to its learner copy, anything else aside.
	NavigatorModule* nav = findNavigator();
	bool empty = APP->engine->getModuleIds().size() <= (nav ? 1u : 0u);
	if (nav && pack().lesson(nav->lesson)) {
		try {
			APP->patch->save(userPatchPath(nav->lesson));
			progress.lessons[nav->lesson].updatedAt = system::getUnixTime();
		}
		catch (Exception& e) {
			WARN("Atlas: cannot save %s: %s", nav->lesson.c_str(), e.what());
		}
	}
	else if (!empty) {
		std::string aside = asset::user("Atlas/patches/_previous-" + string::formatTimeISO(system::getUnixTime()).substr(0, 19) + ".vcv");
		for (char& c : aside)
			if (c == ':')
				c = '-';
		try {
			APP->patch->save(aside);
			notice = "Прежний патч сохранён: " + aside;
		}
		catch (Exception& e) {
			notice = std::string("Прежний патч не сохранён: ") + e.what();
		}
	}
	// 2. The audio device to carry over.
	json_t* audio = audioData();
	// 3. Source: the learner copy if it matches the current layout, else the template.
	std::string user = userPatchPath(id);
	std::string tpl = asset::plugin(pluginInstance, "res/course/" + L->patch);
	std::string source = tpl;
	auto pit = progress.lessons.find(id);
	if (system::exists(user)) {
		std::string hashFile = asset::user("Atlas/patches/" + id + ".hash");
		std::string h;
		if (system::exists(hashFile)) {
			std::vector<uint8_t> d = system::readFile(hashFile);
			h = string::trim(std::string(d.begin(), d.end()));
		}
		if (h == L->layoutHash)
			source = user;
		else {
			// An old layout: keep the learner's copy aside, start from the new template.
			std::string old = asset::user("Atlas/patches/old/" + id + "-" + std::to_string((long long) system::getUnixTime()) + ".vcv");
			system::rename(user, old);
		}
	}
	// 4. Every module of the lesson must exist, or Rack would block with a dialog.
	missingPlugins.clear();
	json_error_t err;
	json_t* t = json_load_file(tpl.c_str(), 0, &err);
	if (t) {
		size_t i;
		json_t* m;
		json_array_foreach(json_object_get(t, "modules"), i, m) {
			const char* p = json_string_value(json_object_get(m, "plugin"));
			const char* s = json_string_value(json_object_get(m, "model"));
			if (p && s && !rack::plugin::getModel(p, s)) {
				std::string miss = std::string(p);
				if (std::find(missingPlugins.begin(), missingPlugins.end(), miss) == missingPlugins.end())
					missingPlugins.push_back(miss);
			}
		}
		json_decref(t);
	}
	else {
		notice = "Нет файла урока: " + tpl;
		if (audio)
			json_decref(audio);
		return;
	}
	if (!missingPlugins.empty()) {
		std::string list;
		for (const std::string& p : missingPlugins)
			list += (list.empty() ? "" : ", ") + p;
		notice = "Нужен плагин " + list + ": подпишись в VCV Library и перезапусти Rack.";
		if (audio)
			json_decref(audio);
		return;
	}
	// 5. Load (never loadAction: it shows a dialog on failure), then point Ctrl+S at the learner copy.
	try {
		APP->patch->load(source);
	}
	catch (Exception& e) {
		notice = std::string("Урок не открылся: ") + e.what();
		if (audio)
			json_decref(audio);
		return;
	}
	APP->patch->path = user;
	if (audio) {
		for (int64_t mid : APP->engine->getModuleIds()) {
			engine::Module* m = APP->engine->getModule(mid);
			// Every Audio module of the lesson (the demo bus and the practice rows) gets the learner's device.
			if (m && m->model && m->model->plugin && m->model->plugin->slug == "Core" && m->model->slug == "AudioInterface2")
				m->dataFromJson(audio);
		}
		json_decref(audio);
	}
	try {
		APP->patch->save(user);
		system::writeFile(asset::user("Atlas/patches/" + id + ".hash"), std::vector<uint8_t>(L->layoutHash.begin(), L->layoutHash.end()));
	}
	catch (Exception& e) {
		WARN("Atlas: cannot save %s: %s", user.c_str(), e.what());
	}
	lesson = id;
	navigatorId = -2;  // re-sync on the next step
	NavigatorModule* n = findNavigator();
	if (n) {
		navigatorId = n->id;
		n->lesson = id;
		n->layoutHash = L->layoutHash;
	}
	// 6. Glide to the saved block or block 1.
	int b = 0;
	if (pit != progress.lessons.end() || progress.lessons.count(id)) {
		const LessonProgress& lp = progress.lessons[id];
		int bi = L->blockIndex(lp.block);
		if (bi >= 0)
			b = bi;
	}
	if (n && n->blockIndex > 0 && source == user)
		b = n->blockIndex;
	mapOpen = false;
	if (!notice.empty() && notice.rfind("Прежний", 0) != 0)
		notice.clear();
	gotoBlock(b, false);
	if (progress.status(id).empty())
		progress.lessons[id].status = "learning";
	progress.last = id;
	saveProgress();
	json_t* body = json_object();
	json_object_set_new(body, "lesson", json_string(id.c_str()));
	client.post("/api/rack/lesson/open", body);
}

void AtlasHost::gotoBlock(int index, bool animate) {
	post([this, index, animate]() {
		const Lesson* L = currentLesson();
		if (!L || L->blocks.empty())
			return;
		int b = std::max(0, std::min((int) L->blocks.size() - 1, index));
		block = b;
		NavigatorModule* n = findNavigator();
		if (n)
			n->blockIndex = b;
		startGlide(b, animate);
		applyMixer();
		resetTextReports();
		framesSinceGoto = 0;
		pulseBlock = b;
		pulseStart = system::getTime();
		progress.reach(lesson, L->blocks[b].id, system::getUnixTime());
		saveProgress();
		json_t* d = json_object();
		json_object_set_new(d, "title", json_string(L->blocks[b].title.c_str()));
		event("block", d);
	});
}

void AtlasHost::completeLesson() {
	post([this]() {
		const Lesson* L = currentLesson();
		if (!L)
			return;
		progress.complete(lesson, system::getUnixTime());
		saveProgress();
		event("completed");
		try {
			APP->patch->save(userPatchPath(lesson));
		}
		catch (Exception& e) {
			WARN("Atlas: %s", e.what());
		}
		std::string next = pack().nextLesson(lesson);
		const Lesson* N = pack().lesson(next);
		notice = N ? "Урок пройден. Дальше — " + N->num + " «" + N->title + "»: нажми «Продолжить» в Навигаторе." : "Курс пройден целиком.";
	});
}

void AtlasHost::togglePanel() {
	settings().panelOpen = !settings().panelOpen;
	settings().save();
	post([this]() { startGlide(block, true); });
}

void AtlasHost::zoomPanel(int dir, float to) {
	float s = to > 0.f ? to : std::round((panelScale() + 0.1f * dir) * 10.f) / 10.f;
	s = math::clamp(s, PANEL_SCALE_MIN, PANEL_SCALE_MAX);
	if (s == settings().panelScale)
		return;
	settings().panelScale = s;
	settings().save();
	post([this]() { startGlide(block, true); });
}

void AtlasHost::saveProgress() {
	progress.save(asset::user("Atlas/progress.json"));
}

void AtlasHost::event(const std::string& name, json_t* data) {
	json_t* body = json_object();
	json_object_set_new(body, "lesson", json_string(lesson.c_str()));
	const Block* b = currentBlock();
	if (b)
		json_object_set_new(body, "block", json_string(b->id.c_str()));
	json_object_set_new(body, "event", json_string(name.c_str()));
	json_object_set_new(body, "data", data ? data : json_object());
	json_object_set_new(body, "at", json_real(system::getUnixTime()));
	client.post("/api/rack/progress", body);
}

// ── glide and mixer ──

void AtlasHost::applyMixer() {
	const Lesson* L = currentLesson();
	if (!L)
		return;
	// Only the active block sounds: its channel at its level, every other channel at 0.
	for (size_t i = 0; i < L->blocks.size(); i++) {
		for (const Block::Channel& c : L->blocks[i].channels) {
			engine::Module* m = APP->engine->getModule(c.mixerId);
			if (!m || c.param < 0 || c.param >= (int) m->params.size())
				continue;
			APP->engine->setParamValue(m, c.param, (int) i == block ? c.level : 0.f);
		}
	}
}

math::Rect AtlasHost::blockRect(int index) {
	const Lesson* L = currentLesson();
	math::Rect r;
	if (!L || index < 0 || index >= (int) L->blocks.size())
		return r;
	const Block& b = L->blocks[index];
	float rowY = rowTop(b.row);
	bool any = false;
	for (widget::Widget* w : APP->scene->rack->getModuleContainer()->children) {
		if (std::fabs(w->box.pos.y - rowY) > 1.f)
			continue;
		r = any ? r.expand(w->box) : w->box;
		any = true;
	}
	if (!any)
		r = math::Rect(RACK_OFFSET.plus(math::Vec(b.x, b.y)), math::Vec(std::max(b.w, 300.f), RACK_GRID_HEIGHT));
	return r;
}

void AtlasHost::startGlide(int index, bool animate) {
	app::RackScrollWidget* sc = APP->scene->rackScroll;
	if (!sc)
		return;
	math::Rect bound = blockRect(index);
	if (bound.size.x <= 0)
		return;
	bound = bound.grow(math::Vec(24, 24));
	math::Vec size = sc->getSize();
	float panelW = (navigatorId >= 0 || findNavigator()) ? panelScreenWidth() : 0.f;
	math::Vec avail(size.x - panelW, size.y);
	float zoom = std::min(avail.x / bound.size.x, avail.y / bound.size.y);
	zoom = math::clamp(zoom, 0.5f, 1.8f);
	math::Vec offset = bound.getCenter().mult(zoom).minus(avail.div(2));
	if (!animate) {
		sc->zoomWidget->setZoom(zoom);
		sc->offset = offset;
		gliding = false;
		return;
	}
	fromOffset = sc->offset;
	fromZoom = sc->getZoom();
	toOffset = offset;
	toZoom = zoom;
	glideStart = system::getTime();
	gliding = true;
}

void AtlasHost::stepGlide() {
	if (!gliding)
		return;
	app::RackScrollWidget* sc = APP->scene->rackScroll;
	float t = (float) ((system::getTime() - glideStart) * 1000.0 / tokens::motion_glideMs);
	if (t >= 1.f) {
		t = 1.f;
		gliding = false;
	}
	float e = t < 0.5f ? 2 * t * t : 1 - std::pow(-2 * t + 2, 2.f) / 2;  // ease-in-out
	// Interpolate in rack coordinates (the view center), so zoom and pan move together.
	float z = fromZoom + (toZoom - fromZoom) * e;
	math::Vec size = sc->getSize();
	math::Vec c0 = fromOffset.plus(size.div(2)).div(fromZoom);
	math::Vec c1 = toOffset.plus(size.div(2)).div(toZoom);
	math::Vec c = c0.plus(c1.minus(c0).mult(e));
	sc->zoomWidget->setZoom(z);
	sc->offset = c.mult(z).minus(size.div(2));
}

// ── tutor ──

void AtlasHost::faq(int index) {
	const Block* b = currentBlock();
	if (!b || index < 0 || index >= (int) b->faq.size())
		return;
	std::vector<TutorMessage>& c = conversation(lesson);
	TutorMessage q;
	q.role = "learner";
	q.text = b->faq[index].q;
	c.push_back(q);
	TutorMessage a;
	a.role = "tutor";
	a.local = true;
	a.text = b->faq[index].a;
	c.push_back(a);
	json_t* d = json_object();
	json_object_set_new(d, "q", json_string(q.text.c_str()));
	json_object_set_new(d, "a", json_string(a.text.c_str()));
	json_object_set_new(d, "local", json_true());
	event("tutor", d);
}

void AtlasHost::quick(int kind) {
	const Block* b = currentBlock();
	if (!b)
		return;
	std::vector<TutorMessage>& c = conversation(lesson);
	auto local = [&](const std::string& q, const std::string& a) {
		TutorMessage m;
		m.role = "learner";
		m.text = q;
		c.push_back(m);
		TutorMessage r;
		r.role = "tutor";
		r.local = true;
		r.text = a;
		c.push_back(r);
	};
	if (kind == QUICK_SIMPLER) {
		if (!b->simpler.empty())
			local("Объясни проще", b->simpler);
		else
			ask("simpler", "Объясни проще");
		return;
	}
	if (kind == QUICK_EXAMPLE) {
		ask("example", "Приведи пример");
		return;
	}
	if (kind == QUICK_HINT) {
		// Practice: the next rung of the hint ladder; elsewhere: a prepared answer not yet shown.
		for (int64_t id : APP->engine->getModuleIds()) {
			TaskModule* t = dynamic_cast<TaskModule*>(APP->engine->getModule(id));
			if (!t || t->lesson != lesson || t->block != b->id)
				continue;
			TaskView v;
			v.item = t->I();
			v.state = &t->state;
			int level = 0, levels = 0;
			std::string h = v.currentHint(&level, &levels);
			if (!h.empty()) {
				local("Подсказка", h);
				const check::Item* f = v.firstFailing();
				if (f && level < levels - 1)
					t->state.hintLevel[f->key] = level + 1;
				return;
			}
			if (t->state.hasResult && t->state.result.passed) {
				local("Подсказка", "Проверка уже пройдена: всё на месте.");
				return;
			}
			local("Подсказка", "Нажми «Проверить» на карточке задания: я покажу, с чего начать.");
			return;
		}
		for (const Faq& f : b->faq) {
			bool shown = false;
			for (const TutorMessage& m : c)
				if (m.text == f.a)
					shown = true;
			if (!shown) {
				local(f.q, f.a);
				return;
			}
		}
		ask("ask", "Дай подсказку по этому блоку");
		return;
	}
	if (kind == QUICK_REVIEW)
		ask("review", "Проверь мой патч");
}

void AtlasHost::ask(const std::string& kind, const std::string& text, const std::string& quizContext, std::function<void(const std::string&)> onDone) {
	const Block* b = currentBlock();
	std::string lessonId = lesson;
	std::vector<TutorMessage>& c = conversation(lessonId);
	if (kind != "grade") {
		TutorMessage q;
		q.role = "learner";
		q.text = text;
		c.push_back(q);
	}
	TutorMessage a;
	a.role = "tutor";
	a.streaming = true;
	c.push_back(a);
	AskRequest r;
	r.message = (int) c.size() - 1;
	r.lesson = lessonId;
	r.block = b ? b->id : "";
	r.kind = kind;
	r.text = text;
	r.patch = patchLine(b);
	r.check = checkLine(b);
	r.quiz = quizContext;
	if (onDone)
		askCallbacks[r.message] = onDone;
	tutorBusy = true;
	client.ask(r);
}

void AtlasHost::stepClient() {
	for (ClientEvent& e : client.drain()) {
		if (e.kind == ClientEvent::PROGRESS) {
			if (e.json) {
				if (progress.merge(e.json))
					saveProgress();
				json_decref(e.json);
			}
			continue;
		}
		std::vector<TutorMessage>& c = conversation(lesson);
		if (e.message < 0 || e.message >= (int) c.size())
			continue;
		TutorMessage& m = c[e.message];
		if (e.kind == ClientEvent::CHUNK)
			m.text += e.text;
		else if (e.kind == ClientEvent::DONE || e.kind == ClientEvent::FAIL) {
			m.streaming = false;
			if (e.kind == ClientEvent::FAIL) {
				m.error = true;
				if (m.text.empty())
					m.text = e.text;
			}
			tutorBusy = false;
			auto cb = askCallbacks.find(e.message);
			if (cb != askCallbacks.end()) {
				cb->second(m.text);
				askCallbacks.erase(cb);
			}
		}
	}
}

/** «VCO.Sawtooth → VCF.Audio; VCF Cutoff frequency = 800 Hz» for the scope of a block. */
std::string AtlasHost::patchLine(const Block* b) {
	if (!b)
		return "";
	const Lesson* L = currentLesson();
	std::set<int64_t> ids;
	const Item* task = nullptr;
	for (const Item& it : b->items)
		if (it.kind == "task")
			task = &it;
	if (task && task->mode != "try") {
		std::string err;
		ids = checkScope(L, b, task, &err);
	}
	if (ids.empty()) {
		float rowY = rowTop(b->row);
		for (widget::Widget* w : APP->scene->rack->getModuleContainer()->children) {
			app::ModuleWidget* mw = dynamic_cast<app::ModuleWidget*>(w);
			if (mw && mw->module && std::fabs(w->box.pos.y - rowY) < 1.f && mw->module->model->plugin->slug != "Atlas")
				ids.insert(mw->module->id);
		}
	}
	if (ids.empty())
		return "";
	check::Patch p = rackPatch(ids);
	std::string out;
	auto name = [&](const std::string& id) {
		const check::Module* m = p.find(id);
		return m ? m->name : id;
	};
	for (const check::Cable& c : p.cables) {
		const check::Module* o = p.find(c.outModule);
		const check::Module* i = p.find(c.inModule);
		std::string on, in;
		for (const check::Port& x : o->outputs)
			if (x.id == c.outId)
				on = x.name;
		for (const check::Port& x : i->inputs)
			if (x.id == c.inId)
				in = x.name;
		out += (out.empty() ? "" : "; ") + name(c.outModule) + "." + on + " → " + name(c.inModule) + "." + in;
	}
	for (int64_t id : ids) {
		engine::Module* m = APP->engine->getModule(id);
		if (!m)
			continue;
		for (ParamQuantity* q : m->paramQuantities) {
			if (!q || std::fabs(q->getValue() - q->getDefaultValue()) < 1e-4f)
				continue;
			out += (out.empty() ? "" : "; ") + m->model->name + " " + q->name + " = " + q->getDisplayValueString() + q->unit;
		}
	}
	if (out.size() > 900)
		out = out.substr(0, 900) + "…";
	return out;
}

std::string AtlasHost::checkLine(const Block* b) {
	if (!b)
		return "";
	auto it = lastChecks.find(lesson + "/" + b->id);
	if (it == lastChecks.end())
		return "";
	const check::Result& r = it->second;
	int ok = 0;
	std::string first;
	for (const check::Item& i : r.items) {
		ok += i.ok;
		if (!i.ok && first.empty())
			first = i.what + " — " + i.hint;
	}
	return std::to_string(ok) + "/" + std::to_string(r.items.size()) + " на месте" + (first.empty() ? "" : "; первое: " + first);
}

// ── checks ──

std::set<int64_t> AtlasHost::checkScope(const Lesson* L, const Block* B, const Item* it, std::string* error) {
	std::set<int64_t> ids;
	if (!L || !B || !it)
		return ids;
	if (it->mode == "debug") {
		ids.insert(it->scope.begin(), it->scope.end());
		return ids;
	}
	// Practice: modules between the two Bench markers of this block's row, plus the given modules.
	float left = -1e9f, right = 1e9f, rowY = -1.f;
	int found = 0;
	for (widget::Widget* w : APP->scene->rack->getModuleContainer()->children) {
		app::ModuleWidget* mw = dynamic_cast<app::ModuleWidget*>(w);
		BenchModule* bm = mw ? dynamic_cast<BenchModule*>(mw->module) : nullptr;
		if (!bm || bm->lesson != L->id || bm->block != B->id)
			continue;
		const Item* bi = bm->I();
		if (bi && bi->side == "right")
			right = mw->box.pos.x;
		else
			left = mw->box.pos.x + mw->box.size.x;
		rowY = mw->box.pos.y;
		found++;
	}
	if (found < 2 && error)
		*error = "Не вижу обоих модулей «Верстак» в этом ряду — проверяю весь ряд.";
	if (rowY < 0)
		rowY = rowTop(B->row);
	for (widget::Widget* w : APP->scene->rack->getModuleContainer()->children) {
		app::ModuleWidget* mw = dynamic_cast<app::ModuleWidget*>(w);
		if (!mw || !mw->module || std::fabs(w->box.pos.y - rowY) > 1.f)
			continue;
		if (w->box.pos.x >= left - 0.5f && w->box.pos.x + w->box.size.x <= right + 0.5f)
			ids.insert(mw->module->id);
	}
	ids.insert(it->given.begin(), it->given.end());
	return ids;
}

void AtlasHost::startCheck(int64_t taskModuleId) {
	post([this, taskModuleId]() {
		TaskModule* t = dynamic_cast<TaskModule*>(APP->engine->getModule(taskModuleId));
		if (!t)
			return;
		const Item* it = t->I();
		if (!it || !it->check)
			return;
		CheckJob job;
		job.taskModuleId = taskModuleId;
		job.lesson = t->lesson;
		job.block = t->block;
		job.item = t->item;
		t->state.error.clear();
		std::set<int64_t> ids = checkScope(t->L(), t->B(), it, &t->state.error);
		job.patch = rackPatch(ids);
		job.targets = check::probeTargets(it->check, job.patch);
		job.started = system::getTime();
		job.nextAt.assign(job.targets.size(), job.started);
		t->state.checking = true;
		checks.push_back(job);
	});
}

void AtlasHost::stepChecks() {
	double now = system::getTime();
	for (size_t j = 0; j < checks.size();) {
		CheckJob& job = checks[j];
		bool done = true;
		for (size_t k = 0; k < job.targets.size(); k++) {
			const check::ProbeTarget& t = job.targets[k];
			std::vector<float>& s = job.samples[t.probe];
			if ((int) s.size() >= t.samples)
				continue;
			done = false;
			if (now >= job.nextAt[k]) {
				bool ok = false;
				float v = outputVoltage(std::stoll(t.moduleId), t.outputId, &ok);
				if (ok)
					s.push_back(v);
				else
					s.resize(t.samples, 0.f);
				job.nextAt[k] = now + t.intervalMs / 1000.0;
			}
		}
		if (!done && now - job.started < 5.0) {
			j++;
			continue;
		}
		struct Samples : check::ProbeSource {
			std::map<int, std::vector<float>>* s;
			std::vector<float> samples(const check::ProbeTarget& t) override { return (*s)[t.probe]; }
		} src;
		src.s = &job.samples;
		TaskModule* tm = dynamic_cast<TaskModule*>(APP->engine->getModule(job.taskModuleId));
		const Item* it = pack().item(job.lesson, job.block, job.item);
		if (tm && it) {
			check::Result r = check::run(it->check, job.patch, &src);
			bool firstPass = r.passed && !(tm->state.hasResult && tm->state.result.passed);
			tm->state.result = r;
			tm->state.hasResult = true;
			tm->state.checking = false;
			tm->state.page = r.passed ? 1 : 1;
			lastChecks[job.lesson + "/" + job.block] = r;
			json_t* d = json_object();
			json_object_set_new(d, "passed", json_boolean(r.passed));
			json_t* items = json_array();
			for (const check::Item& ci : r.items) {
				json_t* x = json_object();
				json_object_set_new(x, "key", json_string(ci.key.c_str()));
				json_object_set_new(x, "ok", json_boolean(ci.ok));
				json_object_set_new(x, "what", json_string(ci.what.c_str()));
				if (!ci.ok)
					json_object_set_new(x, "hint", json_string(ci.hint.c_str()));
				json_array_append_new(items, x);
			}
			json_object_set_new(d, "items", items);
			json_object_set_new(d, "item", json_string(job.item.c_str()));
			event("check", d);
			(void) firstPass;
			checkSerial++;
		}
		checks.erase(checks.begin() + j);
	}
}

} // namespace atlas
