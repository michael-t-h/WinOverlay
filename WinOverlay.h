#ifndef WINOVERLAY_WINOVERLAY_H_
#define WINOVERLAY_WINOVERLAY_H_

#include <windows.h>
#include <d2d1.h>

class WinOverlay {
public:
  WinOverlay(HWND target, unsigned int width, unsigned int height);
  ~WinOverlay();
  bool Initialise();
  void Run();

private:
  HWND target, window;
  unsigned int width, height;
  ID2D1Factory* d2d_factory;
  ID2D1DCRenderTarget* d2d_target;
  ID2D1SolidColorBrush* d2d_brush;
  HDC gdi_device;
  HBITMAP gdi_bitmap;

  bool CreateDeviceIndependentResources();
  bool CreateDeviceResources();
  void DiscardDeviceResources();
  bool Render();
  static LRESULT CALLBACK WindowProc(HWND window, UINT message,
    WPARAM word_param, LPARAM long_param);
};

#endif