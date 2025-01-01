/* Copyright 2024 the SumatraPDF project authors (see AUTHORS file).
   License: GPLv3 */

#include "utils/BaseUtil.h"
#include "utils/ScopedWin.h"
#include "utils/WinUtil.h"
#include "utils/GdiPlusUtil.h"

#include "wingui/UIModels.h"
#include "wingui/Layout.h"
#include "wingui/WinGui.h"
#include "wingui/LabelWithCloseWnd.h"

#include "Settings.h"
#include "DocController.h"
#include "EngineBase.h"
#include "GlobalPrefs.h"
#include "DisplayModel.h"
#include "SumatraPDF.h"
#include "MainWindow.h"
#include "WindowTab.h"
#include "resource.h"
#include "Commands.h"
#include "ThumbnailPanel.h"
#include "Translations.h"

#include "utils/Log.h"

// Window class name for the thumbnail panel
static const WCHAR* kThumbnailPanelClassName = L"SUMATRA_THUMBNAIL_PANEL";
static bool gThumbnailClassRegistered = false;

// Constants
static const int kMinThumbnailWidth = 80;
static const int kMaxThumbnailWidth = 300;
static const int kDefaultThumbnailWidth = 120;
static const int kThumbnailPadding = 8;
static const int kPageNumberHeight = 20;

// ========== ThumbnailItem ==========

ThumbnailItem::~ThumbnailItem() {
    delete thumbnail;
}

// ========== ThumbnailPanel ==========

ThumbnailPanel::ThumbnailPanel() {
    InitializeCriticalSection(&renderLock);
    initialized = true;
}

ThumbnailPanel::~ThumbnailPanel() {
    Clear();
    if (initialized) {
        DeleteCriticalSection(&renderLock);
    }
    if (hwnd) {
        DestroyWindow(hwnd);
    }
}

void ThumbnailPanel::Clear() {
    EnterCriticalSection(&renderLock);
    for (ThumbnailItem* item : items) {
        delete item;
    }
    items.Reset();
    LeaveCriticalSection(&renderLock);

    dm = nullptr;
    currentPage = 0;
    scrollPos = 0;
    totalHeight = 0;
}

HWND ThumbnailPanel::Create(HWND parent, MainWindow* mainWin) {
    hwndParent = parent;
    win = mainWin;

    RegisterThumbnailPanelClass();

    DWORD style = WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_CLIPCHILDREN;
    DWORD exStyle = WS_EX_STATICEDGE;

    hwnd = CreateWindowExW(exStyle, kThumbnailPanelClassName, L"",
                           style, 0, 0, 100, 100, parent,
                           nullptr, GetModuleHandle(nullptr), this);

    if (!hwnd) {
        // logf("ThumbnailPanel::Create failed, error=%d\n", GetLastError());
        return nullptr;
    }

    return hwnd;
}

void ThumbnailPanel::SetDisplayModel(DisplayModel* newDm) {
    Clear();
    dm = newDm;

    if (!dm) {
        if (hwnd) {
            InvalidateRect(hwnd, nullptr, TRUE);
        }
        return;
    }

    int pageCount = dm->PageCount();
    // logf("ThumbnailPanel::SetDisplayModel pageCount=%d\n", pageCount);

    for (int i = 1; i <= pageCount; i++) {
        ThumbnailItem* item = new ThumbnailItem();
        item->pageNo = i;
        items.Append(item);
    }

    currentPage = dm->CurrentPageNo();
    RecalculateLayout();
    RenderVisibleThumbnails();

    if (hwnd) {
        InvalidateRect(hwnd, nullptr, TRUE);
    }
}

void ThumbnailPanel::Refresh() {
    if (dm) {
        currentPage = dm->CurrentPageNo();
    }
    RecalculateLayout();
    RenderVisibleThumbnails();
    if (hwnd) {
        InvalidateRect(hwnd, nullptr, TRUE);
    }
}

void ThumbnailPanel::SetCurrentPage(int pageNo) {
    if (currentPage == pageNo) {
        return;
    }
    currentPage = pageNo;
    if (hwnd) {
        InvalidateRect(hwnd, nullptr, TRUE);
    }
}

