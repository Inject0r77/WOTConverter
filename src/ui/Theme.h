#pragma once

#ifdef _WIN32
#include "ui/WindowsHeaders.h"
#endif

namespace wot::ui {

struct Theme {
#ifdef _WIN32
    // Palette intentionally mirrors the CoD RCE Guard v0.0.5 desktop UI:
    // neutral near-black surfaces, thin cool-gray borders and a violet accent.
    static constexpr COLORREF Background = RGB(12, 14, 19);
    static constexpr COLORREF Sidebar = RGB(14, 17, 24);
    static constexpr COLORREF Panel = RGB(20, 23, 31);
    static constexpr COLORREF PanelAlt = RGB(24, 27, 36);
    static constexpr COLORREF Input = RGB(28, 31, 41);
    static constexpr COLORREF Control = RGB(27, 30, 39);
    static constexpr COLORREF ControlHover = RGB(34, 38, 49);
    static constexpr COLORREF Border = RGB(39, 44, 57);
    static constexpr COLORREF BorderBright = RGB(65, 72, 90);
    static constexpr COLORREF Text = RGB(235, 238, 244);
    static constexpr COLORREF Muted = RGB(122, 137, 160);
    static constexpr COLORREF MutedDim = RGB(91, 103, 123);
    static constexpr COLORREF Accent = RGB(171, 121, 239);
    static constexpr COLORREF AccentHover = RGB(186, 142, 246);
    static constexpr COLORREF AccentSoft = RGB(64, 43, 84);
    static constexpr COLORREF Success = RGB(105, 219, 168);
    static constexpr COLORREF Info = RGB(124, 176, 236);
    static constexpr COLORREF Danger = RGB(238, 107, 128);

    static Gdiplus::Color gdip(COLORREF color, BYTE alpha = 255);
#endif
};

} // namespace wot::ui
