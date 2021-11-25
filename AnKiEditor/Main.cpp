#include "EditorMain/EditorApp.h"
#include <iostream>
using namespace AnKiEditor;
void main()
{
	EditorApp* app = new EditorApp();
	if(app->init() == anki::Error::NONE)
	{
		app->mainLoop();
	}
	else
	{
		std::cout << "editor window init failed!" << std::endl;
	}
}
