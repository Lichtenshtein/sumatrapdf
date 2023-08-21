/* Copyright 2022 the SumatraPDF project authors (see AUTHORS file).
   License: Simplified BSD (see COPYING.BSD) */

#include "utils/BaseUtil.h"
#include "SvgIcons.h"

// https://github.com/tabler/tabler-icons/blob/master/icons/folder.svg
static const char* gIconFileOpen =
    R"(<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" stroke-width="1" stroke="currentColor" fill="none" stroke-linecap="round" stroke-linejoin="round">
  <rect x="0" y="0" width="24" height="24" stroke="none"></rect>
  <path d="M5 4h4l3 3h7a2 2 0 0 1 2 2v8a2 2 0 0 1 -2 2h-14a2 2 0 0 1 -2 -2v-11a2 2 0 0 1 2 -2" />
</svg>)";

// https://github.com/tabler/tabler-icons/blob/master/icons/printer.svg
static const char* gIconPrint =
    R"(<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" stroke-width="1" stroke="currentColor" fill="none" stroke-linecap="round" stroke-linejoin="round">
  <rect x="0" y="0" width="24" height="24" stroke="none"></rect>
  <path d="M17 17h2a2 2 0 0 0 2 -2v-4a2 2 0 0 0 -2 -2h-14a2 2 0 0 0 -2 2v4a2 2 0 0 0 2 2h2" />
  <path d="M17 9v-4a2 2 0 0 0 -2 -2h-6a2 2 0 0 0 -2 2v4" />
  <rect x="7" y="13" width="10" height="8" rx="2" />
</svg>)";

// https://github.com/tabler/tabler-icons/blob/master/icons/arrow-left.svg
static const char* gIconPagePrev =
    R"(<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" stroke-width="1" stroke="currentColor" fill="none" stroke-linecap="round" stroke-linejoin="round">
  <rect x="0" y="0" width="24" height="24" stroke="none"></rect>
  <line x1="5" y1="12" x2="19" y2="12" />
  <line x1="5" y1="12" x2="11" y2="18" />
  <line x1="5" y1="12" x2="11" y2="6" />
</svg>)";

// https://github.com/tabler/tabler-icons/blob/master/icons/arrow-right.svg
static const char* gIconPageNext =
    R"(<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" stroke-width="1" stroke="currentColor" fill="none" stroke-linecap="round" stroke-linejoin="round">
  <rect x="0" y="0" width="24" height="24" stroke="none"></rect>
  <line x1="5" y1="12" x2="19" y2="12" />
  <line x1="13" y1="18" x2="19" y2="12" />
  <line x1="13" y1="6" x2="19" y2="12" />
</svg>)";

static const char* gIconGoBack =
    R"(<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1" stroke-linecap="round" stroke-linejoin="round">
  <rect x="0" y="0" width="24" height="24" stroke="none"></rect>
  <path d="M9 11l-4 4l4 4m-4 -4h11a4 4 0 0 0 0 -8h-1"/>
</svg>)";

static const char* gIconGoForward =
    R"(<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1" stroke-linecap="round" stroke-linejoin="round">
  <rect x="0" y="0" width="24" height="24" stroke="none"></rect>
  <path d="M15 11l4 4l-4 4m4 -4h-11a4 4 0 0 1 0 -8h1"/>
</svg>)";

