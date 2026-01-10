/* Copyright 2022 the SumatraPDF project authors (see AUTHORS file).
   License: Simplified BSD (see COPYING.BSD) */

// must match order in gAllIcons
enum class TbIcon {
    Text = -2,
    None = -1,
    Open = 0,
    Save,
    Favorites,
    Bookmarks,
    Print,
    PageFirst,
    PagePrev,
    GoBack,
    GoForward,
    PageNext,
    PageLast,
    LayoutContinuous,
    LayoutSinglePage,
    ToggleThumbnails,
    RotateLeft,
    RotateRight,
    ZoomOut,
    ZoomIn,
    SearchPrev,
    SearchNext,
    MatchCase,
    MatchCase2,
    DarkMode,
    InvertColors,
    ReadMode,
    DefaultMode,
    CopyPageImage,
    kMax
};

const char* GetSvgIcon(TbIcon);
