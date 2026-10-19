// The Atlas client: HTTP to the Atlas app (`/api/rack/*`) on its own background thread —
// never in process() or on the UI thread. The host feeds it jobs and reads a snapshot each
// step(); nothing here touches widgets, nanovg or the engine.
#pragma once
#include <jansson.h>

#include <atomic>
#include <condition_variable>
#include <deque>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace atlas {

struct AskRequest {
	int message = -1;  // index of the tutor bubble to fill
	std::string lesson, block, kind, text, patch, check, quiz;
};

/** What the worker hands back to the UI thread. */
struct ClientEvent {
	enum Kind { CHUNK, DONE, FAIL, PROGRESS } kind = CHUNK;
	int message = -1;
	std::string text;   // CHUNK: text to append; FAIL: error message
	json_t* json = nullptr;  // PROGRESS: remote progress (owned by the receiver)
};

struct Client {
	std::atomic<bool> connected{false};
	std::atomic<bool> running{true};
	std::string tutorModel;  // from /health

	Client();
	~Client();
	void setUrl(const std::string& url);
	std::string url();

	void ask(const AskRequest& r);
	/** Fire-and-forget POST (progress events, lesson open). Takes ownership of `body`. */
	void post(const std::string& path, json_t* body);
	void requestProgress();
	std::vector<ClientEvent> drain();

  private:
	std::mutex mutex;
	std::condition_variable cv;
	std::string base;
	std::deque<std::function<void()>> jobs;
	std::vector<ClientEvent> events;
	std::thread worker;
	double lastHealth = -1e9;
	void run();
	void push(const ClientEvent& e);
	void enqueue(std::function<void()> job);
	void doAsk(AskRequest r);
	void health();
};

} // namespace atlas