void ThumbnailPanel::ScrollToPage(int pageNo) {
    if (pageNo < 1 || pageNo > items.Size()) {
        return;
    }

    ThumbnailItem* item = items[pageNo - 1];
    Rect visible = GetVisibleRect();

    // Check if page is already visible
    if (item->bounds.y >= scrollPos &&
        item->bounds.y + item->bounds.dy <= scrollPos + visible.dy) {
        return;
    }

    // Scroll to center the page
    int newScrollPos = item->bounds.y - (visible.dy - item->bounds.dy) / 2;
    newScrollPos = std::max(0, std::min(newScrollPos, totalHeight - visible.dy));

    if (newScrollPos != scrollPos) {
        scrollPos = newScrollPos;
        UpdateScrollInfo();
        RenderVisibleThumbnails();
        InvalidateRect(hwnd, nullptr, TRUE);
    }
}

void ThumbnailPanel::RecalculateLayout() {
    if (!hwnd) {
        return;
    }

    RECT rc;
    GetClientRect(hwnd, &rc);
    int clientWidth = rc.right - rc.left;
    int clientHeight = rc.bottom - rc.top;

    if (clientWidth <= 0) {
        return;
    }

    // Calculate how many columns fit
    int availWidth = clientWidth - kThumbnailPadding;
    columnCount = std::max(1, availWidth / (thumbnailWidth + kThumbnailPadding));

    // Calculate thumbnail height based on typical page aspect ratio (letter ~1.29)
    // We'll adjust per-page when we have actual dimensions
    thumbnailHeight = (int)(thumbnailWidth * 1.29f) + kPageNumberHeight;

    // Position each thumbnail
    int x = kThumbnailPadding;
    int y = kThumbnailPadding;
    int col = 0;

    for (ThumbnailItem* item : items) {
        item->bounds.x = x;
        item->bounds.y = y;
        item->bounds.dx = thumbnailWidth;
        item->bounds.dy = thumbnailHeight;

        col++;
        if (col >= columnCount) {
            col = 0;
            x = kThumbnailPadding;
            y += thumbnailHeight + kThumbnailPadding;
        } else {
            x += thumbnailWidth + kThumbnailPadding;
        }
    }

    // Calculate total height
    int rows = (items.Size() + columnCount - 1) / columnCount;
    totalHeight = rows * (thumbnailHeight + kThumbnailPadding) + kThumbnailPadding;

    UpdateScrollInfo();
}

void ThumbnailPanel::OnSize(int width, int height) {
    RecalculateLayout();
    RenderVisibleThumbnails();
    InvalidateRect(hwnd, nullptr, TRUE);
}

void ThumbnailPanel::UpdateScrollInfo() {
    if (!hwnd) {
        return;
    }

    RECT rc;
    GetClientRect(hwnd, &rc);
    int clientHeight = rc.bottom - rc.top;

    SCROLLINFO si = {0};
    si.cbSize = sizeof(si);
    si.fMask = SIF_ALL;
    si.nMin = 0;
    si.nMax = totalHeight;
    si.nPage = clientHeight;
    si.nPos = scrollPos;

    SetScrollInfo(hwnd, SB_VERT, &si, TRUE);
}

Rect ThumbnailPanel::GetVisibleRect() {
    if (!hwnd) {
        return Rect();
    }

    RECT rc;
    GetClientRect(hwnd, &rc);
    return Rect(0, scrollPos, rc.right - rc.left, rc.bottom - rc.top);
}

void ThumbnailPanel::RenderVisibleThumbnails() {
    // No longer needed - we render on-demand during paint
    // Just trigger a repaint
    if (hwnd) {
        InvalidateRect(hwnd, nullptr, FALSE);
    }
}

void ThumbnailPanel::RequestThumbnail(int pageNo) {
    // No longer needed - we render on-demand during paint
    (void)pageNo;
}

void ThumbnailPanel::OnThumbnailRendered(int pageNo, RenderedBitmap* bmp) {
    // No longer needed - we render on-demand during paint
    (void)pageNo;
    delete bmp;
}

