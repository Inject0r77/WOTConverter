#include "ui/MainWindow.h"

#ifdef _WIN32

#include "ui/Theme.h"
#include "resource.h"

#include <commctrl.h>
#include <dwmapi.h>
#include <shellapi.h>
#include <uxtheme.h>
#include <windowsx.h>

#include <algorithm>
#include <cmath>
#include <cwchar>
#include <cwctype>
#include <iomanip>
#include <sstream>
#include <utility>

namespace wot::ui {
namespace {

constexpr wchar_t kWindowClass[] = L"Inject0r.WoTConverter.Window";
constexpr wchar_t kWindowTitle[] = L"WoT Converter";
constexpr wchar_t kVersion[] = L"v1.1.2";

constexpr int kIdMoney = 1001;
constexpr int kIdGold = 1002;
constexpr int kIdCredits = 1003;
constexpr int kIdXp = 1004;
constexpr int kIdBoxes = 1005;

constexpr int kActionConverter = 110;
constexpr int kActionContainers = 111;
constexpr int kActionRates = 112;
constexpr int kActionSettings = 113;
constexpr int kActionAbout = 114;
constexpr int kActionLesta = 210;
constexpr int kActionWg = 211;
constexpr int kActionRu = 212;
constexpr int kActionEn = 213;
constexpr int kActionPromo = 214;
constexpr int kActionDiscount = 215;
constexpr int kActionOpenConfig = 216;

constexpr int kSidebarWidth = 194;

bool contains(const RECT& r, int x, int y) {
    const POINT p{x, y};
    return PtInRect(&r, p) != FALSE;
}

std::wstring trimZeros(std::wstring value) {
    const auto dot = value.find(L'.');
    if (dot == std::wstring::npos) return value;
    while (!value.empty() && value.back() == L'0') value.pop_back();
    if (!value.empty() && value.back() == L'.') value.pop_back();
    return value;
}

std::wstring upperAscii(std::wstring text) {
    for (auto& ch : text) {
        if (ch >= L'a' && ch <= L'z') ch = static_cast<wchar_t>(ch - L'a' + L'A');
    }
    return text;
}

} // namespace

MainWindow::MainWindow(HINSTANCE instance,
                       RatesConfig rates,
                       storage::SettingsStore settingsStore,
                       std::filesystem::path ratesPath,
                       std::string ratesWarning)
    : instance_(instance),
      rates_(std::move(rates)),
      converter_(rates_),
      boxOptimizer_(rates_),
      settingsStore_(std::move(settingsStore)),
      ratesPath_(std::move(ratesPath)),
      ratesWarning_(std::move(ratesWarning)) {
    settings_ = settingsStore_.load();
}

MainWindow::~MainWindow() {
    if (editFont_) DeleteObject(editFont_);
    if (editBrush_) DeleteObject(editBrush_);
}

bool MainWindow::create() {
    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
    wc.lpfnWndProc = &MainWindow::WindowProc;
    wc.hInstance = instance_;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hIcon = LoadIconW(instance_, MAKEINTRESOURCEW(IDI_APP_ICON));
    wc.hIconSm = static_cast<HICON>(LoadImageW(instance_, MAKEINTRESOURCEW(IDI_APP_ICON),
                                                IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR));
    if (!wc.hIcon) wc.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
    if (!wc.hIconSm) wc.hIconSm = wc.hIcon;
    wc.lpszClassName = kWindowClass;
    wc.hbrBackground = nullptr;

    if (!RegisterClassExW(&wc) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) return false;

    const int width = scale(1420);
    const int height = scale(820);
    hwnd_ = CreateWindowExW(
        0,
        kWindowClass,
        kWindowTitle,
        WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        width,
        height,
        nullptr,
        nullptr,
        instance_,
        this);

    if (!hwnd_) return false;

    ShowWindow(hwnd_, SW_SHOW);
    UpdateWindow(hwnd_);
    return true;
}

int MainWindow::run() {
    MSG msg{};
    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return static_cast<int>(msg.wParam);
}

LRESULT CALLBACK MainWindow::WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    MainWindow* self = nullptr;
    if (message == WM_NCCREATE) {
        auto* create = reinterpret_cast<CREATESTRUCTW*>(lParam);
        self = static_cast<MainWindow*>(create->lpCreateParams);
        self->hwnd_ = hwnd;
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
    } else {
        self = reinterpret_cast<MainWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    }

    if (self) return self->handleMessage(message, wParam, lParam);
    return DefWindowProcW(hwnd, message, wParam, lParam);
}

