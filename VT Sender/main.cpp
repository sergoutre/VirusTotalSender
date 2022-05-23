#include "main.h" 

#include "classes/VirusTotal.h"
#include "classes/Requests.h"
#include "classes/Config.h"

#include <imgui.h>
#include <imgui_stdlib.h> // ImGui::InputText() with std::string
#include <imgui_impl_dx9.h>
#include <imgui_impl_win32.h>

#include "other/imgui_notify.h" // https://github.com/patrickcjk/imgui-notify
#include "other/ImGuiFileBrowser.h" // https://github.com/gallickgunner/ImGui-Addons
#include "other/ImGuiExpansion.h" // mine, depends on imgui_notify

#include <nlohmann/json.hpp>

#include <string>
#include <map>
#include <thread>
#include <chrono>
#include <functional>

#include <d3d9.h>
#include <tchar.h>

using nlohmann::json;
using VTSender::Config;
using VTSender::Requests;
using VTSender::VirusTotal;

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
	int dwWidth = GetSystemMetrics(SM_CXSCREEN) / 2; // The width of the screen
	int dwHeight = GetSystemMetrics(SM_CYSCREEN) / 2; // The height of the screen

	WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL,
		appName.c_str(), NULL };

	RegisterClassEx(&wc);

	HWND hwnd = CreateWindow(wc.lpszClassName, appName.c_str(), WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX & ~WS_THICKFRAME,
		dwWidth - (mainWindow["width"] / 2.0f), dwHeight - (mainWindow["height"] / 2.0f), mainWindow["width"], mainWindow["height"], 0, 0, wc.hInstance, 0);

	// Initialize Direct3D

	if (!CreateDeviceD3D(hwnd))
	{
		CleanupDeviceD3D();
		UnregisterClass(wc.lpszClassName, wc.hInstance);
		return 1;
	}

	// Show the window

	ShowWindow(hwnd, SW_SHOWDEFAULT);
	UpdateWindow(hwnd);

	// Setup Dear ImGui context

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	// Setup Platform/Renderer backends

	ImGui_ImplWin32_Init(hwnd);
	ImGui_ImplDX9_Init(g_pd3dDevice);

	// Setup Dear ImGui style

	applyImGuiStyle();

	// Load config

	Config config("config/VTSender.json");

	json configData = config.load(
	{
		{"apiKey", "-"}
	});
	config.save(configData);


	// Main loop

	while (true)
	{
		static MSG msg = { };

		if (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			if (msg.message == WM_QUIT) // If our program was closed
				break;
		}

		// Start the Dear ImGui frame

		ImGui_ImplDX9_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		// Size and position of a child window

		ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize, ImGuiCond_Once);
		ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f), ImGuiCond_Once);

		static bool isWindowOpen = true;

		if (ImGui::Begin(appName.c_str(), &isWindowOpen, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings |
			ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse))
		{
			static VirusTotal vtSearchData;
			static VirusTotal vtScanData;
			static std::string menuTab = "Search";

			static std::function<void()> showPopup = [&]() {};
			static std::map<std::string, float> popupWindow = { {"width", 750.0f}, {"height", 250.0f} };

			if (ImGui::BeginMenuBar())
			{
				if (ImGui::BeginMenu("Menu"))
				{
					if (ImGui::MenuItem(ICON_FA_WRENCH"  Settings"))
					{
						showPopup = [&]()
						{
							if (!ImGui::IsPopupOpen("##SettingsPopup"))
							{
								ImGui::SetNextWindowSize(ImVec2(popupWindow["width"], popupWindow["height"]));
								ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
								ImGui::OpenPopup("##SettingsPopup");
							}
							if (ImGui::BeginPopupModal("##SettingsPopup", 0, ImGuiWindowFlags_NoResize))
							{
								ImGui::CenterText("SETTINGS"); ImGui::Dummy(ImVec2(0.0f, 5.0f));
								ImGui::setToClipboard(ICON_FA_KEY"  Your API key: ", configData.at("apiKey").get<std::string>().c_str(), "API key copied!");
								ImGui::Dummy(ImVec2(0.0f, 5.0f));
								ImGui::Text("Enter API key: "); ImGui::SameLine();
								ImGui::InputText("##apiKey", VirusTotal::getApiKey()); ImGui::Dummy(ImVec2(0.0f, 5.0f));

								ImGui::SetCursorPosY(popupWindow["height"] - 50.0f);
								if (ImGui::Button("Save", ImVec2(popupWindow["width"] / 2.0f - 12.0f, 30.0f)))
								{
									if (VirusTotal::getApiKey()->empty())
										ImGui::InsertNotification({ ImGuiToastType_Error, 3000, "Api key is empty!" });
									else
									{
										if (configData != nullptr && configData.contains("apiKey"))
										{
											configData.at("apiKey") = removeSpaces(*(VirusTotal::getApiKey()));

											config.save(configData);
										}

										ImGui::InsertNotification({ ImGuiToastType_Success, 3000, "API key saved!" });
									}
								}
								ImGui::SameLine();
								if (ImGui::Button("Close", ImVec2(popupWindow["width"] / 2.0f - 12.0f, 30.0f)))
								{
									ImGui::CloseCurrentPopup();
									showPopup = [&]() {};
								}
								ImGui::EndPopup();
							}
						};
					}
					else if (ImGui::MenuItem(ICON_FA_SEARCH"  Search"))
						menuTab = "Search";
					else if (ImGui::MenuItem(ICON_FA_BARCODE"  Scan"))
						menuTab = "Scan";
					else if (ImGui::MenuItem(ICON_FA_TIMES"  Exit"))
						menuTab = "Exit";

					ImGui::EndMenu();
				}
				ImGui::EndMenuBar();
			}
			showPopup();

			if (menuTab.compare("Search") == 0)
			{
				static int searchType = 0;
				static std::string searchDataInput;
				static json responseArr;

				ImGui::CenterText("SEARCH"); ImGui::Dummy(ImVec2(0.0f, 5.0f));
				if (ImGui::BeginChild("searchChild", ImVec2(0.0f, 200.0f), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse))
				{
					ImGui::Columns(2, nullptr, false);
					{
						ImGui::RadioButton("Hash##search", &searchType, 0);
						ImGui::NextColumn();
						ImGui::RadioButton("Url##search", &searchType, 1);
						ImGui::NextColumn();
						ImGui::Columns(1);
					}
					ImGui::Dummy(ImVec2(0.0f, 10.0f));

					ImGui::Text("Enter file hash, url or scan id: "); ImGui::SameLine();
					ImGui::InputText("##searchData", &searchDataInput); ImGui::Dummy(ImVec2(0.0f, 5.0f));

					ImGui::Text(ICON_FA_INFO_CIRCLE"  Status: %s", vtSearchData.getMessage().c_str()); ImGui::Spacing();
					ImGui::Text(ICON_FA_CALENDAR_CHECK"  Scan date: %s", vtSearchData.getScanDate().c_str()); ImGui::Spacing();
					ImGui::Text(ICON_FA_EXTERNAL_LINK_ALT"  Scan link: "); ImGui::SameLine(0.0f, 0.0f);
					ImGui::Link(vtSearchData.getPermalink().c_str(), "follow the link"); ImGui::Spacing();
					ImGui::Text(ICON_FA_SHIELD_VIRUS"  Detections: %d / %d", vtSearchData.getPositives(), vtSearchData.getTotal());
				}
				ImGui::EndChild();
				ImGui::Dummy(ImVec2(0.0f, 5.0f));

				if (ImGui::BeginTable("scanTable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_ScrollY, ImVec2(0.0f, 215.0f)))
				{
					ImGui::TableSetupColumn("Antivirus");
					ImGui::TableSetupColumn("Detected");
					ImGui::TableSetupColumn("About");
					ImGui::TableSetupScrollFreeze(0, 1); // Make top row always visible
					ImGui::TableHeadersRow();

					if (responseArr.contains("scans"))
					{
						json scanObj = responseArr.at("scans");

						for (auto& [avName, avObj] : scanObj.items())
						{
							if (avObj.contains("detected") && avObj.contains("result"))
							{
								ImGui::TableNextRow();

								ImGui::TableSetColumnIndex(0);
								ImGui::Text(avName.c_str());

								ImGui::TableSetColumnIndex(1);
								std::function<std::string(bool)> boolToStr = [](bool value) { return (value) ? "true" : "false"; };
								ImGui::Text(boolToStr(avObj.at("detected").get<bool>()).c_str());

								ImGui::TableSetColumnIndex(2);
								if (!avObj.at("result").is_null())
									ImGui::Text(avObj["result"].get<std::string>().c_str());
								else
									ImGui::Text("-");
							}
						}
					}
					ImGui::EndTable();
				}

				ImGui::Dummy(ImVec2(0.0f, 10.0f));

				ImGui::SetCursorPosY(mainWindow["height"] - 85.0f);
				if (ImGui::Button("Send##search", ImVec2(-0.1f, 30.0f)))
				{
					int httpCode = 0;
					std::string responseStr;
					std::string param = (searchType == 0) ? VirusTotal::urlPaths.at("fReport") : VirusTotal::urlPaths.at("uReport");
					vtSearchData.zeroFields();

					if (searchDataInput.empty())
						ImGui::InsertNotification({ ImGuiToastType_Error, 3000, "Input is empty!" });
					else
						std::tie(httpCode, responseStr) = Requests::makeRequest(VirusTotal::apiUrl + param, "?apikey=" +
							removeSpaces(configData.at("apiKey").get<std::string>()) + "&resource=" + removeSpaces(searchDataInput));

					responseArr = json::parse(responseStr, nullptr, false);

					if (responseArr != nullptr && !responseArr.empty())
					{
						if (responseArr.is_discarded())
						{
							if (httpCode == static_cast<int>(VirusTotal::Responses::RATE_LIMIT_EXCEEDED))
								vtSearchData.setMessage("Request rate limit exceeded. You are making more requests than allowed");
							else if (httpCode == static_cast<int>(VirusTotal::Responses::BAD_REQUEST))
								vtSearchData.setMessage("Bad request. Your request was somehow incorrect");
							else if (httpCode == static_cast<int>(VirusTotal::Responses::FORBIDDEN))
								vtSearchData.setMessage("Forbidden. You may be doing a request without providing an API key");
							else
								vtSearchData.setMessage("Something went wrong");
						}
						else
						{
							if (responseArr.contains("verbose_msg"))
								vtSearchData.setMessage(responseArr.at("verbose_msg").get<std::string>());

							if (responseArr.contains("total"))
								vtSearchData.setTotal(responseArr.at("total").get<int>());

							if (responseArr.contains("positives"))
								vtSearchData.setPositives(responseArr.at("positives").get<int>());

							if (responseArr.contains("scan_date"))
								vtSearchData.setScanDate(responseArr.at("scan_date").get<std::string>());

							if (responseArr.contains("permalink"))
								vtSearchData.setPermalink(responseArr.at("permalink").get<std::string>());
						}
					}

				}

			}
			else if (menuTab.compare("Scan") == 0)
			{
				static int scanType = 0;
				static std::string scanDataInput;
				static std::string filePath;
				static imgui_addons::ImGuiFileBrowser fileDialog;

				ImGui::CenterText("SCAN"); ImGui::Dummy(ImVec2(0.0f, 5.0f));

				if (ImGui::BeginChild("searchChild", ImVec2(0.0f, 210.0f), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse))
				{
					ImGui::Columns(2, nullptr, false);
					{
						ImGui::RadioButton("File##scan", &scanType, 0);
						ImGui::Dummy(ImVec2(0.0f, 10.0f));
						if (ImGui::Button("Select file", ImVec2(100.0f, 30.0f)))
						{
							ImGui::OpenPopup("Select file");
						}
						if (fileDialog.showFileDialog("Select file", imgui_addons::ImGuiFileBrowser::DialogMode::OPEN))
						{
							ImGui::InsertNotification({ ImGuiToastType_Info, 5000, ("Selected path: " + fileDialog.selected_path).c_str() });

							filePath = fileDialog.selected_path;
						}

						ImGui::NextColumn();
						ImGui::RadioButton("Url##scan", &scanType, 1);
						ImGui::Dummy(ImVec2(0.0f, 10.0f));

						ImGui::Text("Enter url: "); ImGui::SameLine();
						ImGui::InputText("##scanData", &scanDataInput); ImGui::Dummy(ImVec2(0.0f, 5.0f));

						ImGui::NextColumn();
						ImGui::Columns(1);
					}
					ImGui::Dummy(ImVec2(0.0f, 5.0f));

					ImGui::Text(ICON_FA_INFO_CIRCLE"  Status: %s", vtScanData.getMessage().c_str()); ImGui::Spacing();
					ImGui::Text(ICON_FA_CALENDAR_CHECK"  Scan date: %s", vtScanData.getScanDate().c_str()); ImGui::Spacing();
					ImGui::Text(ICON_FA_EXTERNAL_LINK_ALT"  Scan link: "); ImGui::SameLine(0.0f, 0.0f);
					ImGui::Link(vtScanData.getPermalink().c_str(), "follow the link"); ImGui::Spacing();
					ImGui::setToClipboard(ICON_FA_FINGERPRINT"  Scan ID: ", vtScanData.getScanId(), "Scan ID copied!");
				}
				ImGui::EndChild();

				ImGui::SetCursorPosY(mainWindow["height"] - 85.0f);
				if (ImGui::Button("Send##scan", ImVec2(-0.1f, 30.0f)))
				{
					std::thread scanThread([&]() // thread for large files
						{
							int httpCode = 0;
							std::string responseStr;
							vtScanData.zeroFields();

							if (scanType == 0)
							{
								if (filePath.empty())
									ImGui::InsertNotification({ ImGuiToastType_Error, 3000, "File not found!" });
								else
									std::tie(httpCode, responseStr) = Requests::sendFile(VirusTotal::apiUrl, removeSpaces(configData.at("apiKey").get<std::string>()), filePath);
							}
							else if (scanType == 1)
							{
								if (scanDataInput.empty())
									ImGui::InsertNotification({ ImGuiToastType_Error, 3000, "No url specified!" });
								else
									std::tie(httpCode, responseStr) = Requests::makeRequest(VirusTotal::apiUrl + VirusTotal::urlPaths.at("uScan"), "apikey=" + removeSpaces(configData.at("apiKey").get<std::string>()) +
										"&url=" + removeSpaces(scanDataInput), static_cast<int>(Requests::Method::POST));
							}

							json responseArr = json::parse(responseStr, nullptr, false);

							if (responseArr != nullptr && !responseArr.empty())
							{
								if (responseArr.is_discarded())
								{
									if (httpCode == static_cast<int>(VirusTotal::Responses::RATE_LIMIT_EXCEEDED))
										vtScanData.setMessage("Request rate limit exceeded. You are making more requests than allowed");
									else if (httpCode == static_cast<int>(VirusTotal::Responses::BAD_REQUEST))
										vtScanData.setMessage("Bad request. Your request was somehow incorrect");
									else if (httpCode == static_cast<int>(VirusTotal::Responses::FORBIDDEN))
										vtScanData.setMessage("Forbidden. You may be doing a request without providing an API key");
									else
										vtScanData.setMessage("Something went wrong");
								}
								else
								{
									if (responseArr.contains("verbose_msg"))
										vtScanData.setMessage(responseArr.at("verbose_msg").get<std::string>());

									if (responseArr.contains("scan_date"))
										vtScanData.setScanDate(responseArr.at("scan_date").get<std::string>());

									if (responseArr.contains("scan_id"))
										vtScanData.setScanId(responseArr.at("scan_id").get<std::string>());

									if (responseArr.contains("permalink"))
										vtScanData.setPermalink(responseArr.at("permalink").get<std::string>());
								}
							}
						});
					scanThread.detach();
				}
			}
			else if (menuTab.compare("Exit") == 0)
				ExitProcess(EXIT_SUCCESS);


			// for notify
			ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 5.0f);
			ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(43.0f / 255.0f, 43.0f / 255.0f, 43.0f / 255.0f, 100.0f / 255.0f));
			ImGui::RenderNotifications();
			ImGui::PopStyleVar(1);
			ImGui::PopStyleColor(1);
			// 

			ImGui::End();
		}

		ImGui::EndFrame();

		g_pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0, 1.0f, 0);
		if (g_pd3dDevice->BeginScene() >= 0)
		{
			ImGui::Render();
			ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
			g_pd3dDevice->EndScene();
		}

		// Handle loss of D3D9 device

		HRESULT result = g_pd3dDevice->Present(NULL, NULL, NULL, NULL);
		if (result == D3DERR_DEVICELOST && g_pd3dDevice->TestCooperativeLevel() == D3DERR_DEVICENOTRESET)
			ResetDevice();

		if (!isWindowOpen) // If we press on the close button, "isWindowOpen" is false. So we end the process
			ExitProcess(EXIT_SUCCESS);

		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}


	ImGui_ImplDX9_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
	CleanupDeviceD3D();
	DestroyWindow(hwnd);
	UnregisterClass(wc.lpszClassName, wc.hInstance);

	return 0;
}

