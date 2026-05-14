#pragma once
#include <iostream>
#include <Windows.h>
#include <Windowsx.h>
#include <d2d1.h>
#include <chrono>
#include "../vm.hpp"

#pragma comment(lib, "d2d1.lib")
#undef max

namespace dlang::functions::graphics
{
	struct GfxState {
		HWND hwnd = nullptr;
		ID2D1Factory* factory = nullptr;
		ID2D1HwndRenderTarget* renderTarget = nullptr;
		ID2D1SolidColorBrush* brush = nullptr;
	};

	struct GfxInput
	{
		std::pair<bool, bool> keys[256] = { };
		bool mouseLeft = false;
		bool mouseRight = false;
		float mouseX = 0.0f;
		float mouseY = 0.0f;
	};
	static GfxInput input;

	LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		switch (uMsg)
		{
		case WM_DESTROY:
			PostQuitMessage(0);
			return 0;
			break;
		case WM_KEYDOWN:
			if (!input.keys[static_cast<unsigned int>(wParam)].first)
				input.keys[static_cast<unsigned int>(wParam)] = { true, false };
			break;
		case WM_KEYUP:
			input.keys[static_cast<unsigned int>(wParam)] = { false, false };
			break;
		case WM_LBUTTONDOWN:
			input.mouseLeft = true;
			break;
		case WM_LBUTTONUP:
			input.mouseLeft = false;
			break;
		case WM_RBUTTONDOWN:
			input.mouseRight = true;
			break;
		case WM_RBUTTONUP:
			input.mouseRight = false;
			break;
		case WM_MOUSEMOVE:
			input.mouseX = static_cast<float>(GET_X_LPARAM(lParam));
			input.mouseY = static_cast<float>(GET_Y_LPARAM(lParam));
			break;
		default:
			return DefWindowProc(hwnd, uMsg, wParam, lParam);
		}
	}

	struct Color
	{
		int r, g, b, a;
	};

	static std::vector<Color> colors;
	static GfxState gfx;
	static std::chrono::high_resolution_clock::time_point lastFrameTime = std::chrono::high_resolution_clock::now();
	static float deltaTime = 0.0f;

	inline void initFunctions(vm::DLangVirtualMachine* vm)
	{
		vm->registerNativeFunction("color", [](vm::DLangVirtualMachine* vm) -> bool {
			if (!vm->checkStack({ DlangType::Integer, DlangType::Integer, DlangType::Integer }))
				throw std::runtime_error("Invalid arguments for gfx.CreateColor, expected (int r, int g, int b)");

			DlangObject a;

			if (vm->getStackSize() > 3)
				a = vm->pop();

			auto b = vm->pop();
			auto g = vm->pop();
			auto r = vm->pop();

			Color color = { r.intValue, g.intValue, b.intValue, a.type == DlangType::Integer ? a.intValue : 255 };
			colors.push_back(color);
			auto index = static_cast<int>(colors.size() - 1);
			vm->push(DlangObject(index));

			return true;
			}, "gfx", 3);

		vm->registerNativeFunction("init", [](vm::DLangVirtualMachine* vm) -> bool {
			if (!vm->checkStack({ DlangType::Integer, DlangType::Integer, DlangType::String }))
				throw std::runtime_error("Invalid arguments for gfx.init, expected (string title, int width, int height)");

			auto height = vm->pop();
			auto width = vm->pop();
			auto title = vm->pop();

			D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &gfx.factory);

			WNDCLASSA wc = { 0 };
			wc.lpfnWndProc = WindowProc; // Voor nu even simpel
			wc.hInstance = GetModuleHandleA(NULL);
			wc.lpszClassName = "DLangWindow";
			RegisterClassA(&wc);

			RECT wr = { 0, 0, width.intValue, height.intValue };
			AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, FALSE);

			gfx.hwnd = CreateWindowExA(0, wc.lpszClassName, vm->getStringFromPool(title.intValue).c_str(),
				WS_OVERLAPPEDWINDOW | WS_VISIBLE,
				CW_USEDEFAULT, CW_USEDEFAULT,
				wr.right - wr.left, wr.bottom - wr.top,
				NULL, NULL, GetModuleHandle(NULL), NULL);

			RECT rc;
			GetClientRect(gfx.hwnd, &rc);
			gfx.factory->CreateHwndRenderTarget(
				D2D1::RenderTargetProperties(),
				D2D1::HwndRenderTargetProperties(gfx.hwnd, D2D1::SizeU(rc.right, rc.bottom)),
				&gfx.renderTarget
			);

			return true;
			}, "gfx", 3);

		vm->registerNativeFunction("clear", [](vm::DLangVirtualMachine* vm) -> bool {
			if (!vm->checkStack({ DlangType::Integer }))
				throw std::runtime_error("Invalid arguments for gfx.clear, expected (int colorIndex)");

			auto colorObj = vm->pop();
			if (colors.size() < colorObj.intValue)
				throw std::runtime_error("Invalid color index for gfx.clear, index out of bounds");

			auto color = D2D1::ColorF(colors[colorObj.intValue].r / 255.0f, colors[colorObj.intValue].g / 255.0f, colors[colorObj.intValue].b / 255.0f, colors[colorObj.intValue].a / 255.0f);

			gfx.renderTarget->Clear(color);
			return true;
			}, "gfx", 1);

		vm->registerNativeFunction("update", [](vm::DLangVirtualMachine* vm) -> bool {
			auto currentFrameTime = std::chrono::high_resolution_clock::now();
			std::chrono::duration<float> elapsed = currentFrameTime - lastFrameTime;
			lastFrameTime = currentFrameTime;
			deltaTime = elapsed.count();

			MSG msg;
			while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE))
			{
				TranslateMessage(&msg);
				DispatchMessageA(&msg);
			}
			return true;
			}, "gfx", 0);

		vm->registerNativeFunction("begin", [](vm::DLangVirtualMachine* vm) -> bool {
			if (!gfx.renderTarget) return false;
			gfx.renderTarget->BeginDraw();
			return true;
			}, "gfx", 0);

		vm->registerNativeFunction("end", [](vm::DLangVirtualMachine* vm) -> bool {
			HRESULT hr = gfx.renderTarget->EndDraw();
			if (FAILED(hr)) {
				printf("Direct2D EndDraw Error: 0x%08X\n", hr);
				return false;
			}
			return true;
			}, "gfx", 0);

		vm->registerNativeFunction("draw_rect", [](vm::DLangVirtualMachine* vm) -> bool {
			if (!vm->checkStack({ DlangType::Integer, DlangType::Integer, DlangType::Integer, DlangType::Integer, DlangType::Integer }))
				throw std::runtime_error("Invalid arguments for gfx.draw_rect, expected (int x, int y, int width, int height, int colorIndex)");

			DlangObject filled = DlangObject(1);
			if (vm->getStackSize() >= 6)
				filled = vm->pop();
			auto colorObj = vm->pop();
			auto height = vm->pop();
			auto width = vm->pop();
			auto y = vm->pop();
			auto x = vm->pop();
			if (colors.size() < colorObj.intValue)
				throw std::runtime_error("Invalid color index for gfx.draw_rect, index out of bounds");
			auto color = D2D1::ColorF(colors[colorObj.intValue].r / 255.0f, colors[colorObj.intValue].g / 255.0f, colors[colorObj.intValue].b / 255.0f, colors[colorObj.intValue].a / 255.0f);
			if (!gfx.brush)
				gfx.renderTarget->CreateSolidColorBrush(color, &gfx.brush);
			else
				gfx.brush->SetColor(color);

			if (filled.type == DlangType::Integer && filled.intValue == 1)
				gfx.renderTarget->FillRectangle(D2D1::RectF(x.intValue, y.intValue, x.intValue + width.intValue, y.intValue + height.intValue), gfx.brush);
			else
				gfx.renderTarget->DrawRectangle(D2D1::RectF(x.intValue, y.intValue, x.intValue + width.intValue, y.intValue + height.intValue), gfx.brush);
			return true;
			}, "gfx", 5);

		vm->registerNativeFunction("key_pressed", [](vm::DLangVirtualMachine* vm) -> bool {
			if (!vm->checkStack({ DlangType::Integer }))
				throw std::runtime_error("Invalid arguments for gfx.key_pressed, expected (int keyCode)");

			auto keyCode = vm->pop();
			int k = keyCode.intValue;
			bool pressed = input.keys[k].first && !input.keys[k].second;

			if (pressed) {
				input.keys[k].second = true;
			}

			vm->push(DlangObject(pressed ? 1 : 0));
			return true;
			}, "gfx", 1);

		vm->registerNativeFunction("key_down", [](vm::DLangVirtualMachine* vm) -> bool {
			if (!vm->checkStack({ DlangType::Integer }))
				throw std::runtime_error("Invalid arguments for gfx.key_down, expected (int keyCode)");

			auto keyCode = vm->pop();
			vm->push(DlangObject(input.keys[keyCode.intValue].first ? 1 : 0));
			return true;
			}, "gfx", 1);

		vm->registerNativeFunction("get_deltatime", [](vm::DLangVirtualMachine* vm) -> bool {
			vm->push(DlangObject(static_cast<int>(deltaTime)));
			return true;
			}, "gfx", 0);
	}
}