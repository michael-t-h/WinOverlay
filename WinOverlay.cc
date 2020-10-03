#include "WinOverlay.h"

EXTERN_C IMAGE_DOS_HEADER __ImageBase;

WinOverlay::WinOverlay(HWND target, unsigned int width, unsigned int height) :
  target(target), width(width), height(height), window(nullptr),
  d2d_factory(nullptr), d2d_target(nullptr), d2d_brush(nullptr),
  gdi_device(nullptr), gdi_bitmap(nullptr) {
}

WinOverlay::~WinOverlay() {
  if (d2d_factory) d2d_factory->Release();
  DiscardDeviceResources();
}

bool WinOverlay::Initialise() {
  const HINSTANCE kInstance = (HINSTANCE)&__ImageBase;
  const wchar_t kClassName[] = L"WinOverlay";

  // Create Direct2D resources
  if (!CreateDeviceIndependentResources()) return false;

  // Create window class
  WNDCLASS window_class = {};
  window_class.lpfnWndProc = WindowProc;
  window_class.hInstance = kInstance;
  window_class.lpszClassName = kClassName;
  RegisterClass(&window_class);

  // Create window
  window = CreateWindowEx(WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST,
    kClassName, nullptr, WS_POPUP | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT,
    CW_USEDEFAULT, CW_USEDEFAULT, target, nullptr, kInstance, this);
  if (!window) return false;
  SetParent(window, target);
  SetWindowPos(window, HWND_TOP, 0, 0, width, height, SWP_NOACTIVATE);

  return true;
}

bool WinOverlay::CreateDeviceIndependentResources() {
  // Create Direct2D factory
  if (FAILED(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED,
    &d2d_factory))) return false;

  return true;
}

bool WinOverlay::CreateDeviceResources() {
  // Check if resources already created
  if (d2d_target) return true;

  // Create Direct2D target
  D2D1_RENDER_TARGET_PROPERTIES properties = D2D1::RenderTargetProperties(
    D2D1_RENDER_TARGET_TYPE_DEFAULT, D2D1::PixelFormat(
      DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED), 0, 0,
    D2D1_RENDER_TARGET_USAGE_NONE, D2D1_FEATURE_LEVEL_DEFAULT);
  if (FAILED(d2d_factory->CreateDCRenderTarget(&properties, &d2d_target)))
    return false;

  // Create GDI resources
  RECT geometry;
  GetClientRect(window, &geometry);
  HDC window_device = GetDC(window);
  gdi_device = CreateCompatibleDC(window_device);
  gdi_bitmap = CreateCompatibleBitmap(window_device, geometry.right -
    geometry.left, geometry.bottom - geometry.top);
  ReleaseDC(window, window_device);
  SelectObject(gdi_device, gdi_bitmap);
  if (d2d_target) d2d_target->BindDC(gdi_device, &geometry);

  // Create Direct2D resources
  d2d_target->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Red),
    &d2d_brush);

  return true;
}

void WinOverlay::DiscardDeviceResources() {
  if (d2d_brush) d2d_brush->Release();
  if (d2d_target) d2d_target->Release();
  if (gdi_device) DeleteDC(gdi_device);
  if (gdi_bitmap) DeleteObject(gdi_bitmap);
}

void WinOverlay::Run() {
  MSG message;

  for (;;) {
    // Handle window messages
    while (PeekMessage(&message, 0, 0, 0, PM_REMOVE)) {
      TranslateMessage(&message);
      DispatchMessage(&message);
      if (message.message == WM_QUIT) return;
    }

    Render();
  }
}

bool WinOverlay::Render() {
  // Create needed resources
  if (!CreateDeviceResources()) return false;

  // Initialise Direct2D drawing
  d2d_target->BeginDraw();
  d2d_target->SetTransform(D2D1::Matrix3x2F::Identity());
  d2d_target->Clear(nullptr);

  // Draw Direct2D objects
  d2d_target->FillRectangle(D2D1::RectF(100, 100, 200, 200), d2d_brush);

  // Finish Direct2D drawing
  if (d2d_target->EndDraw() == D2DERR_RECREATE_TARGET)
    DiscardDeviceResources();

  // Update overlay window
  SIZE size = { width, height };
  POINT point = { 0, 0 };
  BLENDFUNCTION blend = {};
  blend.AlphaFormat = AC_SRC_ALPHA;
  blend.SourceConstantAlpha = 255;
  UpdateLayeredWindow(window, nullptr, nullptr, &size, gdi_device,
    &point, 0, &blend, ULW_ALPHA);

  return true;
}

LRESULT CALLBACK WinOverlay::WindowProc(HWND window, UINT message,
  WPARAM word_param, LPARAM long_param) {
  // Get overlay object
  WinOverlay* test = reinterpret_cast<WinOverlay*>(::GetWindowLongPtrW(
    window, GWLP_USERDATA));

  switch (message) {
  case WM_CREATE:
    // Set overlay object
    test = (WinOverlay*)((LPCREATESTRUCT)long_param)->lpCreateParams;
    SetWindowLongPtrW(window, GWLP_USERDATA,
      reinterpret_cast<LONG_PTR>(test));
    return 1;
  case WM_DESTROY:
    PostQuitMessage(0);
    return 1;
  }

  return DefWindowProc(window, message, word_param, long_param);
}

int WINAPI WinMain(HINSTANCE instance, HINSTANCE previous_instance, LPSTR command, int show) {
  const unsigned int kWidth = 4096, kHeight = 4096;

  // Find target window
  HWND target = FindWindow(L"Notepad", nullptr);
  if (!target) return 0;

  // Create overlay
  WinOverlay overlay(target, kWidth, kHeight);
  if (overlay.Initialise()) overlay.Run();

  return 0;
}