LRESULT MainWindow::handleMessage(UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_CREATE:
            return onCreate() ? 0 : -1;
        case WM_DESTROY:
            onDestroy();
            return 0;
        case WM_PAINT:
            onPaint();
            return 0;
        case WM_ERASEBKGND:
            return 1;
        case WM_SIZE:
            onSize(LOWORD(lParam), HIWORD(lParam));
            return 0;
        case WM_COMMAND:
            onCommand(wParam, lParam);
            return 0;
        case WM_LBUTTONUP:
            onClick(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            return 0;
        case WM_MOUSEMOVE:
            onMouseMove(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            return 0;
        case WM_MOUSELEAVE:
            trackingMouse_ = false;
            if (hoverAction_ != 0) {
                hoverAction_ = 0;
                InvalidateRect(hwnd_, nullptr, FALSE);
            }
            return 0;
        case WM_DPICHANGED:
            onDpiChanged(HIWORD(wParam), reinterpret_cast<const RECT*>(lParam));
            return 0;
        case WM_GETMINMAXINFO: {
            auto* mmi = reinterpret_cast<MINMAXINFO*>(lParam);
            mmi->ptMinTrackSize.x = scale(1180);
            mmi->ptMinTrackSize.y = scale(690);
            return 0;
        }
        case WM_CTLCOLOREDIT: {
            const HDC dc = reinterpret_cast<HDC>(wParam);
            SetTextColor(dc, Theme::Text);
            SetBkColor(dc, Theme::Input);
            return reinterpret_cast<LRESULT>(editBrush_);
        }
        default:
            return DefWindowProcW(hwnd_, message, wParam, lParam);
    }
}

bool MainWindow::onCreate() {
    dpi_ = GetDpiForWindow(hwnd_);
    if (dpi_ == 0) dpi_ = 96;

    BOOL dark = TRUE;
    DwmSetWindowAttribute(hwnd_, 20, &dark, sizeof(dark));

    editBrush_ = CreateSolidBrush(Theme::Input);
    createEditControls();
    applySettings();
    showControlsForPage();
    return true;
}

void MainWindow::onDestroy() {
    saveSettings();
    PostQuitMessage(0);
}

void MainWindow::onPaint() {
    PAINTSTRUCT ps{};
    HDC dc = BeginPaint(hwnd_, &ps);

    RECT client{};
    GetClientRect(hwnd_, &client);
    const int width = client.right - client.left;
    const int height = client.bottom - client.top;

    HDC mem = CreateCompatibleDC(dc);
    HBITMAP bitmap = CreateCompatibleBitmap(dc, width, height);
    HGDIOBJ oldBitmap = SelectObject(mem, bitmap);

    Gdiplus::Graphics g(mem);
    g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
    g.SetTextRenderingHint(Gdiplus::TextRenderingHintClearTypeGridFit);
    g.Clear(Theme::gdip(Theme::Background));

    hitRects_.clear();
    drawSidebar(g, client);
    drawTopBar(g, client);

    switch (page_) {
        case Page::Converter: drawConverter(g); break;
        case Page::Containers: drawContainers(g); break;
        case Page::Rates: drawRates(g); break;
        case Page::Settings: drawSettings(g); break;
        case Page::About: drawAbout(g); break;
    }

    BitBlt(dc, 0, 0, width, height, mem, 0, 0, SRCCOPY);
    SelectObject(mem, oldBitmap);
    DeleteObject(bitmap);
    DeleteDC(mem);
    EndPaint(hwnd_, &ps);
}

void MainWindow::onSize(int width, int height) {
    clientWidth_ = width;
    clientHeight_ = height;
    layoutEditControls();
    InvalidateRect(hwnd_, nullptr, FALSE);
}

void MainWindow::onCommand(WPARAM wParam, LPARAM) {
    const int id = LOWORD(wParam);
    const int notify = HIWORD(wParam);
    if (updatingFields_) return;

    if (notify == EN_CHANGE) {
        if (id >= kIdMoney && id <= kIdXp) {
            const auto source = sourceFromControlId(id);
            lastSource_ = source;
            recalcFrom(source);
        } else if (id == kIdBoxes) {
            recalcBoxes();
        }
        return;
    }

    if (notify == EN_KILLFOCUS) {
        if (id >= kIdMoney && id <= kIdXp) {
            normalizeField(sourceFromControlId(id));
        } else if (id == kIdBoxes) {
            normalizeBoxField();
        }
    }
}

void MainWindow::onClick(int x, int y) {
    for (auto it = hitRects_.rbegin(); it != hitRects_.rend(); ++it) {
        if (!contains(it->rect, x, y)) continue;
        switch (it->action) {
            case kActionConverter: setPage(Page::Converter); break;
            case kActionContainers: setPage(Page::Containers); break;
            case kActionRates: setPage(Page::Rates); break;
            case kActionSettings: setPage(Page::Settings); break;
            case kActionAbout: setPage(Page::About); break;
            case kActionLesta: setRegion(Region::Lesta); break;
            case kActionWg: setRegion(Region::WG); break;
            case kActionRu: setLanguage(false); break;
            case kActionEn: setLanguage(true); break;
            case kActionPromo: togglePromo(); break;
            case kActionDiscount: toggleDiscount(); break;
            case kActionOpenConfig: openConfigFolder(); break;
            default: break;
        }
        return;
    }
}

void MainWindow::onMouseMove(int x, int y) {
    if (!trackingMouse_) {
        TRACKMOUSEEVENT tme{};
        tme.cbSize = sizeof(tme);
        tme.dwFlags = TME_LEAVE;
        tme.hwndTrack = hwnd_;
        TrackMouseEvent(&tme);
        trackingMouse_ = true;
    }

    int next = 0;
    for (auto it = hitRects_.rbegin(); it != hitRects_.rend(); ++it) {
        if (contains(it->rect, x, y)) {
            next = it->action;
            break;
        }
    }
    if (next != hoverAction_) {
        hoverAction_ = next;
        InvalidateRect(hwnd_, nullptr, FALSE);
    }
}

void MainWindow::onDpiChanged(UINT newDpi, const RECT* suggested) {
    dpi_ = newDpi ? newDpi : 96;
    if (suggested) {
        SetWindowPos(hwnd_, nullptr,
                     suggested->left, suggested->top,
                     suggested->right - suggested->left,
                     suggested->bottom - suggested->top,
                     SWP_NOZORDER | SWP_NOACTIVATE);
    }

    if (editFont_) {
        DeleteObject(editFont_);
        editFont_ = nullptr;
    }
    editFont_ = CreateFontW(-scale(18), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    for (HWND field : fields_) {
        SendMessageW(field, WM_SETFONT, reinterpret_cast<WPARAM>(editFont_), TRUE);
    }
    SendMessageW(boxEdit_, WM_SETFONT, reinterpret_cast<WPARAM>(editFont_), TRUE);
    layoutEditControls();
    InvalidateRect(hwnd_, nullptr, FALSE);
}

LRESULT CALLBACK MainWindow::EditSubclassProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam,
                                              UINT_PTR, DWORD_PTR refData) {
    auto* self = reinterpret_cast<MainWindow*>(refData);

    if (message == WM_KEYDOWN) {
        const bool ctrlDown = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
        if (ctrlDown && (wParam == 'A' || wParam == 'a')) {
            SendMessageW(hwnd, EM_SETSEL, 0, -1);
            return 0;
        }

        if (wParam == VK_RETURN) {
            if (self) {
                const int id = GetDlgCtrlID(hwnd);
                if (id >= kIdMoney && id <= kIdXp) {
                    const auto source = self->sourceFromControlId(id);
                    self->lastSource_ = source;
                    self->recalcFrom(source);
                    self->normalizeField(source);
                } else if (id == kIdBoxes) {
                    self->recalcBoxes();
                    self->normalizeBoxField();
                }
            }
            return 0;
        }
    }

    if (message == WM_CHAR) {
        if (wParam == 0x01 || wParam == L'\r' || wParam == L'\n') return 0;
    }

    return DefSubclassProc(hwnd, message, wParam, lParam);
}

void MainWindow::createEditControls() {
    editFont_ = CreateFontW(-scale(18), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");

    const int ids[] = {kIdMoney, kIdGold, kIdCredits, kIdXp};
    for (std::size_t i = 0; i < fields_.size(); ++i) {
        fields_[i] = CreateWindowExW(
            0, L"EDIT", L"",
            WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_LEFT,
            0, 0, 100, 28,
            hwnd_, reinterpret_cast<HMENU>(static_cast<INT_PTR>(ids[i])), instance_, nullptr);
        styleNativeControl(fields_[i]);
        SetWindowSubclass(fields_[i], &MainWindow::EditSubclassProc, 1,
                          reinterpret_cast<DWORD_PTR>(this));
    }

    boxEdit_ = CreateWindowExW(
        0, L"EDIT", L"",
        WS_CHILD | ES_AUTOHSCROLL | ES_LEFT,
        0, 0, 100, 28,
        hwnd_, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kIdBoxes)), instance_, nullptr);
    styleNativeControl(boxEdit_);
    SetWindowSubclass(boxEdit_, &MainWindow::EditSubclassProc, 1,
                      reinterpret_cast<DWORD_PTR>(this));

    SendMessageW(fields_[0], EM_SETCUEBANNER, TRUE, reinterpret_cast<LPARAM>(L"1000"));
    SendMessageW(fields_[1], EM_SETCUEBANNER, TRUE, reinterpret_cast<LPARAM>(L"2 500"));
    SendMessageW(fields_[2], EM_SETCUEBANNER, TRUE, reinterpret_cast<LPARAM>(L"1 000 000"));
    SendMessageW(fields_[3], EM_SETCUEBANNER, TRUE, reinterpret_cast<LPARAM>(L"100 000"));
    SendMessageW(boxEdit_, EM_SETCUEBANNER, TRUE, reinterpret_cast<LPARAM>(L"30"));

    layoutEditControls();
}

