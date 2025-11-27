/* Copyright 2024 the SumatraPDF project authors (see AUTHORS file).
   License: GPLv3 */

#pragma once

struct DisplayModel;
struct RenderedBitmap;
struct MainWindow;

// Represents a single page thumbnail in the panel
struct ThumbnailItem {
    int pageNo = 0;                     // 1-based page number
    RenderedBitmap* thumbnail = nullptr; // Rendered thumbnail bitmap (owned)
    bool isLoading = false;             // Currently being rendered
    Rect bounds;                        // Position in grid (client coords)

    ThumbnailItem() = default;
    ~ThumbnailItem();
};

// Custom control for displaying page thumbnails in a scrollable grid
struct ThumbnailPanel {
    HWND hwnd = nullptr;
    HWND hwndParent = nullptr;
    MainWindow* win = nullptr;          // Parent MainWindow
    DisplayModel* dm = nullptr;         // Associated display model (not owned)

    // All page thumbnails
    Vec<ThumbnailItem*> items;

    // Layout configuration
    int thumbnailWidth = 120;           // Base thumbnail width
    int thumbnailHeight = 160;          // Calculated from aspect ratio
    int columnCount = 1;                // Calculated based on panel width
    int itemPadding = 8;                // Space between thumbnails
    int scrollPos = 0;                  // Current scroll position (pixels)
    int totalHeight = 0;                // Total content height

    // Current page highlight
    int currentPage = 0;                // Page currently highlighted (synced with doc view)

    // Rendering state
    CRITICAL_SECTION renderLock;
    bool initialized = false;

    ThumbnailPanel();
    ~ThumbnailPanel();

    // Creation
    HWND Create(HWND parent, MainWindow* mainWin);

    // Public API
    void SetDisplayModel(DisplayModel* dm);
    void Clear();
    void Refresh();
    void SetCurrentPage(int pageNo);
    void ScrollToPage(int pageNo);

    // Layout
    void RecalculateLayout();
    void OnSize(int width, int height);

    // Rendering
    void OnPaint(HDC hdc);
    void RenderVisibleThumbnails();
    void RequestThumbnail(int pageNo);
    void OnThumbnailRendered(int pageNo, RenderedBitmap* bmp);

    // Input
    int PageAtPoint(int x, int y);
    void OnLButtonDown(int x, int y);
    void OnVScroll(WPARAM wp);
    void OnMouseWheel(int delta);

    // Drawing helpers
    void DrawThumbnail(HDC hdc, ThumbnailItem* item, bool isCurrentPage);
    void DrawPlaceholder(HDC hdc, ThumbnailItem* item);
    void DrawPageNumber(HDC hdc, ThumbnailItem* item);

private:
    void UpdateScrollInfo();
    Rect GetVisibleRect();
};

// Window procedure for ThumbnailPanel
LRESULT CALLBACK ThumbnailPanelWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);

// Register window class
void RegisterThumbnailPanelClass();

// Layout function for thumbnail panel container (matches LayoutTreeContainer pattern)
void LayoutThumbnailContainer(struct LabelWithCloseWnd* l, HWND hwndPanel);

// Public API functions (called from SumatraPDF.cpp)
void CreateThumbnailPanel(MainWindow* win);
void LoadThumbnailPanel(MainWindow* win);
void ToggleThumbnails(MainWindow* win);
void UpdateThumbnailSelection(MainWindow* win);
void UnsubclassThumbnailBox(MainWindow* win);
