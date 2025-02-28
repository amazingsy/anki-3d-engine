// Copyright (C) 2009-2021, Panagiotis Christopoulos Charitos and contributors.
// All rights reserved.
// Code licensed under the BSD License.
// http://www.anki3d.org/LICENSE

#include <AnKi/AnKi.h>

using namespace anki;

class TextureViewerUiNode : public SceneNode
{
public:
	ImageResourcePtr m_imageResource;

	TextureViewerUiNode(SceneGraph* scene, CString name)
		: SceneNode(scene, name)
	{
		SpatialComponent* spatialc = newComponent<SpatialComponent>();
		spatialc->setAlwaysVisible(true);

		UiComponent* uic = newComponent<UiComponent>();
		uic->init(
			[](CanvasPtr& canvas, void* ud) {
				static_cast<TextureViewerUiNode*>(ud)->draw(canvas);
			},
			this);

		ANKI_CHECK_AND_IGNORE(getSceneGraph().getUiManager().newInstance(m_font, "EngineAssets/UbuntuMonoRegular.ttf",
																		 Array<U32, 1>{16}));

		ANKI_CHECK_AND_IGNORE(getSceneGraph().getResourceManager().loadResource(
			"AnKi/Shaders/UiVisualizeImage.ankiprog", m_imageProgram));
	}

	Error frameUpdate(Second prevUpdateTime, Second crntTime)
	{
		if(!m_textureView.isCreated())
		{
			m_textureView = m_imageResource->getTextureView();
		}

		return Error::NONE;
	}

private:
	FontPtr m_font;
	ShaderProgramResourcePtr m_imageProgram;
	ShaderProgramPtr m_imageGrProgram;
	TextureViewPtr m_textureView;
	U32 m_crntMip = 0;
	F32 m_zoom = 1.0f;
	F32 m_depth = 0.0f;
	Bool m_pointSampling = true;
	Array<Bool, 4> m_colorChannel = {true, true, true, true};