void MainWindow::styleNativeControl(HWND control) {
    SendMessageW(control, WM_SETFONT, reinterpret_cast<WPARAM>(editFont_), TRUE);
    SendMessageW(control, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN,
                 MAKELPARAM(scale(2), scale(2)));
    SetWindowTheme(control, L"DarkMode_Explorer", nullptr);
}

void MainWindow::layoutEditControls() {
    if (!hwnd_) return;

    if (page_ == Page::Converter) {
        for (int row = 0; row < 2; ++row) {
            for (int col = 0; col < 2; ++col) {
                const int index = row * 2 + col;
                const RECT input = converterInputRect(col, row);
                const int x = input.left + scale(12);
                const int y = input.top + scale(31);
                const int w = std::max(scale(80), static_cast<int>(input.right - input.left) - scale(24));
                MoveWindow(fields_[index], x, y, w, scale(28), TRUE);
            }
        }
    }

    if (page_ == Page::Containers) {
        const RECT input = containerInputRect();
        const int x = input.left + scale(12);
        const int y = input.top + scale(31);
        const int w = std::max(scale(100), static_cast<int>(input.right - input.left) - scale(24));
        MoveWindow(boxEdit_, x, y, w, scale(28), TRUE);
    }
}

void MainWindow::showControlsForPage() {
    const bool converter = page_ == Page::Converter;
    for (HWND field : fields_) ShowWindow(field, converter ? SW_SHOW : SW_HIDE);
    ShowWindow(boxEdit_, page_ == Page::Containers ? SW_SHOW : SW_HIDE);
    layoutEditControls();
}

void MainWindow::recalcFrom(SourceKind source) {
    bool ok = false;
    const double value = parseField(source, &ok);
    if (!ok) {
        hasResult_ = false;
        InvalidateRect(hwnd_, nullptr, FALSE);
        return;
    }

    try {
        ConversionInput input;
        input.region = settings_.region;
        input.source = source;
        input.value = value;
        input.modifiers.promoFreeXp = settings_.promoFreeXp;
        input.modifiers.premiumShopDiscount = settings_.premiumDiscount;
        lastResult_ = converter_.convert(input);
        hasResult_ = true;
        updateAllFields(lastResult_, source);
    } catch (...) {
        hasResult_ = false;
    }
    InvalidateRect(hwnd_, nullptr, FALSE);
}

void MainWindow::recalcBoxes() {
    bool ok = false;
    const int count = parseBoxCount(&ok);
    if (!ok) {
        boxResult_.clear();
        InvalidateRect(hwnd_, nullptr, FALSE);
        return;
    }

    try {
        const auto result = boxOptimizer_.calculate(settings_.region, count);
        std::wostringstream out;
        out << tr(L"Итого: ", L"Total: ") << formatMoney(result.totalPrice) << L' '
            << (settings_.region == Region::Lesta ? L"RUB" : L"EUR");
        if (settings_.region == Region::WG) {
            out << L"\n" << tr(L"Оптимальная покупка: ", L"Optimal purchase: ")
                << boxCombinationText(result);
            if (result.purchased > result.requested) {
                out << L"\n" << tr(L"Будет куплено: ", L"Boxes purchased: ") << result.purchased;
            }
        }
        boxResult_ = out.str();
    } catch (...) {
        boxResult_ = tr(L"Ошибка расчёта.", L"Calculation error.");
    }
    InvalidateRect(hwnd_, nullptr, FALSE);
}

void MainWindow::updateAllFields(const ConversionResult& result, SourceKind source) {
    updatingFields_ = true;
    if (source != SourceKind::Money) setFieldText(SourceKind::Money, formatMoney(result.money));
    if (source != SourceKind::Gold) setFieldText(SourceKind::Gold, formatNumber(result.gold, 2));
    if (source != SourceKind::Credits) setFieldText(SourceKind::Credits, formatNumber(result.credits, 0));
    if (source != SourceKind::FreeXp) setFieldText(SourceKind::FreeXp, formatNumber(result.freeXp, 0));
    updatingFields_ = false;
}

void MainWindow::setFieldText(SourceKind kind, const std::wstring& text) {
    SetWindowTextW(fieldHandle(kind), text.c_str());
}

std::wstring MainWindow::fieldText(SourceKind kind) const {
    const HWND field = fieldHandle(kind);
    const int length = GetWindowTextLengthW(field);
    std::wstring text(static_cast<std::size_t>(length) + 1, L'\0');
    if (length > 0) GetWindowTextW(field, text.data(), length + 1);
    text.resize(static_cast<std::size_t>(length));
    return text;
}

double MainWindow::parseField(SourceKind kind, bool* ok) const {
    auto text = fieldText(kind);
    text.erase(std::remove_if(text.begin(), text.end(), [](wchar_t ch) {
        return std::iswspace(ch) != 0 || ch == L'\'';
    }), text.end());

    // Accept both normal decimals and common grouping forms such as 1.000.000.
    const auto commaCount = static_cast<int>(std::count(text.begin(), text.end(), L','));
    const auto dotCount = static_cast<int>(std::count(text.begin(), text.end(), L'.'));
    if (dotCount > 1 && commaCount == 0) {
        text.erase(std::remove(text.begin(), text.end(), L'.'), text.end());
    } else if (commaCount == 1) {
        text.erase(std::remove(text.begin(), text.end(), L'.'), text.end());
        std::replace(text.begin(), text.end(), L',', L'.');
    } else if (commaCount > 1) {
        text.erase(std::remove(text.begin(), text.end(), L','), text.end());
    }

    if (text.empty()) {
        if (ok) *ok = false;
        return 0.0;
    }

    wchar_t* end = nullptr;
    const double value = std::wcstod(text.c_str(), &end);
    const bool valid = end && *end == L'\0' && std::isfinite(value) && value >= 0.0;
    if (ok) *ok = valid;
    return valid ? value : 0.0;
}

int MainWindow::parseBoxCount(bool* ok) const {
    const int length = GetWindowTextLengthW(boxEdit_);
    std::wstring text(static_cast<std::size_t>(length) + 1, L'\0');
    if (length > 0) GetWindowTextW(boxEdit_, text.data(), length + 1);
    text.resize(static_cast<std::size_t>(length));
    text.erase(std::remove_if(text.begin(), text.end(), [](wchar_t ch) {
        return std::iswspace(ch) != 0 || ch == L'.' || ch == L',' || ch == L'\'';
    }), text.end());

    if (text.empty()) {
        if (ok) *ok = false;
        return 0;
    }

    wchar_t* end = nullptr;
    const long value = std::wcstol(text.c_str(), &end, 10);
    const bool valid = end && *end == L'\0' && value >= 0 && value <= 1'000'000;
    if (ok) *ok = valid;
    return valid ? static_cast<int>(value) : 0;
}

