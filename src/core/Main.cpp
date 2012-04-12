#include <windows.h>

extern int APIENTRY aWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdLine, int nCmdShow);
int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdLine, int nCmdShow)
{

	aWinMain(hInstance, hPrevInstance, lpszCmdLine, nCmdShow);
}