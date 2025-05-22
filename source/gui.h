#pragma once
#include <d3d9.h>
#include "../imgui/imgui.h"


namespace gui
{
	//window size
	inline int WIDTH = 1000;
	inline int HEIGHT = 1100;

	inline bool isRunning = true;

	inline ImGuiContext* parserContext = nullptr;

	//winapi window vars
	inline HWND parserWindow = nullptr;
	inline WNDCLASSEX windowClass = {};

	//point for window movement
	inline POINTS position = {};

	//directx state vars
	inline PDIRECT3D9 d3d = nullptr;
	inline LPDIRECT3DDEVICE9 parserDevice = nullptr;
	inline D3DPRESENT_PARAMETERS presentParameters = { };

	//handle window creation & destruction
	void CreateHWindow(const char* windowName, HWND& window) noexcept;
	void DestroyHWindow(HWND& window) noexcept;

	// handle device creation & destruction
	bool CreateDevice(HWND& window, LPDIRECT3DDEVICE9& device) noexcept;
	bool ResetDevice(LPDIRECT3DDEVICE9 device) noexcept;
	void DestroyDevice(LPDIRECT3DDEVICE9& device) noexcept;

	// handle InGui creation & destruction
	void CreateImGui(HWND& window, LPDIRECT3DDEVICE9& device, ImGuiContext*& context) noexcept;
	void DestroyImGui(ImGuiContext*& context) noexcept;
	void BeginRender(ImGuiContext*& context) noexcept;
	void EndRender(LPDIRECT3DDEVICE9& device, ImGuiContext*& context) noexcept;
	void Render() noexcept;

}