bool CreateDeviceD3D(HWND hWnd)
{
	if ((g_pD3D = Direct3DCreate9(D3D_SDK_VERSION)) == NULL)
		return false;

	// Create the D3DDevice

	ZeroMemory(&g_d3dpp, sizeof(g_d3dpp));
	g_d3dpp.Windowed = TRUE;
	g_d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	g_d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;
	g_d3dpp.EnableAutoDepthStencil = TRUE;
	g_d3dpp.AutoDepthStencilFormat = D3DFMT_D16;
	g_d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_ONE; // Present with vsync
	//g_d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE; // Present without vsync, maximum unthrottled framerate
	if (g_pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, D3DCREATE_HARDWARE_VERTEXPROCESSING, &g_d3dpp, &g_pd3dDevice) < 0)
		return false;

	return true;
}

void CleanupDeviceD3D()
{
	if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = NULL; }
	if (g_pD3D) { g_pD3D->Release(); g_pD3D = NULL; }
}

void ResetDevice()
{
	ImGui_ImplDX9_InvalidateDeviceObjects();
	HRESULT hr = g_pd3dDevice->Reset(&g_d3dpp);
	if (hr == D3DERR_INVALIDCALL)
		IM_ASSERT(0);
	ImGui_ImplDX9_CreateDeviceObjects();
}

