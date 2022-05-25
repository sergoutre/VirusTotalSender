#ifndef MAIN_H
#define MAIN_H

//#define DEBUG

#include <string>
#include <map>

#include <d3d9.h>
#include <tchar.h>

bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void ResetDevice();
void applyImGuiStyle();

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LPDIRECT3D9 g_pD3D = NULL;
LPDIRECT3DDEVICE9 g_pd3dDevice = NULL;
D3DPRESENT_PARAMETERS g_d3dpp = { };

std::string appName = "VT Sender";
std::map<std::string, float> mainWindow = { {"width", 800.0f}, {"height", 600.0f} };

inline static std::string removeSpaces(std::string input)
{
    input.erase(std::remove(input.begin(), input.end(), ' '), input.end());
    return input;
}

inline static bool hasLoaded()
{
    CreateMutex(NULL, FALSE, "VT Sender");
    return GetLastError() == ERROR_ALREADY_EXISTS;
}

#endif // MAIN_H