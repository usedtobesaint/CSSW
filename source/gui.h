#pragma once
#include <d3d9.h>


namespace gui
{
	//window size
	constexpr int WIDTH = 800;
	constexpr int HEIGHT = 500;

	inline bool isRunning = true;

	//winapi window vars
	inline HWND window = nullptr;
	inline WNDCLASSEX windowClass = {};

	//point for window movement
	inline POINTS position = {};

	//directx state vars
	inline PDIRECT3D9 d3d = nullptr;
	inline LPDIRECT3DDEVICE9 device = nullptr;
	inline D3DPRESENT_PARAMETERS presentParameters = { };

	//handle window creation & destruction
	void CreateHWindow(
		const char* windowName,
		const char* className) noexcept;
	void DestroyHWindow() noexcept;

	// handle device creation & destruction
	bool CreateDevice() noexcept;
	void ResetDevice() noexcept;
	void DestroyDevice() noexcept;

	// handle InGui creation & destruction
	void CreateImGui() noexcept;
	void DestroyImGui() noexcept;
	void BeginRender() noexcept;
	void EndRender() noexcept;
	void Render() noexcept;

	LPCWSTR convertToLPCWSTR(const char* str);
}