// https://github.com/tabler/tabler-icons/raw/147130cd0e67a9cfc70538f8b3a14a48a695d90b/icons/outline/stars.svg
static const char* gIconFavorites =
    R"(<svg xmlns="http://www.w3.org/2000/svg" class="icon icon-tabler icon-tabler-stars" width="24" height="24" viewBox="0 0 24 24" stroke-width="1" stroke="currentColor" fill="none" stroke-linecap="round" stroke-linejoin="round">
  <rect x="0" y="0" width="24" height="24" stroke="none"></rect>
  <path d="M17.8 19.817l-2.172 1.138a0.392 .392 0 0 1 -.568 -.41l.415 -2.411l-1.757 -1.707a0.389 .389 0 0 1 .217 -.665l2.428 -.352l1.086 -2.193a0.392 .392 0 0 1 .702 0l1.086 2.193l2.428 .352a0.39 .39 0 0 1 .217 .665l-1.757 1.707l.414 2.41a0.39 .39 0 0 1 -.567 .411l-2.172 -1.138z"/>
  <path d="M6.2 19.817l-2.172 1.138a0.392 .392 0 0 1 -.568 -.41l.415 -2.411l-1.757 -1.707a0.389 .389 0 0 1 .217 -.665l2.428 -.352l1.086 -2.193a0.392 .392 0 0 1 .702 0l1.086 2.193l2.428 .352a0.39 .39 0 0 1 .217 .665l-1.757 1.707l.414 2.41a0.39 .39 0 0 1 -.567 .411l-2.172 -1.138z"/>
  <path d="M12 9.817l-2.172 1.138a0.392 .392 0 0 1 -.568 -.41l.415 -2.411l-1.757 -1.707a0.389 .389 0 0 1 .217 -.665l2.428 -.352l1.086 -2.193a0.392 .392 0 0 1 .702 0l1.086 2.193l2.428 .352a0.39 .39 0 0 1 .217 .665l-1.757 1.707l.414 2.41a0.39 .39 0 0 1 -.567 .411l-2.172 -1.138z"/>
</svg>)";

// https://github.com/tabler/tabler-icons/raw/147130cd0e67a9cfc70538f8b3a14a48a695d90b/icons/outline/bookmarks.svg
static const char* gIconBookmarks =
    R"(<svg xmlns="http://www.w3.org/2000/svg" class="icon icon-tabler icon-tabler-bookmarks" width="24" height="24" viewBox="0 0 24 24" stroke-width="1" stroke="currentColor" fill="none" stroke-linecap="round" stroke-linejoin="round">
  <rect x="0" y="0" width="24" height="24" stroke="none"></rect>
  <path d="M13 7a2 2 0 0 1 2 2v12l-5 -3l-5 3v-12a2 2 0 0 1 2 -2h6z"/>
  <path d="M9.265 4a2 2 0 0 1 1.735 -1h6a2 2 0 0 1 2 2v12l-1 -.6"/>
</svg>)";

// https://github.com/tabler/tabler-icons/blob/master/icons/layout-rows.svg
static const char* gIconLayoutContinuous =
    R"(<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1" stroke-linecap="round" stroke-linejoin="round">
  <rect x="0" y="0" width="24" height="24" stroke="none"></rect>
  <path d="M8 12h-6"/>
  <path d="M5 15l-3 -3l3 -3"/>
  <path d="M22 12h-6"/>
  <path d="M19 15l3 -3l-3 -3"/>
  <path d="M12 4v16"/>
</svg>)";

// https://github.com/tabler/tabler-icons/blob/master/icons/layout-rows.svg
static const char* gIconLayoutContinuous =
    R"(<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1" stroke-linecap="round" stroke-linejoin="round">
  <rect x="0" y="0" width="24" height="24" stroke="none"></rect>
  <path d="M8 12h-6"/>
  <path d="M5 15l-3 -3l3 -3"/>
  <path d="M22 12h-6"/>
  <path d="M19 15l3 -3l-3 -3"/>
  <path d="M12 4v16"/>
</svg>)";

// https://github.com/tabler/tabler-icons/blob/master/icons/square.svg
static const char* gIconLayoutSinglePage =
    R"(<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1" stroke-linecap="round" stroke-linejoin="round">
  <rect x="0" y="0" width="24" height="24" stroke="none"></rect>
  <path d="M4 12v-6a2 2 0 0 1 2 -2h12a2 2 0 0 1 2 2v6"/>
  <path d="M10 18h-7"/>
  <path d="M21 18h-7"/>
  <path d="M6 15l-3 3l3 3"/>
  <path d="M18 15l3 3l-3 3"/>
