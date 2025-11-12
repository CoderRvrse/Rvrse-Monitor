#define NOMINMAX
#include <windows.h>
#include <commctrl.h>
#include <strsafe.h>

#include <algorithm>
#include <array>
#include <cwchar>
#include <cwctype>
#include <numeric>
#include <sstream>
#include <string>
#include <vector>

#include "rvrse_monitor.h"
#include "rvrse/common/formatting.h"
#include "process_snapshot.h"
#include "handle_snapshot.h"

#pragma comment(lib, "Comctl32.lib")

namespace
{
    constexpr wchar_t kWindowClassName[] = L"RvrseMonitorMainWindow";
    constexpr UINT_PTR kRefreshTimerId = 0x2001;
    constexpr UINT kRefreshIntervalMs = 4000;
    constexpr int kListViewId = 0x3001;
    constexpr int kRefreshButtonId = 0x3002;
    constexpr int kFilterEditId = 0x3003;
    constexpr int kDetailsStaticId = 0x3004;

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

            HFONT defaultFont = static_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
            SendMessageW(refreshButton_, WM_SETFONT, reinterpret_cast<WPARAM>(defaultFont), TRUE);
            SendMessageW(filterEdit_, WM_SETFONT, reinterpret_cast<WPARAM>(defaultFont), TRUE);
            SendMessageW(listView_, WM_SETFONT, reinterpret_cast<WPARAM>(defaultFont), TRUE);
            SendMessageW(detailsStatic_, WM_SETFONT, reinterpret_cast<WPARAM>(defaultFont), TRUE);

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

        void LayoutControls(int width, int height)
        {
            const int topBarHeight = 42;
            const int buttonWidth = 90;
            const int buttonHeight = 26;
            const int padding = 10;
            const int detailsHeight = 28;

            if (refreshButton_)
            {
                MoveWindow(refreshButton_, padding, padding, buttonWidth, buttonHeight, TRUE);
            }

            if (filterEdit_)
            {
                int editLeft = padding + buttonWidth + 8;
                int editWidth = std::max(120, width - editLeft - padding);
                MoveWindow(filterEdit_, editLeft, padding, editWidth, buttonHeight, TRUE);
            }

            if (listView_)
            {
                int listTop = topBarHeight;
                int listHeight = std::max(0, height - topBarHeight - detailsHeight);
                MoveWindow(listView_, 0, listTop, width, listHeight, TRUE);
            }

            if (detailsStatic_)
            {
                MoveWindow(detailsStatic_, padding, height - detailsHeight, width - (padding * 2), detailsHeight, TRUE);
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

        std::wstring FormatProcessDetails(const rvrse::core::ProcessEntry &process) const
        {
            std::wstring workingSet = rvrse::common::FormatSize(process.workingSetBytes);
            std::wstring privateBytes = rvrse::common::FormatSize(process.privateBytes);
            auto handleCount = handleSnapshot_.HandleCountForProcess(process.processId);

            wchar_t buffer[512];
            StringCchPrintfW(buffer, std::size(buffer),
                             L"%s (PID %u) | Threads: %u | Handles: %zu | WS: %s | Private: %s",
                             process.imageName.empty() ? L"[Unnamed]" : process.imageName.c_str(),
                             process.processId,
                             process.threadCount,
                             handleCount,
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
            wchar_t buffer[256];
            StringCchPrintfW(buffer, std::size(buffer),
                             L"Processes: %zu | Threads: %llu | Working Set Total: %s",
                             totalProcesses,
                             static_cast<unsigned long long>(totalThreads),
                             workingSet.c_str());
            return buffer;
        }

        HINSTANCE instance_;
        HWND hwnd_ = nullptr;
        HWND listView_ = nullptr;
        HWND refreshButton_ = nullptr;
        HWND filterEdit_ = nullptr;
        HWND detailsStatic_ = nullptr;
        bool columnsCreated_ = false;
        rvrse::core::ProcessSnapshot snapshot_;
        rvrse::core::HandleSnapshot handleSnapshot_;
        std::vector<rvrse::core::ProcessEntry> visibleProcesses_;
        std::wstring filterText_;
        int sortColumn_ = 0;
        bool sortAscending_ = true;
    };
}

_Use_decl_annotations_
int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int)
{
    MainWindow window(instance);
    if (!window.Create())
    {
        MessageBoxW(nullptr, L"Failed to create the main window.", rvrse::kProductName, MB_OK | MB_ICONERROR);
        return -1;
    }

    return window.Run();
}
