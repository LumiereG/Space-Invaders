
#include "app_s.h"
#include <dwmapi.h>
#include <stdexcept>
#include <iostream>
#include "resource.h"

#define CONFIG_FILE L"D:\\config.ini"

std::wstring const app_s::s_class_name{ L" Space Invaders" };

INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
bool SaveHBITMAPToFile(HBITMAP hBitmap, const std::wstring& filePath);

#define CONFIG_FILE L"D:\\config.ini"
void EnsureConfigExists()
{
	FILE* file;
	_wfopen_s(&file, CONFIG_FILE, L"a+"); 
	if (file) fclose(file);
}

// Register the window class
bool app_s::register_class() {
	WNDCLASSEXW desc{};

	// Check if the class is already registered
	if (GetClassInfoExW(m_instance, s_class_name.c_str(),&desc) != 0) 
		return true;

	// Describe the window class
	desc = {
	.cbSize = sizeof(WNDCLASSEXW),
	.lpfnWndProc = window_proc_static,
	.hInstance = m_instance,
	.hIcon = LoadIconW(m_instance, MAKEINTRESOURCEW(ID_APPICON)),
	.hCursor = LoadCursorW(nullptr, L"IDC_ARROW"),
	.hbrBackground =
		CreateSolidBrush(RGB(255, 255, 255)),
	.lpszMenuName = MAKEINTRESOURCEW(IDR_MENU1),
	.lpszClassName = s_class_name.c_str() 
	};

	return RegisterClassExW(&desc) != 0;
}

// Function to create the main window
HWND app_s::create_window(DWORD style)
{
	RECT size{ 0, 0, 120, 120 };
	AdjustWindowRectEx(&size, style, true, 0);
	HWND window = CreateWindowExW(
		WS_OVERLAPPED | WS_SYSMENU | WS_EX_TOPMOST | WS_EX_LAYERED | WS_MINIMIZEBOX,
		s_class_name.c_str(),
		L"Space Invaders",
		style,
		(GetSystemMetrics(SM_CXSCREEN) - mainWidth) / 2, (GetSystemMetrics(SM_CYSCREEN) - mainHeight) / 2,
		mainWidth, mainHeight,
		nullptr,
		nullptr,
		m_instance,
		this);

	return window;
}