</svg>)";

// https://github.com/tabler/tabler-icons/blob/master/icons/chevron-left.svg
static const char* gIconSearchPrev =
    R"(<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" stroke-width="1" stroke="currentColor" fill="none" stroke-linecap="round" stroke-linejoin="round">
  <rect x="0" y="0" width="24" height="24" stroke="none"></rect>
  <polyline points="15 6 9 12 15 18" />
</svg>)";

// https://github.com/tabler/tabler-icons/blob/master/icons/chevron-right.svg
static const char* gIconSearchNext =
    R"(<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" stroke-width="1" stroke="currentColor" fill="none" stroke-linecap="round" stroke-linejoin="round">
  <rect x="0" y="0" width="24" height="24" stroke="none"></rect>
  <polyline points="9 6 15 12 9 18" />
</svg>)";

// https://github.com/tabler/tabler-icons/blob/master/icons/letter-case.svg
static const char* gIconMatchCase =
    R"(<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1" stroke-linecap="round" stroke-linejoin="round">
  <rect x="0" y="0" width="24" height="24" stroke="none"></rect>
  <path d="M3 16v-6a2 2 0 1 1 4 0v6"/>
  <path d="M3 13h4"/>
  <path d="M10 8v6a2 2 0 1 0 4 0v-1a2 2 0 1 0 -4 0v1"/>
  <path d="M20.732 12a2 2 0 0 0 -3.732 1v1a2 2 0 0 0 3.726 1.01"/>
</svg>)";

// https://github.com/tabler/tabler-icons/blob/master/icons/zoom-in.svg
static const char* gIconZoomIn =
    R"(<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" stroke-width="1" stroke="currentColor" fill="none" stroke-linecap="round" stroke-linejoin="round">
  <rect x="0" y="0" width="24" height="24" stroke="none"></rect>
  <circle cx="10" cy="10" r="7" />
  <line x1="7" y1="10" x2="13" y2="10" />
  <line x1="10" y1="7" x2="10" y2="13" />
  <line x1="21" y1="21" x2="15" y2="15" />
</svg>)";

// https://github.com/tabler/tabler-icons/blob/master/icons/zoom-out.svg
static const char* gIconZoomOut =
    R"(<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" stroke-width="1" stroke="currentColor" fill="none" stroke-linecap="round" stroke-linejoin="round">
  <rect x="0" y="0" width="24" height="24" stroke="none"></rect>
  <circle cx="10" cy="10" r="7" />
  <line x1="7" y1="10" x2="13" y2="10" />
  <line x1="21" y1="21" x2="15" y2="15" />
</svg>)";

// https://github.com/tabler/tabler-icons/blob/master/icons/floppy-disk.svg
static const char* gIconSave =
    R"(<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" stroke-width="1" stroke="currentColor" fill="none" stroke-linecap="round" stroke-linejoin="round">
  <path stroke="none" d="M0 0h24v24H0z"/>
  <path d="M18 20h-12a2 2 0 0 1 -2 -2v-12a2 2 0 0 1 2 -2h9l5 5v9a2 2 0 0 1 -2 2" />
  <circle cx="12" cy="13" r="2" />
  <polyline points="4 8 10 8 10 4" />
</svg>)";

// https://github.com/tabler/tabler-icons/blob/master/icons/rotate-2.svg - modified
static const char* gIconRotateLeft =
    R"(<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" stroke-width="1" stroke="currentColor" fill="none" stroke-linecap="round" stroke-linejoin="round">
  <path stroke="none" d="M0 0h24v24H0z" fill="none"/>
  <path d="M15 4.55a8 8 0 0 0 -6 14.9m0 -5.45v6h-6"/>
  <circle cx="18.37" cy="7.16" r="0.15"/>
  <circle cx="13" cy="19.94" r="0.15"/>
  <circle cx="16.84" cy="18.37" r="0.15"/>
  <circle cx="19.37" cy="15.1" r="0.15"/>
  <circle cx="19.94" cy="11" r="0.15"/>