	void draw(CanvasPtr& canvas)
	{
		const Texture& grTex = *m_imageResource->getTexture().get();
		const U32 colorComponentCount = getFormatInfo(grTex.getFormat()).m_componentCount;
		ANKI_ASSERT(grTex.getTextureType() == TextureType::_2D || grTex.getTextureType() == TextureType::_3D);

		if(!m_imageGrProgram.isCreated())
		{
			ShaderProgramResourceVariantInitInfo variantInit(m_imageProgram);
			variantInit.addMutation("TEXTURE_TYPE", (grTex.getTextureType() == TextureType::_2D) ? 0 : 1);

			const ShaderProgramResourceVariant* variant;
			m_imageProgram->getOrCreateVariant(variantInit, variant);
			m_imageGrProgram = variant->getProgram();
		}

		ImGui::Begin("Console", nullptr,
					 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove);

		canvas->pushFont(m_font, 16);

		ImGui::SetWindowPos(Vec2(0.0f, 0.0f));
		ImGui::SetWindowSize(Vec2(F32(canvas->getWidth()), F32(canvas->getHeight())));

		ImGui::BeginChild("Tools", Vec2(-1.0f, 30.0f), false, ImGuiWindowFlags_AlwaysAutoResize);

		// Zoom
		if(ImGui::Button("-"))
		{
			m_zoom -= 0.1f;
		}
		ImGui::SameLine();
		ImGui::DragFloat("", &m_zoom, 0.01f, 0.1f, 20.0f, "Zoom %.3f");
		ImGui::SameLine();
		if(ImGui::Button("+"))
		{
			m_zoom += 0.1f;
		}
		ImGui::SameLine();
		ImGui::Spacing();
		ImGui::SameLine();

		// Sampling
		ImGui::Checkbox("Point sampling", &m_pointSampling);
		ImGui::SameLine();
		ImGui::Spacing();
		ImGui::SameLine();

		// Colors
		ImGui::Checkbox("Red", &m_colorChannel[0]);
		ImGui::SameLine();
		ImGui::Checkbox("Green", &m_colorChannel[1]);
		ImGui::SameLine();
		ImGui::Checkbox("Blue", &m_colorChannel[2]);
		ImGui::SameLine();
		if(colorComponentCount == 4)
		{
			ImGui::Checkbox("Alpha", &m_colorChannel[3]);
			ImGui::SameLine();
		}
		ImGui::Spacing();
		ImGui::SameLine();

		// Mips combo
		{
			StringListAuto mipLabels(getFrameAllocator());
			for(U32 mip = 0; mip < grTex.getMipmapCount(); ++mip)
			{
				mipLabels.pushBackSprintf("Mip %u (%llu x %llu)", mip, grTex.getWidth() >> mip,
										  grTex.getHeight() >> mip);
			}

			const U32 lastCrntMip = m_crntMip;
			if(ImGui::BeginCombo("##Mipmap", (mipLabels.getBegin() + m_crntMip)->cstr(), ImGuiComboFlags_HeightLarge))
			{
				for(U32 mip = 0; mip < grTex.getMipmapCount(); ++mip)
				{
					const Bool isSelected = (m_crntMip == mip);
					if(ImGui::Selectable((mipLabels.getBegin() + mip)->cstr(), isSelected))
					{
						m_crntMip = mip;
					}

					if(isSelected)
					{
						ImGui::SetItemDefaultFocus();
					}
				}
				ImGui::EndCombo();
			}

			if(lastCrntMip != m_crntMip)
			{
				// Re-create the image view
				TextureViewInitInfo viewInitInf(m_imageResource->getTexture());
				viewInitInf.m_firstMipmap = m_crntMip;
				viewInitInf.m_mipmapCount = 1;
				m_textureView = getSceneGraph().getGrManager().newTextureView(viewInitInf);
			}

			ImGui::SameLine();
		}

		// Depth
		if(grTex.getTextureType() == TextureType::_3D)
		{
			StringListAuto labels(getFrameAllocator());
			for(U32 d = 0; d < grTex.getDepth(); ++d)
			{
				labels.pushBackSprintf("Depth %u", d);
			}

			if(ImGui::BeginCombo("##Depth", (labels.getBegin() + U32(m_depth))->cstr(), ImGuiComboFlags_HeightLarge))
			{
				for(U32 d = 0; d < grTex.getDepth(); ++d)
				{
					const Bool isSelected = (m_depth == F32(d));
					if(ImGui::Selectable((labels.getBegin() + d)->cstr(), isSelected))
					{
						m_depth = F32(d);
					}

					if(isSelected)
					{
						ImGui::SetItemDefaultFocus();
					}
				}
				ImGui::EndCombo();
			}

			ImGui::SameLine();
		}

		ImGui::EndChild();
		ImGui::BeginChild("Image", Vec2(-1.0f, -1.0f), false,
						  ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_HorizontalScrollbar);

		// Image
		{
			// Center image
			const Vec2 imageSize = Vec2(F32(grTex.getWidth()), F32(grTex.getHeight())) * m_zoom;

			class ExtraPushConstants
			{
			public:
				Vec4 m_colorScale;
				Vec4 m_depth;
			} pc;
			pc.m_colorScale.x() = F32(m_colorChannel[0]);
			pc.m_colorScale.y() = F32(m_colorChannel[1]);
			pc.m_colorScale.z() = F32(m_colorChannel[2]);
			pc.m_colorScale.w() = F32(m_colorChannel[3]);

			pc.m_depth = Vec4((m_depth + 0.5f) / F32(grTex.getDepth()));

			canvas->setShaderProgram(m_imageGrProgram, &pc, sizeof(pc));

			ImGui::Image(UiImageId(m_textureView, m_pointSampling), imageSize, Vec2(0.0f, 1.0f), Vec2(1.0f, 0.0f),
						 Vec4(1.0f), Vec4(0.0f, 0.0f, 0.0f, 1.0f));

			canvas->clearShaderProgram();

			if(ImGui::IsItemHovered())
			{
				if(ImGui::GetIO().KeyCtrl)
				{
					// Zoom
					const F32 zoomSpeed = 0.05f;
					if(ImGui::GetIO().MouseWheel > 0.0f)
					{
						m_zoom *= 1.0f + zoomSpeed;
					}
					else if(ImGui::GetIO().MouseWheel < 0.0f)
					{
						m_zoom *= 1.0f - zoomSpeed;
					}

					// Pan
					if(ImGui::GetIO().MouseDown[0])
					{
						const Vec2 velocity = toAnki(ImGui::GetIO().MouseDelta);

						if(velocity.x() != 0.0f)
						{
							ImGui::SetScrollX(ImGui::GetScrollX() - velocity.x());
						}

						if(velocity.y() != 0.0f)
						{
							ImGui::SetScrollY(ImGui::GetScrollY() - velocity.y());
						}
					}
				}
			}
		}

		ImGui::EndChild();

		canvas->popFont();
		ImGui::End();
	}
};

class MyApp : public App
{
public:
	Error init(int argc, char** argv, CString appName)
	{
		if(argc < 2)
		{
			ANKI_LOGE("Wrong number of arguments");
			return Error::USER_DATA;
		}

		HeapAllocator<U32> alloc(allocAligned, nullptr);
		StringAuto mainDataPath(alloc, ANKI_SOURCE_DIRECTORY);

		ConfigSet config = DefaultConfigSet::get();
		config.set("window_fullscreen", false);
		config.set("rsrc_dataPaths", mainDataPath);
		config.set("gr_validation", 0);
		ANKI_CHECK(config.setFromCommandLineArguments(argc - 2, argv + 2));

		ANKI_CHECK(App::init(config, allocAligned, nullptr));

		// Load the texture
		ImageResourcePtr image;
		ANKI_CHECK(getResourceManager().loadResource(argv[1], image, false));

		// Change window name
		StringAuto title(alloc);
		title.sprintf("%s %llu x %llu Mips %u Format %s", argv[1], image->getWidth(), image->getHeight(),
					  image->getTexture()->getMipmapCount(), getFormatInfo(image->getTexture()->getFormat()).m_name);
		getWindow().setWindowTitle(title);

		// Create the node
		SceneGraph& scene = getSceneGraph();
		TextureViewerUiNode* node;
		ANKI_CHECK(scene.newSceneNode("TextureViewer", node));
		node->m_imageResource = image;

		return Error::NONE;
	}

	Error userMainLoop(Bool& quit, Second elapsedTime) override
	{
		Input& input = getInput();
		if(input.getKey(KeyCode::ESCAPE))
		{
			quit = true;
		}

		return Error::NONE;
	}
};

int main(int argc, char* argv[])
{
	Error err = Error::NONE;

	MyApp* app = new MyApp;
	err = app->init(argc, argv, "Texture Viewer");
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
