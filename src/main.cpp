#include <windows.h>

#include <cassert>
#include <cstdint>
#include <print>
#include <string>
#include <vector>

template <typename NumberT = int>
struct vec2 {
   NumberT x;
   NumberT y;
};

namespace win32 {

class device_context {
public:
   device_context(HWND window);
   ~device_context();

   auto value() const -> HDC;

private:
   HWND window;
   HDC dc;
};

device_context::device_context(HWND window)
: window { window },
   dc { GetDC(window) }
{
}

device_context::~device_context()
{
   ReleaseDC(window, dc);
}

auto device_context::value() const -> HDC
{
   return dc;
}

struct screenbuffer {
   BITMAPINFO info;
   std::vector<uint32_t> pixels;
   int width;
   int height;
};

class window {
public:
   window(HINSTANCE instance, const std::string& window_name, const std::string& class_name, LRESULT callback_fn(HWND, UINT, WPARAM, LPARAM), int backbuffer_width, int backbuffer_height);
   ~window();

   auto is_open() const -> bool;
   auto resize_buffer(int width, int height) -> void;
   auto render() -> void;
   auto width() const -> int;
   auto height() const -> int;
   auto get_backbuffer() -> screenbuffer*;

private:
   WNDCLASSA window_class;
   HWND window_handle;
   bool opened;
   screenbuffer backbuffer;
};

window::window(HINSTANCE instance, const std::string& window_name, const std::string& class_name, LRESULT callback_fn(HWND, UINT, WPARAM, LPARAM), int backbuffer_width = 1280, int backbuffer_height = 720)
: window_class {},
   window_handle {},
   opened { false },
   backbuffer {}
{
   window::resize_buffer(backbuffer_width, backbuffer_height);

   window_class = {
      .style = CS_HREDRAW|CS_VREDRAW,
      .lpfnWndProc = callback_fn,
      .hInstance = instance,
      .lpszClassName = class_name.c_str(),
   };

   if (!RegisterClassA(&window_class)) {
      return;
   }

   window_handle = CreateWindowExA(0, window_class.lpszClassName, window_name.c_str(), WS_OVERLAPPEDWINDOW|WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, nullptr, nullptr, instance, nullptr);
   if (!window_handle) {
      return;
   }

   opened = true;
}

window::~window()
{
   DestroyWindow(window_handle);
   opened = false;
}

auto window::is_open() const -> bool
{
   return opened;
}

auto window::resize_buffer(int width, int height) -> void
{
   backbuffer = screenbuffer{
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

   backbuffer.pixels.resize(width * height, 0x00000000);
}

auto window::render() -> void
{
   auto dc { device_context(window_handle) };
   StretchDIBits(dc.value(),
                 0, 0, width(), height(),
                 0, 0, backbuffer.width, backbuffer.height,
                 backbuffer.pixels.data(),
                 &backbuffer.info,
                 DIB_RGB_COLORS, SRCCOPY);
}

auto window::width() const -> int
{
   RECT r;
   GetClientRect(window_handle, &r);

   return r.right - r.left;
}

auto window::height() const -> int
{
   RECT r;
   GetClientRect(window_handle, &r);

   return r.bottom - r.top;
}

auto window::get_backbuffer() -> screenbuffer*
{
   return &backbuffer;
}

}

///

namespace {
// BEGIN namespace


static auto Running { false };

auto draw_weird_gradient(win32::window& window) -> void
{
   auto buffer { window.get_backbuffer() };
   for (int y = 0; y < buffer->height; y++) {
      for (int x = 0; x < buffer->width; x++) {
         uint8_t blue = (uint8_t)x;
         uint8_t green = (uint8_t)y;
         uint8_t red = (uint8_t)0;

         buffer->pixels[x + buffer->width * y] = (red << 16) | (green << 8) | blue;
      }
   }
}

struct color_rgba {
   uint8_t r;
   uint8_t g;
   uint8_t b;
   uint8_t a;
};

struct rectangle {
   int x;
   int y;
   int width;
   int height;
};

auto draw_rectangle(win32::window& window, rectangle rect, color_rgba color) -> void
{
   auto buffer { window.get_backbuffer() };
   int y_end = rect.y + rect.height;
   int x_end = rect.x + rect.width;

   if (x_end >= buffer->width) {
      x_end = buffer->width - 1;
   }

   if (y_end >= buffer->height) {
      y_end = buffer->height - 1;
   }

   for (int y = rect.y; y < y_end; y++) {
      for (int x = rect.x; x < x_end; x++) {
         buffer->pixels[x + buffer->width * y] = (color.r << 16) | (color.g << 8) | color.b;
      }
   }
}

auto main_window_callback(HWND window_handle, UINT message, WPARAM wparam, LPARAM lparam) -> LRESULT
{
   LRESULT result {};

   switch (message) {
      case WM_QUIT:
      case WM_CLOSE:
      case WM_DESTROY: {

         Running = false;
      } break;

      default: {
         result = DefWindowProc(window_handle, message, wparam, lparam);
      } break;
   }

   return result;
}


} // END namespace

auto WinMain(HINSTANCE instance, HINSTANCE prev_instance, LPSTR cmd_line, int cmd_show) -> int
{
   auto window { win32::window(instance, "Win32 Pong", "Win32PongWindowClass", main_window_callback) };
   assert(window.is_open());

   Running = true;
   while (Running) {
      MSG msg {};
      while (PeekMessageA(&msg, nullptr, 0, 0, PM_REMOVE)) {
         TranslateMessage(&msg);
         DispatchMessage(&msg);
      }

      // draw_weird_gradient(window);

      auto buffer { window.get_backbuffer() };
      draw_rectangle(window, rectangle{ 10, (buffer->height - 125) / 2, 10, 125 }, color_rgba{ 0xFF, 0xFF, 0xFF, 0xFF });
      draw_rectangle(window, rectangle{ buffer->width - 10 - 10, (buffer->height - 125) / 2, 10, 125 }, color_rgba{ 0xFF, 0xFF, 0xFF, 0xFF });
      draw_rectangle(window, rectangle{ (buffer->width - 10) / 2, (buffer->height - 10) / 2, 10, 10 }, color_rgba{0xff, 0xff, 0xff, 0xff});

      window.render();
   }

   return 0;
}
