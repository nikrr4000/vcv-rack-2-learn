// The plugin's Atlas client end to end (plan W3.9): the real `Client` (src/net/Client.cpp) on
// the Rack app's libRack (`rack::network::requestJson`), against a running Atlas server with the
// stub tutor. Run by tests/client/run.mjs, which starts the server on 127.0.0.1:5185.
//
//   client_e2e <atlas url> <lesson> <block>
//
// Checks: lesson open, an «ask» streams chunks and ends with DONE, a «grade» reply starts with a
// verdict, a progress event lands and GET progress comes back as a PROGRESS event, and an
// unreachable URL gives the offline FAIL text. Exit code = number of failed checks.
#include <net/Client.hpp>
#include <rack.hpp>

#include <chrono>
#include <cstdio>
#include <string>
#include <thread>

using namespace atlas;

static int failures = 0;
static void check(bool ok, const char* what) {
	std::printf("%s %s\n", ok ? "PASS" : "FAIL", what);
	if (!ok)
		failures++;
}

struct Reply {
	std::string text, error;
	int chunks = 0;
	bool done = false;
};

static Reply wait(Client& c, int message, double seconds) {
	Reply r;
	auto end = std::chrono::steady_clock::now() + std::chrono::milliseconds((int) (seconds * 1000));
	while (std::chrono::steady_clock::now() < end) {
		for (ClientEvent& e : c.drain()) {
			if (e.kind == ClientEvent::PROGRESS) {
				if (e.json)
					json_decref(e.json);
				continue;
			}
			if (e.message != message)
				continue;
			if (e.kind == ClientEvent::CHUNK) {
				r.text += e.text;
				r.chunks++;
			}
			else if (e.kind == ClientEvent::DONE) {
				r.done = true;
				return r;
			}
			else if (e.kind == ClientEvent::FAIL) {
				r.error = e.text;
				return r;
			}
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(20));
	}
	return r;
}

int main(int argc, char** argv) {
	if (argc < 4) {
		std::fprintf(stderr, "usage: client_e2e <atlas url> <lesson> <block>\n");
		return 2;
	}
	std::string url = argv[1], lesson = argv[2], block = argv[3];
	{
		Client c;
		c.setUrl(url);
		json_t* open = json_object();
		json_object_set_new(open, "lesson", json_string(lesson.c_str()));
		c.post("/api/rack/lesson/open", open);

		AskRequest a;
		a.message = 1;
		a.lesson = lesson;
		a.block = block;
		a.kind = "ask";
		a.text = "Зачем здесь фильтр?";
		a.patch = "VCO.Sawtooth → VCF.Audio";
		a.check = "1/5 на месте";
		c.ask(a);
		Reply r = wait(c, 1, 30);
		check(r.done && r.error.empty(), "ask: DONE without error");
		check(r.chunks > 1, "ask: streamed in more than one chunk");
		check(r.text.find(block) != std::string::npos || r.text.find("Коротко") == 0, "ask: reply text arrived whole");
		std::printf("     ask → %s\n", r.text.c_str());

		AskRequest g = a;
		g.message = 3;
		g.kind = "grade";
		g.text = "это верно";
		g.quiz = "Вопрос: …";
		c.ask(g);
		Reply rg = wait(c, 3, 30);
		check(rg.done && (rg.text.rfind("✓", 0) == 0 || rg.text.rfind("✗", 0) == 0 || rg.text.rfind("≈", 0) == 0), "grade: verdict first");

		json_t* ev = json_object();
		json_object_set_new(ev, "lesson", json_string(lesson.c_str()));
		json_object_set_new(ev, "block", json_string(block.c_str()));
		json_object_set_new(ev, "event", json_string("block"));
		json_object_set_new(ev, "data", json_object());
		json_object_set_new(ev, "at", json_real(2000000000.0));
		c.post("/api/rack/progress", ev);
		c.requestProgress();
		bool got = false;
		auto end = std::chrono::steady_clock::now() + std::chrono::seconds(10);
		while (!got && std::chrono::steady_clock::now() < end) {
			for (ClientEvent& e : c.drain()) {
				if (e.kind != ClientEvent::PROGRESS || !e.json)
					continue;
				json_t* L = json_object_get(json_object_get(e.json, "lessons"), lesson.c_str());
				const char* b = json_string_value(json_object_get(L, "block"));
				got = b && block == b;
				json_decref(e.json);
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(20));
		}
		check(got, "progress: the posted block comes back in GET progress");
		check(c.connected, "connected flag set");
	}
	{
		Client off;
		off.setUrl("http://127.0.0.1:5199");
		AskRequest a;
		a.message = 0;
		a.lesson = lesson;
		a.kind = "ask";
		a.text = "есть кто?";
		off.ask(a);
		Reply r = wait(off, 0, 15);
		check(r.error.find("Atlas не запущен") != std::string::npos, "offline: «Репетитор недоступен: Atlas не запущен»");
		check(!off.connected, "offline: connected flag clear");
	}
	std::printf("%d failed checks\n", failures);
	return failures;
}
