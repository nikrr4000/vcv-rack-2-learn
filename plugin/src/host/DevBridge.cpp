// Dev bridge (testing only, plan §6.3.8): with ATLAS_DEV_PORT set, a tiny HTTP API on
// 127.0.0.1 for the automated in-Rack checks (rack/scripts/rack-test.mjs). Handlers run on
// httplib threads: they enqueue work for the host and wait (10 s at most); /state is served
// from the snapshot the host publishes in step(). Never active without the env var.
#include <chrono>
#include <future>

#include "Host.hpp"
#include <patch.hpp>
#include "core/Report.hpp"
#include "modules/Modules.hpp"

#define CPPHTTPLIB_NO_EXCEPTIONS
#include "httplib.h"

namespace atlas {

struct DevBridge {
	httplib::Server server;
	std::thread thread;
	AtlasHost* host;
};

static bool runOnHost(AtlasHost* h, std::function<void()> fn, bool waitNextStep = false) {
	auto done = std::make_shared<std::promise<void>>();
	std::future<void> f = done->get_future();
	h->post([h, fn, done, waitNextStep]() {
		fn();
		if (waitNextStep)
			h->post([done]() { done->set_value(); });
		else
			done->set_value();
	});
	return f.wait_for(std::chrono::seconds(10)) == std::future_status::ready;
}

static json_t* body(const httplib::Request& req) {
	json_error_t err;
	json_t* j = json_loads(req.body.c_str(), 0, &err);
	return j ? j : json_object();
}

static void reply(httplib::Response& res, const std::string& s, int status = 200) {
	res.status = status;
	res.set_content(s, "application/json");
}

DevBridge* startDevBridge(AtlasHost* host) {
	const char* portEnv = std::getenv("ATLAS_DEV_PORT");
	int port = portEnv ? std::atoi(portEnv) : 0;
	if (port <= 0)
		return nullptr;
	DevBridge* b = new DevBridge;
	b->host = host;
	httplib::Server& s = b->server;
	s.Get("/state", [host](const httplib::Request&, httplib::Response& res) {
		std::lock_guard<std::mutex> lock(host->snapshotMutex);
		reply(res, host->snapshot);
	});
	s.Post("/open", [host](const httplib::Request& req, httplib::Response& res) {
		json_t* j = body(req);
		json_t* l = json_object_get(j, "lesson");
		std::string id = l && json_is_string(l) ? json_string_value(l) : "";
		json_decref(j);
		bool ok = runOnHost(host, [host, id]() { host->doOpenLesson(id); }, true);
		reply(res, ok ? "{\"ok\":true}" : "{\"ok\":false,\"error\":\"timeout\"}", ok ? 200 : 504);
	});
	s.Post("/goto", [host](const httplib::Request& req, httplib::Response& res) {
		json_t* j = body(req);
		json_t* bj = json_object_get(j, "block");
		int index = -1;
		std::string id = bj && json_is_string(bj) ? json_string_value(bj) : "";
		if (bj && json_is_number(bj))
			index = (int) json_number_value(bj);
		json_decref(j);
		bool ok = runOnHost(host, [host, id, index]() {
			int i = index;
			const Lesson* L = host->currentLesson();
			if (L && !id.empty())
				i = L->blockIndex(id);
			host->gotoBlock(i, false);
		}, true);
		reply(res, ok ? "{\"ok\":true}" : "{\"ok\":false,\"error\":\"timeout\"}", ok ? 200 : 504);
	});
	s.Post("/check", [host](const httplib::Request&, httplib::Response& res) {
		int before = host->checkSerial;
		bool started = false;
		runOnHost(host, [host, &started]() {
			const Block* B = host->currentBlock();
			for (int64_t id : APP->engine->getModuleIds()) {
				TaskModule* t = dynamic_cast<TaskModule*>(APP->engine->getModule(id));
				if (t && B && t->lesson == host->lesson && t->block == B->id && t->I() && t->I()->check) {
					host->startCheck(id);
					started = true;
				}
			}
		});
		if (!started) {
			reply(res, "{\"ok\":false,\"error\":\"no check in this block\"}", 404);
			return;
		}
		for (int i = 0; i < 100 && host->checkSerial == before; i++)
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		std::string out;
		runOnHost(host, [host, &out]() {
			const Block* B = host->currentBlock();
			auto it = B ? host->lastChecks.find(host->lesson + "/" + B->id) : host->lastChecks.end();
			json_t* r = json_object();
			if (it != host->lastChecks.end()) {
				json_object_set_new(r, "passed", json_boolean(it->second.passed));
				json_t* items = json_array();
				for (const check::Item& ci : it->second.items) {
					json_t* x = json_object();
					json_object_set_new(x, "key", json_string(ci.key.c_str()));
					json_object_set_new(x, "ok", json_boolean(ci.ok));
					json_object_set_new(x, "hint", json_string(ci.hint.c_str()));
					json_array_append_new(items, x);
				}
				json_object_set_new(r, "items", items);
			}
			char* s = json_dumps(r, 0);
			out = s;
			free(s);
			json_decref(r);
		});
		reply(res, out.empty() ? "{}" : out);
	});
	s.Post("/quiz", [host](const httplib::Request& req, httplib::Response& res) {
		json_t* j = body(req);
		json_t* ij = json_object_get(j, "item");
		json_t* oj = json_object_get(j, "option");
		std::string item = ij && json_is_string(ij) ? json_string_value(ij) : "";
		int option = oj && json_is_number(oj) ? (int) json_number_value(oj) : 0;
		json_decref(j);
		bool found = false;
		runOnHost(host, [host, item, option, &found]() {
			const Block* B = host->currentBlock();
			for (int64_t id : APP->engine->getModuleIds()) {
				QuizModule* q = dynamic_cast<QuizModule*>(APP->engine->getModule(id));
				if (q && B && q->block == B->id && (item.empty() || q->item == item)) {
					const Item* it = q->I();
					if (!it)
						continue;
					q->state.fit(it->questions.size());
					q->state.answers[q->state.current].chosen = option;
					found = true;
				}
			}
		});
		reply(res, found ? "{\"ok\":true}" : "{\"ok\":false}", found ? 200 : 404);
	});
	s.Post("/quit", [host](const httplib::Request&, httplib::Response& res) {
		host->post([]() { APP->window->close(); });
		reply(res, "{\"ok\":true}");
	});
	b->thread = std::thread([b, port]() {
		INFO("Atlas dev bridge on 127.0.0.1:%d", port);
		b->server.listen("127.0.0.1", port);
	});
	return b;
}

void stopDevBridge(DevBridge* b) {
	if (!b)
		return;
	b->server.stop();
	if (b->thread.joinable())
		b->thread.join();
	delete b;
}

void AtlasHost::publishSnapshot() {
	json_t* r = json_object();
	json_object_set_new(r, "lesson", json_string(lesson.c_str()));
	const Lesson* L = currentLesson();
	json_object_set_new(r, "blockCount", json_integer(L ? (int) L->blocks.size() : 0));
	json_object_set_new(r, "block", json_integer(block));
	const Block* B = currentBlock();
	json_object_set_new(r, "blockId", json_string(B ? B->id.c_str() : ""));
	json_object_set_new(r, "navigator", json_boolean(navigatorId >= 0));
	json_object_set_new(r, "panelOpen", json_boolean(settings().panelOpen));
	json_object_set_new(r, "framesSinceGoto", json_integer(framesSinceGoto));
	json_object_set_new(r, "connected", json_boolean(client.connected));
	json_object_set_new(r, "notice", json_string(notice.c_str()));
	json_object_set_new(r, "patchPath", json_string(APP->patch->path.c_str()));
	json_t* texts = json_object();
	json_t* overflow = json_array();
	for (const auto& kv : textReports()) {
		json_t* e = json_object();
		json_object_set_new(e, "pages", json_integer(kv.second.pages));
		json_object_set_new(e, "overflow", json_boolean(kv.second.overflow));
		json_object_set_new(texts, kv.first.c_str(), e);
		if (kv.second.overflow || kv.second.pages > 3)
			json_array_append_new(overflow, json_string(kv.first.c_str()));
	}
	json_object_set_new(r, "texts", texts);
	json_object_set_new(r, "overflow", overflow);
	json_t* mt = json_array();
	for (const std::string& m : missingTargets)
		json_array_append_new(mt, json_string(m.c_str()));
	json_object_set_new(r, "missingTargets", mt);
	json_t* mp = json_array();
	for (const std::string& m : missingPlugins)
		json_array_append_new(mp, json_string(m.c_str()));
	json_object_set_new(r, "missingPlugins", mp);
	json_t* ch = json_object();
	for (const auto& kv : lastChecks)
		json_object_set_new(ch, kv.first.c_str(), json_boolean(kv.second.passed));
	json_object_set_new(r, "checks", ch);
	char* s = json_dumps(r, JSON_COMPACT);
	{
		std::lock_guard<std::mutex> lock(snapshotMutex);
		snapshot = s;
	}
	free(s);
	json_decref(r);
}

} // namespace atlas