int ThumbnailPanel::PageAtPoint(int x, int y) {
    int scrolledY = y + scrollPos;

    for (ThumbnailItem* item : items) {
        if (x >= item->bounds.x && x < item->bounds.x + item->bounds.dx &&
            scrolledY >= item->bounds.y && scrolledY < item->bounds.y + item->bounds.dy) {
            return item->pageNo;
        }
    }
    return 0;
}

void ThumbnailPanel::OnLButtonDown(int x, int y) {
    int pageNo = PageAtPoint(x, y);
    if (pageNo <= 0) {
        return;
    }

    // Check keyboard modifiers for multi-select
    bool ctrlDown = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
    bool shiftDown = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;

    if (ctrlDown) {
        // Ctrl+Click: Toggle selection of this page
        ToggleSelection(pageNo);
    } else if (shiftDown && lastClickedPage > 0) {
        // Shift+Click: Select range from last clicked to this page
        SelectRange(lastClickedPage, pageNo);
    } else {
        // Normal click: Clear selection and select only this page
        ClearSelection();
        SelectPage(pageNo, false);
    }

    lastClickedPage = pageNo;

    // Still navigate to the clicked page
    if (win && dm) {
        // logf("ThumbnailPanel: Navigating to page %d\n", pageNo);
        dm->GoToPage(pageNo, true);
        SetCurrentPage(pageNo);
    }

    InvalidateRect(hwnd, nullptr, FALSE);
}

void ThumbnailPanel::OnVScroll(WPARAM wp) {
    RECT rc;
    GetClientRect(hwnd, &rc);
    int clientHeight = rc.bottom - rc.top;

    int newPos = scrollPos;
    int scrollAmount = thumbnailHeight + kThumbnailPadding;

    switch (LOWORD(wp)) {
        case SB_TOP:
            newPos = 0;
            break;
        case SB_BOTTOM:
            newPos = totalHeight - clientHeight;
            break;
        case SB_LINEUP:
            newPos -= scrollAmount / 4;
            break;
        case SB_LINEDOWN:
            newPos += scrollAmount / 4;
            break;
        case SB_PAGEUP:
            newPos -= clientHeight;
            break;
        case SB_PAGEDOWN:
            newPos += clientHeight;
            break;
        case SB_THUMBTRACK:
        case SB_THUMBPOSITION:
            newPos = HIWORD(wp);
            break;
    }

    newPos = std::max(0, std::min(newPos, totalHeight - clientHeight));

    if (newPos != scrollPos) {
        scrollPos = newPos;
        UpdateScrollInfo();
        RenderVisibleThumbnails();
        InvalidateRect(hwnd, nullptr, TRUE);
    }
}

void ThumbnailPanel::OnMouseWheel(int delta) {
    int scrollAmount = (thumbnailHeight + kThumbnailPadding) / 2;
    int newPos = scrollPos - (delta / WHEEL_DELTA) * scrollAmount;

    RECT rc;
    GetClientRect(hwnd, &rc);
    int clientHeight = rc.bottom - rc.top;

    newPos = std::max(0, std::min(newPos, totalHeight - clientHeight));

    if (newPos != scrollPos) {
        scrollPos = newPos;
        UpdateScrollInfo();
        RenderVisibleThumbnails();
        InvalidateRect(hwnd, nullptr, TRUE);
    }
}

// ========== Selection Management ==========

void ThumbnailPanel::SelectPage(int pageNo, bool addToSelection) {
    if (pageNo < 1 || pageNo > (int)items.Size()) {
        return;
    }

    if (!addToSelection) {
        ClearSelection();
    }

    ThumbnailItem* item = items[pageNo - 1];
    item->isSelected = true;
}

