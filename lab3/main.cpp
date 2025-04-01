#include <windows.h>
#include "app_s.h"

int WINAPI wWinMain(HINSTANCE instance,
	HINSTANCE /*prevInstance*/,
	LPWSTR /*command_line*/,
	int show_command)
{
	app_s app{ instance };
	return app.run(show_command);

}