void MainWindow::normalizeField(SourceKind kind) {
    bool ok = false;
    const double value = parseField(kind, &ok);
    if (!ok) return;

    std::wstring formatted;
    switch (kind) {
        case SourceKind::Money: formatted = formatMoney(value); break;
        case SourceKind::Gold: formatted = formatNumber(value, 2); break;
        case SourceKind::Credits: formatted = formatNumber(value, 0); break;
        case SourceKind::FreeXp: formatted = formatNumber(value, 0); break;
    }

    updatingFields_ = true;
    setFieldText(kind, formatted);
    SendMessageW(fieldHandle(kind), EM_SETSEL,
                 static_cast<WPARAM>(formatted.size()), static_cast<LPARAM>(formatted.size()));
    updatingFields_ = false;
}

void MainWindow::normalizeBoxField() {
    bool ok = false;
    const int value = parseBoxCount(&ok);
    if (!ok) return;
    const auto formatted = formatNumber(static_cast<double>(value), 0);
    updatingFields_ = true;
    SetWindowTextW(boxEdit_, formatted.c_str());
    SendMessageW(boxEdit_, EM_SETSEL,
                 static_cast<WPARAM>(formatted.size()), static_cast<LPARAM>(formatted.size()));
    updatingFields_ = false;
}

void MainWindow::saveSettings() {
    settingsStore_.save(settings_);
}

void MainWindow::applySettings() {
    if (GetWindowTextLengthW(fieldHandle(lastSource_)) > 0) recalcFrom(lastSource_);
    InvalidateRect(hwnd_, nullptr, FALSE);
}

void MainWindow::setPage(Page page) {
    if (page_ == page) return;
    page_ = page;
    hoverAction_ = 0;
    showControlsForPage();
    InvalidateRect(hwnd_, nullptr, FALSE);
}

void MainWindow::setRegion(Region region) {
    if (settings_.region == region) return;
    settings_.region = region;
    saveSettings();
    if (GetWindowTextLengthW(fieldHandle(lastSource_)) > 0) recalcFrom(lastSource_);
    if (GetWindowTextLengthW(boxEdit_) > 0) recalcBoxes();
    InvalidateRect(hwnd_, nullptr, FALSE);
}

void MainWindow::setLanguage(bool english) {
    if (settings_.english == english) return;
    settings_.english = english;
    saveSettings();
    if (GetWindowTextLengthW(boxEdit_) > 0) recalcBoxes();
    InvalidateRect(hwnd_, nullptr, FALSE);
}

void MainWindow::togglePromo() {
    settings_.promoFreeXp = !settings_.promoFreeXp;
    saveSettings();
    if (GetWindowTextLengthW(fieldHandle(lastSource_)) > 0) recalcFrom(lastSource_);
    InvalidateRect(hwnd_, nullptr, FALSE);
}

void MainWindow::toggleDiscount() {
    settings_.premiumDiscount = !settings_.premiumDiscount;
    saveSettings();
    if (GetWindowTextLengthW(fieldHandle(lastSource_)) > 0) recalcFrom(lastSource_);
    InvalidateRect(hwnd_, nullptr, FALSE);
}

void MainWindow::openConfigFolder() {
    std::error_code ec;
    std::filesystem::create_directories(ratesPath_.parent_path(), ec);
    ShellExecuteW(hwnd_, L"open", ratesPath_.parent_path().c_str(), nullptr, nullptr, SW_SHOWNORMAL);
}

int MainWindow::scale(int px) const {
    return MulDiv(px, static_cast<int>(dpi_), 96);
}

RECT MainWindow::contentRect() const {
    const int left = scale(kSidebarWidth + 18);
    const int top = scale(22);
    return RECT{
        left,
        top,
        std::max(left + scale(760), clientWidth_ - scale(16)),
        std::max(top + scale(610), clientHeight_ - scale(18))
    };
}

RECT MainWindow::converterPanelRect() const {
    const RECT c = contentRect();
    return RECT{c.left, c.top + scale(76), c.right, c.top + scale(330)};
}

RECT MainWindow::converterInputRect(int column, int row) const {
    const RECT panel = converterPanelRect();
    const int gap = scale(12);
    const int pad = scale(16);
    const int top = panel.top + scale(36);
    const int innerWidth = (panel.right - panel.left) - pad * 2;
    const int width = (innerWidth - gap) / 2;
    const int height = scale(86);
    const int x = panel.left + pad + column * (width + gap);
    const int y = top + row * (height + gap);
    return RECT{x, y, x + width, y + height};
}

RECT MainWindow::containerInputRect() const {
    const RECT c = contentRect();
    const RECT panel{c.left, c.top + scale(76), c.right, c.top + scale(220)};
    const int width = std::min(scale(430), static_cast<int>(panel.right - panel.left) - scale(32));
    return RECT{panel.left + scale(16), panel.top + scale(38), panel.left + scale(16) + width,
                panel.top + scale(112)};
}

HWND MainWindow::fieldHandle(SourceKind kind) const {
    return fields_[static_cast<std::size_t>(kind)];
}

SourceKind MainWindow::sourceFromControlId(int id) const {
    switch (id) {
        case kIdGold: return SourceKind::Gold;
        case kIdCredits: return SourceKind::Credits;
        case kIdXp: return SourceKind::FreeXp;
        case kIdMoney:
        default: return SourceKind::Money;
    }
}

std::wstring MainWindow::sourceLabel(SourceKind source) const {
    switch (source) {
        case SourceKind::Money: return tr(L"Деньги", L"Money");
        case SourceKind::Gold: return tr(L"Золото", L"Gold");
        case SourceKind::Credits: return tr(L"Кредиты", L"Credits");
        case SourceKind::FreeXp: return tr(L"Свободный опыт", L"Free XP");
    }
    return L"—";
}

std::wstring MainWindow::tr(const wchar_t* ru, const wchar_t* en) const {
    return settings_.english ? en : ru;
}

std::wstring MainWindow::formatNumber(double value, int decimals) const {
    if (!std::isfinite(value)) return L"0";
    std::wostringstream out;
    out.setf(std::ios::fixed);
    out << std::setprecision(decimals) << value;
    std::wstring raw = decimals > 0 ? trimZeros(out.str()) : out.str();

    const auto dot = raw.find(L'.');
    const std::size_t integerEnd = dot == std::wstring::npos ? raw.size() : dot;
    for (std::ptrdiff_t pos = static_cast<std::ptrdiff_t>(integerEnd) - 3; pos > 0; pos -= 3) {
        raw.insert(static_cast<std::size_t>(pos), 1, L' ');
    }
    return raw;
}

std::wstring MainWindow::formatMoney(double value) const {
    return formatNumber(value, 2);
}

std::wstring MainWindow::boxCombinationText(const BoxPurchaseResult& result) const {
    if (result.lines.empty()) return L"—";
    std::wostringstream out;
    for (std::size_t i = 0; i < result.lines.size(); ++i) {
        if (i) out << L" + ";
        const auto& line = result.lines[i];
        out << line.packageCount << L"×" << line.packageSize;
    }
    return out.str();
}

void MainWindow::drawRect(Gdiplus::Graphics& g, const RECT& rect, COLORREF fill,
                          COLORREF border, float borderWidth) {
    const Gdiplus::RectF rf(
        static_cast<Gdiplus::REAL>(rect.left),
        static_cast<Gdiplus::REAL>(rect.top),
        static_cast<Gdiplus::REAL>(rect.right - rect.left),
        static_cast<Gdiplus::REAL>(rect.bottom - rect.top));
    Gdiplus::SolidBrush brush(Theme::gdip(fill));
    Gdiplus::Pen pen(Theme::gdip(border), borderWidth);
    g.FillRectangle(&brush, rf);
    g.DrawRectangle(&pen, rf);
}

