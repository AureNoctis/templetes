#include <Windows.h>
#include <stdint.h>


//---- for: SetProcessDpiAwarenessContext() -----
#include <ShellScalingApi.h>
#pragma comment(lib, "Shcore.lib")
//-----------------------------------------------

#pragma comment(lib, "User32")
#pragma comment(lib, "gdi32")



typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef int32_t bool32;

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

struct Win32_offscrean_buffer {
	BITMAPINFO info;													// This is you telling Windows the "Rules" of your DIB
	void* memory;														// A raw pointer for you to touch the pixels directly
	int width;
	int height;
	int bytesPerPixel = 4;
	int pitch;															// gap between tow rows
};

struct Win32_window_dimension {
	int width;
	int height;
};

static bool globalRunning;
static Win32_offscrean_buffer globalBackBuffer;
static bool key_Space_pressed = false;

static Win32_window_dimension win32_getWindowDimensions(HWND window) {
	Win32_window_dimension dimension;

	RECT clientRect;
	GetClientRect(window, &clientRect);						// area in window where you can draw
	dimension.width = clientRect.right - clientRect.left;
	dimension.height = clientRect.bottom - clientRect.top;
	return dimension;
}

static void win32_renderWeirdGradiant(Win32_offscrean_buffer* buffer, int blueOffset, int greenOffset) {

	uint8* row = (uint8*)buffer->memory;
	for (int y = 0; y < buffer->height; ++y) {
		uint32* pixle = (uint32*)row;
		for (int x = 0; x < buffer->width; ++x) {

			uint8 blue = (x + blueOffset);
			uint8 green = (y + greenOffset);

			*pixle++ = (green << 8) | blue;

		}
		row += buffer->pitch;
	}

}
static void win32_resizeDIBSection(Win32_offscrean_buffer* buffer, int width, int height) {					// DIB: device independent buffer->


	if (buffer->memory) {
		VirtualFree(buffer->memory, 0, MEM_RELEASE);
	}

	buffer->width = width;
	buffer->height = height;

	buffer->info.bmiHeader.biSize 			=	 sizeof(buffer->info.bmiHeader);	// specifing the rules
	buffer->info.bmiHeader.biWidth 			=	 buffer->width;						// ...
	buffer->info.bmiHeader.biHeight         =	 -buffer->height;					// ...
	buffer->info.bmiHeader.biPlanes 		=	 1;									// ...
	buffer->info.bmiHeader.biBitCount 		=	 32;								// 4 bytes = 3(rgb) + 1(padding: for proper aligment) : size of each pixle
	buffer->info.bmiHeader.biCompression 	=	 BI_RGB;						    // ...

	int bitMapmemorySize = (width * height) * buffer->bytesPerPixel;
	buffer->memory = VirtualAlloc(0, bitMapmemorySize, MEM_COMMIT, PAGE_READWRITE);
	buffer->pitch = width * buffer->bytesPerPixel;
}


static void win32_updateWindow(HDC deviceContext, int width, int height, Win32_offscrean_buffer* buffer) {

	StretchDIBits(deviceContext,
		0, 0, width, height,					// destination where we are bliting
		0, 0, buffer->width, buffer->height,	// source from where we are bliting
		buffer->memory, &buffer->info,
		DIB_RGB_COLORS, SRCCOPY);    			// SRCCOPY: what bitwise operation we want to do, we just want to copy
}

LRESULT CALLBACK win32_mainWindowCallback(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {

	switch (message) {

	case WM_DESTROY: {
		PostQuitMessage(0);
		return 0;
	}break;

	case WM_CLOSE: {
		return 0;
	}break;

	case WM_PAINT: {
		PAINTSTRUCT paint;
		HDC deviceContext = BeginPaint(window, &paint);

		Win32_window_dimension dimension = win32_getWindowDimensions(window);
		win32_updateWindow(deviceContext, dimension.width, dimension.height, &globalBackBuffer);
		EndPaint(window, &paint);
		return 0;
	}break;

	default: {
		return DefWindowProc(window, message, wParam, lParam);
	}break;
	}
}

int CALLBACK WinMain(HINSTANCE instance, HINSTANCE prevInstance, LPSTR cmdLine, int cmdShow) {

	/* NOTE: Modern Windows (10/11) uses "DPI Scaling" to make windows larger on high-res screens.
	   By default, Windows will draw our game at a small size and then "stretch" the pixels,
	   making them look blurry.

	   This call tells Windows: "Don't touch my pixels."
	   - It makes the window crisp/pixel-perfect on 4K/high-DPI monitors.
	   - It ensures GetClientRect returns ACTUAL physical pixel counts, not "virtual" ones.
	   - PER_MONITOR_AWARE_V2 is the modern standard for Win10 (v1703) and beyond.
	*/
	SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

	(void)prevInstance;  // (void) casts is a great way
	(void)cmdLine;		 // to keep the compiler quiet
	(void)cmdShow;		 // about unused parameters


	WNDCLASS WindowClass = {
	.style = CS_HREDRAW | CS_VREDRAW ,
	.lpfnWndProc = win32_mainWindowCallback,
	.hInstance = instance,
	.lpszClassName = "BM_wnd_class"
	};
	win32_resizeDIBSection(&globalBackBuffer, 1280, 720);


	if (RegisterClass(&WindowClass)) {
		HWND window = CreateWindowEx(0, WindowClass.lpszClassName, "BioMancer", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
									 CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
									 CW_USEDEFAULT, 0, 0, instance, 0);


		if (window) {
			HDC deviceContext = GetDC(window);

			globalRunning = true;
			while (globalRunning) {

				MSG message;

				while (PeekMessage(&message, 0, 0, 0, PM_REMOVE)) {
					if (message.message == WM_QUIT)
						globalRunning = false;
					TranslateMessage(&message);
					DispatchMessage(&message);
				}

				win32_renderWeirdGradiant(&globalBackBuffer, blue_offset, green_offset);
				Win32_window_dimension dimension = win32_getWindowDimensions(window);
				win32_updateWindow(deviceContext, dimension.width, dimension.height, &globalBackBuffer);
			}
			ReleaseDC(window, deviceContext);
		}
	}
	return 0;
}