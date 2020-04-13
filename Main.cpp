#include "Menu.h"
#include <ctime>
#include <dwmapi.h>

vars Vars;
c_config g_configs;
c_offsets g_offsets;

LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

class KeyToggle {
public:
	KeyToggle(int key) :mKey(key), mActive(false) {}
	operator bool() {
		if (GetAsyncKeyState(mKey)) {
			if (!mActive) {
				mActive = true;
				return true;
			}
		}
		else
			mActive = false;
		return false;
	}
private:
	int mKey;
	bool mActive;
};

KeyToggle toggleF1(VK_F1);
KeyToggle toggleF2(VK_F2);
KeyToggle toggleF3(VK_F3);
KeyToggle toggleF4(VK_F4);
KeyToggle toggleF5(VK_F5);

POINT p;
bool moving = false;
int windowPosX = 0;
int windowPosY = 0;
int clickX = 0;
int clickY = 0;

double clockToMilliseconds(clock_t ticks) {
	// units/(units/time) => time (seconds) * 1000 = milliseconds
	return (ticks / (double)CLOCKS_PER_SEC)*1000.0;
}

clock_t deltaTime = 0;
unsigned int frames = 0;
double  frameRate = 30;
double  averageFrameTimeMilliseconds = 33.333;

int WINAPI WinMain(HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR lpCmdLine,
	int nCmdShow)
{
	if (!Process->setWindow("Sea of Thieves"))
	{
		MessageBoxA(NULL, "Failed To Find The Window", "Failed To Find The Window", MB_OK);
		return 1;
	}

	if (!Process->attachProcess("SoTGame.exe"))
	{
		MessageBoxA(NULL, "Failed To Find The Process", "Failed To Find The Process", MB_OK);
		return 1;
	}

	HWND hWnd;
	WNDCLASSEX wc;

	ZeroMemory(&wc, sizeof(WNDCLASSEX));

	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WindowProc;
	wc.hInstance = hInstance;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)COLOR_WINDOW;
	wc.lpszClassName = "WindowClass";

	RegisterClassEx(&wc);

	hWnd = CreateWindowEx(WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TOOLWINDOW| WS_EX_TRANSPARENT,
		"WindowClass",
		"RandomTitle",
		WS_POPUP,
		Process->Position[0], Process->Position[1],
		Process->Size[0], Process->Size[1],
		NULL,
		NULL,
		hInstance,
		NULL);
	SetLayeredWindowAttributes(hWnd, RGB(0, 0, 0), 255,  LWA_ALPHA);

	MARGINS margins = { -1 };
	DwmExtendFrameIntoClientArea(hWnd, &margins);

	ShowWindow(hWnd, nCmdShow);

	// set up and initialize Direct3D
	directX->initD3D(hWnd);
	Process->myWindow= hWnd;

	g_offsets.init();
	if (!g_offsets.load("offsets"))
	{
		MessageBoxA(NULL, "Failed To Find The Offsets File", "Please place the offsets file in the same folder as the Application", MB_OK);
		return 1;
	}

	g_configs.init();
	if (!g_configs.load("default"))
		g_configs.save("default");

	g_configs.save("default");
	MSG msg;

	while (TRUE)
	{
		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);

		}

		if (msg.message == WM_QUIT)
			break;

		if (toggleF1)
			Menu->menuOpen = !Menu->menuOpen;
		if (toggleF2)
			Vars.ESP.Animals.bActive = !Vars.ESP.Animals.bActive;
		if (toggleF3)
			Vars.ESP.Player.bActive = !Vars.ESP.Player.bActive;
		if (toggleF4)
			Vars.ESP.Ships.bActive = !Vars.ESP.Ships.bActive;
		if (toggleF5)
			Vars.ESP.Treasure.bActive = !Vars.ESP.Treasure.bActive;

		if (GetAsyncKeyState(VK_INSERT))
			g_configs.load("default");

		if (GetAsyncKeyState(VK_END))
			break;

		Process->getSize();
		MoveWindow(hWnd, Process->Position[0], Process->Position[1], Process->Size[0], Process->Size[1], false);

		if (Process->isWindowMaximized())
			directX->Reset();
		else
		{
			clock_t beginFrame = clock();
			if (!directX->resetLock)
			directX->Render(Process->isWindowActive());
			clock_t endFrame = clock();

			deltaTime += endFrame - beginFrame;
			frames++;

			//if you really want FPS
			if (clockToMilliseconds(deltaTime) > 1000.0) { //every second
				directX->frames = frames;//(double)frames * 0.5 + frameRate * 0.5; //more stable
				frames = 0;
				deltaTime -= CLOCKS_PER_SEC;
				averageFrameTimeMilliseconds = 1000.0 / (frameRate == 0 ? 0.001 : frameRate);
			}
		}
	}

	// clean up DirectX and COM
	directX->cleanD3D();

	return msg.wParam;
}

LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_SIZE:
		Process->zoomOnce = false;
		directX->Reset();
		break;
	case WM_MOUSEMOVE:
		Menu->curPos.x = GET_X_LPARAM(lParam);
		Menu->curPos.y = GET_Y_LPARAM(lParam);
		break;
	case WM_LBUTTONDOWN:
	{

		break;
	}
	case WM_LBUTTONUP:
	{

		break;
	}
	case WM_DESTROY:
	{
		PostQuitMessage(0);
		return 0;
	} break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}

	return 0;
}
