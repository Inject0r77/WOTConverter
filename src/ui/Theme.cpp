#include "ui/Theme.h"

namespace wot::ui {

#ifdef _WIN32
Gdiplus::Color Theme::gdip(COLORREF color, BYTE alpha) {
    return Gdiplus::Color(alpha, GetRValue(color), GetGValue(color), GetBValue(color));
}
#endif

} // namespace wot::ui
