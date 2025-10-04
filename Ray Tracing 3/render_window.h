#ifndef RENDER_WINDOW_H
#define RENDER_WINDOW_H

#include <windows.h>
#include <string>
#include <mutex>
#include <cstdint>
#include <cstring>

class RenderWindow {
public:
    RenderWindow() = default;
    ~RenderWindow() { destroy(); }

    bool create(int width, int height, const std::wstring& title) {
        if (hwnd_) return true;
        width_ = width;
        height_ = height;

        WNDCLASSEXW wc{};
        wc.cbSize = sizeof(WNDCLASSEXW);
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = &RenderWindow::WndProc;
        wc.hInstance = GetModuleHandleW(nullptr);
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        wc.lpszClassName = L"RayTracerWindowClass";

        if (!RegisterClassExW(&wc) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
            return false;
        }

        DWORD style = WS_OVERLAPPEDWINDOW & ~(WS_MAXIMIZEBOX | WS_THICKFRAME);
        RECT rc{0, 0, width, height};
        AdjustWindowRect(&rc, style, FALSE);

        hwnd_ = CreateWindowExW(
            0,
            wc.lpszClassName,
            title.c_str(),
            style,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            rc.right - rc.left,
            rc.bottom - rc.top,
            nullptr,
            nullptr,
            wc.hInstance,
            this
        );

        if (!hwnd_) {
            return false;
        }

        ShowWindow(hwnd_, SW_SHOW);
        UpdateWindow(hwnd_);

        hdc_ = GetDC(hwnd_);
        memory_dc_ = CreateCompatibleDC(hdc_);

        BITMAPINFO bmi{};
        bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth = width_;
        bmi.bmiHeader.biHeight = -height_; // top-down bitmap
        bmi.bmiHeader.biPlanes = 1;
        bmi.bmiHeader.biBitCount = 32;
        bmi.bmiHeader.biCompression = BI_RGB;

        uint8_t* bits = nullptr;
        hbitmap_ = CreateDIBSection(memory_dc_, &bmi, DIB_RGB_COLORS, reinterpret_cast<void**>(&bits), nullptr, 0);
        if (!hbitmap_) {
            destroy();
            return false;
        }

        dib_bits_ = bits;
        SelectObject(memory_dc_, hbitmap_);
        return true;
    }

    void destroy() {
        if (memory_dc_) {
            if (hbitmap_) {
                DeleteObject(hbitmap_);
                hbitmap_ = nullptr;
            }
            DeleteDC(memory_dc_);
            memory_dc_ = nullptr;
        }
        if (hwnd_) {
            ReleaseDC(hwnd_, hdc_);
            DestroyWindow(hwnd_);
            hwnd_ = nullptr;
        }
    }

    bool process_events() {
        MSG msg;
        while (PeekMessageW(&msg, hwnd_, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                return false;
            }
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
        return hwnd_ != nullptr;
    }

    void present(const uint8_t* bgra_pixels) {
        if (!hwnd_ || !bgra_pixels) return;
        const size_t bytes = static_cast<size_t>(width_) * static_cast<size_t>(height_) * 4;
        {
            std::lock_guard<std::mutex> lock(buffer_mutex_);
            std::memcpy(dib_bits_, bgra_pixels, bytes);
        }
        BitBlt(hdc_, 0, 0, width_, height_, memory_dc_, 0, 0, SRCCOPY);
    }

    int width() const { return width_; }
    int height() const { return height_; }

private:
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
        if (msg == WM_NCCREATE) {
            auto cs = reinterpret_cast<CREATESTRUCTW*>(lparam);
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(cs->lpCreateParams));
        }

        auto self = reinterpret_cast<RenderWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        if (self) {
            switch (msg) {
                case WM_CLOSE:
                    DestroyWindow(hwnd);
                    return 0;
                case WM_DESTROY:
                    PostQuitMessage(0);
                    return 0;
                default:
                    break;
            }
        }
        return DefWindowProcW(hwnd, msg, wparam, lparam);
    }

    HWND hwnd_ = nullptr;
    HDC hdc_ = nullptr;
    HDC memory_dc_ = nullptr;
    HBITMAP hbitmap_ = nullptr;
    uint8_t* dib_bits_ = nullptr;
    std::mutex buffer_mutex_;
    int width_ = 0;
    int height_ = 0;
};

#endif