</svg>)";

// https://github.com/tabler/tabler-icons/blob/master/icons/rotate-clockwise-2.svg - modified
static const char* gIconRotateRight =
    R"(<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" stroke-width="1" stroke="currentColor" fill="none" stroke-linecap="round" stroke-linejoin="round">
  <path stroke="none" d="M0 0h24v24H0z" fill="none"/>
  <path d="M9 4.55a8 8 0 0 1 6 14.9m0 -5.45v6h6"/>
  <circle cx="5.63" cy="7.16" r="0.15"/>
  <circle cx="4.06" cy="11" r="0.15"/>
  <circle cx="4.63" cy="15.1" r="0.15"/>
  <circle cx="7.16" cy="18.37" r="0.15"/>
  <circle cx="11" cy="19.94" r="0.15"/>
</svg>)";

static const char* gIconDarkMode =
    R"(<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1" stroke-linecap="round" stroke-linejoin="round">
  <rect x="0" y="0" width="24" height="24" stroke="none"></rect>
  <path d="M3 16h18"/>
  <path d="M3 20h18"/>
  <path d="M8.296 16c-2.268 -1.4 -3.598 -4.087 -3.237 -6.916c.443 -3.48 3.308 -6.083 6.698 -6.084v.006h.296c-1.991 1.916 -2.377 5.03 -.918 7.405c1.459 2.374 4.346 3.33 6.865 2.275a6.888 6.888 0 0 1 -2.777 3.314"/>
</svg>)";

static const char* gIconReadMode =
    R"(<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1" stroke-linecap="round" stroke-linejoin="round">
  <rect x="0" y="0" width="24" height="24" stroke="none"></rect>
  <path d="M8 4h-2l-3 10"/>
  <path d="M16 4h2l3 10"/>
  <path d="M10 16l4 0"/>
  <path d="M21 16.5a3.5 3.5 0 0 1 -7 0v-2.5h7v2.5"/>
  <path d="M10 16.5a3.5 3.5 0 0 1 -7 0v-2.5h7v2.5"/>
</svg>)";

static const char* gIconDefaultMode =
    R"(<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1" stroke-linecap="round" stroke-linejoin="round">
  <rect x="0" y="0" width="24" height="24" stroke="none"></rect>
  <path d="M12 12m-4 0a4 4 0 1 0 8 0a4 4 0 1 0 -8 0"/>
  <path d="M3 12h1m8 -9v1m8 8h1m-9 8v1m-6.4 -15.4l.7 .7m12.1 -.7l-.7 .7m0 11.4l.7 .7m-12.1 -.7l-.7 .7"/>
 </svg>)";

// must match order in enum class TbIcon
// clang-format off
static const char* gIcons[] = {
    gIconFileOpen,
    gIconPrint,
    gIconPagePrev,
    gIconPageNext,
    gIconGoBack,
    gIconGoForward,
    gIconFavorites,
    gIconBookmarks,
    gIconLayoutContinuous,
    gIconLayoutSinglePage,
    gIconRotateLeft,
    gIconRotateRight,
    gIconZoomOut,
    gIconZoomIn,
    gIconSearchPrev,
    gIconSearchNext,
    gIconMatchCase,
    gIconMatchCase,  // TODO: remove this, is for compatiblity with bitmap icons
    gIconDarkMode,
    gIconReadMode,
    gIconDefaultMode,
    gIconSave,
};
// clang-format on

const char* GetSvgIcon(TbIcon idx) {
    int n = (int)idx;
    ReportIf(n < 0 || n >= dimofi(gIcons));
    if (n >= dimofi(gIcons)) {
        return nullptr;
    }
    return gIcons[n];
}
