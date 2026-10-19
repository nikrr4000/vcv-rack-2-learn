// Module data structs shared by the widgets and the host (which finds them by dynamic_cast),
// and the text field used by the Quiz, the Tutor and the docked panel.
#pragma once
#include "Base.hpp"
#include "views/QuizView.hpp"
#include "views/TaskView.hpp"

namespace atlas {

struct NavigatorModule : AtlasModule {
	std::string layoutHash;
	int blockIndex = 0;
	NavigatorModule() { config(0, 0, 0, 0); }
	void saveState(json_t* s) override;
	void loadState(json_t* s) override;
};

struct QuizModule : AtlasModule {
	QuizState state;
	QuizModule() { config(0, 0, 0, 0); }
	void saveState(json_t* s) override;
	void loadState(json_t* s) override;
};

struct TaskModule : AtlasModule {
	TaskState state;
	TaskModule() { config(0, 0, 0, 0); }
	void saveState(json_t* s) override;
	void loadState(json_t* s) override;
};

struct NoteModule : AtlasModule {
	int page = 0;
	NoteModule() { config(0, 0, 0, 0); }
};

struct BenchModule : AtlasModule {
	BenchModule() { config(0, 0, 0, 0); }
};

struct TutorModule : AtlasModule {
	TutorModule() { config(0, 0, 0, 0); }
};

/** A Rack TextField that draws nothing itself (the view draws it) and sends on Enter. */
struct AtlasField : ui::TextField {
	std::function<void(const std::string&)> onSubmit;
	std::function<int(math::Vec)> indexAt;
	bool focused() { return APP->event->getSelectedWidget() == this; }
	void draw(const DrawArgs& args) override {}
	int getTextPosition(math::Vec mousePos) override { return indexAt ? indexAt(mousePos) : ui::TextField::getTextPosition(mousePos); }
	void onSelectKey(const SelectKeyEvent& e) override;
	void onSelectText(const SelectTextEvent& e) override;
};

/** «Адрес Atlas» and the panel toggle for a context menu. */
void appendAtlasMenu(ui::Menu* menu);

} // namespace atlas