void MainWindow::drawText(Gdiplus::Graphics& g, const std::wstring& text, const RECT& rect,
                          float size, COLORREF color, bool bold,
                          Gdiplus::StringAlignment align,
                          Gdiplus::StringAlignment lineAlign) {
    Gdiplus::FontFamily family(L"Segoe UI");
    const float readableSize = size + 1.0f;
    Gdiplus::Font font(&family, static_cast<Gdiplus::REAL>(scale(static_cast<int>(readableSize))),
                       bold ? Gdiplus::FontStyleBold : Gdiplus::FontStyleRegular,
                       Gdiplus::UnitPixel);
    Gdiplus::SolidBrush brush(Theme::gdip(color));
    Gdiplus::StringFormat format;
    format.SetAlignment(align);
    format.SetLineAlignment(lineAlign);
    format.SetTrimming(Gdiplus::StringTrimmingEllipsisCharacter);
    const Gdiplus::RectF rf(
        static_cast<Gdiplus::REAL>(rect.left),
        static_cast<Gdiplus::REAL>(rect.top),
        static_cast<Gdiplus::REAL>(rect.right - rect.left),
        static_cast<Gdiplus::REAL>(rect.bottom - rect.top));
    g.DrawString(text.c_str(), -1, &font, rf, &format, &brush);
}

void MainWindow::drawSection(Gdiplus::Graphics& g, const RECT& rect, const std::wstring& label) {
    drawRect(g, rect, Theme::Panel, Theme::Border);
    RECT labelRect{rect.left + scale(16), rect.top + scale(11), rect.right - scale(16), rect.top + scale(29)};
    drawText(g, upperAscii(label), labelRect, 12.0f, Theme::Muted, false);
}

void MainWindow::drawActionButton(Gdiplus::Graphics& g, const RECT& rect, const std::wstring& text,
                                  bool active, int action, bool primary) {
    const bool hovered = hoverAction_ == action;
    COLORREF fill = Theme::Control;
    COLORREF border = Theme::BorderBright;
    COLORREF textColor = active ? Theme::Text : Theme::Muted;

    if (primary) {
        fill = hovered ? Theme::AccentHover : Theme::Accent;
        border = fill;
        textColor = RGB(22, 17, 28);
    } else if (active) {
        fill = Theme::AccentSoft;
        border = Theme::Accent;
        textColor = Theme::Text;
    } else if (hovered) {
        fill = Theme::ControlHover;
        border = Theme::BorderBright;
        textColor = Theme::Text;
    }

    drawRect(g, rect, fill, border);
    RECT tx = rect;
    drawText(g, text, tx, 12.0f, textColor, active || primary,
             Gdiplus::StringAlignmentCenter, Gdiplus::StringAlignmentCenter);
    hitRects_.push_back({rect, action});
}

void MainWindow::drawStatusTile(Gdiplus::Graphics& g, const RECT& rect, const std::wstring& label,
                                const std::wstring& value, COLORREF dotColor) {
    drawRect(g, rect, Theme::PanelAlt, Theme::Border);
    const int cy = rect.top + (rect.bottom - rect.top) / 2;
    Gdiplus::SolidBrush dot(Theme::gdip(dotColor));
    g.FillEllipse(&dot, Gdiplus::Rect(rect.left + scale(12), cy - scale(3), scale(6), scale(6)));

    RECT labelRect{rect.left + scale(26), rect.top + scale(7), rect.right - scale(12), rect.top + scale(25)};
    drawText(g, upperAscii(label), labelRect, 12.0f, Theme::Muted, false);
    RECT valueRect{rect.left + scale(26), rect.top + scale(24), rect.right - scale(12), rect.bottom - scale(5)};
    drawText(g, value, valueRect, 13.0f, Theme::Text, true);
}

void MainWindow::drawSidebar(Gdiplus::Graphics& g, const RECT& client) {
    RECT sidebar{0, 0, scale(kSidebarWidth), client.bottom};
    Gdiplus::SolidBrush brush(Theme::gdip(Theme::Sidebar));
    g.FillRectangle(&brush, Gdiplus::Rect(sidebar.left, sidebar.top,
                                           sidebar.right - sidebar.left,
                                           sidebar.bottom - sidebar.top));

    if (HICON logo = LoadIconW(instance_, MAKEINTRESOURCEW(IDI_APP_ICON))) {
        const int iconSize = scale(34);
        HDC hdc = g.GetHDC();
        DrawIconEx(hdc, scale(16), scale(20), logo, iconSize, iconSize, 0, nullptr, DI_NORMAL);
        g.ReleaseHDC(hdc);
    }
    RECT brand{scale(58), scale(18), scale(188), scale(48)};
    drawText(g, L"WoT Converter", brand, 18.0f, Theme::Text, true,
             Gdiplus::StringAlignmentNear, Gdiplus::StringAlignmentCenter);
    RECT subtitle{scale(58), scale(46), scale(188), scale(68)};
    drawText(g, tr(L"конвертер ресурсов", L"resource utility"), subtitle, 11.0f, Theme::Muted, false);

    auto sectionLabel = [&](int y, const wchar_t* ru, const wchar_t* en) {
        RECT r{scale(18), scale(y), scale(180), scale(y + 20)};
        drawText(g, upperAscii(tr(ru, en)), r, 11.0f, Theme::MutedDim, false);
    };

    auto navItem = [&](int y, Page page, int action, const std::wstring& icon, const wchar_t* ru, const wchar_t* en) {
        RECT r{scale(14), scale(y), scale(180), scale(y + 42)};
        const bool active = page_ == page;
        const bool hovered = hoverAction_ == action;
        if (active || hovered) {
            drawRect(g, r, active ? Theme::PanelAlt : Theme::ControlHover,
                     active ? Theme::BorderBright : Theme::Sidebar);
        }
        if (active) {
            Gdiplus::SolidBrush accent(Theme::gdip(Theme::Accent));
            g.FillRectangle(&accent, Gdiplus::Rect(r.left, r.top + scale(8), scale(3), scale(26)));
        }
        RECT iconRect{r.left + scale(10), r.top, r.left + scale(34), r.bottom};
        drawText(g, icon, iconRect, 17.0f, active ? Theme::Text : Theme::Muted, false,
                 Gdiplus::StringAlignmentCenter, Gdiplus::StringAlignmentCenter);
        RECT textRect{r.left + scale(38), r.top, r.right - scale(8), r.bottom};
        drawText(g, tr(ru, en), textRect, 13.0f, active ? Theme::Text : Theme::Muted,
                 active, Gdiplus::StringAlignmentNear, Gdiplus::StringAlignmentCenter);
        hitRects_.push_back({r, action});
    };

    sectionLabel(90, L"КОНВЕРТЕР", L"CONVERTER");
    navItem(110, Page::Converter, kActionConverter, L"□", L"Главная", L"Main");
    navItem(154, Page::Containers, kActionContainers, L"◇", L"Контейнеры", L"Containers");

    sectionLabel(214, L"ДАННЫЕ", L"DATA");
    navItem(234, Page::Rates, kActionRates, L"≡", L"Курсы", L"Rates");

    sectionLabel(294, L"ИНФО", L"INFO");
    navItem(314, Page::About, kActionAbout, L"i", L"О программе", L"About");

    RECT footer{scale(18), client.bottom - scale(42), scale(182), client.bottom - scale(18)};
    drawText(g, L"inject0r / WoT utility", footer, 10.0f, Theme::Muted, false);
}

