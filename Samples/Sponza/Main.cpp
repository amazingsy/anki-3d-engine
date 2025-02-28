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

		getMainRenderer().getOffscreenRenderer().getVolumetricFog().setFogParticleColor(Vec3(1.0f, 0.9f, 0.9f));
		getMainRenderer().getOffscreenRenderer().getVolumetricFog().setParticleDensity(2.0f);
		return Error::NONE;
	}
};

ANKI_MAIN_FUNCTION(myMain)
int myMain(int argc, char* argv[])
{
	Error err = Error::NONE;

	MyApp* app = new MyApp;
	err = app->init(argc, argv, "Sponza");
	if(!err)
	{
		err = app->mainLoop();
	}

	delete app;
	if(err)
	{
		ANKI_LOGE("Error reported. Bye!!");
	}
	else
	{
		ANKI_LOGI("Bye!!");
	}

	return 0;
}
