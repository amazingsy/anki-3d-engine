// Copyright (C) 2009-2021, Panagiotis Christopoulos Charitos and contributors.
// All rights reserved.
// Code licensed under the BSD License.
// http://www.anki3d.org/LICENSE

#include <cstdio>
#include <Samples/Common/Framework.h>

using namespace anki;

class MyApp : public SampleApp
{
public:
	Error sampleExtraInit()
	{
		ScriptResourcePtr script;
		ANKI_CHECK(getResourceManager().loadResource("Assets/Scene.lua", script));
		ANKI_CHECK(getScriptManager().evalString(script->getSource()));

		return Error::NONE;
	}
};

int main(int argc, char* argv[])
{
	Error err = Error::NONE;

	MyApp* app = new MyApp;
	err = app->init(argc, argv, "SimpleScene");
	if(!err)
	{
		err = app->mainLoop();
	}

	delete app;
	if(err)
	{
		ANKI_LOGE("Error reported. Bye!");
	}
	else
	{
		ANKI_LOGI("Bye!!");
	}

	return 0;
}
