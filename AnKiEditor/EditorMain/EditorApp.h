#pragma once
#include "AnKi/AnKi.h"
namespace AnKiEditor
{
class EditorApp : public anki::App
{
public:
	anki::Error init();
};
} // namespace AnKiEditor