void MainWindow::drawTopBar(Gdiplus::Graphics& g, const RECT&) {
    const RECT c = contentRect();
    const int buttonY = c.top + scale(10);
    const int buttonH = scale(34);
    const int right = c.right;

    RECT gear{right - scale(40), buttonY, right, buttonY + buttonH};
    drawActionButton(g, gear, L"⚙", page_ == Page::Settings, kActionSettings);

    const int regionRight = gear.left - scale(16);
    RECT regionLabel{regionRight - scale(156), c.top - scale(3), regionRight, c.top + scale(12)};
    drawText(g, L"REGION", regionLabel, 10.0f, Theme::MutedDim, false);
    RECT wg{regionRight - scale(64), buttonY, regionRight, buttonY + buttonH};
    RECT lesta{wg.left - scale(82), buttonY, wg.left - scale(8), buttonY + buttonH};
    drawActionButton(g, lesta, L"LESTA", settings_.region == Region::Lesta, kActionLesta);
    drawActionButton(g, wg, L"WG", settings_.region == Region::WG, kActionWg);

    RECT version{lesta.left - scale(92), buttonY, lesta.left - scale(18), buttonY + buttonH};
    drawText(g, kVersion, version, 11.0f, Theme::Muted, false,
             Gdiplus::StringAlignmentFar, Gdiplus::StringAlignmentCenter);
}

void MainWindow::drawConverter(Gdiplus::Graphics& g) {
    const RECT c = contentRect();
    RECT title{c.left, c.top + scale(6), c.right - scale(430), c.top + scale(33)};
    drawText(g, tr(L"Конвертер ресурсов", L"Resource converter"), title, 23.0f, Theme::Text, true);
    RECT sub{c.left, c.top + scale(35), c.right - scale(430), c.top + scale(55)};
    drawText(g, tr(L"Меняй любое поле — остальные значения синхронизируются автоматически.",
                   L"Edit any field and the remaining values stay synchronized."),
             sub, 11.0f, Theme::Muted, false);

    const RECT resources = converterPanelRect();
    drawSection(g, resources, tr(L"Ресурсы", L"Resources"));

    const wchar_t* labelsRu[] = {L"Деньги", L"Золото", L"Кредиты", L"Свободный опыт"};
    const wchar_t* labelsEn[] = {L"Money", L"Gold", L"Credits", L"Free XP"};
    const wchar_t* tags[] = {L"RUB / EUR", L"GOLD", L"CREDITS", L"XP"};

    for (int row = 0; row < 2; ++row) {
        for (int col = 0; col < 2; ++col) {
            const int idx = row * 2 + col;
            const RECT input = converterInputRect(col, row);
            drawRect(g, input, Theme::Input,
                     idx == static_cast<int>(lastSource_) ? Theme::Accent : Theme::Border);
            RECT label{input.left + scale(12), input.top + scale(8), input.right - scale(105), input.top + scale(27)};
            drawText(g, settings_.english ? labelsEn[idx] : labelsRu[idx], label, 11.0f, Theme::Muted, false);
            RECT tag{input.right - scale(100), input.top + scale(8), input.right - scale(12), input.top + scale(27)};
            drawText(g, tags[idx], tag, 10.0f,
                     idx == static_cast<int>(lastSource_) ? Theme::Accent : Theme::MutedDim,
                     idx == static_cast<int>(lastSource_), Gdiplus::StringAlignmentFar);
        }
    }

    const int modifiersTop = resources.bottom + scale(14);
    RECT modifiers{c.left, modifiersTop, c.right, modifiersTop + scale(88)};
    drawSection(g, modifiers, tr(L"Модификаторы", L"Modifiers"));

    const int pad = scale(16);
    const int gap = scale(12);
    const int buttonTop = modifiers.top + scale(36);
    const int buttonBottom = modifiers.bottom - scale(14);
    const int half = ((modifiers.right - modifiers.left) - pad * 2 - gap) / 2;
    RECT promo{modifiers.left + pad, buttonTop, modifiers.left + pad + half, buttonBottom};
    RECT discount{promo.right + gap, buttonTop, modifiers.right - pad, buttonBottom};
    drawActionButton(g, promo, tr(L"СВОБОДНЫЙ ОПЫТ 1:35", L"FREE XP 1:35"),
                     settings_.promoFreeXp, kActionPromo);
    drawActionButton(g, discount, tr(L"ПРЕМ-МАГАЗИН −15%", L"PREMIUM SHOP −15%"),
                     settings_.premiumDiscount, kActionDiscount);

    const int summaryTop = modifiers.bottom + scale(14);
    RECT summary{c.left, summaryTop, c.right, std::min(static_cast<int>(c.bottom), summaryTop + scale(146))};
    drawSection(g, summary, tr(L"Текущее преобразование", L"Current conversion"));

    RECT premiumLabel{summary.left + scale(16), summary.top + scale(34), summary.left + scale(220), summary.top + scale(52)};
    drawText(g, tr(L"Эквивалент премиум-аккаунта", L"Premium account equivalent"),
             premiumLabel, 9.0f, Theme::Muted, false);
    const std::wstring days = hasResult_ ? formatNumber(lastResult_.premiumDays, 1) : L"0";
    RECT premiumValue{summary.left + scale(16), summary.top + scale(52), summary.left + scale(220), summary.top + scale(76)};
    drawText(g, days + tr(L" дней", L" days"), premiumValue, 16.0f, Theme::Success, true);

    const int tilesLeft = summary.left + scale(250);
    const int tilesRight = summary.right - scale(16);
    const int tileGap = scale(10);
    const int tileWidth = std::max(scale(120), (tilesRight - tilesLeft - tileGap * 3) / 4);
    const int tileTop = summary.top + scale(38);
    const int tileBottom = std::min(static_cast<int>(summary.bottom) - scale(14), tileTop + scale(72));

    const CommonRates& common = settings_.region == Region::Lesta ? rates_.lesta.common : rates_.wg.common;
    const double xpRate = settings_.promoFreeXp ? common.promoFreeXpPerGold : common.freeXpPerGold;
    const std::wstring region = settings_.region == Region::Lesta ? L"LESTA" : L"WG";
    const std::wstring xpText = L"1:" + formatNumber(xpRate, 0);
    const std::wstring discountText = settings_.premiumDiscount ? L"−15%" : tr(L"Выкл.", L"Off");

    RECT tile1{tilesLeft, tileTop, tilesLeft + tileWidth, tileBottom};
    RECT tile2{tile1.right + tileGap, tileTop, tile1.right + tileGap + tileWidth, tileBottom};
    RECT tile3{tile2.right + tileGap, tileTop, tile2.right + tileGap + tileWidth, tileBottom};
    RECT tile4{tile3.right + tileGap, tileTop, tilesRight, tileBottom};
    drawStatusTile(g, tile1, tr(L"Регион", L"Region"), region, Theme::Info);
    drawStatusTile(g, tile2, tr(L"Источник", L"Source"), sourceLabel(lastSource_), Theme::Accent);
    drawStatusTile(g, tile3, tr(L"Свободный опыт", L"Free XP"), xpText, Theme::Success);
    drawStatusTile(g, tile4, tr(L"Скидка прем", L"Premium discount"), discountText,
                   settings_.premiumDiscount ? Theme::Success : Theme::MutedDim);
}

