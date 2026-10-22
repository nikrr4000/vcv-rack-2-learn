// AtlasHost: a scene child that outlives patch loads (plan §6.3.1). Owns the action queue
// (patch loads, glides, saves — never run inside a module's event handler), learner progress,
// the docked panel, the pointer overlay, the Atlas client, the tutor conversations, practice
// checks and the dev bridge. Never caches widget pointers across frames: modules are found by
// id every time.
#pragma once
#include <atomic>
#include <functional>
#include <map>
#include <mutex>
#include <set>
#include <vector>

#include "atlas.hpp"
#include "check/Checker.hpp"
#include "core/Progress.hpp"
#include "net/Client.hpp"
#include "views/NavView.hpp"
#include "views/TutorView.hpp"

namespace atlas {

struct DockPanel;
struct PointerOverlay;
struct DevBridge;
struct NavigatorModule;
struct TaskModule;

struct CheckJob {
	int64_t taskModuleId = -1;
	std::string lesson, block, item;
	check::Patch patch;
	std::vector<check::ProbeTarget> targets;
	std::map<int, std::vector<float>> samples;  // probe index → voltages
	std::vector<double> nextAt;
	double started = 0;
};

struct AtlasHost : widget::Widget {
	static AtlasHost* instance;

	// Action queue: run in step() on the UI thread.
	std::mutex queueMutex;
	std::vector<std::function<void()>> queue;
	void post(std::function<void()> fn);

	// Learner state.
	Progress progress;
	std::string lesson;  // lesson of the open course patch ("" = none)
	int block = 0;
	int64_t navigatorId = -1;
	std::string notice;
	bool mapOpen = false;

	// Tutor.
	std::map<std::string, std::vector<TutorMessage>> conversations;
	bool tutorBusy = false;
	Client client;

	// Widgets owned by the scene.
	DockPanel* panel = nullptr;
	PointerOverlay* overlay = nullptr;
	DevBridge* bridge = nullptr;

	// Glide.
	bool gliding = false;
	double glideStart = 0;
	math::Vec fromOffset, toOffset;
	float fromZoom = 1.f, toZoom = 1.f;
	int pulseBlock = -1;
	double pulseStart = 0;
	int framesSinceGoto = 0;

	// Checks in progress (probes sampled from the UI thread, like the REST checker).
	std::vector<CheckJob> checks;
	std::map<std::string, check::Result> lastChecks;  // "<lesson>/<block>" → result
	std::atomic<int> checkSerial{0};

	// Dev bridge snapshot.
	std::mutex snapshotMutex;
	std::string snapshot = "{}";
	std::vector<std::string> missingTargets;
	std::vector<std::string> missingPlugins;

	AtlasHost();
	~AtlasHost();
	void step() override;
	void draw(const DrawArgs& args) override;

	// Course actions (queue them; safe to call from module event handlers).
	void openLesson(const std::string& id);
	void gotoBlock(int index, bool animate = true);
	void next() { gotoBlock(block + 1); }
	void prev() { gotoBlock(block - 1); }
	void completeLesson();
	void togglePanel();
	/** Side panel bigger (+1) or smaller (−1) by 0.1, or to the scale `to` when it is > 0; saved. */
	void zoomPanel(int dir, float to = 0.f);

	const Lesson* currentLesson() const { return pack().lesson(lesson); }
	const Block* currentBlock() const;
	std::vector<TutorMessage>& conversation(const std::string& lessonId);
	NavState navState() const;

	// Tutor.
	void faq(int index);
	void quick(int kind);
	void ask(const std::string& kind, const std::string& text, const std::string& quizContext = "", std::function<void(const std::string&)> onDone = nullptr);
	std::map<int, std::function<void(const std::string&)>> askCallbacks;
	std::string patchLine(const Block* b);
	std::string checkLine(const Block* b);

	// Checks.
	void startCheck(int64_t taskModuleId);
	std::set<int64_t> checkScope(const Lesson* L, const Block* B, const Item* it, std::string* error);

	// Progress.
	void saveProgress();
	void event(const std::string& name, json_t* data = nullptr);

	// Internals.
	void sync();
	void applyMixer();
	math::Rect blockRect(int index);
	void startGlide(int index, bool animate);
	void stepGlide();
	void stepChecks();
	void stepClient();
	void publishSnapshot();
	NavigatorModule* findNavigator();
	std::string userPatchPath(const std::string& id) const;
	void doOpenLesson(const std::string& id);

	int frame = 0;
	double lastProgressPull = -1e9;
};

/** The host if it exists (it is created by the first Atlas widget with a module). */
inline AtlasHost* host() { return AtlasHost::instance; }

void ensureHost();

} // namespace atlas
