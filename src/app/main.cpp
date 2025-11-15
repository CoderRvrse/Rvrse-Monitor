#define NOMINMAX
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <commctrl.h>
#include <strsafe.h>
#include <iphlpapi.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cwchar>
#include <cwctype>
#include <deque>
#include <memory>
#include <numeric>
#include <sstream>
#include <string>
#include <vector>

#include "rvrse_monitor.h"
#include "rvrse/common/formatting.h"
#include "process_snapshot.h"
#include "network_snapshot.h"
#include "handle_snapshot.h"
#include "plugin_loader.h"

#pragma comment(lib, "Comctl32.lib")
#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "Iphlpapi.lib")

namespace
{
    constexpr wchar_t kWindowClassName[] = L"RvrseMonitorMainWindow";
    constexpr UINT_PTR kRefreshTimerId = 0x2001;
    constexpr UINT kRefreshIntervalMs = 4000;
    constexpr int kListViewId = 0x3001;
    constexpr int kRefreshButtonId = 0x3002;
    constexpr int kFilterEditId = 0x3003;
    constexpr int kDetailsStaticId = 0x3004;
    constexpr int kModulesButtonId = 0x3005;
    constexpr int kConnectionsButtonId = 0x3006;
    constexpr int kMenuTerminateId = 0x4001;
    constexpr int kMenuTerminateTreeId = 0x4002;

    class ResourceGraphView
    {
    public:
        bool Create(HWND parent, HINSTANCE instance)
        {
            if (!EnsureClassRegistered(instance))
            {
                return false;
            }

            hwnd_ = CreateWindowExW(
                0,
                kClassName,
                nullptr,
                WS_CHILD | WS_VISIBLE,
                0,
                0,
                0,
                0,
                parent,
                nullptr,
                instance,
                this);
            return hwnd_ != nullptr;
        }

        void Destroy()
        {
            if (hwnd_)
            {
                DestroyWindow(hwnd_);
                hwnd_ = nullptr;
            }
        }

        void Resize(int left, int top, int width, int height)
        {
            if (hwnd_)
            {
                MoveWindow(hwnd_, left, top, width, height, TRUE);
            }
        }

        void AddSample(double cpuPercent, double memoryPercent)
        {
            AppendSample(cpuHistory_, cpuPercent);
            AppendSample(memoryHistory_, memoryPercent);
            latestCpu_ = cpuPercent;
            latestMemory_ = memoryPercent;

            if (hwnd_)
            {
                InvalidateRect(hwnd_, nullptr, FALSE);
            }
        }

        bool IsCreated() const
        {
            return hwnd_ != nullptr;
        }

    private:
        static constexpr const wchar_t *kClassName = L"RvrseResourceGraphView";
        static constexpr size_t kMaxSamples = 180;
        static constexpr int kPadding = 10;

        static bool EnsureClassRegistered(HINSTANCE instance)
        {
            static ATOM classAtom = 0;
            if (classAtom != 0)
            {
                return true;
            }

            WNDCLASSW windowClass{};
            windowClass.style = CS_HREDRAW | CS_VREDRAW;
            windowClass.lpfnWndProc = &ResourceGraphView::WndProc;
            windowClass.hInstance = instance;
            windowClass.lpszClassName = kClassName;
            windowClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
            windowClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
            classAtom = RegisterClassW(&windowClass);
            return classAtom != 0;
        }

        static void AppendSample(std::deque<double> &samples, double value)
        {
            value = std::clamp(value, 0.0, 100.0);
            if (samples.size() >= kMaxSamples)
            {
                samples.pop_front();
            }
            samples.push_back(value);
        }

        static LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
        {
            ResourceGraphView *self = nullptr;

            if (message == WM_NCCREATE)
            {
                auto *create = reinterpret_cast<CREATESTRUCTW *>(lParam);
                self = static_cast<ResourceGraphView *>(create->lpCreateParams);
                self->hwnd_ = hwnd;
                SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
                return TRUE;
            }

            self = reinterpret_cast<ResourceGraphView *>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
            if (!self)
            {
                return DefWindowProcW(hwnd, message, wParam, lParam);
            }

            switch (message)
            {
            case WM_PAINT:
                self->OnPaint();
                return 0;
            case WM_ERASEBKGND:
                return 1;
            default:
                break;
            }

            return DefWindowProcW(hwnd, message, wParam, lParam);
        }

        void OnPaint()
        {
            if (!hwnd_)
            {
                return;
            }

            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd_, &ps);
            if (!hdc)
            {
                return;
            }

            RECT client{};
            GetClientRect(hwnd_, &client);
            const int width = client.right - client.left;
            const int height = client.bottom - client.top;

            HDC memoryDc = CreateCompatibleDC(hdc);
            HBITMAP buffer = CreateCompatibleBitmap(hdc, width, height);
            HGDIOBJ oldBitmap = SelectObject(memoryDc, buffer);

            HBRUSH background = CreateSolidBrush(GetSysColor(COLOR_WINDOW));
            FillRect(memoryDc, &client, background);
            DeleteObject(background);

            RECT plotRect = client;
            InflateRect(&plotRect, -kPadding, -kPadding);
            if (plotRect.right <= plotRect.left || plotRect.bottom <= plotRect.top)
            {
                plotRect = client;
            }

            DrawGrid(memoryDc, plotRect);
            DrawSeries(memoryDc, plotRect, memoryHistory_, RGB(214, 137, 16));
            DrawSeries(memoryDc, plotRect, cpuHistory_, RGB(40, 120, 255));
            DrawLegend(memoryDc, plotRect);

            HBRUSH border = CreateSolidBrush(RGB(200, 200, 200));
            FrameRect(memoryDc, &client, border);
            DeleteObject(border);

            BitBlt(hdc, 0, 0, width, height, memoryDc, 0, 0, SRCCOPY);

            SelectObject(memoryDc, oldBitmap);
            DeleteObject(buffer);
            DeleteDC(memoryDc);
            EndPaint(hwnd_, &ps);
        }

        void DrawGrid(HDC hdc, const RECT &plotRect)
        {
            if (plotRect.bottom <= plotRect.top)
            {
                return;
            }

            HPEN gridPen = CreatePen(PS_DOT, 1, RGB(220, 220, 220));
            HGDIOBJ oldPen = SelectObject(hdc, gridPen);

            const int height = plotRect.bottom - plotRect.top;
            for (int i = 1; i <= 3; ++i)
            {
                int y = plotRect.top + MulDiv(height, i, 4);
                MoveToEx(hdc, plotRect.left, y, nullptr);
                LineTo(hdc, plotRect.right, y);
            }

            SelectObject(hdc, oldPen);
            DeleteObject(gridPen);
        }

