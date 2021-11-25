#include "EditorApp.h"
using namespace anki;
using namespace AnKiEditor;

Error EditorApp::init()
{
	HeapAllocator<U32> alloc(allocAligned, nullptr);
	StringAuto mainDataPath(alloc, ANKI_SOURCE_DIRECTORY);
	StringAuto assetsDataPath(alloc);
	assetsDataPath.sprintf("%s/AnKiEditor", ANKI_SOURCE_DIRECTORY);
	ConfigSet cfg = DefaultConfigSet::get();
	cfg.set("window_fullscreen", false);
	cfg.set("rsrc_dataPaths", StringAuto(alloc).sprintf("%s:%s", mainDataPath.cstr(), assetsDataPath.cstr()));
	// cfg.set("Width", 800);
	// cfg.set("Height", 600);
	ANKI_CHECK(App::init(cfg, allocAligned, nullptr));
	return Error::NONE;
}