void MainWindow::drawContainers(Gdiplus::Graphics& g) {
    const RECT c = contentRect();
    RECT title{c.left, c.top + scale(6), c.right - scale(430), c.top + scale(33)};
    drawText(g, tr(L"Калькулятор контейнеров", L"Container calculator"), title, 20.0f, Theme::Text, true);
    RECT sub{c.left, c.top + scale(35), c.right - scale(430), c.top + scale(55)};
    drawText(g, settings_.region == Region::Lesta
                 ? tr(L"LESTA: фиксированная цена контейнера.", L"LESTA: fixed per-container price.")
                 : tr(L"WG: подбирается минимальная стоимость набора пакетов.",
                      L"WG: the cheapest available package combination is selected."),
             sub, 11.0f, Theme::Muted, false);

    RECT inputPanel{c.left, c.top + scale(76), c.right, c.top + scale(220)};
    drawSection(g, inputPanel, tr(L"Контейнеры", L"Containers"));
    const RECT input = containerInputRect();
    drawRect(g, input, Theme::Input, Theme::Border);
    RECT label{input.left + scale(12), input.top + scale(8), input.right - scale(12), input.top + scale(27)};
    drawText(g, tr(L"Количество", L"Quantity"), label, 10.0f, Theme::Muted, false);

    RECT regionTile{input.right + scale(14), input.top, std::min(static_cast<int>(inputPanel.right) - scale(16), static_cast<int>(input.right) + scale(210)), input.bottom};
    if (regionTile.right > regionTile.left + scale(80)) {
        drawStatusTile(g, regionTile, tr(L"Регион", L"Region"),
                       settings_.region == Region::Lesta ? L"LESTA" : L"WG", Theme::Info);
    }

    RECT result{c.left, inputPanel.bottom + scale(14), c.right, inputPanel.bottom + scale(220)};
    drawSection(g, result, tr(L"Результат", L"Result"));
    RECT value{result.left + scale(16), result.top + scale(42), result.right - scale(16), result.bottom - scale(16)};
    drawText(g, boxResult_.empty() ? tr(L"Введите количество контейнеров выше.", L"Enter a container count above.") : boxResult_,
             value, boxResult_.empty() ? 12.0f : 16.0f,
             boxResult_.empty() ? Theme::Muted : Theme::Text, !boxResult_.empty());
}

void MainWindow::drawRates(Gdiplus::Graphics& g) {
    const RECT c = contentRect();
    RECT title{c.left, c.top + scale(6), c.right - scale(430), c.top + scale(33)};
    drawText(g, tr(L"Курсы и экономика", L"Rates & economy"), title, 20.0f, Theme::Text, true);
    RECT sub{c.left, c.top + scale(35), c.right - scale(430), c.top + scale(55)};
    drawText(g, tr(L"Значения берутся из config/rates.json; при ошибке используются встроенные дефолты.",
                   L"Values come from config/rates.json; built-in defaults are used on failure."),
             sub, 11.0f, Theme::Muted, false);

    RECT panel{c.left, c.top + scale(76), c.right, std::min(static_cast<int>(c.bottom), static_cast<int>(c.top) + scale(500))};
    drawSection(g, panel, tr(L"Текущая экономика", L"Current economy"));

    const CommonRates& common = settings_.region == Region::Lesta ? rates_.lesta.common : rates_.wg.common;
    struct Row { std::wstring label; std::wstring value; };
    std::vector<Row> rows;
    rows.push_back({tr(L"Кредитов за 1 золото", L"Credits per 1 gold"), formatNumber(common.creditsPerGold, 0)});
    rows.push_back({tr(L"Свободного опыта за 1 золото", L"Free XP per 1 gold"), formatNumber(common.freeXpPerGold, 0)});
    rows.push_back({tr(L"Акционный свободный опыт", L"Promo Free XP per 1 gold"), formatNumber(common.promoFreeXpPerGold, 0)});
    rows.push_back({tr(L"30 дней премиума", L"30 days premium"), formatNumber(common.premiumGold30Days, 0) + L" gold"});
    if (settings_.region == Region::WG) {
        rows.push_back({tr(L"EUR за 1 золото", L"EUR per 1 gold"), formatNumber(rates_.wg.eurPerGold, 4)});
    } else {
        rows.push_back({tr(L"Цена контейнера", L"Single container"), formatMoney(rates_.lesta.singleBoxPriceRub) + L" RUB"});
    }

    int y = panel.top + scale(38);
    for (const auto& row : rows) {
        RECT line{panel.left + scale(16), y, panel.right - scale(16), y + scale(56)};
        drawRect(g, line, Theme::PanelAlt, Theme::Border);
        RECT left{line.left + scale(14), line.top, line.right - scale(220), line.bottom};
        drawText(g, row.label, left, 11.0f, Theme::Muted, false,
                 Gdiplus::StringAlignmentNear, Gdiplus::StringAlignmentCenter);
        RECT right{line.right - scale(250), line.top, line.right - scale(14), line.bottom};
        drawText(g, row.value, right, 12.0f, Theme::Text, true,
                 Gdiplus::StringAlignmentFar, Gdiplus::StringAlignmentCenter);
        y += scale(64);
    }
}