        void DrawSeries(HDC hdc, const RECT &plotRect, const std::deque<double> &samples, COLORREF color)
        {
            if (samples.empty())
            {
                return;
            }

            const int width = plotRect.right - plotRect.left;
            const int height = plotRect.bottom - plotRect.top;
            if (width <= 0 || height <= 0)
            {
                return;
            }

            std::vector<POINT> points(samples.size());
            double step = samples.size() > 1 ? static_cast<double>(width) / (samples.size() - 1) : 0.0;

            size_t index = 0;
            for (double value : samples)
            {
                double clamped = std::clamp(value, 0.0, 100.0) / 100.0;
                LONG x = plotRect.left + static_cast<LONG>(std::round(step * index));
                LONG y = plotRect.bottom - static_cast<LONG>(std::round(clamped * height));
                points[index] = {x, y};
                ++index;
            }

            HPEN seriesPen = CreatePen(PS_SOLID, 2, color);
            HGDIOBJ oldPen = SelectObject(hdc, seriesPen);
            Polyline(hdc, points.data(), static_cast<int>(points.size()));
            SelectObject(hdc, oldPen);
            DeleteObject(seriesPen);
        }

        void DrawLegend(HDC hdc, const RECT &plotRect)
        {
            SetBkMode(hdc, TRANSPARENT);

            const int legendTop = plotRect.top + 6;
            int legendLeft = plotRect.left + 6;

            DrawLegendEntry(hdc, legendLeft, legendTop, RGB(40, 120, 255), L"CPU", latestCpu_);
            legendLeft += 110;
            DrawLegendEntry(hdc, legendLeft, legendTop, RGB(214, 137, 16), L"Memory", latestMemory_);
        }

        void DrawLegendEntry(HDC hdc, int left, int top, COLORREF color, const wchar_t *label, double value)
        {
            RECT swatch{left, top, left + 10, top + 10};
            HBRUSH brush = CreateSolidBrush(color);
            FillRect(hdc, &swatch, brush);
            DeleteObject(brush);
            FrameRect(hdc, &swatch, reinterpret_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)));

            wchar_t text[64];
            StringCchPrintfW(text, std::size(text), L"%s: %.1f%%", label, value);
            SetTextColor(hdc, RGB(32, 32, 32));
            TextOutW(hdc, left + 14, top - 1, text, lstrlenW(text));
        }

        HWND hwnd_ = nullptr;
        std::deque<double> cpuHistory_;
        std::deque<double> memoryHistory_;
        double latestCpu_ = 0.0;
        double latestMemory_ = 0.0;
    };

