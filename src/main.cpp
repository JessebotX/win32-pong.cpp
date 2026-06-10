#include <windows.h>

#include <cassert>
#include <cstdint>
#include <print>
#include <string>
#include <vector>

template <typename NumberT = int>
struct vec2
{
  NumberT x;
  NumberT y;
};

class w32_device_context
{
public:
  w32_device_context(HWND window);
  ~w32_device_context();

  auto value() const -> HDC;

private:
  HWND window;
  HDC device_context;
};

w32_device_context::w32_device_context(HWND window)
  : window { window },
    device_context { GetDC(window) }
{
}

w32_device_context::~w32_device_context()
{
  ReleaseDC(window, device_context);
}

auto w32_device_context::value() const -> HDC
{
  return device_context;
}

struct w32_backbuffer
{
  BITMAPINFO info;
  std::vector<uint32_t> pixels;
  int width;
  int height;
};

class w32_window
{
public:
  w32_window(HINSTANCE instance, const std::string& window_name, const std::string& class_name, LRESULT callback_fn(HWND, UINT, WPARAM, LPARAM), int backbuffer_width, int backbuffer_height);
  ~w32_window();

  auto is_open() const -> bool;
  auto resize_buffer(int width, int height) -> void;
  auto render() -> void;
  auto width() const -> int;
  auto height() const -> int;

private:
  WNDCLASSA window_class;
  HWND window;
  bool opened;
  w32_backbuffer backbuffer;
};

w32_window::w32_window(HINSTANCE instance, const std::string& window_name, const std::string& class_name, LRESULT callback_fn(HWND, UINT, WPARAM, LPARAM), int backbuffer_width = 1280, int backbuffer_height = 720)
  : window_class {},
    window {},
    opened { false },
    backbuffer {}
{
  w32_window::resize_buffer(backbuffer_width, backbuffer_height);

  window_class = {
    .style = CS_HREDRAW|CS_VREDRAW,
    .lpfnWndProc = callback_fn,
    .hInstance = instance,
    .lpszClassName = class_name.c_str(),
  };

  if (!RegisterClassA(&window_class)) {
    return;
  }

  window = CreateWindowExA(0, window_class.lpszClassName, window_name.c_str(), WS_OVERLAPPEDWINDOW|WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, nullptr, nullptr, instance, nullptr);
  if (!window) {
    return;
  }

  opened = true;
}

w32_window::~w32_window()
{
  DestroyWindow(window);
  opened = false;
}

auto w32_window::is_open() const -> bool
{
  return opened;
}

auto w32_window::resize_buffer(int width, int height) -> void
{
  backbuffer = w32_backbuffer{
    .info = {
      .bmiHeader = {
        .biSize = sizeof(backbuffer.info.bmiHeader),
        .biWidth = width,
        .biHeight = -height, // NOTE: negative biHeight for top-down coordinate system
        .biPlanes = 1,
        .biBitCount = 32,
        .biCompression = BI_RGB,
      }
    },
    .width = width,
    .height = height,
  };

  backbuffer.pixels.resize(width * height, 0xFF0000FF);
}

auto w32_window::render() -> void
{
  auto dc { w32_device_context(window) };
  StretchDIBits(dc.value(),
                0, 0, backbuffer.width, backbuffer.height,
                0, 0, backbuffer.width, backbuffer.height,
                backbuffer.pixels.data(),
                &backbuffer.info,
                DIB_RGB_COLORS, SRCCOPY);
}

auto w32_window::width() const -> int
{
  RECT r;
  GetClientRect(window, &r);

  return r.right - r.left;
}

auto w32_window::height() const -> int
{
  RECT r;
  GetClientRect(window, &r);

  return r.bottom - r.top;
}

static bool Running { false };

auto w32_main_window_callback(HWND window, UINT message, WPARAM wparam, LPARAM lparam) -> LRESULT
{
  LRESULT result {};

  switch (message) {
    case WM_QUIT:
    case WM_CLOSE:
    case WM_DESTROY: {
      Running = false;
    } break;

    default: {
      result = DefWindowProc(window, message, wparam, lparam);
    } break;
  }

  return result;
}

auto WinMain(HINSTANCE instance, HINSTANCE prev_instance, LPSTR cmd_line, int cmd_show) -> int
{
  auto window { w32_window(instance, "Win32 Pong", "Win32PongWindowClass", w32_main_window_callback) };
  assert(window.is_open());

  Running = true;
  while (Running) {
    MSG msg {};
    while (PeekMessageA(&msg, nullptr, 0, 0, PM_REMOVE)) {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }

    window.render();
  }

  return 0;
}