// Forward declare message handler from imgui_impl_win32.cpp

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_SIZE:
	{
		if (g_pd3dDevice != NULL && wParam != SIZE_MINIMIZED)
		{
			g_d3dpp.BackBufferWidth = LOWORD(lParam);
			g_d3dpp.BackBufferHeight = HIWORD(lParam);
			ResetDevice();
		}
		return 0;
	}

	case WM_SYSCOMMAND:
	{
		if ((wParam & 0xfff0) == SC_KEYMENU) return 0; // Disable ALT application menu
		break;
	}

	case WM_DESTROY:
	{
		PostQuitMessage(0);
		return 0;
	}

	case WM_CHAR:
	{
		wchar_t wch = '\0';
		MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, reinterpret_cast<char*>(&wParam), 1, &wch, 1);
		wParam = wch;
		break;
	}
	default:
		break;
	}

	if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
		return true;

	return DefWindowProc(hWnd, msg, wParam, lParam);
}

void applyImGuiStyle() // https://github.com/ocornut/imgui/issues/707#issuecomment-512669512
{
	ImGuiIO& io = ImGui::GetIO();
	io.IniFilename = NULL;
	io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\Segoeui.ttf", 20.0f, NULL, io.Fonts->GetGlyphRangesCyrillic());

	ImGui::MergeIconsWithLatestFont(20.0f, false); // init notify and add font-awesome font [imgui_notify.h -> line 350]

	ImGui::GetStyle().FrameRounding = 4.0f;
	ImGui::GetStyle().GrabRounding = 4.0f;

	ImVec4* colors = ImGui::GetStyle().Colors;
	colors[ImGuiCol_Text] = ImVec4(0.95f, 0.96f, 0.98f, 1.00f);
	colors[ImGuiCol_TextDisabled] = ImVec4(0.36f, 0.42f, 0.47f, 1.00f);
	colors[ImGuiCol_WindowBg] = ImVec4(0.11f, 0.15f, 0.17f, 1.00f);
	colors[ImGuiCol_ChildBg] = ImVec4(0.15f, 0.18f, 0.22f, 1.00f);
	colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
	colors[ImGuiCol_Border] = ImVec4(0.08f, 0.10f, 0.12f, 1.00f);
	colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	colors[ImGuiCol_FrameBg] = ImVec4(0.20f, 0.25f, 0.29f, 1.00f);
	colors[ImGuiCol_FrameBgHovered] = ImVec4(0.12f, 0.20f, 0.28f, 1.00f);
	colors[ImGuiCol_FrameBgActive] = ImVec4(0.09f, 0.12f, 0.14f, 1.00f);
	colors[ImGuiCol_TitleBg] = ImVec4(0.09f, 0.12f, 0.14f, 0.65f);
	colors[ImGuiCol_TitleBgActive] = ImVec4(0.08f, 0.10f, 0.12f, 1.00f);
	colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
	colors[ImGuiCol_MenuBarBg] = ImVec4(0.15f, 0.18f, 0.22f, 1.00f);
	colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.39f);
	colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.20f, 0.25f, 0.29f, 1.00f);
	colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.18f, 0.22f, 0.25f, 1.00f);
	colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.09f, 0.21f, 0.31f, 1.00f);
	colors[ImGuiCol_CheckMark] = ImVec4(0.28f, 0.56f, 1.00f, 1.00f);
	colors[ImGuiCol_SliderGrab] = ImVec4(0.28f, 0.56f, 1.00f, 1.00f);
	colors[ImGuiCol_SliderGrabActive] = ImVec4(0.37f, 0.61f, 1.00f, 1.00f);
	colors[ImGuiCol_Button] = ImVec4(0.20f, 0.25f, 0.29f, 1.00f);
	colors[ImGuiCol_ButtonHovered] = ImVec4(0.28f, 0.56f, 1.00f, 1.00f);
	colors[ImGuiCol_ButtonActive] = ImVec4(0.06f, 0.53f, 0.98f, 1.00f);
	colors[ImGuiCol_Header] = ImVec4(0.20f, 0.25f, 0.29f, 0.55f);
	colors[ImGuiCol_HeaderHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
	colors[ImGuiCol_HeaderActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
	colors[ImGuiCol_Separator] = ImVec4(0.20f, 0.25f, 0.29f, 1.00f);
	colors[ImGuiCol_SeparatorHovered] = ImVec4(0.10f, 0.40f, 0.75f, 0.78f);
	colors[ImGuiCol_SeparatorActive] = ImVec4(0.10f, 0.40f, 0.75f, 1.00f);
	colors[ImGuiCol_ResizeGrip] = ImVec4(0.26f, 0.59f, 0.98f, 0.25f);
	colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
	colors[ImGuiCol_ResizeGripActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
	colors[ImGuiCol_Tab] = ImVec4(0.11f, 0.15f, 0.17f, 1.00f);
	colors[ImGuiCol_TabHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
	colors[ImGuiCol_TabActive] = ImVec4(0.20f, 0.25f, 0.29f, 1.00f);
	colors[ImGuiCol_TabUnfocused] = ImVec4(0.11f, 0.15f, 0.17f, 1.00f);
	colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.11f, 0.15f, 0.17f, 1.00f);
	colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
	colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
	colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
	colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
	colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
	colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
	colors[ImGuiCol_NavHighlight] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
	colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
	colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
	colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
}