void ThumbnailPanel::SelectRange(int startPage, int endPage) {
    if (startPage < 1 || endPage < 1) {
        return;
    }

    // Ensure start <= end
    int minPage = std::min(startPage, endPage);
    int maxPage = std::max(startPage, endPage);

    // Clamp to valid range
    minPage = std::max(1, minPage);
    maxPage = std::min((int)items.Size(), maxPage);

    // Clear existing selection and select the range
    ClearSelection();

    for (int p = minPage; p <= maxPage; p++) {
        ThumbnailItem* item = items[p - 1];
        item->isSelected = true;
    }
}

void ThumbnailPanel::ToggleSelection(int pageNo) {
    if (pageNo < 1 || pageNo > (int)items.Size()) {
        return;
    }

    ThumbnailItem* item = items[pageNo - 1];
    item->isSelected = !item->isSelected;
}

void ThumbnailPanel::ClearSelection() {
    for (ThumbnailItem* item : items) {
        item->isSelected = false;
    }
}

void ThumbnailPanel::SelectAll() {
    for (ThumbnailItem* item : items) {
        item->isSelected = true;
    }
}

bool ThumbnailPanel::IsPageSelected(int pageNo) {
    if (pageNo < 1 || pageNo > (int)items.Size()) {
        return false;
    }
    return items[pageNo - 1]->isSelected;
}

Vec<int> ThumbnailPanel::GetSelectedPages() {
    Vec<int> selected;
    for (ThumbnailItem* item : items) {
        if (item->isSelected) {
            selected.Append(item->pageNo);
        }
    }
    return selected;
}

int ThumbnailPanel::GetSelectionCount() {
    int count = 0;
    for (ThumbnailItem* item : items) {
        if (item->isSelected) {
            count++;
        }
    }
    return count;
}

// ========== Context Menu ==========

void ThumbnailPanel::ShowContextMenu(int screenX, int screenY) {
    // Convert screen coords to client coords to find clicked page
    POINT pt = {screenX, screenY};
    ScreenToClient(hwnd, &pt);
    int clickedPage = PageAtPoint(pt.x, pt.y);

    // If right-clicked on a page that's not selected, select only that page
    if (clickedPage > 0 && !IsPageSelected(clickedPage)) {
        ClearSelection();
        SelectPage(clickedPage, false);
        lastClickedPage = clickedPage;
        InvalidateRect(hwnd, nullptr, FALSE);
    }

    int selCount = GetSelectionCount();
    if (selCount == 0) {
        return;  // No selection, no menu
    }

    // logf("ThumbnailPanel::ShowContextMenu: %d pages selected\n", selCount);

    // Build context menu
    HMENU popup = CreatePopupMenu();

    // Dynamic labels showing selection count
    char deleteLabel[64];
    char extractLabel[64];
    sprintf_s(deleteLabel, sizeof(deleteLabel), "Delete %d Page(s)...", selCount);
    sprintf_s(extractLabel, sizeof(extractLabel), "Extract %d Page(s)...", selCount);

    AppendMenuA(popup, MF_STRING, CmdDeleteSelectedPages, deleteLabel);
    AppendMenuA(popup, MF_STRING, CmdExtractSelectedPages, extractLabel);
    AppendMenuA(popup, MF_SEPARATOR, 0, nullptr);
    AppendMenuA(popup, MF_STRING, CmdSelectAllThumbnails, "Select All\tCtrl+A");
    AppendMenuA(popup, MF_STRING, CmdDeselectAllThumbnails, "Deselect All");

    // Show menu at screen position
    UINT flags = TPM_RETURNCMD | TPM_RIGHTBUTTON;
    int cmdId = TrackPopupMenu(popup, flags, screenX, screenY, 0, hwnd, nullptr);
    DestroyMenu(popup);

    // Handle commands locally for Select/Deselect, send others to main window
    if (cmdId == CmdSelectAllThumbnails) {
        SelectAll();
        InvalidateRect(hwnd, nullptr, FALSE);
    } else if (cmdId == CmdDeselectAllThumbnails) {
        ClearSelection();
        InvalidateRect(hwnd, nullptr, FALSE);
    } else if (cmdId > 0 && win) {
        // Send command to main window for processing
        SendMessage(win->hwndFrame, WM_COMMAND, cmdId, 0);
    }
}

