#pragma once
#include <windows.h>
#include <wingdi.h>
#include <string>
#include <chrono>
#include <cwchar>
#include <vector>

class app_s
{
private:
	bool register_class();
	static std::wstring const s_class_name;
	static LRESULT CALLBACK window_proc_static(
		HWND window, UINT message,
		WPARAM wparam, LPARAM lparam);
	LRESULT window_proc(
		HWND window, UINT message,
		WPARAM wparam, LPARAM lparam);
	HWND create_window(DWORD style);

	const int Width[3] = { 800, 800, 1000 };
	const int Height[3] = { 600, 800, 800 };

	HWND m_main;
	int mainWidth = 800, mainHeight = 600;
	COLORREF backgroundColor = RGB(255, 255, 255);

	HINSTANCE m_instance;


	HWND hPlayer;
	std::vector<HWND> bullets;
	std::vector<HWND> enemies;

	int playerWidth = 50, playerHight = 50, enemyWidth = 50, enemyHeight = 40, bulletWidth = 5, bulletHeight = 15;
	const int playerMargin = 10;

	HBRUSH hbrBullet = CreateSolidBrush(RGB(0, 0, 0));

	int enemy_start_posx = 375;
	int enemy_start_posy = 50;

	int player_start_posx = 375;
	int player_start_posy = 400;

	const int enemy_per_row[3] = { 5, 8, 10 };
	const int row[3] = { 2, 3, 5 };

	int enemy_animation = 0;
	BOOL enemy_direction_left = true;
	const int enemy_step = 5;
	int number_of_steps = 0;
	void MoveEnemy();
	void MoveBullets();

	HBITMAP BMPlayer;
	HBITMAP BMPEnemy;


	HBITMAP backgroundImage = NULL; 
	wchar_t imageFilePath[MAX_PATH] = L"";
	
	void RenderSprites(HDC hdc);
	void UpdateMenuChecks(HMENU menu, int selectedId);

	void HandleKeydown(HWND window, WPARAM wparam);
	void HandleTimer(WPARAM wparam);
	void HandleMenuCommands(HWND window, UINT message, WPARAM wparam, LPARAM lparam);
	void HandleBackgroundColorChange(HWND window);
	void HandleBackgroundImageChange(HWND window);
	void HandlePaint(HWND window);
	void DrawBackground(HDC hdc, PAINTSTRUCT& ps);
	void DrawScore(HDC hdc, HWND window);

	int score = 0;
	int sizemode = 2;
	int background = 0;
	int backgroundmode = 3;

	void UpdateMenuChecks_BG(HMENU menu, int selectedId);
	void SaveConfig();
	void LoadConfig();
	void LoadEnemy();
	void LoadPlayer();

	void StartNewGame();

public:
	app_s(HINSTANCE instance);
	int run(int show_command);
};
