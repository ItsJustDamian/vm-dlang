#pragma once
#include <iostream>
#include <Windows.h>
#include <Windowsx.h>
#include <d2d1.h>
#include <dwrite.h>
#include <wincodec.h>
#include <chrono>
#include "../vm.hpp"

#include "../../dependencies/soloud/soloud.h"
#include "../../dependencies/soloud/soloud_wav.h"

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")
#pragma comment(lib, "windowscodecs.lib")
#undef max

namespace dlang::functions::graphics
{
	struct GfxState {
		HWND hwnd = nullptr;
		ID2D1Factory* factory = nullptr;
		ID2D1HwndRenderTarget* renderTarget = nullptr;
		ID2D1SolidColorBrush* brush = nullptr;
		IDWriteFactory* pDWriteFactory = nullptr;
		IDWriteTextFormat* pTextFormat = nullptr;
		bool windowClosed = false;
		IWICImagingFactory* wicFactory = nullptr;
		std::vector<ID2D1Bitmap*> textures;
		int width = 800;
		int height = 600;
		SoLoud::Soloud soloud;
		std::vector<SoLoud::Wav*> sounds;
	};

	struct GfxInput
	{
		std::pair<bool, bool> keys[256] = { };
		bool mouseLeft = false;
		bool mouseRight = false;
		float mouseX = 0.0f;
		float mouseY = 0.0f;
	};

	struct Color
	{
		int r, g, b, a;
	};

	static std::vector<Color> colors;
	static GfxState gfx;
	static std::chrono::high_resolution_clock::time_point lastFrameTime = std::chrono::high_resolution_clock::now();
	static float deltaTime = 0.0f;
	static GfxInput input;

	inline int GetKeyIndex(const std::string& c)
	{
		auto upperString = [](const std::string& str) -> std::string {
			std::string result = str;
			for (char& ch : result)
				ch = toupper(ch);
			return result;
		};

		if (c.length() == 1)
		{
			char ch = toupper(c[0]);
			if (ch >= 'A' && ch <= 'Z')
				return ch;
			if (ch >= '0' && ch <= '9')
				return ch;
		}
		else
		{
			if (upperString(c) == "LEFT")
				return VK_LEFT;
			if (upperString(c) == "RIGHT")
				return VK_RIGHT;
			if (upperString(c) == "UP")
				return VK_UP;
			if (upperString(c) == "DOWN")
				return VK_DOWN;
			if (upperString(c) == "SPACE")
				return VK_SPACE;
			if (upperString(c) == "SHIFT")
				return VK_SHIFT;
			if (upperString(c) == "CTRL")
				return VK_CONTROL;
			if (upperString(c) == "ALT")
				return VK_MENU;
			if(upperString(c) == "ENTER")
			return VK_RETURN;
		}
		throw std::runtime_error("Invalid key name: '" + c + "'");
	}

	LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		switch (uMsg)
		{
		case WM_DESTROY:
			PostQuitMessage(0);
			gfx.windowClosed = true;
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

			HRESULT hrCom = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

			auto height = vm->pop();
			auto width = vm->pop();
			auto title = vm->pop();

			gfx.width = width.intValue;
			gfx.height = height.intValue;

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

			DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), reinterpret_cast<IUnknown**>(&gfx.pDWriteFactory));

			gfx.pDWriteFactory->CreateTextFormat(
				L"Consolas",                // Lettertype
				NULL,                       // Font collection
				DWRITE_FONT_WEIGHT_NORMAL,
				DWRITE_FONT_STYLE_NORMAL,
				DWRITE_FONT_STRETCH_NORMAL,
				24.0f,                      // Font size
				L"en-us",                   // Locale
				&gfx.pTextFormat
			);

			HRESULT hrFactory = CoCreateInstance(
				CLSID_WICImagingFactory,
				NULL,
				CLSCTX_INPROC_SERVER,
				IID_PPV_ARGS(&gfx.wicFactory)
			);

			if (FAILED(hrFactory)) {
				char errorMsg[256];
				sprintf_s(errorMsg, "WIC Factory creation failed! HRESULT: 0x%08X\n", hrFactory);
				MessageBoxA(NULL, errorMsg, "GFX Error", MB_ICONERROR);
				return false; // Stop de init zodat we niet later crashen
			}

			gfx.soloud.init();

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
			int argc = vm->getStackSize();

			DlangObject filled(1);

			if (argc >= 6) filled = vm->pop();
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
				gfx.renderTarget->FillRectangle(D2D1::RectF(x.floatValue, y.floatValue, x.floatValue + width.intValue, y.floatValue + height.intValue), gfx.brush);
			else
				gfx.renderTarget->DrawRectangle(D2D1::RectF(x.floatValue, y.floatValue, x.floatValue + width.intValue, y.floatValue + height.intValue), gfx.brush);
			return true;
			}, "gfx", 5);

		vm->registerNativeFunction("draw_circle", [](vm::DLangVirtualMachine* vm) -> bool {
			int argc = vm->getStackSize();
			DlangObject filled(1);
			if (argc >= 5) filled = vm->pop();
			auto colorObj = vm->pop();
			auto radius = vm->pop();
			auto y = vm->pop();
			auto x = vm->pop();
			
			if (colors.size() < colorObj.intValue)
				throw std::runtime_error("Invalid color index for gfx.draw_circle, index out of bounds");
			
			auto color = D2D1::ColorF(colors[colorObj.intValue].r / 255.0f, colors[colorObj.intValue].g / 255.0f, colors[colorObj.intValue].b / 255.0f, colors[colorObj.intValue].a / 255.0f);
			
			if (!gfx.brush)
				gfx.renderTarget->CreateSolidColorBrush(color, &gfx.brush);
			else
				gfx.brush->SetColor(color);

			if (filled.type == DlangType::Integer && filled.intValue == 1)
				gfx.renderTarget->FillEllipse(D2D1::Ellipse(D2D1::Point2F(x.floatValue, y.floatValue), radius.floatValue, radius.floatValue), gfx.brush);
			else
				gfx.renderTarget->DrawEllipse(D2D1::Ellipse(D2D1::Point2F(x.floatValue, y.floatValue), radius.floatValue, radius.floatValue), gfx.brush);
			
			return true;
			}, "gfx", 4);

		vm->registerNativeFunction("draw_text", [](vm::DLangVirtualMachine* vm) -> bool {
			auto stackSize = vm->getStackSize();

			DlangObject hAlignObj(0);
			DlangObject vAlignObj(0);

			if (stackSize > 5) vAlignObj = vm->pop();
			if (stackSize > 4) hAlignObj = vm->pop();
			auto colorObj = vm->pop();
			auto yObj = vm->pop();
			auto xObj = vm->pop();
			auto textObj = vm->pop();

			auto color = D2D1::ColorF(colors[colorObj.intValue].r / 255.0f, colors[colorObj.intValue].g / 255.0f, colors[colorObj.intValue].b / 255.0f, colors[colorObj.intValue].a / 255.0f);

			if (!gfx.brush)
				gfx.renderTarget->CreateSolidColorBrush(color, &gfx.brush);
			else
				gfx.brush->SetColor(color);

			float x = (xObj.type == DlangType::Float) ? xObj.floatValue : (float)xObj.intValue;
			float y = (yObj.type == DlangType::Float) ? yObj.floatValue : (float)yObj.intValue;

			DWRITE_TEXT_ALIGNMENT hAlign = DWRITE_TEXT_ALIGNMENT_LEADING;
			if (hAlignObj.intValue == 1) hAlign = DWRITE_TEXT_ALIGNMENT_CENTER;
			else if (hAlignObj.intValue == 2) hAlign = DWRITE_TEXT_ALIGNMENT_TRAILING;
			gfx.pTextFormat->SetTextAlignment(hAlign);

			DWRITE_PARAGRAPH_ALIGNMENT vAlign = DWRITE_PARAGRAPH_ALIGNMENT_NEAR;
			if (vAlignObj.intValue == 1) vAlign = DWRITE_PARAGRAPH_ALIGNMENT_CENTER;
			else if (vAlignObj.intValue == 2) vAlign = DWRITE_PARAGRAPH_ALIGNMENT_FAR;
			gfx.pTextFormat->SetParagraphAlignment(vAlign);

			D2D1_SIZE_F rtSize = gfx.renderTarget->GetSize();
			D2D1_RECT_F layoutRect = D2D1::RectF(0.0f, y, rtSize.width, y + 100.0f);

			if (hAlign == DWRITE_TEXT_ALIGNMENT_LEADING) {
				layoutRect.left = x;
			}
			else if (hAlign == DWRITE_TEXT_ALIGNMENT_TRAILING) {
				layoutRect.right = x;
			}

			std::string text = vm->getStringFromPool(textObj.intValue);
			std::wstring widestr = std::wstring(text.begin(), text.end());

			gfx.renderTarget->DrawText(
				widestr.c_str(),
				(UINT32)widestr.length(),
				gfx.pTextFormat,
				layoutRect,
				gfx.brush
			);

			return true;
			}, "gfx", 3);

		vm->registerNativeFunction("key_pressed", [](vm::DLangVirtualMachine* vm) -> bool {
			int k = 0;
			auto keyCode = vm->pop();
			if (keyCode.type == DlangType::String)
				k = GetKeyIndex(vm->getStringFromPool(keyCode.intValue));
			else if (keyCode.type == DlangType::Integer)
				k = keyCode.intValue;
			else
				throw std::runtime_error("Invalid argument type for gfx.key_pressed, expected (int keyCode) or (string keyName)");

			bool pressed = input.keys[k].first && !input.keys[k].second;

			if (pressed) {
				input.keys[k].second = true;
			}

			vm->push(DlangObject(pressed ? 1 : 0));
			return true;
			}, "gfx", 1);

		vm->registerNativeFunction("key_down", [](vm::DLangVirtualMachine* vm) -> bool {
			int k = 0;
			auto keyCode = vm->pop();
			if (keyCode.type == DlangType::String)
				k = GetKeyIndex(vm->getStringFromPool(keyCode.intValue));
			else if (keyCode.type == DlangType::Integer)
				k = keyCode.intValue;
			else
				throw std::runtime_error("Invalid argument type for gfx.key_pressed, expected (int keyCode) or (string keyName)");

			vm->push(DlangObject(input.keys[k].first ? 1 : 0));
			return true;
			}, "gfx", 1);

		vm->registerNativeFunction("get_deltatime", [](vm::DLangVirtualMachine* vm) -> bool {
			vm->push(DlangObject(deltaTime));
			return true;
			}, "gfx", 0);

		vm->registerNativeFunction("window_closed", [](vm::DLangVirtualMachine* vm) -> bool {
			vm->push(DlangObject(gfx.windowClosed ? 1 : 0));
			return true;
			}, "gfx", 0);

		vm->registerNativeFunction("load_image", [](vm::DLangVirtualMachine* vm) -> bool {
			if (!vm->checkStack({ DlangType::String }))
				throw std::runtime_error("Invalid arguments for gfx.load_image, expected (string filePath)");
			
			auto filePathObj = vm->pop();
			
			std::string filePath = vm->getStringFromPool(filePathObj.intValue);
			IWICBitmapDecoder* decoder = nullptr;
			
			HRESULT hr = gfx.wicFactory->CreateDecoderFromFilename(std::wstring(filePath.begin(), filePath.end()).c_str(), NULL, GENERIC_READ, WICDecodeMetadataCacheOnLoad, &decoder);
			if (FAILED(hr)) {
				printf("Failed to load texture: 0x%08X\n", hr);
				vm->push(DlangObject(-1));
				return false;
			}

			IWICBitmapFrameDecode* frame = nullptr;
			hr = decoder->GetFrame(0, &frame);
			if (FAILED(hr)) {
				printf("Failed to get texture frame: 0x%08X\n", hr);
				decoder->Release();
				vm->push(DlangObject(-1));
				return false;
			}

			IWICFormatConverter* converter = nullptr;
			hr = gfx.wicFactory->CreateFormatConverter(&converter);

			if (SUCCEEDED(hr)) {
				hr = converter->Initialize(
					frame,
					GUID_WICPixelFormat32bppPBGRA,
					WICBitmapDitherTypeNone,
					NULL,
					0.0f,
					WICBitmapPaletteTypeMedianCut
				);
			}

			if (FAILED(hr)) {
				printf("Failed to convert texture format: 0x%08X\n", hr);
				if (converter) converter->Release();
				frame->Release();
				decoder->Release();
				vm->push(DlangObject(-1));
				return false;
			}

			ID2D1Bitmap* d2dBitmap = nullptr;
			hr = gfx.renderTarget->CreateBitmapFromWicBitmap(converter, NULL, &d2dBitmap);
			if (FAILED(hr)) {
				printf("Failed to create D2D bitmap: 0x%08X\n", hr);
				frame->Release();
				decoder->Release();
				vm->push(DlangObject(-1));
				return false;
			}

			gfx.textures.push_back(d2dBitmap);
			int textureIndex = static_cast<int>(gfx.textures.size() - 1);

			converter->Release();
			frame->Release();
			decoder->Release();

			vm->push(DlangObject(textureIndex));
			return true;
			}, "gfx", 1);

		vm->registerNativeFunction("draw_image", [](vm::DLangVirtualMachine* vm) -> bool {
			auto stackSize = vm->getStackSize();

			auto sizeX = DlangObject(-1.f);
			auto sizeY = DlangObject(-1.f);

			if (stackSize > 4)
			{
				sizeY = vm->pop();
				sizeX = vm->pop();
			}
			
			auto yObj = vm->pop();
			auto xObj = vm->pop();
			auto textureIndexObj = vm->pop();
			int textureIndex = textureIndexObj.intValue;

			if (textureIndex < 0 || textureIndex >= gfx.textures.size())
				throw std::runtime_error("Invalid texture index for gfx.draw_texture, index out of bounds (" + std::to_string(textureIndex) + ")");
			ID2D1Bitmap* texture = gfx.textures[textureIndex];

			float x = (xObj.type == DlangType::Float) ? xObj.floatValue : (float)xObj.intValue;
			float y = (yObj.type == DlangType::Float) ? yObj.floatValue : (float)yObj.intValue;

			gfx.renderTarget->DrawBitmap(texture, D2D1::RectF(x, y, x + (sizeX.floatValue > 0 ? sizeX.floatValue : texture->GetSize().width), y + (sizeY.floatValue > 0 ? sizeY.floatValue : texture->GetSize().height)));
			return true;
		}, "gfx", 3);

		vm->registerNativeFunction("load_sound", [](vm::DLangVirtualMachine* vm) -> bool {
			auto pathObj = vm->pop();
			std::string path = vm->getStringFromPool(pathObj.intValue);

			SoLoud::Wav* wav = new SoLoud::Wav();
			if (wav->load(path.c_str()) != SoLoud::SO_NO_ERROR) {
				delete wav;
				throw std::runtime_error("Failed to load audio file: " + path);
			}

			gfx.sounds.push_back(wav);
			int soundIndex = (int)gfx.sounds.size() - 1;

			vm->push(DlangObject(soundIndex));
			return true;
		}, "gfx", 1);

		vm->registerNativeFunction("play_sound", [](vm::DLangVirtualMachine* vm) -> bool {
			auto stackSize = vm->getStackSize();

			DlangObject loopObj(0);
			DlangObject volumeObj(1.0f);

			if (stackSize > 2) loopObj = vm->pop();
			if (stackSize > 1) volumeObj = vm->pop();
			auto soundIndexObj = vm->pop();

			int index = soundIndexObj.intValue;
			if (index < 0 || index >= gfx.sounds.size()) {
				throw std::runtime_error("Invalid sound index for gfx.play_sound");
			}

			float volume = (volumeObj.type == DlangType::Float) ? volumeObj.floatValue : (float)volumeObj.intValue;
			bool loop = (loopObj.intValue == 1);

			int handle = gfx.soloud.play(*gfx.sounds[index], volume);
			gfx.soloud.setLooping(handle, loop);

			return true;
		}, "gfx", 1);
	}
}