class ModuleViewerWindow
    {
    public:
        static void Show(HWND owner, HINSTANCE instance, const rvrse::core::ProcessEntry &process)
        {
            auto window = std::unique_ptr<ModuleViewerWindow>(new ModuleViewerWindow(instance, process));
            if (window->Create(owner))
            {
                window.release();
            }
            else
            {
                MessageBoxW(owner,
                            L"Failed to create the module viewer window.",
                            rvrse::kProductName,
                            MB_OK | MB_ICONERROR);
            }
        }

        ~ModuleViewerWindow() = default;

    private:
        static constexpr const wchar_t *kClassName = L"RvrseModuleViewerWindow";

        ModuleViewerWindow(HINSTANCE instance, const rvrse::core::ProcessEntry &process)
            : instance_(instance), process_(process)
        {
            modules_ = rvrse::core::ProcessSnapshot::EnumerateModules(process.processId);
        }

        bool Create(HWND owner)
        {
            if (!EnsureClassRegistered())
            {
                return false;
            }

            hwnd_ = CreateWindowExW(
                0,
                kClassName,
                L"",
                WS_OVERLAPPEDWINDOW | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_SIZEBOX,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                720,
                480,
                owner,
                nullptr,
                instance_,
                this);

            if (!hwnd_)
            {
                return false;
            }

            ShowWindow(hwnd_, SW_SHOWNORMAL);
            UpdateWindow(hwnd_);
            return true;
        }

        static bool EnsureClassRegistered()
        {
            static ATOM classAtom = 0;
            if (classAtom != 0)
            {
                return true;
            }

            WNDCLASSW windowClass{};
            windowClass.lpfnWndProc = &ModuleViewerWindow::WndProc;
            windowClass.hInstance = GetModuleHandleW(nullptr);
            windowClass.lpszClassName = kClassName;
            windowClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
            windowClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
            classAtom = RegisterClassW(&windowClass);
            return classAtom != 0;
        }

        static LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
        {
            ModuleViewerWindow *self = nullptr;

            if (message == WM_NCCREATE)
            {
                auto *create = reinterpret_cast<CREATESTRUCTW *>(lParam);
                self = static_cast<ModuleViewerWindow *>(create->lpCreateParams);
                self->hwnd_ = hwnd;
                self->instance_ = create->hInstance;
                SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
            }
            else
            {
                self = reinterpret_cast<ModuleViewerWindow *>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
            }

            if (!self)
            {
                return DefWindowProcW(hwnd, message, wParam, lParam);
            }

            switch (message)
            {
            case WM_CREATE:
                self->OnCreate();
                return 0;
            case WM_SIZE:
                self->OnSize(LOWORD(lParam), HIWORD(lParam));
                return 0;
            case WM_DESTROY:
                self->OnDestroy();
                return 0;
            case WM_NCDESTROY:
                delete self;
                return 0;
            default:
                break;
            }

            return DefWindowProcW(hwnd, message, wParam, lParam);
        }

        void OnCreate()
        {
            INITCOMMONCONTROLSEX icex = {sizeof(icex)};
            icex.dwICC = ICC_LISTVIEW_CLASSES;
            InitCommonControlsEx(&icex);

            listView_ = CreateWindowExW(
                WS_EX_CLIENTEDGE,
                WC_LISTVIEWW,
                nullptr,
                WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_SINGLESEL | LVS_NOSORTHEADER,
                0,
                0,
                0,
                0,
                hwnd_,
                nullptr,
                instance_,
                nullptr);

            ListView_SetExtendedListViewStyle(listView_, LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_GRIDLINES);
            EnsureColumns();
            PopulateModules();
            UpdateWindowTitle();
        }

        void EnsureColumns()
        {
            if (!listView_)
            {
                return;
            }

            struct ColumnInfo
            {
                const wchar_t *title;
                int width;
            } columns[] = {
                {L"Module", 180},
                {L"Base Address", 140},
                {L"Size", 120},
                {L"Path", 320}};

            LVCOLUMNW column{};
            column.mask = LVCF_TEXT | LVCF_WIDTH;

            for (int i = 0; i < static_cast<int>(std::size(columns)); ++i)
            {
                column.pszText = const_cast<wchar_t *>(columns[i].title);
                column.cx = columns[i].width;
                ListView_InsertColumn(listView_, i, &column);
            }
        }

        void PopulateModules()
        {
            if (!listView_)
            {
                return;
            }

            ListView_DeleteAllItems(listView_);

            for (int index = 0; index < static_cast<int>(modules_.size()); ++index)
            {
                const auto &module = modules_[index];
                std::wstring displayName = module.name.empty() ? L"[Unknown]" : module.name;

                LVITEMW item{};
                item.mask = LVIF_TEXT;
                item.iItem = index;
                item.pszText = displayName.data();
                ListView_InsertItem(listView_, &item);

                wchar_t baseBuffer[64];
                StringCchPrintfW(baseBuffer, std::size(baseBuffer), L"0x%p", reinterpret_cast<void *>(module.baseAddress));
                ListView_SetItemText(listView_, index, 1, baseBuffer);

                std::wstring sizeText = rvrse::common::FormatSize(module.sizeBytes);
                ListView_SetItemText(listView_, index, 2, sizeText.data());

                std::wstring pathText = module.path.empty() ? L"(unknown)" : module.path;
                ListView_SetItemText(listView_, index, 3, pathText.data());
            }
        }

        void UpdateWindowTitle()
        {
            wchar_t title[256];
            StringCchPrintfW(title,
                             std::size(title),
                             L"Modules - %s (PID %u)",
                             process_.imageName.empty() ? L"[Unnamed]" : process_.imageName.c_str(),
                             process_.processId);
            SetWindowTextW(hwnd_, title);
        }

        void OnSize(int width, int height)
        {
            if (listView_)
            {
                MoveWindow(listView_, 0, 0, std::max(0, width), std::max(0, height), TRUE);
            }
        }

        void OnDestroy()
        {
            // Nothing to do; deletion handled on WM_NCDESTROY.
        }

        HINSTANCE instance_;
        HWND hwnd_ = nullptr;
        HWND listView_ = nullptr;
        rvrse::core::ProcessEntry process_;
        std::vector<rvrse::core::ModuleEntry> modules_;
    };

    class ConnectionViewerWindow
    {
    public:
        static void Show(HWND owner,
                         HINSTANCE instance,
                         const rvrse::core::ProcessEntry &process,
                         const rvrse::core::NetworkSnapshot &snapshot)
        {
            auto connections = snapshot.ConnectionsForProcess(process.processId);
            auto window = std::unique_ptr<ConnectionViewerWindow>(
                new ConnectionViewerWindow(instance, process, std::move(connections)));
            if (window->Create(owner))
            {
                window.release();
            }
            else
            {
                MessageBoxW(owner,
                            L"Failed to create the connection viewer window.",
                            rvrse::kProductName,
                            MB_OK | MB_ICONERROR);
            }
        }

    private:
        static constexpr const wchar_t *kClassName = L"RvrseConnectionViewerWindow";

        ConnectionViewerWindow(HINSTANCE instance,
                               const rvrse::core::ProcessEntry &process,
                               std::vector<rvrse::core::ConnectionEntry> connections)
            : instance_(instance), process_(process), connections_(std::move(connections))
        {
        }

        bool Create(HWND owner)
        {
            if (!EnsureClassRegistered())
            {
                return false;
            }

            hwnd_ = CreateWindowExW(
                0,
                kClassName,
                L"",
                WS_OVERLAPPEDWINDOW | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_SIZEBOX,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                860,
                520,
                owner,
                nullptr,
                instance_,
                this);

            if (!hwnd_)
            {
                return false;
            }

            ShowWindow(hwnd_, SW_SHOWNORMAL);
            UpdateWindow(hwnd_);
            return true;
        }

        static bool EnsureClassRegistered()
        {
            static ATOM classAtom = 0;
            if (classAtom != 0)
            {
                return true;
            }

            WNDCLASSW windowClass{};
            windowClass.lpfnWndProc = &ConnectionViewerWindow::WndProc;
            windowClass.hInstance = GetModuleHandleW(nullptr);
            windowClass.lpszClassName = kClassName;
            windowClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
            windowClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
            classAtom = RegisterClassW(&windowClass);
            return classAtom != 0;
        }

        static LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
        {
            ConnectionViewerWindow *self = nullptr;

            if (message == WM_NCCREATE)
            {
                auto *create = reinterpret_cast<CREATESTRUCTW *>(lParam);
                self = static_cast<ConnectionViewerWindow *>(create->lpCreateParams);
                self->hwnd_ = hwnd;
                self->instance_ = create->hInstance;
                SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
            }
            else
            {
                self = reinterpret_cast<ConnectionViewerWindow *>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
            }

            if (!self)
            {
                return DefWindowProcW(hwnd, message, wParam, lParam);
            }

            switch (message)
            {
            case WM_CREATE:
                self->OnCreate();
                return 0;
            case WM_SIZE:
                self->OnSize(LOWORD(lParam), HIWORD(lParam));
                return 0;
            case WM_DESTROY:
                self->OnDestroy();
                return 0;
            case WM_NCDESTROY:
                delete self;
                return 0;
            default:
                break;
            }

            return DefWindowProcW(hwnd, message, wParam, lParam);
        }

        void OnCreate()
        {
            INITCOMMONCONTROLSEX icex = {sizeof(icex)};
            icex.dwICC = ICC_LISTVIEW_CLASSES;
            InitCommonControlsEx(&icex);

            listView_ = CreateWindowExW(
                WS_EX_CLIENTEDGE,
                WC_LISTVIEWW,
                nullptr,
                WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_SINGLESEL | LVS_NOSORTHEADER,
                0,
                0,
                0,
                0,
                hwnd_,
                nullptr,
                instance_,
                nullptr);

            ListView_SetExtendedListViewStyle(listView_, LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_GRIDLINES);
            EnsureColumns();
            PopulateConnections();
            UpdateWindowTitle();
        }

        void EnsureColumns()
        {
            if (!listView_)
            {
                return;
            }

            struct ColumnInfo
            {
                const wchar_t *title;
                int width;
            } columns[] = {
                {L"Protocol", 90},
                {L"Local Address", 200},
                {L"Remote Address", 200},
                {L"State", 120}};

            LVCOLUMNW column{};
            column.mask = LVCF_TEXT | LVCF_WIDTH;

            for (int i = 0; i < static_cast<int>(std::size(columns)); ++i)
            {
                column.pszText = const_cast<wchar_t *>(columns[i].title);
                column.cx = columns[i].width;
                ListView_InsertColumn(listView_, i, &column);
            }
        }

        void PopulateConnections()
        {
            if (!listView_)
            {
                return;
            }

            ListView_DeleteAllItems(listView_);

            if (connections_.empty())
            {
                LVITEMW item{};
                item.mask = LVIF_TEXT;
                item.iItem = 0;
                item.pszText = const_cast<wchar_t *>(L"(no active connections)");
                ListView_InsertItem(listView_, &item);
                for (int column = 1; column < 4; ++column)
                {
                    ListView_SetItemText(listView_, 0, column, const_cast<wchar_t *>(L"-"));
                }
                return;
            }

            for (int index = 0; index < static_cast<int>(connections_.size()); ++index)
            {
                const auto &connection = connections_[index];
                std::wstring protocolText = connection.protocol == rvrse::core::TransportProtocol::Tcp ? L"TCP" : L"UDP";

                LVITEMW item{};
                item.mask = LVIF_TEXT;
                item.iItem = index;
                item.pszText = protocolText.data();
                ListView_InsertItem(listView_, &item);

                auto localEndpoint = FormatEndpoint(connection);
                ListView_SetItemText(listView_, index, 1, localEndpoint.data());

                auto remoteEndpoint = connection.protocol == rvrse::core::TransportProtocol::Tcp
                                           ? FormatRemoteEndpoint(connection)
                                           : std::wstring(L"-");
                ListView_SetItemText(listView_, index, 2, remoteEndpoint.data());

                std::wstring stateText = connection.protocol == rvrse::core::TransportProtocol::Tcp
                                             ? DescribeTcpState(connection.state)
                                             : L"-";
                ListView_SetItemText(listView_, index, 3, stateText.data());
            }
        }

        void UpdateWindowTitle()
        {
            wchar_t title[256];
            StringCchPrintfW(title,
                             std::size(title),
                             L"Connections - %s (PID %u)",
                             process_.imageName.empty() ? L"[Unnamed]" : process_.imageName.c_str(),
                             process_.processId);
            SetWindowTextW(hwnd_, title);
        }

        void OnSize(int width, int height)
        {
            if (listView_)
            {
                MoveWindow(listView_, 0, 0, std::max(0, width), std::max(0, height), TRUE);
            }
        }

        void OnDestroy()
        {
        }

        static std::wstring FormatEndpoint(const rvrse::core::ConnectionEntry &connection)
        {
            wchar_t buffer[INET6_ADDRSTRLEN] = L"";

            if (connection.family == rvrse::core::AddressFamily::IPv4)
            {
                if (connection.localAddress == 0)
                {
                    return std::wstring(L"-");
                }

                IN_ADDR addr{};
                addr.S_un.S_addr = connection.localAddress;
                if (!InetNtopW(AF_INET, &addr, buffer, static_cast<DWORD>(std::size(buffer))))
                {
                    StringCchCopyW(buffer, std::size(buffer), L"0.0.0.0");
                }
            }
            else // IPv6
            {
                if (!InetNtopW(AF_INET6, const_cast<std::uint8_t *>(connection.localAddress6), buffer, static_cast<DWORD>(std::size(buffer))))
                {
                    StringCchCopyW(buffer, std::size(buffer), L"::");
                }
            }

            if (connection.localPort == 0)
            {
                return std::wstring(buffer);
            }

            wchar_t endpoint[128];
            if (connection.family == rvrse::core::AddressFamily::IPv6)
            {
                StringCchPrintfW(endpoint, std::size(endpoint), L"[%s]:%u", buffer, connection.localPort);
            }
            else
            {
                StringCchPrintfW(endpoint, std::size(endpoint), L"%s:%u", buffer, connection.localPort);
            }
            return endpoint;
        }

        static std::wstring FormatRemoteEndpoint(const rvrse::core::ConnectionEntry &connection)
        {
            wchar_t buffer[INET6_ADDRSTRLEN] = L"";

            if (connection.family == rvrse::core::AddressFamily::IPv4)
            {
                if (connection.remoteAddress == 0)
                {
                    return std::wstring(L"-");
                }

                IN_ADDR addr{};
                addr.S_un.S_addr = connection.remoteAddress;
                if (!InetNtopW(AF_INET, &addr, buffer, static_cast<DWORD>(std::size(buffer))))
                {
                    StringCchCopyW(buffer, std::size(buffer), L"0.0.0.0");
                }
            }
            else // IPv6
            {
                if (!InetNtopW(AF_INET6, const_cast<std::uint8_t *>(connection.remoteAddress6), buffer, static_cast<DWORD>(std::size(buffer))))
                {
                    StringCchCopyW(buffer, std::size(buffer), L"::");
                }
            }

            if (connection.remotePort == 0)
            {
                return std::wstring(buffer);
            }

            wchar_t endpoint[128];
            if (connection.family == rvrse::core::AddressFamily::IPv6)
            {
                StringCchPrintfW(endpoint, std::size(endpoint), L"[%s]:%u", buffer, connection.remotePort);
            }
            else
            {
                StringCchPrintfW(endpoint, std::size(endpoint), L"%s:%u", buffer, connection.remotePort);
            }
            return endpoint;
        }

        static const wchar_t *DescribeTcpState(std::uint8_t state)
        {
            switch (state)
            {
            case MIB_TCP_STATE_CLOSED:
                return L"CLOSED";
            case MIB_TCP_STATE_LISTEN:
                return L"LISTEN";
            case MIB_TCP_STATE_SYN_SENT:
                return L"SYN-SENT";
            case MIB_TCP_STATE_SYN_RCVD:
                return L"SYN-RECEIVED";
            case MIB_TCP_STATE_ESTAB:
                return L"ESTABLISHED";
            case MIB_TCP_STATE_FIN_WAIT1:
                return L"FIN-WAIT-1";
            case MIB_TCP_STATE_FIN_WAIT2:
                return L"FIN-WAIT-2";
            case MIB_TCP_STATE_CLOSE_WAIT:
                return L"CLOSE-WAIT";
            case MIB_TCP_STATE_CLOSING:
                return L"CLOSING";
            case MIB_TCP_STATE_LAST_ACK:
                return L"LAST-ACK";
            case MIB_TCP_STATE_TIME_WAIT:
                return L"TIME-WAIT";
            case MIB_TCP_STATE_DELETE_TCB:
                return L"DELETE-TCB";
            default:
                return L"UNKNOWN";
            }
        }

        HINSTANCE instance_ = nullptr;
        HWND hwnd_ = nullptr;
        HWND listView_ = nullptr;
        rvrse::core::ProcessEntry process_;
        std::vector<rvrse::core::ConnectionEntry> connections_;
    };

    class MainWindow
    {
    public:
        explicit MainWindow(HINSTANCE instance) : instance_(instance) {}

        bool Create()
        {
            WNDCLASSEXW windowClass = {0};
            windowClass.cbSize = sizeof(windowClass);
            windowClass.lpfnWndProc = &MainWindow::WndProc;
            windowClass.hInstance = instance_;
            windowClass.lpszClassName = kWindowClassName;
            windowClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
            windowClass.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
            windowClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
            RegisterClassExW(&windowClass);

            hwnd_ = CreateWindowExW(
                0,
                kWindowClassName,
                rvrse::kProductName,
                WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                1024,
                640,
                nullptr,
                nullptr,
                instance_,
                this);

            return hwnd_ != nullptr;
        }

        int Run()
        {
            MSG message;
            while (GetMessageW(&message, nullptr, 0, 0) > 0)
            {
                TranslateMessage(&message);
                DispatchMessageW(&message);
            }
            return static_cast<int>(message.wParam);
        }

    private:
        static LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
        {
            MainWindow *self = nullptr;

            if (message == WM_NCCREATE)
            {
                auto create = reinterpret_cast<CREATESTRUCTW *>(lParam);
                self = static_cast<MainWindow *>(create->lpCreateParams);
                self->hwnd_ = hwnd;
                SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
            }
            else
            {
                self = reinterpret_cast<MainWindow *>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
            }

            if (!self)
            {
                return DefWindowProcW(hwnd, message, wParam, lParam);
            }

            switch (message)
            {
            case WM_CREATE:
                self->OnCreate();
                return 0;
            case WM_SIZE:
                self->OnSize(LOWORD(lParam), HIWORD(lParam));
                return 0;
            case WM_COMMAND:
                self->OnCommand(LOWORD(wParam), HIWORD(wParam));
                return 0;
            case WM_TIMER:
                if (wParam == kRefreshTimerId)
                {
                    self->RefreshProcesses();
                    return 0;
                }
                break;
            case WM_KEYDOWN:
                self->OnKeyDown(static_cast<UINT>(wParam));
                return 0;
            case WM_NOTIFY:
                if (self->OnNotify(lParam))
                {
                    return 0;
                }
                break;
            case WM_DESTROY:
                self->OnDestroy();
                PostQuitMessage(0);
                return 0;
            default:
                break;
            }

            return DefWindowProcW(hwnd, message, wParam, lParam);
        }

        void OnCreate()
        {
            if (!pluginLoader_)
            {
                pluginLoader_ = std::make_unique<rvrse::core::PluginLoader>();
                pluginLoader_->LoadPlugins();
            }

            INITCOMMONCONTROLSEX icex = {sizeof(icex)};
            icex.dwICC = ICC_LISTVIEW_CLASSES;
            InitCommonControlsEx(&icex);

            refreshButton_ = CreateWindowExW(
                0,
                L"BUTTON",
                L"Refresh",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                0,
                0,
                0,
                0,
                hwnd_,
                reinterpret_cast<HMENU>(kRefreshButtonId),
                instance_,
                nullptr);

            modulesButton_ = CreateWindowExW(
                0,
                L"BUTTON",
                L"Modules...",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                0,
                0,
                0,
                0,
                hwnd_,
                reinterpret_cast<HMENU>(kModulesButtonId),
                instance_,
                nullptr);

            connectionsButton_ = CreateWindowExW(
                0,
                L"BUTTON",
                L"Connections...",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                0,
                0,
                0,
                0,
                hwnd_,
                reinterpret_cast<HMENU>(kConnectionsButtonId),
                instance_,
                nullptr);

            filterEdit_ = CreateWindowExW(
                WS_EX_CLIENTEDGE,
                L"EDIT",
                nullptr,
                WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
                0,
                0,
                0,
                0,
                hwnd_,
                reinterpret_cast<HMENU>(kFilterEditId),
                instance_,
                nullptr);

            listView_ = CreateWindowExW(
                WS_EX_CLIENTEDGE,
                WC_LISTVIEWW,
                nullptr,
                WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_SINGLESEL | LVS_NOSORTHEADER,
                0,
                0,
                0,
                0,
                hwnd_,
                reinterpret_cast<HMENU>(kListViewId),
                instance_,
                nullptr);

            ListView_SetExtendedListViewStyle(listView_, LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_GRIDLINES);
            detailsStatic_ = CreateWindowExW(
                0,
                L"STATIC",
                nullptr,
                WS_CHILD | WS_VISIBLE | SS_LEFTNOWORDWRAP,
                0,
                0,
                0,
                0,
                hwnd_,
                reinterpret_cast<HMENU>(kDetailsStaticId),
                instance_,
                nullptr);

            graphView_.Create(hwnd_, instance_);

            HFONT defaultFont = static_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
            if (refreshButton_)
            {
                SendMessageW(refreshButton_, WM_SETFONT, reinterpret_cast<WPARAM>(defaultFont), TRUE);
            }
            if (modulesButton_)
            {
                SendMessageW(modulesButton_, WM_SETFONT, reinterpret_cast<WPARAM>(defaultFont), TRUE);
            }
            if (connectionsButton_)
            {
                SendMessageW(connectionsButton_, WM_SETFONT, reinterpret_cast<WPARAM>(defaultFont), TRUE);
            }
            if (filterEdit_)
            {
                SendMessageW(filterEdit_, WM_SETFONT, reinterpret_cast<WPARAM>(defaultFont), TRUE);
            }
            if (listView_)
            {
                SendMessageW(listView_, WM_SETFONT, reinterpret_cast<WPARAM>(defaultFont), TRUE);
            }
            if (detailsStatic_)
            {
                SendMessageW(detailsStatic_, WM_SETFONT, reinterpret_cast<WPARAM>(defaultFont), TRUE);
            }

            EnsureColumns();
            RefreshProcesses();
            SetTimer(hwnd_, kRefreshTimerId, kRefreshIntervalMs, nullptr);
        }

        void OnSize(int width, int height)
        {
            LayoutControls(width, height);
        }

        void OnDestroy()
        {
            if (hwnd_)
            {
                KillTimer(hwnd_, kRefreshTimerId);
            }

            graphView_.Destroy();

            if (pluginLoader_)
            {
                pluginLoader_->UnloadPlugins();
            }
        }

        void OnKeyDown(UINT key)
        {
            if (key == VK_F5)
            {
                RefreshProcesses();
            }
        }

        void OnCommand(int controlId, int code)
        {
            if (controlId == kRefreshButtonId && code == BN_CLICKED)
            {
                RefreshProcesses();
            }
            else if (controlId == kFilterEditId && code == EN_CHANGE)
            {
                OnFilterChanged();
            }
            else if (controlId == kModulesButtonId && code == BN_CLICKED)
            {
                ShowModulesForSelection();
            }
            else if (controlId == kConnectionsButtonId && code == BN_CLICKED)
            {
                ShowConnectionsForSelection();
            }
            else if (controlId == kMenuTerminateId)
            {
                TerminateSelectedProcess(false);
            }
            else if (controlId == kMenuTerminateTreeId)
            {
                TerminateSelectedProcess(true);
            }
        }

        bool OnNotify(LPARAM lParam)
        {
            if (!listView_)
            {
                return false;
            }

            auto *header = reinterpret_cast<LPNMHDR>(lParam);
            if (header->hwndFrom != listView_)
            {
                return false;
            }

            switch (header->code)
            {
            case LVN_COLUMNCLICK:
            {
                auto *info = reinterpret_cast<NMLISTVIEW *>(lParam);
                OnColumnClick(info->iSubItem);
                return true;
            }
            case LVN_ITEMCHANGED:
            {
                auto *info = reinterpret_cast<NMLISTVIEW *>(lParam);
                if ((info->uChanged & LVIF_STATE) != 0)
                {
                    UpdateDetailsPanel();
                }
                return true;
            }
            case NM_DBLCLK:
            {
                ShowModulesForSelection();
                return true;
            }
            case NM_RCLICK:
            {
                ShowContextMenu();
                return true;
            }
            default:
                break;
            }

            return false;
        }

        void EnsureColumns()
        {
            if (columnsCreated_ || !listView_)
            {
                return;
            }

            constexpr struct
            {
                const wchar_t *title;
                int width;
            } columns[] = {
                {L"Process", 320},
                {L"PID", 80},
                {L"Threads", 90},
                {L"Working Set", 140},
                {L"Private Bytes", 140}};

            LVCOLUMNW column{};
            column.mask = LVCF_TEXT | LVCF_WIDTH;

            for (int i = 0; i < static_cast<int>(std::size(columns)); ++i)
            {
                column.pszText = const_cast<wchar_t *>(columns[i].title);
                column.cx = columns[i].width;
                ListView_InsertColumn(listView_, i, &column);
            }

            columnsCreated_ = true;
        }

        void RefreshProcesses()
        {
            snapshot_ = rvrse::core::ProcessSnapshot::Capture();
            handleSnapshot_ = rvrse::core::HandleSnapshot::Capture();
            networkSnapshot_ = rvrse::core::NetworkSnapshot::Capture();
            UpdateResourceGraphs();

            if (connectionsButton_)
            {
                EnableWindow(connectionsButton_, !(networkSnapshot_.AccessDenied() || networkSnapshot_.CaptureFailed()));
            }

            if (pluginLoader_)
            {
                pluginLoader_->BroadcastProcessSnapshot(snapshot_);
                pluginLoader_->BroadcastHandleSnapshot(handleSnapshot_);
            }

            ApplyFilterAndSort();
            UpdateDetailsPanel();
        }

        void PopulateList()
        {
            if (!listView_)
            {
                return;
            }

            ListView_DeleteAllItems(listView_);

            for (int index = 0; index < static_cast<int>(visibleProcesses_.size()); ++index)
            {
                const auto &process = visibleProcesses_[index];

                std::wstring displayName = process.imageName.empty() ? L"[Unnamed]" : process.imageName;
                LVITEMW item{};
                item.mask = LVIF_TEXT;
                item.iItem = index;
                item.pszText = displayName.data();
                ListView_InsertItem(listView_, &item);

                wchar_t pidBuffer[32];
                StringCchPrintfW(pidBuffer, std::size(pidBuffer), L"%u", process.processId);
                ListView_SetItemText(listView_, index, 1, pidBuffer);

                wchar_t threadBuffer[32];
                StringCchPrintfW(threadBuffer, std::size(threadBuffer), L"%u", process.threadCount);
                ListView_SetItemText(listView_, index, 2, threadBuffer);

                std::wstring workingSet = rvrse::common::FormatSize(process.workingSetBytes);
                ListView_SetItemText(listView_, index, 3, workingSet.data());

                std::wstring privateBytes = rvrse::common::FormatSize(process.privateBytes);
                ListView_SetItemText(listView_, index, 4, privateBytes.data());
            }
        }

        void UpdateResourceGraphs()
        {
            double memoryPercent = 0.0;
            MEMORYSTATUSEX memoryStatus{};
            memoryStatus.dwLength = sizeof(memoryStatus);
            if (GlobalMemoryStatusEx(&memoryStatus) && memoryStatus.ullTotalPhys > 0)
            {
                unsigned __int64 used = memoryStatus.ullTotalPhys - memoryStatus.ullAvailPhys;
                memoryPercent = (static_cast<double>(used) / static_cast<double>(memoryStatus.ullTotalPhys)) * 100.0;
            }

            double cpuPercent = cpuUsagePercent_;
            FILETIME idle{}, kernel{}, user{};
            if (GetSystemTimes(&idle, &kernel, &user))
            {
                ULARGE_INTEGER idle64{};
                idle64.LowPart = idle.dwLowDateTime;
                idle64.HighPart = idle.dwHighDateTime;

                ULARGE_INTEGER kernel64{};
                kernel64.LowPart = kernel.dwLowDateTime;
                kernel64.HighPart = kernel.dwHighDateTime;

                ULARGE_INTEGER user64{};
                user64.LowPart = user.dwLowDateTime;
                user64.HighPart = user.dwHighDateTime;

                if (hasCpuBaseline_)
                {
                    ULONGLONG idleDelta = idle64.QuadPart - previousIdleTime_.QuadPart;
                    ULONGLONG kernelDelta = kernel64.QuadPart - previousKernelTime_.QuadPart;
                    ULONGLONG userDelta = user64.QuadPart - previousUserTime_.QuadPart;
                    ULONGLONG total = kernelDelta + userDelta;
                    if (total > 0 && idleDelta <= total)
                    {
                        cpuPercent = (static_cast<double>(total - idleDelta) / static_cast<double>(total)) * 100.0;
                    }
                }

                previousIdleTime_ = idle64;
                previousKernelTime_ = kernel64;
                previousUserTime_ = user64;
                hasCpuBaseline_ = true;
            }

            cpuUsagePercent_ = std::clamp(cpuPercent, 0.0, 100.0);
            memoryUsagePercent_ = std::clamp(memoryPercent, 0.0, 100.0);
            graphView_.AddSample(cpuUsagePercent_, memoryUsagePercent_);
        }

        void LayoutControls(int width, int height)
        {
            const int topBarHeight = 42;
            const int buttonWidth = 90;
            const int modulesButtonWidth = 110;
            const int connectionsButtonWidth = 130;
            const int buttonHeight = 26;
            const int padding = 10;
            const int detailsHeight = 34;
            const int graphHeight = graphView_.IsCreated() ? 140 : 0;
            const int graphSpacing = graphHeight > 0 ? 8 : 0;
            const int buttonSpacing = 8;

            if (refreshButton_)
            {
                MoveWindow(refreshButton_, padding, padding, buttonWidth, buttonHeight, TRUE);
            }

            if (modulesButton_)
            {
                int moduleLeft = padding + buttonWidth + buttonSpacing;
                MoveWindow(modulesButton_, moduleLeft, padding, modulesButtonWidth, buttonHeight, TRUE);
            }

            if (connectionsButton_)
            {
                int connectionsLeft = padding + buttonWidth + modulesButtonWidth + (buttonSpacing * 2);
                MoveWindow(connectionsButton_, connectionsLeft, padding, connectionsButtonWidth, buttonHeight, TRUE);
            }

            if (filterEdit_)
            {
                int editLeft = padding + buttonWidth + modulesButtonWidth + connectionsButtonWidth + (buttonSpacing * 3);
                int editWidth = std::max(120, width - editLeft - padding);
                MoveWindow(filterEdit_, editLeft, padding, editWidth, buttonHeight, TRUE);
            }

            int detailsTop = height - detailsHeight;
            if (detailsStatic_)
            {
                MoveWindow(detailsStatic_, padding, detailsTop, width - (padding * 2), detailsHeight, TRUE);
            }

            int graphBottom = detailsTop - graphSpacing;
            if (graphView_.IsCreated())
            {
                int graphTop = std::max(topBarHeight, graphBottom - graphHeight);
                int graphWidth = std::max(0, width - (padding * 2));
                int graphActualHeight = std::max(0, graphBottom - graphTop);
                graphView_.Resize(padding, graphTop, graphWidth, graphActualHeight);
                graphBottom = graphTop - graphSpacing;
            }

            if (listView_)
            {
                int listHeight = std::max(0, graphBottom - topBarHeight);
                MoveWindow(listView_, 0, topBarHeight, width, listHeight, TRUE);
            }
        }

        void OnFilterChanged()
        {
            if (!filterEdit_)
            {
                return;
            }

            int length = GetWindowTextLengthW(filterEdit_);
            if (length > 0)
            {
                std::vector<wchar_t> buffer(length + 1, L'\0');
                GetWindowTextW(filterEdit_, buffer.data(), length + 1);
                filterText_.assign(buffer.data(), buffer.data() + length);
            }
            else
            {
                filterText_.clear();
            }
            ApplyFilterAndSort();
            UpdateDetailsPanel();
        }

        void ApplyFilterAndSort()
        {
            BuildVisibleProcesses();
            SortVisibleProcesses();
            PopulateList();
        }

        void BuildVisibleProcesses()
        {
            visibleProcesses_.clear();
            const auto &processes = snapshot_.Processes();

            std::wstring trimmedFilter = TrimWhitespace(filterText_);
            bool digitsOnly = !trimmedFilter.empty() &&
                              std::all_of(trimmedFilter.begin(), trimmedFilter.end(),
                                          [](wchar_t ch) { return std::iswdigit(ch); });

            std::uint32_t pidFilter = 0;
            if (digitsOnly && !trimmedFilter.empty())
            {
                pidFilter = static_cast<std::uint32_t>(std::wcstoul(trimmedFilter.c_str(), nullptr, 10));
            }

            std::wstring filterLower = (!digitsOnly && !trimmedFilter.empty()) ? ToLower(trimmedFilter) : std::wstring();

            for (const auto &process : processes)
            {
                if (trimmedFilter.empty())
                {
                    visibleProcesses_.push_back(process);
                }
                else if (digitsOnly)
                {
                    if (process.processId == pidFilter)
                    {
                        visibleProcesses_.push_back(process);
                    }
                }
                else
                {
                    std::wstring processNameLower = ToLower(process.imageName);
                    if (processNameLower.find(filterLower) != std::wstring::npos)
                    {
                        visibleProcesses_.push_back(process);
                    }
                }
            }
        }

        static std::wstring ToLower(const std::wstring &value)
        {
            std::wstring lower = value;
            std::transform(lower.begin(), lower.end(), lower.begin(),
                           [](wchar_t ch) { return static_cast<wchar_t>(std::towlower(ch)); });
            return lower;
        }

        static std::wstring TrimWhitespace(const std::wstring &value)
        {
            auto begin = std::find_if_not(value.begin(), value.end(), [](wchar_t ch)
                                          { return std::iswspace(ch); });
            auto end = std::find_if_not(value.rbegin(), value.rend(), [](wchar_t ch)
                                        { return std::iswspace(ch); })
                           .base();
            if (begin >= end)
            {
                return std::wstring();
            }
            return std::wstring(begin, end);
        }

        void SortVisibleProcesses()
        {
            auto comparator = [this](const rvrse::core::ProcessEntry &lhs, const rvrse::core::ProcessEntry &rhs)
            {
                auto compare = [this](const auto &a, const auto &b) -> int
                {
                    switch (sortColumn_)
                    {
                    case 0:
                        return _wcsicmp(a.imageName.c_str(), b.imageName.c_str());
                    case 1:
                        return (a.processId < b.processId) ? -1 : (a.processId > b.processId ? 1 : 0);
                    case 2:
                        return (a.threadCount < b.threadCount) ? -1 : (a.threadCount > b.threadCount ? 1 : 0);
                    case 3:
                        return (a.workingSetBytes < b.workingSetBytes) ? -1 : (a.workingSetBytes > b.workingSetBytes ? 1 : 0);
                    case 4:
                        return (a.privateBytes < b.privateBytes) ? -1 : (a.privateBytes > b.privateBytes ? 1 : 0);
                    default:
                        return 0;
                    }
                };

                int result = compare(lhs, rhs);
                if (result == 0)
                {
                    result = lhs.processId < rhs.processId ? -1 : (lhs.processId > rhs.processId ? 1 : 0);
                }

                return sortAscending_ ? (result < 0) : (result > 0);
            };

            std::sort(visibleProcesses_.begin(), visibleProcesses_.end(), comparator);
        }

        void OnColumnClick(int column)
        {
            if (column == sortColumn_)
            {
                sortAscending_ = !sortAscending_;
            }
            else
            {
                sortColumn_ = column;
                sortAscending_ = true;
            }

            SortVisibleProcesses();
            PopulateList();
        }

        void UpdateDetailsPanel()
        {
            if (!detailsStatic_)
            {
                return;
            }

            int selectedIndex = ListView_GetNextItem(listView_, -1, LVNI_SELECTED);
            if (selectedIndex >= 0 && selectedIndex < static_cast<int>(visibleProcesses_.size()))
            {
                const auto &process = visibleProcesses_[selectedIndex];
                SetWindowTextW(detailsStatic_, FormatProcessDetails(process).c_str());
            }
            else
            {
                SetWindowTextW(detailsStatic_, FormatSummaryDetails().c_str());
            }
        }

        void ShowModulesForSelection()
        {
            if (!listView_)
            {
                return;
            }

            int selectedIndex = ListView_GetNextItem(listView_, -1, LVNI_SELECTED);
            if (selectedIndex < 0 || selectedIndex >= static_cast<int>(visibleProcesses_.size()))
            {
                MessageBoxW(hwnd_,
                            L"Select a process in the list before opening the module viewer.",
                            rvrse::kProductName,
                            MB_OK | MB_ICONINFORMATION);
                return;
            }

            ModuleViewerWindow::Show(hwnd_, instance_, visibleProcesses_[selectedIndex]);
        }

        void ShowConnectionsForSelection()
        {
            if (!listView_)
            {
                return;
            }

            int selectedIndex = ListView_GetNextItem(listView_, -1, LVNI_SELECTED);
            if (selectedIndex < 0 || selectedIndex >= static_cast<int>(visibleProcesses_.size()))
            {
                MessageBoxW(hwnd_,
                            L"Select a process in the list before opening the connection viewer.",
                            rvrse::kProductName,
                            MB_OK | MB_ICONINFORMATION);
                return;
            }

            if (networkSnapshot_.AccessDenied())
            {
                MessageBoxW(hwnd_,
                            L"Viewing per-process network connections requires Administrator privileges. "
                            L"Restart Rvrse Monitor with elevated rights to enable this feature.",
                            rvrse::kProductName,
                            MB_OK | MB_ICONINFORMATION);
                return;
            }

            if (networkSnapshot_.CaptureFailed())
            {
                MessageBoxW(hwnd_,
                            L"Network connection data is currently unavailable. Try refreshing or restart with elevated rights.",
                            rvrse::kProductName,
                            MB_OK | MB_ICONWARNING);
                return;
            }

            ConnectionViewerWindow::Show(hwnd_, instance_, visibleProcesses_[selectedIndex], networkSnapshot_);
        }

        void ShowContextMenu()
        {
            if (!listView_)
            {
                return;
            }

            int selectedIndex = ListView_GetNextItem(listView_, -1, LVNI_SELECTED);
            if (selectedIndex < 0)
            {
                return; // No selection, no context menu
            }

            POINT cursorPos{};
            GetCursorPos(&cursorPos);

            HMENU menu = CreatePopupMenu();
            if (!menu)
            {
                return;
            }

            AppendMenuW(menu, MF_STRING, kMenuTerminateId, L"&Terminate Process");
            AppendMenuW(menu, MF_STRING, kMenuTerminateTreeId, L"Terminate Process &Tree");

            TrackPopupMenu(menu, TPM_RIGHTBUTTON, cursorPos.x, cursorPos.y, 0, hwnd_, nullptr);
            DestroyMenu(menu);
        }

        void TerminateSelectedProcess(bool includeTree)
        {
            if (!listView_)
            {
                return;
            }

            int selectedIndex = ListView_GetNextItem(listView_, -1, LVNI_SELECTED);
            if (selectedIndex < 0 || selectedIndex >= static_cast<int>(visibleProcesses_.size()))
            {
                return;
            }

            const auto &process = visibleProcesses_[selectedIndex];

            // Confirmation dialog
            wchar_t message[512];
            if (includeTree)
            {
                StringCchPrintfW(message, std::size(message),
                                 L"Terminate process '%s' (PID %u) and all its child processes?\n\n"
                                 L"This action cannot be undone.",
                                 process.imageName.empty() ? L"[Unnamed]" : process.imageName.c_str(),
                                 process.processId);
            }
            else
            {
                StringCchPrintfW(message, std::size(message),
                                 L"Terminate process '%s' (PID %u)?\n\n"
                                 L"This action cannot be undone.",
                                 process.imageName.empty() ? L"[Unnamed]" : process.imageName.c_str(),
                                 process.processId);
            }

            int result = MessageBoxW(hwnd_, message, rvrse::kProductName, MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2);
            if (result != IDYES)
            {
                return;
            }

            // Collect PIDs to terminate
            std::vector<std::uint32_t> pidsToTerminate;
            pidsToTerminate.push_back(process.processId);

            if (includeTree)
            {
                CollectChildProcesses(process.processId, pidsToTerminate);
            }

            // Terminate processes
            int successCount = 0;
            int failureCount = 0;
            std::wstring errorDetails;

            for (auto pid : pidsToTerminate)
            {
                HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
                if (hProcess)
                {
                    if (TerminateProcess(hProcess, 1))
                    {
                        ++successCount;
                    }
                    else
                    {
                        ++failureCount;
                        DWORD error = GetLastError();
                        wchar_t errBuf[128];
                        StringCchPrintfW(errBuf, std::size(errBuf), L"PID %u (Error %u)\n", pid, error);
                        errorDetails += errBuf;
                    }
                    CloseHandle(hProcess);
                }
                else
                {
                    ++failureCount;
                    DWORD error = GetLastError();
                    wchar_t errBuf[128];
                    if (error == ERROR_ACCESS_DENIED)
                    {
                        StringCchPrintfW(errBuf, std::size(errBuf), L"PID %u (Access Denied - Protected Process)\n", pid);
                    }
                    else
                    {
                        StringCchPrintfW(errBuf, std::size(errBuf), L"PID %u (Error %u)\n", pid, error);
                    }
                    errorDetails += errBuf;
                }
            }

            // Show result
            wchar_t resultMsg[1024];
            if (failureCount == 0)
            {
                StringCchPrintfW(resultMsg, std::size(resultMsg),
                                 L"Successfully terminated %d process(es).",
                                 successCount);
                MessageBoxW(hwnd_, resultMsg, rvrse::kProductName, MB_OK | MB_ICONINFORMATION);
            }
            else
            {
                StringCchPrintfW(resultMsg, std::size(resultMsg),
                                 L"Terminated %d process(es).\n"
                                 L"Failed to terminate %d process(es):\n\n%s",
                                 successCount, failureCount, errorDetails.c_str());
                MessageBoxW(hwnd_, resultMsg, rvrse::kProductName, MB_OK | MB_ICONWARNING);
            }

            // Refresh the process list
            RefreshProcesses();
        }

        void CollectChildProcesses(std::uint32_t parentPid, std::vector<std::uint32_t> &pids)
        {
            // Recursively collect all child processes
            for (const auto &proc : snapshot_.Processes())
            {
                if (proc.parentProcessId == parentPid)
                {
                    // Add this child
                    pids.push_back(proc.processId);
                    // Recursively collect its children
                    CollectChildProcesses(proc.processId, pids);
                }
            }
        }

        std::wstring FormatProcessDetails(const rvrse::core::ProcessEntry &process) const
        {
            bool connectionUnavailable = networkSnapshot_.AccessDenied() || networkSnapshot_.CaptureFailed();
            std::wstring workingSet = rvrse::common::FormatSize(process.workingSetBytes);
            std::wstring privateBytes = rvrse::common::FormatSize(process.privateBytes);
            auto handleCount = handleSnapshot_.HandleCountForProcess(process.processId);
            std::wstring connectionText = connectionUnavailable
                                              ? std::wstring(L"N/A")
                                              : std::to_wstring(networkSnapshot_.ConnectionCountForProcess(process.processId));

            wchar_t buffer[512];
            StringCchPrintfW(buffer, std::size(buffer),
                             L"%s (PID %u) | Threads: %u | Handles: %zu | Connections: %s | WS: %s | Private: %s",
                             process.imageName.empty() ? L"[Unnamed]" : process.imageName.c_str(),
                             process.processId,
                             process.threadCount,
                             handleCount,
                             connectionText.c_str(),
                             workingSet.c_str(),
                             privateBytes.c_str());
            return buffer;
        }

        std::wstring FormatSummaryDetails() const
        {
            const auto totalProcesses = snapshot_.Processes().size();
            std::uint64_t totalThreads = 0;
            std::uint64_t totalWorkingSet = 0;

            for (const auto &process : snapshot_.Processes())
            {
                totalThreads += process.threadCount;
                totalWorkingSet += process.workingSetBytes;
            }

            std::wstring workingSet = rvrse::common::FormatSize(totalWorkingSet);
            std::wstring connectionSummary;
            if (networkSnapshot_.AccessDenied())
            {
                connectionSummary = L"N/A (requires elevation)";
            }
            else if (networkSnapshot_.CaptureFailed())
            {
                connectionSummary = L"N/A (unavailable)";
            }
            else
            {
                connectionSummary = std::to_wstring(networkSnapshot_.Connections().size());
            }

            wchar_t buffer[256];
            StringCchPrintfW(buffer, std::size(buffer),
                             L"Processes: %zu | Threads: %llu | Working Set Total: %s | Connections: %s | CPU: %.1f%% | Memory: %.1f%%",
                             totalProcesses,
                             static_cast<unsigned long long>(totalThreads),
                             workingSet.c_str(),
                             connectionSummary.c_str(),
                             cpuUsagePercent_,
                             memoryUsagePercent_);
            return buffer;
        }

        HINSTANCE instance_;
        HWND hwnd_ = nullptr;
        HWND listView_ = nullptr;
        HWND refreshButton_ = nullptr;
        HWND modulesButton_ = nullptr;
        HWND connectionsButton_ = nullptr;
        HWND filterEdit_ = nullptr;
        HWND detailsStatic_ = nullptr;
        bool columnsCreated_ = false;
        rvrse::core::ProcessSnapshot snapshot_;
        rvrse::core::HandleSnapshot handleSnapshot_;
        rvrse::core::NetworkSnapshot networkSnapshot_;
        std::vector<rvrse::core::ProcessEntry> visibleProcesses_;
        std::wstring filterText_;
        int sortColumn_ = 0;
        bool sortAscending_ = true;
        std::unique_ptr<rvrse::core::PluginLoader> pluginLoader_;
        ResourceGraphView graphView_;
        double cpuUsagePercent_ = 0.0;
        double memoryUsagePercent_ = 0.0;
        ULARGE_INTEGER previousIdleTime_{};
        ULARGE_INTEGER previousKernelTime_{};
        ULARGE_INTEGER previousUserTime_{};
        bool hasCpuBaseline_ = false;
    };
}

_Use_decl_annotations_
int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int)
{
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        MessageBoxW(nullptr, L"Failed to initialize Winsock.", rvrse::kProductName, MB_OK | MB_ICONERROR);
        return -1;
    }

    MainWindow window(instance);
    if (!window.Create())
    {
        MessageBoxW(nullptr, L"Failed to create the main window.", rvrse::kProductName, MB_OK | MB_ICONERROR);
        WSACleanup();
        return -1;
    }

    int result = window.Run();
    WSACleanup();
    return result;
}