void ThumbnailPanel::OnPaint(HDC hdc) {
    RECT rc;
    GetClientRect(hwnd, &rc);
    int width = rc.right - rc.left;
    int height = rc.bottom - rc.top;

    // Create double-buffer to prevent flicker
    HDC memDC = CreateCompatibleDC(hdc);
    HBITMAP memBitmap = CreateCompatibleBitmap(hdc, width, height);
    HBITMAP oldBitmap = (HBITMAP)SelectObject(memDC, memBitmap);

    // Fill background on memory DC
    HBRUSH bgBrush = CreateSolidBrush(GetSysColor(COLOR_WINDOW));
    FillRect(memDC, &rc, bgBrush);
    DeleteObject(bgBrush);

    if (items.IsEmpty()) {
        // Draw "No document" message
        SetTextColor(memDC, GetSysColor(COLOR_GRAYTEXT));
        SetBkMode(memDC, TRANSPARENT);
        DrawTextW(memDC, L"No document loaded", -1, &rc,
                  DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    } else {
        // Set up for drawing
        int savedDC = SaveDC(memDC);
        SetBkMode(memDC, TRANSPARENT);

        Rect visible = GetVisibleRect();

        // Get engine for rendering thumbnails on-demand
        EngineBase* engine = nullptr;
        if (dm) {
            engine = dm->GetEngine();
        }

        for (ThumbnailItem* item : items) {
            // Check if thumbnail is visible
            Rect itemRect = item->bounds;
            itemRect.y -= scrollPos;

            if (itemRect.y + itemRect.dy < 0 || itemRect.y > visible.dy) {
                continue;
            }

            // Render thumbnail on-demand if not yet available
            if (!item->thumbnail && engine && !item->isLoading) {
                // Calculate zoom to fit thumbnail size
                float zoom = (float)(thumbnailWidth - 4) / (float)engine->PageMediabox(item->pageNo).dx;
                RenderPageArgs args(item->pageNo, zoom, 0);
                item->thumbnail = engine->RenderPage(args);
            }

            bool isCurrentPage = (item->pageNo == currentPage);
            DrawThumbnail(memDC, item, isCurrentPage);
        }

        RestoreDC(memDC, savedDC);
    }

    // Copy from memory DC to screen
    BitBlt(hdc, 0, 0, width, height, memDC, 0, 0, SRCCOPY);

    // Cleanup
    SelectObject(memDC, oldBitmap);
    DeleteObject(memBitmap);
    DeleteDC(memDC);
}

void ThumbnailPanel::DrawThumbnail(HDC hdc, ThumbnailItem* item, bool isCurrentPage) {
    Rect bounds = item->bounds;
    bounds.y -= scrollPos;

    // Calculate thumbnail area (excluding page number)
    Rect thumbRect = bounds;
    thumbRect.dy -= kPageNumberHeight;

    // Available space with padding
    int availW = thumbRect.dx - 4;
    int availH = thumbRect.dy - 4;

    // Draw the thumbnail bitmap using simple Blit() - same pattern as popup test
    if (item->thumbnail && item->thumbnail->IsValid()) {
        Size bmpSize = item->thumbnail->GetSize();

        // Calculate scaled size preserving aspect ratio
        float scaleX = (float)availW / (float)bmpSize.dx;
        float scaleY = (float)availH / (float)bmpSize.dy;
        float scale = (scaleX < scaleY) ? scaleX : scaleY;

        int targetW = (int)(bmpSize.dx * scale);
        int targetH = (int)(bmpSize.dy * scale);

        // Center in available space
        int offsetX = (availW - targetW) / 2;
        int offsetY = (availH - targetH) / 2;

        Rect target(thumbRect.x + 2 + offsetX, thumbRect.y + 2 + offsetY,
                    targetW, targetH);

        // Use Blit directly - this is the pattern that works
        item->thumbnail->Blit(hdc, target);
    } else {
        // Fallback: draw placeholder if no bitmap
        DrawPlaceholder(hdc, item);
    }

    // Determine border color based on state:
    // - Selected: Green border (RGB 0, 180, 0)
    // - Current page (not selected): Blue border (RGB 0, 120, 215)
    // - Both selected AND current: Green border takes precedence (shows selection)
    bool needsBorder = item->isSelected || isCurrentPage;
    COLORREF borderColor = item->isSelected ? RGB(0, 180, 0) : RGB(0, 120, 215);

    if (needsBorder && item->thumbnail && item->thumbnail->IsValid()) {
        Size bmpSize = item->thumbnail->GetSize();
        float scaleX = (float)availW / (float)bmpSize.dx;
        float scaleY = (float)availH / (float)bmpSize.dy;
        float scale = (scaleX < scaleY) ? scaleX : scaleY;
        int targetW = (int)(bmpSize.dx * scale);
        int targetH = (int)(bmpSize.dy * scale);
        int offsetX = (availW - targetW) / 2;
        int offsetY = (availH - targetH) / 2;

        RECT r = {thumbRect.x + 2 + offsetX, thumbRect.y + 2 + offsetY,
                  thumbRect.x + 2 + offsetX + targetW, thumbRect.y + 2 + offsetY + targetH};
        HPEN borderPen = CreatePen(PS_SOLID, 3, borderColor);
        HPEN oldPen = (HPEN)SelectObject(hdc, borderPen);
        HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
        Rectangle(hdc, r.left, r.top, r.right, r.bottom);
        SelectObject(hdc, oldBrush);
        SelectObject(hdc, oldPen);
        DeleteObject(borderPen);
    } else if (needsBorder) {
        // Fallback border for placeholder
        RECT r = {thumbRect.x + 2, thumbRect.y + 2,
                  thumbRect.x + thumbRect.dx - 2, thumbRect.y + thumbRect.dy - 2};
        HPEN borderPen = CreatePen(PS_SOLID, 3, borderColor);
        HPEN oldPen = (HPEN)SelectObject(hdc, borderPen);
        HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
        Rectangle(hdc, r.left, r.top, r.right, r.bottom);
        SelectObject(hdc, oldBrush);
        SelectObject(hdc, oldPen);
        DeleteObject(borderPen);
    }

    // Draw page number
    DrawPageNumber(hdc, item);
}

void ThumbnailPanel::DrawPlaceholder(HDC hdc, ThumbnailItem* item) {
    Rect bounds = item->bounds;
    bounds.y -= scrollPos;
    bounds.dy -= kPageNumberHeight;

    // Draw light gray box
    HBRUSH placeholderBrush = CreateSolidBrush(RGB(240, 240, 240));
    RECT r = {bounds.x + 2, bounds.y + 2, bounds.x + bounds.dx - 2, bounds.y + bounds.dy - 4};
    FillRect(hdc, &r, placeholderBrush);
    DeleteObject(placeholderBrush);

    // Draw border
    HPEN pen = CreatePen(PS_SOLID, 1, RGB(200, 200, 200));
    HPEN oldPen = (HPEN)SelectObject(hdc, pen);
    HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
    Rectangle(hdc, r.left, r.top, r.right, r.bottom);
    SelectObject(hdc, oldBrush);
    SelectObject(hdc, oldPen);
    DeleteObject(pen);

    // Draw status in center
    SetTextColor(hdc, RGB(128, 128, 128));
    if (item->isLoading) {
        DrawTextA(hdc, "Loading...", -1, &r, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    } else {
        DrawTextA(hdc, "Not loaded", -1, &r, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }
}

void ThumbnailPanel::DrawPageNumber(HDC hdc, ThumbnailItem* item) {
    Rect bounds = item->bounds;
    bounds.y -= scrollPos;

    // Page number area at bottom of thumbnail
    RECT numRect = {
        bounds.x,
        bounds.y + bounds.dy - kPageNumberHeight,
        bounds.x + bounds.dx,
        bounds.y + bounds.dy
    };

    SetTextColor(hdc, GetSysColor(COLOR_WINDOWTEXT));
    char buf[32];
    sprintf_s(buf, sizeof(buf), "%d", item->pageNo);
    DrawTextA(hdc, buf, -1, &numRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}

// ========== Window Procedure ==========

LRESULT CALLBACK ThumbnailPanelWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    ThumbnailPanel* panel = (ThumbnailPanel*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

    switch (msg) {
        case WM_CREATE: {
            CREATESTRUCT* cs = (CREATESTRUCT*)lp;
            panel = (ThumbnailPanel*)cs->lpCreateParams;
            SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)panel);
            return 0;
        }

        case WM_SIZE:
            if (panel) {
                panel->OnSize(LOWORD(lp), HIWORD(lp));
            }
            return 0;

        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            if (panel) {
                panel->OnPaint(hdc);
            }
            EndPaint(hwnd, &ps);
            return 0;
        }

        case WM_VSCROLL:
            if (panel) {
                panel->OnVScroll(wp);
            }
            return 0;

        case WM_MOUSEWHEEL:
            if (panel) {
                panel->OnMouseWheel(GET_WHEEL_DELTA_WPARAM(wp));
            }
            return 0;

        case WM_LBUTTONDOWN:
            if (panel) {
                panel->OnLButtonDown(GET_X_LPARAM(lp), GET_Y_LPARAM(lp));
            }
            return 0;

        case WM_CONTEXTMENU:
            if (panel) {
                int x = GET_X_LPARAM(lp);
                int y = GET_Y_LPARAM(lp);
                // Handle keyboard-invoked context menu (x,y = -1,-1)
                if (x == -1 || y == -1) {
                    POINT pt;
                    GetCursorPos(&pt);
                    x = pt.x;
                    y = pt.y;
                }
                panel->ShowContextMenu(x, y);
            }
            return 0;

        case WM_KEYDOWN:
            if (panel) {
                // Handle Ctrl+A for Select All
                if (wp == 'A' && (GetAsyncKeyState(VK_CONTROL) & 0x8000)) {
                    panel->SelectAll();
                    InvalidateRect(hwnd, nullptr, FALSE);
                    return 0;
                }
            }
            break;

        case WM_ERASEBKGND:
            return 1; // Handled in WM_PAINT
    }

    return DefWindowProc(hwnd, msg, wp, lp);
}

void RegisterThumbnailPanelClass() {
    if (gThumbnailClassRegistered) {
        return;
    }

    WNDCLASSEXW wc = {0};
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = ThumbnailPanelWndProc;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = kThumbnailPanelClassName;

    if (RegisterClassExW(&wc)) {
        gThumbnailClassRegistered = true;
    }
}

void LayoutThumbnailContainer(LabelWithCloseWnd* l, HWND hwndPanel) {
    HWND hwndContainer = GetParent(hwndPanel);
    Size labelSize = l->GetIdealSize();
    Rect rc = WindowRect(hwndContainer);
    int dy = rc.dy;
    int y = 0;
    MoveWindow(l->hwnd, y, 0, rc.dx, labelSize.dy, TRUE);
    dy -= labelSize.dy;
    y += labelSize.dy;
    MoveWindow(hwndPanel, 0, y, rc.dx, dy, TRUE);
}

// ========== Subclass for container window ==========

static LRESULT CALLBACK WndProcThumbnailBox(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp,
                                            UINT_PTR subclassId, DWORD_PTR data) {
    MainWindow* win = (MainWindow*)data;
    if (!win) {
        return DefSubclassProc(hwnd, msg, wp, lp);
    }

    switch (msg) {
        case WM_SIZE:
            if (win->thumbnailPanel && win->thumbnailLabelWithClose) {
                LayoutThumbnailContainer(win->thumbnailLabelWithClose, win->thumbnailPanel->hwnd);
            }
            break;

        case WM_COMMAND:
            if (LOWORD(wp) == IDC_THUMBNAIL_LABEL_WITH_CLOSE) {
                // Close button clicked - hide thumbnails
                win->thumbnailsVisible = false;
                HwndSetVisibility(win->hwndThumbnailBox, false);
                // Update sidebar splitter visibility
                bool showSidebar = win->tocVisible || gGlobalPrefs->showFavorites;
                HwndSetVisibility(win->sidebarSplitter->hwnd, showSidebar);
                RelayoutWindow(win);
            }
            break;
    }
    return DefSubclassProc(hwnd, msg, wp, lp);
}

static void SubclassThumbnailBox(MainWindow* win) {
    if (win->thumbnailBoxSubclassId == 0) {
        win->thumbnailBoxSubclassId = NextSubclassId();
        BOOL ok = SetWindowSubclass(win->hwndThumbnailBox, WndProcThumbnailBox,
                                    win->thumbnailBoxSubclassId, (DWORD_PTR)win);
        ReportDebugIf(!ok);
    }
}

void UnsubclassThumbnailBox(MainWindow* win) {
    if (win->thumbnailBoxSubclassId != 0) {
        RemoveWindowSubclass(win->hwndThumbnailBox, WndProcThumbnailBox, win->thumbnailBoxSubclassId);
        win->thumbnailBoxSubclassId = 0;
    }
}

// ========== Public API functions ==========

void CreateThumbnailPanel(MainWindow* win) {
    HMODULE hmod = GetModuleHandle(nullptr);
    int dx = gGlobalPrefs->sidebarDx;
    DWORD style = WS_CHILD | WS_CLIPCHILDREN;
    HWND parent = win->hwndFrame;

    win->hwndThumbnailBox = CreateWindowExW(0, WC_STATIC, L"", style,
                                            0, 0, dx, 0, parent, nullptr, hmod, nullptr);

    // Create header with close button
    auto l = new LabelWithCloseWnd();
    {
        LabelWithCloseCreateArgs args;
        args.parent = win->hwndThumbnailBox;
        args.cmdId = IDC_THUMBNAIL_LABEL_WITH_CLOSE;
        args.font = GetDefaultGuiFont(true, false);
        l->Create(args);
    }
    win->thumbnailLabelWithClose = l;
    l->SetPaddingXY(2, 2);
    l->SetLabel(_TRA("Page Thumbnails"));

    // Create thumbnail panel
    win->thumbnailPanel = new ThumbnailPanel();
    win->thumbnailPanel->Create(win->hwndThumbnailBox, win);

    SubclassThumbnailBox(win);

    // logf("CreateThumbnailPanel: Created thumbnail panel for window\n");
}

void LoadThumbnailPanel(MainWindow* win) {
    if (!win || !win->thumbnailPanel) {
        return;
    }

    WindowTab* tab = win->CurrentTab();
    if (!tab || !tab->ctrl) {
        win->thumbnailPanel->Clear();
        return;
    }

    DisplayModel* dm = tab->ctrl->AsFixed();
    if (!dm) {
        win->thumbnailPanel->Clear();
        return;
    }

    win->thumbnailPanel->SetDisplayModel(dm);
}

void ToggleThumbnails(MainWindow* win) {
    if (!win) {
        return;
    }

    win->thumbnailsVisible = !win->thumbnailsVisible;
    // logf("ToggleThumbnails: thumbnailsVisible=%d\n", win->thumbnailsVisible);

    if (win->thumbnailsVisible) {
        // Mutual exclusion: hide ToC when showing thumbnails
        if (win->tocVisible) {
            win->tocVisible = false;
            HwndSetVisibility(win->hwndTocBox, false);
        }
        LoadThumbnailPanel(win);
    }

    HwndSetVisibility(win->hwndThumbnailBox, win->thumbnailsVisible);

    // Update sidebar splitter visibility
    bool showSidebar = win->thumbnailsVisible || win->tocVisible || gGlobalPrefs->showFavorites;
    HwndSetVisibility(win->sidebarSplitter->hwnd, showSidebar);

    RelayoutWindow(win);
}

void UpdateThumbnailSelection(MainWindow* win) {
    if (!win || !win->thumbnailPanel || !win->thumbnailsVisible) {
        return;
    }

    WindowTab* tab = win->CurrentTab();
    if (!tab || !tab->ctrl) {
        return;
    }

    int currentPage = tab->ctrl->CurrentPageNo();
    win->thumbnailPanel->SetCurrentPage(currentPage);
}
