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
	AnimationResourcePtr m_floatAnim;
	AnimationResourcePtr m_waveAnim;

	Error sampleExtraInit() override
	{
		ScriptResourcePtr script;
		ANKI_CHECK(getResourceManager().loadResource("Assets/Scene.lua", script));
		ANKI_CHECK(getScriptManager().evalString(script->getSource()));

		ANKI_CHECK(getResourceManager().loadResource("Assets/float.001.ankianim", m_floatAnim));
		ANKI_CHECK(getResourceManager().loadResource("Assets/wave.001.ankianim", m_waveAnim));

		AnimationPlayInfo animInfo;
		animInfo.m_startTime = 2.0;
		animInfo.m_repeatTimes = -1.0;
		getSceneGraph()
			.findSceneNode("droid.001")
			.getFirstComponentOfType<SkinComponent>()
			.playAnimation(0, m_floatAnim, animInfo);

		getMainRenderer().getOffscreenRenderer().getVolumetricFog().setFogParticleColor(Vec3(1.0f, 0.9f, 0.9f));
		getMainRenderer().getOffscreenRenderer().getVolumetricFog().setParticleDensity(2.0f);

		getMainRenderer().getOffscreenRenderer().getBloom().setThreshold(5.0f);
		return Error::NONE;
	}

	Error userMainLoop(Bool& quit, Second elapsedTime) override
	{
		if(getInput().getKey(KeyCode::Z) == 1)
		{
			AnimationPlayInfo animInfo;
			animInfo.m_startTime = 0.5;
			animInfo.m_repeatTimes = 3.0;
			animInfo.m_blendInTime = 0.5;
			animInfo.m_blendOutTime = 0.35;
			getSceneGraph()
				.findSceneNode("droid.001")
				.getFirstComponentOfType<SkinComponent>()
				.playAnimation(1, m_waveAnim, animInfo);
		}

		return SampleApp::userMainLoop(quit, elapsedTime);
	}
};

int main(int argc, char* argv[])
{
	Error err = Error::NONE;

	MyApp* app = new MyApp;
	err = app->init(argc, argv, "SkeletalAnimation");
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