void MainWindow::drawSettings(Gdiplus::Graphics& g) {
    const RECT c = contentRect();
    RECT title{c.left, c.top + scale(6), c.right - scale(330), c.top + scale(38)};
    drawText(g, tr(L"Настройки", L"Settings"), title, 23.0f, Theme::Text, true);
    RECT sub{c.left, c.top + scale(39), c.right - scale(330), c.top + scale(64)};
    drawText(g, tr(L"Язык и служебные параметры WoT Converter.",
                   L"Language and utility preferences for WoT Converter."),
             sub, 11.0f, Theme::Muted, false);

    RECT general{c.left, c.top + scale(82), c.right, c.top + scale(242)};
    drawSection(g, general, tr(L"Общие", L"General"));

    RECT languageLabel{general.left + scale(20), general.top + scale(42), general.left + scale(240), general.top + scale(66)};
    drawText(g, tr(L"Язык", L"Language"), languageLabel, 12.0f, Theme::Muted, false);
    RECT ru{general.left + scale(20), general.top + scale(72), general.left + scale(150), general.top + scale(112)};
    RECT en{ru.right + scale(10), ru.top, ru.right + scale(140), ru.bottom};
    drawActionButton(g, ru, L"Русский / RU", !settings_.english, kActionRu);
    drawActionButton(g, en, L"English / EN", settings_.english, kActionEn);

    RECT formatLabel{general.left + scale(340), general.top + scale(42), general.left + scale(560), general.top + scale(66)};
    drawText(g, tr(L"Формат крупных чисел", L"Large number format"), formatLabel, 12.0f, Theme::Muted, false);
    RECT formatBox{general.left + scale(340), general.top + scale(72), general.left + scale(570), general.top + scale(112)};
    drawRect(g, formatBox, Theme::PanelAlt, Theme::Border);
    RECT formatText{formatBox.left + scale(14), formatBox.top, formatBox.right - scale(14), formatBox.bottom};
    drawText(g, L"1 000 000", formatText, 13.0f, Theme::Text, true,
             Gdiplus::StringAlignmentNear, Gdiplus::StringAlignmentCenter);

    RECT config{c.left, general.bottom + scale(14), c.right, general.bottom + scale(224)};
    drawSection(g, config, tr(L"Конфигурация", L"Configuration"));
    RECT pathLabel{config.left + scale(20), config.top + scale(42), config.right - scale(20), config.top + scale(66)};
    drawText(g, tr(L"Файл курсов", L"Rates file"), pathLabel, 11.0f, Theme::Muted, false);
    RECT pathBox{config.left + scale(20), config.top + scale(72), config.right - scale(20), config.top + scale(114)};
    drawRect(g, pathBox, Theme::Input, Theme::Border);
    RECT pathText{pathBox.left + scale(12), pathBox.top, pathBox.right - scale(12), pathBox.bottom};
    drawText(g, ratesPath_.wstring(), pathText, 11.0f, Theme::Text, false,
             Gdiplus::StringAlignmentNear, Gdiplus::StringAlignmentCenter);

    RECT status{config.left + scale(20), config.top + scale(130), config.right - scale(270), config.top + scale(170)};
    drawText(g, ratesWarning_.empty()
                 ? tr(L"●  Конфигурация загружена", L"●  Configuration loaded")
                 : tr(L"●  Используется встроенный fallback", L"●  Built-in fallback is active"),
             status, 11.0f, ratesWarning_.empty() ? Theme::Success : Theme::Danger, true,
             Gdiplus::StringAlignmentNear, Gdiplus::StringAlignmentCenter);

    RECT open{config.right - scale(248), config.top + scale(130), config.right - scale(20), config.top + scale(170)};
    drawActionButton(g, open, tr(L"ОТКРЫТЬ ПАПКУ CONFIG", L"OPEN CONFIG FOLDER"), true, kActionOpenConfig);
}

void MainWindow::drawAbout(Gdiplus::Graphics& g) {
    const RECT c = contentRect();
    RECT title{c.left, c.top + scale(6), c.right - scale(330), c.top + scale(38)};
    drawText(g, L"WoT Converter", title, 23.0f, Theme::Text, true);
    RECT sub{c.left, c.top + scale(39), c.right - scale(330), c.top + scale(64)};
    drawText(g, tr(L"Возможности программы и история изменений.",
                   L"Program features and update history."),
             sub, 11.0f, Theme::Muted, false);

    const int gap = scale(14);
    const int totalWidth = c.right - c.left;
    const int leftWidth = static_cast<int>(totalWidth * 0.48);
    RECT features{c.left, c.top + scale(82), c.left + leftWidth, c.bottom};
    RECT changelog{features.right + gap, features.top, c.right, c.bottom};
    drawSection(g, features, tr(L"Возможности", L"Features"));
    drawSection(g, changelog, tr(L"История изменений", L"Changelog"));

    RECT product{features.left + scale(20), features.top + scale(42), features.right - scale(20), features.top + scale(74)};
    drawText(g, L"WoT Converter  v1.1.2", product, 18.0f, Theme::Text, true);

    RECT featureText{features.left + scale(20), features.top + scale(88), features.right - scale(20), features.bottom - scale(20)};
    const std::wstring featuresBody = tr(
        L"• Мгновенная конвертация денег, золота, кредитов и свободного опыта.\n\n"
        L"• Отдельные режимы LESTA и WG с быстрым переключением региона.\n\n"
        L"• Акционный курс свободного опыта 1:35 и скидка прем-магазина −15%.\n\n"
        L"• Калькулятор контейнеров; для WG подбирается наиболее выгодная комбинация пакетов.\n\n"
        L"• Автоматическая синхронизация полей и форматирование крупных чисел: 1 000 000.\n\n"
        L"• Русский и английский интерфейс, настройки сохраняются между запусками.",
        L"• Live conversion between money, gold, credits and Free XP.\n\n"
        L"• Separate LESTA and WG modes with quick region switching.\n\n"
        L"• Free XP 1:35 promo rate and −15% premium-shop discount.\n\n"
        L"• Container calculator; WG mode finds the cheapest package combination.\n\n"
        L"• Synchronized fields and readable large-number formatting: 1 000 000.\n\n"
        L"• Russian and English UI with preferences saved between launches.");
    drawText(g, featuresBody, featureText, 12.0f, Theme::Text, false);

    RECT logText{changelog.left + scale(20), changelog.top + scale(42), changelog.right - scale(20), changelog.bottom - scale(20)};
    const std::wstring changelogBody = tr(
        L"v1.1.2\n"
        L"• Release-сборка теперь использует статический MSVC Runtime и не требует Visual C++ Redistributable.\n\n"
        L"v1.1.1\n"
        L"• Увеличены шрифты и иконки для лучшей читаемости.\n"
        L"• Добавлена фирменная иконка с зацикленными стрелками для EXE и интерфейса.\n"
        L"• Настройки перенесены в шестерёнку; язык теперь находится внутри Settings.\n"
        L"• Раздел «О программе» заменён на список возможностей и changelog.\n\n"
        L"v1.1.0\n"
        L"• Интерфейс полностью переработан в стиле CoD RCE Guard.\n\n"
        L"v1.0.3\n"
        L"• Убран системный Windows-звук в полях при Enter и Ctrl+A.\n\n"
        L"v1.0.2\n"
        L"• Добавлено форматирование больших чисел, Ctrl+A и исправления компоновки.\n\n"
        L"v1.0.1\n"
        L"• Первый нативный релиз нового WoT Converter.",
        L"v1.1.2\n"
        L"• Release builds now use the static MSVC Runtime and do not require the Visual C++ Redistributable.\n\n"
        L"v1.1.1\n"
        L"• Larger fonts and icons for improved readability.\n"
        L"• Added a loop-arrows app icon for the EXE and sidebar branding.\n"
        L"• Settings moved to the gear button; language now lives inside Settings.\n"
        L"• About page rebuilt around features and changelog.\n\n"
        L"v1.1.0\n"
        L"• UI fully redesigned in the CoD RCE Guard style.\n\n"
        L"v1.0.3\n"
        L"• Removed the Windows system beep from Enter and Ctrl+A in edit fields.\n\n"
        L"v1.0.2\n"
        L"• Added large-number formatting, Ctrl+A and layout fixes.\n\n"
        L"v1.0.1\n"
        L"• First native release of the rebuilt WoT Converter.");
    drawText(g, changelogBody, logText, 11.5f, Theme::Text, false);
}

} // namespace wot::ui

#endif // _WIN32
