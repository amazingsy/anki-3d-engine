// Copyright (C) 2009-2021, Panagiotis Christopoulos Charitos and contributors.
// All rights reserved.
// Code licensed under the BSD License.
// http://www.anki3d.org/LICENSE

#include <AnKi/Input/InputAndroid.h>
#include <AnKi/Core/NativeWindowAndroid.h>
#include <AnKi/Util/Logger.h>

namespace anki {

Error Input::newInstance(AllocAlignedCallback allocCallback, void* allocCallbackUserData, NativeWindow* nativeWindow,
						 Input*& input)
{
	ANKI_ASSERT(allocCallback && nativeWindow);

	HeapAllocator<U8> alloc(allocCallback, allocCallbackUserData, "Input");
	InputAndroid* ainput =
		static_cast<InputAndroid*>(alloc.getMemoryPool().allocate(sizeof(InputAndroid), alignof(InputAndroid)));
	::new(ainput) InputAndroid();

	ainput->m_alloc = alloc;
	ainput->m_nativeWindow = nativeWindow;

	const Error err = ainput->init();
	if(err)
	{
		ainput->~InputAndroid();
		alloc.getMemoryPool().free(ainput);
		input = nullptr;
		return err;
	}
	else
	{
		input = ainput;
		return Error::NONE;
	}
}

void Input::deleteInstance(Input* input)
{
	if(input)
	{
		InputAndroid* self = static_cast<InputAndroid*>(input);
		HeapAllocator<U8> alloc = self->m_alloc;
		self->~InputAndroid();
		alloc.getMemoryPool().free(self);
	}
}

Error Input::handleEvents()
{
	for(U32& k : m_touchPointers)
	{
		if(k)
		{
			++k;
		}
	}

	int ident;
	int events;
	android_poll_source* source;

	while((ident = ALooper_pollAll(0, nullptr, &events, reinterpret_cast<void**>(&source))) >= 0)
	{
		if(source != nullptr)
		{
			source->process(g_androidApp, source);
		}
	}

	return Error::NONE;
}

void Input::moveCursor(const Vec2& posNdc)
{
	m_mousePosNdc = posNdc;
	m_mousePosWin =
		UVec2((posNdc * 0.5f + 0.5f) * Vec2(F32(m_nativeWindow->getWidth()), F32(m_nativeWindow->getHeight())));
}

void Input::hideCursor(Bool hide)
{
	// do nothing
}

Error InputAndroid::init()
{
	ANKI_ASSERT(m_nativeWindow);
	g_androidApp->userData = this;

	g_androidApp->onAppCmd = [](android_app* app, int32_t cmd) {
		InputAndroid* self = static_cast<InputAndroid*>(app->userData);
		self->handleAndroidEvents(app, cmd);
	};

	g_androidApp->onInputEvent = [](android_app* app, AInputEvent* event) -> int {
		InputAndroid* self = static_cast<InputAndroid*>(app->userData);
		return self->handleAndroidInput(app, event);
	};

	return Error::NONE;
}

void InputAndroid::handleAndroidEvents(android_app* app, int32_t cmd)
{
	switch(cmd)
	{
	case APP_CMD_TERM_WINDOW:
	case APP_CMD_LOST_FOCUS:
		addEvent(InputEvent::WINDOW_CLOSED);
		break;
	}
}

int InputAndroid::handleAndroidInput(android_app* app, AInputEvent* event)
{
	const I32 type = AInputEvent_getType(event);
	const I32 source = AInputEvent_getSource(event);
	I32 handled = 0;

	switch(type)
	{
	case AINPUT_EVENT_TYPE_KEY:
		// TODO
		break;

	case AINPUT_EVENT_TYPE_MOTION:
	{
		const I32 pointer = AMotionEvent_getAction(event);
		const I32 action = pointer & AMOTION_EVENT_ACTION_MASK;
		const I32 index =
			(pointer & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;

		if(source & AINPUT_SOURCE_JOYSTICK)
		{
			// TODO
		}
		else if(source & AINPUT_SOURCE_TOUCHSCREEN)
		{
			auto update = [event, this](U32 index, U32 pressValue) {
				const F32 x = AMotionEvent_getX(event, index);
				const F32 y = AMotionEvent_getY(event, index);
				const I32 id = AMotionEvent_getPointerId(event, index);

				m_touchPointerPosWin[id] = UVec2(U32(x), U32(y));

				m_touchPointerPosNdc[id].x() = F32(x) / F32(m_nativeWindow->getWidth()) * 2.0f - 1.0f;
				m_touchPointerPosNdc[id].y() = -(F32(y) / F32(m_nativeWindow->getHeight()) * 2.0f - 1.0f);

				if(pressValue == 0 || pressValue == 1)
				{
					m_touchPointers[id] = pressValue;
				}
			};

			switch(action)
			{
			case AMOTION_EVENT_ACTION_DOWN:
			case AMOTION_EVENT_ACTION_POINTER_DOWN:
				update(index, 1);
				break;
			case AMOTION_EVENT_ACTION_MOVE:
			{
				const U32 count = U32(AMotionEvent_getPointerCount(event));
				for(U32 i = 0; i < count; i++)
				{
					update(i, 2);
				}
				break;
			}
			case AMOTION_EVENT_ACTION_UP:
			case AMOTION_EVENT_ACTION_POINTER_UP:
				update(index, 0);
				break;

			default:
				break;
			}
		}
		break;
	}

	default:
		break;
	}

	return handled;
}

} // end namespace anki
