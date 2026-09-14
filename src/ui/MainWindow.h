#pragma once

#ifdef _WIN32

#include "core/BoxOptimizer.h"
#include "core/Converter.h"
#include "storage/Settings.h"
#include "ui/WindowsHeaders.h"

#include <array>
#include <filesystem>
#include <string>
#include <vector>

namespace wot::ui {

class MainWindow {
public:
    MainWindow(HINSTANCE instance,
               RatesConfig rates,
               storage::SettingsStore settingsStore,
               std::filesystem::path ratesPath,
               std::string ratesWarning);
    ~MainWindow();

    bool create();
    int run();

private:
    enum class Page { Converter, Containers, Rates, Settings, About };

    struct HitRect {
        RECT rect{};
        int action = 0;
    };

    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK EditSubclassProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam,
                                             UINT_PTR subclassId, DWORD_PTR refData);
    LRESULT handleMessage(UINT message, WPARAM wParam, LPARAM lParam);

    bool onCreate();
    void onDestroy();
    void onPaint();
    void onSize(int width, int height);
    void onCommand(WPARAM wParam, LPARAM lParam);
    void onClick(int x, int y);
    void onMouseMove(int x, int y);
    void onDpiChanged(UINT newDpi, const RECT* suggested);

    void createEditControls();
    void layoutEditControls();
    void showControlsForPage();
    void styleNativeControl(HWND control);

    void recalcFrom(SourceKind source);
    void recalcBoxes();
    void updateAllFields(const ConversionResult& result, SourceKind source);
    void setFieldText(SourceKind kind, const std::wstring& text);
    [[nodiscard]] std::wstring fieldText(SourceKind kind) const;
    [[nodiscard]] double parseField(SourceKind kind, bool* ok = nullptr) const;
    [[nodiscard]] int parseBoxCount(bool* ok = nullptr) const;
    void normalizeField(SourceKind kind);
    void normalizeBoxField();

    void saveSettings();
    void applySettings();
    void setPage(Page page);
    void setRegion(Region region);
    void setLanguage(bool english);
    void togglePromo();
    void toggleDiscount();
    void openConfigFolder();

    [[nodiscard]] int scale(int px) const;
    [[nodiscard]] RECT contentRect() const;
    [[nodiscard]] RECT converterPanelRect() const;
    [[nodiscard]] RECT converterInputRect(int column, int row) const;
    [[nodiscard]] RECT containerInputRect() const;
    [[nodiscard]] HWND fieldHandle(SourceKind kind) const;
    [[nodiscard]] SourceKind sourceFromControlId(int id) const;
    [[nodiscard]] std::wstring sourceLabel(SourceKind source) const;

    std::wstring tr(const wchar_t* ru, const wchar_t* en) const;
    std::wstring formatNumber(double value, int decimals = 0) const;
    std::wstring formatMoney(double value) const;
    std::wstring boxCombinationText(const BoxPurchaseResult& result) const;

    void drawRect(Gdiplus::Graphics& g, const RECT& rect, COLORREF fill, COLORREF border,
                  float borderWidth = 1.0f);
    void drawText(Gdiplus::Graphics& g, const std::wstring& text, const RECT& rect,
                  float size, COLORREF color, bool bold = false,
                  Gdiplus::StringAlignment align = Gdiplus::StringAlignmentNear,
                  Gdiplus::StringAlignment lineAlign = Gdiplus::StringAlignmentNear);
    void drawSection(Gdiplus::Graphics& g, const RECT& rect, const std::wstring& label);
    void drawActionButton(Gdiplus::Graphics& g, const RECT& rect, const std::wstring& text,
                          bool active, int action, bool primary = false);
    void drawStatusTile(Gdiplus::Graphics& g, const RECT& rect, const std::wstring& label,
                        const std::wstring& value, COLORREF dotColor);
    void drawSidebar(Gdiplus::Graphics& g, const RECT& client);
    void drawTopBar(Gdiplus::Graphics& g, const RECT& client);
    void drawConverter(Gdiplus::Graphics& g);
    void drawContainers(Gdiplus::Graphics& g);
    void drawRates(Gdiplus::Graphics& g);
    void drawSettings(Gdiplus::Graphics& g);
    void drawAbout(Gdiplus::Graphics& g);

    HINSTANCE instance_{};
    HWND hwnd_{};
    UINT dpi_ = 96;
    int clientWidth_ = 1380;
    int clientHeight_ = 800;
    HFONT editFont_{};
    HBRUSH editBrush_{};

    std::array<HWND, 4> fields_{};
    HWND boxEdit_{};
    bool updatingFields_ = false;
    SourceKind lastSource_ = SourceKind::Money;
    Page page_ = Page::Converter;

    RatesConfig rates_;
    Converter converter_;
    BoxOptimizer boxOptimizer_;
    storage::SettingsStore settingsStore_;
    storage::AppSettings settings_{};
    std::filesystem::path ratesPath_;
    std::string ratesWarning_;

    std::vector<HitRect> hitRects_;
    int hoverAction_ = 0;
    bool trackingMouse_ = false;
    ConversionResult lastResult_{};
    bool hasResult_ = false;
    std::wstring boxResult_;
};

} // namespace wot::ui

#endif // _WIN32