// Static window procedure function
LRESULT app_s::window_proc_static(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{
	app_s* app = nullptr;
	if (message == WM_NCCREATE)
	{
		auto p = reinterpret_cast<LPCREATESTRUCTW>(lparam);
		app = static_cast<app_s*>(p->lpCreateParams);
		SetWindowLongPtrW(window, GWLP_USERDATA,
			reinterpret_cast<LONG_PTR>(app));
	}
	else
	{
		app = reinterpret_cast<app_s*>(
			GetWindowLongPtrW(window, GWLP_USERDATA));
	}
	if (app != nullptr)
	{
		return app->window_proc(window, message,
			wparam, lparam);
	}
	return DefWindowProcW(window, message, wparam, lparam);
}

// Main window message handling function
LRESULT app_s::window_proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{
	switch (message) {
	case WM_CLOSE:
		DestroyWindow(window);
		return 0;
	case WM_DESTROY:
		if (window == m_main)
		{
			SaveConfig();
		}
		PostQuitMessage(EXIT_SUCCESS);
		return 0;
	case WM_ACTIVATE:
		if (LOWORD(wparam) == WA_INACTIVE) {
			// Set window transparency when inactive
			SetLayeredWindowAttributes(window, 0, (BYTE)(255 * 40 / 100), LWA_ALPHA);
		}
		else {
			// Restore window opacity when active
			SetLayeredWindowAttributes(window, 0, 255, LWA_ALPHA);
		}
		return 0;
	case WM_CTLCOLORSTATIC:
		// Check if any bullet is over a static control
		for (auto& f : bullets)
		{
			if ((HWND)lparam == f)
				return reinterpret_cast<INT_PTR>(hbrBullet);
		}
		return 0;
	case WM_KEYDOWN:
		// Handle key press for player movement and shooting
		HandleKeydown(window, wparam);
		return 0;
	case WM_TIMER:
		// Handle timers for enemy movement and bullet movement
		HandleTimer(wparam);
		return 0;

	case WM_COMMAND:
	{
		// Handle menu command actions
		HandleMenuCommands(window, message, wparam, lparam);
		return 0;
	}
	case WM_PAINT:
	{
		// Handle window painting (drawing)
		HandlePaint(window);
		return 0;
	}
	}

	return DefWindowProcW(window, message, wparam, lparam);
}

// Handle key press for player movement and shooting
void app_s::HandleKeydown(HWND window, WPARAM wparam) 
{
	RECT r;
	GetWindowRect(hPlayer, &r);
	MapWindowPoints(HWND_DESKTOP, m_main, (LPPOINT)&r, 2);

	if (wparam == VK_LEFT) {
		// Move left if not at the edge
		if (r.left > 10) r.left -= 10;
		SetWindowPos(hPlayer, nullptr, r.left, r.top, 0, 0, SWP_NOZORDER | SWP_NOSIZE | SWP_NOREDRAW);
		InvalidateRect(window, nullptr, TRUE);
	}
	else if (wparam == VK_RIGHT) {
		// Move right if not at the edge
		if (r.right < mainWidth - 10) r.left += 10;
		SetWindowPos(hPlayer, nullptr, r.left, r.top, 0, 0, SWP_NOZORDER | SWP_NOSIZE | SWP_NOREDRAW);
		InvalidateRect(window, nullptr, TRUE);
	}
	else if (wparam == VK_SPACE)
	{
		// Handle space bar to shoot
		HWND bullet = CreateWindowExW(0, L"STATIC", nullptr, WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_VISIBLE,
			r.left + playerWidth / 2, r.top + playerHight / 2, bulletWidth, bulletHeight, m_main, nullptr, m_instance, nullptr);
		if (bullets.size() == 0) SetTimer(m_main, 2, 50, nullptr);
		bullets.push_back(bullet);
	}
}

// Handle timer events for moving enemies and bullets
void app_s::HandleTimer(WPARAM wparam) {
	if (wparam == 0)
	{
		MoveEnemy();
		InvalidateRect(m_main, nullptr, TRUE);
	}
	else
	{
		MoveBullets();
	}
}

// Handle commands from the menu
void app_s::HandleMenuCommands(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{
	int wmId = LOWORD(wparam);
	HMENU menu = GetMenu(window);
	switch (wmId)
	{
	case ID_ABOUT:
		// Open about dialog box
		DialogBox(m_instance, MAKEINTRESOURCE(IDD_DIALOG1), window, About);
		break;
	case IDM_EXIT:
		// Close the application
		DestroyWindow(window);
		break;
	case ID_GAME_NEWGAME:
		// Start a new game
		StartNewGame();
		break;
	case ID_SIZE_SMALL:
		// Change to small size
		sizemode = 0;
		StartNewGame();
		break;
	case ID_SIZE_MEDIUM:
		// Change to medium size
		sizemode = 1;
		StartNewGame();
		break;
	case ID_SIZE_LARGE:
		// Change to large size
		sizemode = 2;
		StartNewGame();
		break;
	case ID_BACKGROUND_COLOR:
		// Change background color
		HandleBackgroundColorChange(window);
		break;
	case ID_BACKGROUND_IMAGE:
		// Change background image
		HandleBackgroundImageChange(window);
		break;
	case ID_IMAGE_CENTER:
		// Set background image mode to center
		backgroundmode = 0;
		UpdateMenuChecks_BG(menu, ID_IMAGE_CENTER);
		InvalidateRect(window, nullptr, TRUE);
		break;
	case ID_IMAGE_FILL:
		// Set background image mode to fill
		backgroundmode = 1;
		UpdateMenuChecks_BG(menu, ID_IMAGE_FILL);
		InvalidateRect(window, nullptr, TRUE);
		break;
	case ID_IMAGE_TILE:
		// Set background image mode to tile
		backgroundmode = 2;
		UpdateMenuChecks_BG(menu, ID_IMAGE_TILE);
		InvalidateRect(window, nullptr, TRUE);
		break;
	case ID_IMAGE_FIT:
		// Set background image mode to fit
		backgroundmode = 3;
		UpdateMenuChecks_BG(menu, ID_IMAGE_FIT);
		InvalidateRect(window, nullptr, TRUE);
		break;
	default:
		DefWindowProcW(window, message, wparam, lparam);
	}
}

// Function to handle background color change
void app_s::HandleBackgroundColorChange(HWND window) 
{
	background = 0;
	CHOOSECOLOR cc;
	static COLORREF acrCustClr[16];
	ZeroMemory(&cc, sizeof(cc));
	cc.lStructSize = sizeof(cc);
	cc.hwndOwner = window;
	cc.lpCustColors = (LPDWORD)acrCustClr;
	cc.Flags = CC_FULLOPEN | CC_RGBINIT;
	
	if (ChooseColor(&cc) == TRUE)
	{
		backgroundColor = cc.rgbResult;
		InvalidateRect(window, nullptr, TRUE);
	}
	backgroundImage = nullptr;
}

// Function to handle background image change
void app_s::HandleBackgroundImageChange(HWND window) 
{
	background = 1;
	OPENFILENAME ofn;
	wchar_t szFile[260];
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = window;
	ofn.lpstrFile = szFile;
	ofn.nMaxFile = sizeof(szFile);
	ofn.lpstrFilter = L"Image Files\0*.BMP\0All Files\0*.*\0";
	ofn.nFilterIndex = 1;
	ofn.lpstrFile[0] = '\0';
	ofn.nMaxFile = sizeof(szFile);
	ofn.lpstrFileTitle = nullptr;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = nullptr;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
	if (GetOpenFileName(&ofn) == TRUE)
	{
		wcscpy_s(imageFilePath, ARRAYSIZE(imageFilePath), ofn.lpstrFile);
		backgroundImage = (HBITMAP)LoadImageW(nullptr, imageFilePath, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
		backgroundColor = RGB(255, 255, 255);
		InvalidateRect(window, nullptr, TRUE);
	}
}

// Constructor for the app_s class, initializes the application
app_s::app_s(HINSTANCE instance) : m_instance{ instance }, m_main{}
{
	register_class();

	DWORD main_style = WS_OVERLAPPED | WS_SYSMENU | WS_CAPTION | WS_MINIMIZEBOX;

	// Load bitmaps for player and enemy
	BMPlayer = LoadBitmapW(m_instance, MAKEINTRESOURCE(IDB_BITMAP2));
	BMPEnemy = LoadBitmapW(m_instance, MAKEINTRESOURCE(IDB_BITMAP1));

	// Load the configuration settings from a file
	LoadConfig();

	// Set the dimensions based on the size mode
	mainWidth = Width[sizemode];
	mainHeight = Height[sizemode];

	// Calculate starting positions for the player and enemies
	player_start_posx = mainWidth / 2 - playerWidth / 2;
	enemy_start_posx = mainWidth / 2 - enemy_per_row[sizemode] * (enemyWidth + 10) / 2;
	player_start_posy = mainHeight - 150;

	// Create the main window
	m_main = create_window(main_style);

	// Set the extended window style for layering
	SetWindowLongW(m_main, GWL_EXSTYLE, GetWindowLongW(m_main, GWL_EXSTYLE | WS_EX_LAYERED));

	// Create player window and set initial position
	hPlayer = CreateWindowExW(0, L"STATIC", nullptr, WS_CHILD | WS_VISIBLE | SS_BITMAP, player_start_posx, player_start_posy, playerWidth, playerHight, m_main, nullptr, m_instance, nullptr);

	// Load the enemies from the saved configuration
	LoadEnemy();

	// If no enemies are present, start a new game
	if (score == 0 && enemies.size() == 0) StartNewGame();

	// Update the menu items based on the current selection
	UpdateMenuChecks(GetMenu(m_main), ID_SIZE_SMALL + sizemode);
	UpdateMenuChecks_BG(GetMenu(m_main), ID_IMAGE_CENTER + backgroundmode);
}

// Main run loop for the application
int app_s::run(int show_command)
{
	// Set a timer for the game to update every 50 milliseconds
	SetTimer(m_main, 0, 50, nullptr);

	ShowWindow(m_main, show_command);

	MSG msg{};
	BOOL result = TRUE;
	HACCEL shortcuts = LoadAcceleratorsW(m_instance, MAKEINTRESOURCEW(IDR_ACCELERATOR1));

	// Message loop to handle events
	while ((result = GetMessageW(&msg, nullptr, 0, 0)) != 0)
	{
		if (result == -1)
			return EXIT_FAILURE;
		if (!TranslateAcceleratorW(msg.hwnd, shortcuts, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessageW(&msg);
		}
	}
	return EXIT_SUCCESS;
}

// Moves enemies across the screen

void app_s::MoveEnemy()
{
	for (int i = 0; i < enemies.size(); i++)
	{
		RECT r;
		GetWindowRect(enemies[i], &r);
		MapWindowPoints(HWND_DESKTOP, m_main, (LPPOINT)&r, 2);

		// Move enemy left or right based on the direction flag
		if (enemy_direction_left)
		{
			if (number_of_steps < 10) r.left -= enemy_step;
		}
		else
		{
			if (number_of_steps < 10) r.left += enemy_step;
		}

		// Set the new position of the enemy
		SetWindowPos(enemies[i], nullptr, r.left, r.top, 0, 0, SWP_NOZORDER | SWP_NOSIZE | SWP_NOREDRAW);
	}

	// Update the step count and switch direction when limit is reached
	number_of_steps++;
	if (number_of_steps >= 10)
	{
		number_of_steps = 0;
		enemy_direction_left = !enemy_direction_left;
	}
}

// Moves bullets upwards on the screen

void app_s::MoveBullets()
{
	for (int i = 0; i < bullets.size(); i++) {
		RECT r, enemy_position, inter;
		GetWindowRect(bullets[i], &r);
		MapWindowPoints(HWND_DESKTOP, m_main, (LPPOINT)&r, 2);

		// Check if bullet is out of screen, destroy it if so
		if (r.top <= 0)
		{
			DestroyWindow(bullets[i]);
			bullets.erase(bullets.begin() + i);
			--i;
			if (bullets.size() == 0) KillTimer(m_main, 2);
		}
		else
		{
			r.top -= 15; // Move the bullet upwards
			SetWindowPos(bullets[i], nullptr, r.left, r.top, bulletWidth, bulletHeight, SWP_NOZORDER | SWP_NOSIZE | SWP_NOREDRAW);
			
			// Check for collision with enemies
			for (int j = 0; j < enemies.size(); j++) {
				GetWindowRect(enemies[j], &enemy_position);
				MapWindowPoints(HWND_DESKTOP, m_main, (LPPOINT)&enemy_position, 2);
				
				// If collision, destroy the enemy and the bullet
				if (IntersectRect(&inter, &r, &enemy_position)) {
					DestroyWindow(enemies[j]);
					enemies.erase(enemies.begin() + j);
					--j;
					if (enemies.size() == 0) KillTimer(m_main, 0);
					score++;
					DestroyWindow(bullets[i]);
					bullets.erase(bullets.begin() + i);
					--i;
					break;
				}
			}
		}
	}
}

// Dialog procedure for the "About" dialog box

INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}

// Renders the player and enemy sprites to the screen

void app_s::RenderSprites(HDC hdc) {

	RECT player;
	GetWindowRect(hPlayer, &player);
	MapWindowPoints(HWND_DESKTOP, m_main, (LPPOINT)&player, 2);

	HDC hdcMem = CreateCompatibleDC(hdc);
	SelectObject(hdcMem, BMPlayer);  // Select player bitmap

	COLORREF transparentColor = GetPixel(hdcMem, 0, 0); // Get transparent color for the player sprite

	// Use TransparentBlt to render player sprite with transparency
	TransparentBlt(hdc, player.left, player.top, playerWidth, playerHight, hdcMem, 0, 0, playerWidth, playerHight, transparentColor);

	SelectObject(hdcMem, BMPEnemy); // Select enemy bitmap
	COLORREF transparentCr = GetPixel(hdcMem, 0, 0); // Get transparent color for the enemy sprite

	// Render each enemy sprite
	for (int i = 0; i < enemies.size(); i++)
	{
		RECT r;
		GetWindowRect(enemies[i], &r);
		MapWindowPoints(HWND_DESKTOP, m_main, (LPPOINT)&r, 2);
		TransparentBlt(hdc, r.left, r.top, enemyWidth, enemyHeight, hdcMem, 0, 0, enemyWidth, enemyHeight, transparentCr);
	}
	enemy_animation++; // Increment enemy animation step
	DeleteDC(hdcMem);
}

// Updates the menu checks for size options
void app_s::UpdateMenuChecks(HMENU menu, int selectedId) {
	for (int i = ID_SIZE_SMALL; i <= ID_SIZE_LARGE; i++) {
		CheckMenuItem(menu, i, (i == selectedId) ? MF_CHECKED : MF_UNCHECKED);
	}
}

// Updates the menu checks for background options
void app_s::UpdateMenuChecks_BG(HMENU menu, int selectedId) {
	for (int i = ID_IMAGE_CENTER; i <= ID_IMAGE_FIT; i++) {
		CheckMenuItem(menu, i, (i == selectedId) ? MF_CHECKED : MF_UNCHECKED);
	}
}

// Saves the current configuration settings to a file


void app_s::SaveConfig() {

	EnsureConfigExists();  // Ensure the config file exists

	// Save various settings such as size mode, background, score, and enemy positions
	WritePrivateProfileString(L"Settings", L"Sizemode", std::to_wstring(sizemode).c_str(), CONFIG_FILE);
	WritePrivateProfileString(L"Settings", L"Background", std::to_wstring(background).c_str(), CONFIG_FILE);
	WritePrivateProfileString(L"Settings", L"Background_mode", std::to_wstring(backgroundmode).c_str(), CONFIG_FILE);
	WritePrivateProfileString(L"Settings", L"Score", std::to_wstring(score).c_str(), CONFIG_FILE);

	if (background == 0)
	{
		// If background is a color, save its value
		wchar_t colorString[10];
		wsprintf(colorString, L"#%02X%02X%02X", GetRValue(backgroundColor), GetGValue(backgroundColor), GetBValue(backgroundColor));
		WritePrivateProfileString(L"Settings", L"Background_Color", colorString, CONFIG_FILE);
	}
	else
	{
		// Otherwise, save the image file path
		WritePrivateProfileString(L"Settings", L"Background_Image_Path", imageFilePath, CONFIG_FILE);
	}

	// Save each enemy's position in the configuration file
	for (int i = 0; i < enemies.size(); i++)
	{
		std::wstring section = L"Enemy_" + std::to_wstring(i);
		RECT enemy;
		GetWindowRect(enemies[i], &enemy);
		MapWindowPoints(HWND_DESKTOP, m_main, (LPPOINT)&enemy, 2);
		WritePrivateProfileString(section.c_str(), L"X", std::to_wstring(enemy.left).c_str(), CONFIG_FILE);
		WritePrivateProfileString(section.c_str(), L"Y", std::to_wstring(enemy.top).c_str(), CONFIG_FILE);
	}

	WritePrivateProfileString(L"Settings", L"EnemyCount", std::to_wstring(enemies.size()).c_str(), CONFIG_FILE);
}

// Loads the configuration settings from a file

void app_s::LoadConfig() 
{
	// Load settings like size mode, background, score, etc.
	sizemode = GetPrivateProfileInt(L"Settings", L"Sizemode", 0, CONFIG_FILE);
	background = GetPrivateProfileInt(L"Settings", L"Background", 0, CONFIG_FILE);
	backgroundmode = GetPrivateProfileInt(L"Settings", L"Background_mode", 3, CONFIG_FILE);
	score = GetPrivateProfileInt(L"Settings", L"Score", 0, CONFIG_FILE);

	if (background == 0)
	{
		wchar_t colorString[10];
		GetPrivateProfileString(L"Settings", L"Background_Color", L"#FFFFFF", colorString, 10, CONFIG_FILE);
		int r, g, b;
		if (swscanf_s(colorString, L"#%02X%02X%02X", &r, &g, &b) == 3)
		{
			backgroundColor = RGB(r, g, b);
		}
		else
		{
			backgroundColor = RGB(255, 255, 255);
		}
	}
	else
	{
		// Load background image path if background is an image
		GetPrivateProfileString(L"Settings", L"Background_Image_Path", L"", imageFilePath, MAX_PATH, CONFIG_FILE);
		backgroundImage = (HBITMAP)LoadImageW(nullptr, imageFilePath, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
	}
}

// Loads enemies' positions from configuration file and places them on the screen
void app_s::LoadEnemy()
{
	enemies.clear(); // Clear the current list of enemies
	int enemyCount = GetPrivateProfileInt(L"Settings", L"EnemyCount", 0, CONFIG_FILE);
	
	// For each enemy, load its position and create a new window for it
	for (int i = 0; i < enemyCount; i++) {
		std::wstring section = L"Enemy_" + std::to_wstring(i);
		int x = GetPrivateProfileInt(section.c_str(), L"X", 0, CONFIG_FILE);
		int y = GetPrivateProfileInt(section.c_str(), L"Y", 0, CONFIG_FILE);
		HWND enemy = CreateWindowExW(0, L"STATIC", nullptr, WS_CHILD | WS_VISIBLE | SS_BITMAP,
			x, y, enemyWidth, enemyHeight, m_main, nullptr, m_instance, nullptr);
		enemies.push_back(enemy);
	}
}

// Starts a new game by resetting positions, score, and enemies
void app_s::StartNewGame()
{
	// Destroy any existing enemies and reset game state
	for (int i = 0; i < enemies.size(); i++)
		DestroyWindow(enemies[i]);
	enemies.clear();

	// Get screen dimensions for centering the game window
	int screenWidth = GetSystemMetrics(SM_CXSCREEN);
	int screenHeight = GetSystemMetrics(SM_CYSCREEN);
	int xPos, yPos;

	// Center window on screen
	xPos = (screenWidth - Width[sizemode]) / 2;
	yPos = (screenHeight - Height[sizemode]) / 2;

	// Reset window and player positions, and score
	mainWidth = Width[sizemode];
	mainHeight = Height[sizemode];
	player_start_posy = mainHeight - 150;
	player_start_posx = mainWidth / 2 - playerWidth / 2;
	enemy_start_posx = mainWidth / 2 - enemy_per_row[sizemode] * enemyWidth / 2;
	score = 0;

	// Set window size and position
	SetWindowPos(m_main, nullptr, xPos, yPos, mainWidth, mainHeight, SWP_NOZORDER);
	SetWindowPos(hPlayer, nullptr, player_start_posx, player_start_posy, playerWidth, playerHight, SWP_NOZORDER);

	// Create new enemies based on the selected size mode
	for (int i = 0; i < row[sizemode]; i++)
		for (int j = 0; j < enemy_per_row[sizemode]; j++)
		{
			int posx = j * (enemyWidth + 10) + enemy_start_posx;
			int posy = i * (enemyHeight + 10) + enemy_start_posy;

			HWND enemy = CreateWindowExW(0, L"STATIC", nullptr, WS_CHILD | WS_VISIBLE | SS_BITMAP,posx, posy, enemyWidth, enemyHeight, m_main, nullptr, m_instance, nullptr);
			enemies.push_back(enemy);
		}

	// Update menu items for size and background options
	UpdateMenuChecks(GetMenu(m_main), ID_SIZE_SMALL + sizemode);
	UpdateMenuChecks_BG(GetMenu(m_main), ID_IMAGE_CENTER + backgroundmode);

	// Set a timer for game updates
	SetTimer(m_main, 0, 50, nullptr);

}

// Draws the background based on the selected background color or image
void app_s::DrawBackground(HDC hdc, PAINTSTRUCT& ps) {
	// Draw the background color
	if (backgroundColor != RGB(255, 255, 255)) {
		HBRUSH hBrush = CreateSolidBrush(backgroundColor);
		FillRect(hdc, &ps.rcPaint, hBrush);
		DeleteObject(hBrush);
	}
	// Draw the background image
	else if (backgroundImage != nullptr) {
		HDC hdcMem = CreateCompatibleDC(hdc);
		SelectObject(hdcMem, backgroundImage);
		BITMAP bitmap;
		GetObjectW(backgroundImage, sizeof(bitmap), &bitmap);

		// Choose the drawing mode based on the selected background mode
		switch (backgroundmode) {
		case 0:
			BitBlt(hdc, (ps.rcPaint.right - bitmap.bmWidth) / 2,
				(ps.rcPaint.bottom - bitmap.bmHeight) / 2,
				bitmap.bmWidth, bitmap.bmHeight, hdcMem, 0, 0, SRCCOPY);
			break;
		case 1:
			StretchBlt(hdc, 0, 0, mainWidth, mainHeight, hdcMem, 0, 0,
				bitmap.bmWidth, bitmap.bmHeight, SRCCOPY);
			break;
		case 2:
			for (int y = 0; y < ps.rcPaint.bottom; y += bitmap.bmHeight) {
				for (int x = 0; x < ps.rcPaint.right; x += bitmap.bmWidth) {
					BitBlt(hdc, x, y, bitmap.bmWidth, bitmap.bmHeight, hdcMem, 0, 0, SRCCOPY);
				}
			}
			break;
		case 3:
			// Scale the background to fit the window while maintaining aspect ratio
			float ratio = min((float)(ps.rcPaint.right - ps.rcPaint.left) / bitmap.bmWidth,
				(float)(ps.rcPaint.bottom - ps.rcPaint.top) / bitmap.bmHeight);
			int newWidth = (int)(bitmap.bmWidth * ratio);
			int newHeight = (int)(bitmap.bmHeight * ratio);
			int xPos = (ps.rcPaint.right - newWidth) / 2;
			int yPos = (ps.rcPaint.bottom - newHeight) / 2;
			StretchBlt(hdc, xPos, yPos, newWidth, newHeight, hdcMem, 0, 0,
				bitmap.bmWidth, bitmap.bmHeight, SRCCOPY);
			break;
		}
		DeleteDC(hdcMem);
	}
}

// Draws the score text on the screen
void app_s::DrawScore(HDC hdc, HWND window) {
	HFONT hFont = CreateFont(25, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FF_DONTCARE, L"Lucida Console");
	HFONT oldFont = (HFONT)SelectObject(hdc, hFont);

	// Prepare the score text
	std::wstring scoreText = L"Score: " + std::to_wstring(score);
	SetTextColor(hdc, RGB(0, 0, 0));
	SetBkMode(hdc, TRANSPARENT);

	// Define the area for the score text
	RECT rect;
	GetClientRect(window, &rect);
	rect.top = rect.bottom - 30;
	rect.left = 10;
	DrawTextW(hdc, scoreText.c_str(), -1, &rect, DT_LEFT | DT_NOCLIP);

	// Clean up the font
	SelectObject(hdc, oldFont);
	DeleteObject(hFont);
}

// Handles the paint event, which includes rendering the background, score, and sprites
void app_s::HandlePaint(HWND window) {
	PAINTSTRUCT ps;
	HDC hdcPaint = BeginPaint(window, &ps);

	// Draw the background (either color or image)
	DrawBackground(hdcPaint, ps);

	// Draw the score text
	DrawScore(hdcPaint, window);

	// Render sprites (player, enemies, etc.)
	RenderSprites(hdcPaint);

	EndPaint(window, &ps);
}