#include "Client.hpp"

#include <rack.hpp>

namespace atlas {

using namespace rack;

Client::Client() {
	worker = std::thread([this]() { run(); });
}

Client::~Client() {
	running = false;
	cv.notify_all();
	if (worker.joinable())
		worker.join();
}

void Client::setUrl(const std::string& u) {
	std::lock_guard<std::mutex> lock(mutex);
	base = u;
}

std::string Client::url() {
	std::lock_guard<std::mutex> lock(mutex);
	return base;
}

void Client::enqueue(std::function<void()> job) {
	{
		std::lock_guard<std::mutex> lock(mutex);
		jobs.push_back(std::move(job));
	}
	cv.notify_all();
}

void Client::push(const ClientEvent& e) {
	std::lock_guard<std::mutex> lock(mutex);
	events.push_back(e);
}

std::vector<ClientEvent> Client::drain() {
	std::lock_guard<std::mutex> lock(mutex);
	std::vector<ClientEvent> out;
	out.swap(events);
	return out;
}

void Client::post(const std::string& path, json_t* body) {
	enqueue([this, path, body]() {
		json_t* res = network::requestJson(network::METHOD_POST, url() + path, body);
		json_decref(body);
		connected = res != nullptr;
		if (res)
			json_decref(res);
	});
}

void Client::requestProgress() {
	enqueue([this]() {
		json_t* res = network::requestJson(network::METHOD_GET, url() + "/api/rack/progress", nullptr);
		connected = res != nullptr;
		if (res) {
			ClientEvent e;
			e.kind = ClientEvent::PROGRESS;
			e.json = res;
			push(e);
		}
	});
}

void Client::ask(const AskRequest& r) {
	enqueue([this, r]() { doAsk(r); });
}

void Client::doAsk(AskRequest r) {
	json_t* body = json_object();
	json_object_set_new(body, "lesson", json_string(r.lesson.c_str()));
	json_object_set_new(body, "block", json_string(r.block.c_str()));
	json_object_set_new(body, "kind", json_string(r.kind.c_str()));
	json_object_set_new(body, "text", json_string(r.text.c_str()));
	if (!r.patch.empty())
		json_object_set_new(body, "patch", json_string(r.patch.c_str()));
	if (!r.check.empty())
		json_object_set_new(body, "check", json_string(r.check.c_str()));
	if (!r.quiz.empty())
		json_object_set_new(body, "quiz", json_string(r.quiz.c_str()));
	json_t* res = network::requestJson(network::METHOD_POST, url() + "/api/rack/ask", body);
	json_decref(body);
	ClientEvent fail;
	fail.kind = ClientEvent::FAIL;
	fail.message = r.message;
	if (!res) {
		connected = false;
		fail.text = "Репетитор недоступен: Atlas не запущен.";
		push(fail);
		return;
	}
	connected = true;
	json_t* sid = json_object_get(res, "stream");
	std::string id = sid && json_is_string(sid) ? json_string_value(sid) : "";
	json_t* err = json_object_get(res, "error");
	if (id.empty()) {
		fail.text = err && json_is_string(err) ? json_string_value(err) : "Репетитор не ответил.";
		json_decref(res);
		push(fail);
		return;
	}
	json_decref(res);
	// Short polling: new chunks every 250 ms until done (2 min at most).
	int after = 0;
	double start = system::getTime();
	while (running && system::getTime() - start < 120.0) {
		json_t* s = network::requestJson(network::METHOD_GET, url() + "/api/rack/stream/" + id + "?after=" + std::to_string(after), nullptr);
		if (!s) {
			fail.text = "Связь с Atlas прервалась.";
			push(fail);
			return;
		}
		size_t i;
		json_t* c;
		json_array_foreach(json_object_get(s, "chunks"), i, c) {
			if (json_is_string(c)) {
				ClientEvent e;
				e.kind = ClientEvent::CHUNK;
				e.message = r.message;
				e.text = json_string_value(c);
				push(e);
			}
			after++;
		}
		bool done = json_is_true(json_object_get(s, "done"));
		json_t* serr = json_object_get(s, "error");
		std::string errText = serr && json_is_string(serr) ? json_string_value(serr) : "";
		json_decref(s);
		if (!errText.empty()) {
			fail.text = errText;
			push(fail);
			return;
		}
		if (done) {
			ClientEvent e;
			e.kind = ClientEvent::DONE;
			e.message = r.message;
			push(e);
			return;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(250));
	}
	fail.text = "Репетитор думает слишком долго. Спроси ещё раз.";
	push(fail);
}

void Client::health() {
	json_t* res = network::requestJson(network::METHOD_GET, url() + "/api/rack/health", nullptr);
	connected = res != nullptr;
	if (res)
		json_decref(res);
}

void Client::run() {
	while (running) {
		std::function<void()> job;
		{
			std::unique_lock<std::mutex> lock(mutex);
			cv.wait_for(lock, std::chrono::milliseconds(500), [this]() { return !jobs.empty() || !running; });
			if (!running)
				break;
			if (!jobs.empty()) {
				job = std::move(jobs.front());
				jobs.pop_front();
			}
		}
		if (job) {
			job();
			continue;
		}
		double now = system::getTime();
		if (now - lastHealth > 5.0 && !url().empty()) {
			lastHealth = now;
			health();
		}
	}
}

} // namespace atlas
