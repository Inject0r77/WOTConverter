#pragma once

#ifdef _WIN32

// Keep the Windows/GDI+ include order in one place. With WIN32_LEAN_AND_MEAN
// enabled by CMake, <windows.h> intentionally omits parts of COM/OLE. GDI+
// still relies on those declarations (IStream, IUnknown, PROPID,
// MIDL_INTERFACE), so objidl.h must be included explicitly before gdiplus.h.
#include <windows.h>
#include <objidl.h>
#include <gdiplus.h>

#endif // _WIN32
