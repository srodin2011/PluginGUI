#include "PluginGUI\include\Tooltip.h"
#include "PluginGUI\include\Utils.h"
#include <gdiplus.h>
#pragma comment(lib, "gdiplus.lib")

namespace PluginGUI
{
    using namespace Gdiplus;

    BEGIN_MESSAGE_MAP(CCustomTooltip, CWnd)
        ON_WM_CREATE()
        ON_WM_PAINT()
        ON_WM_DESTROY()
        ON_WM_ERASEBKGND()
        ON_WM_SIZE()
    END_MESSAGE_MAP()

    CCustomTooltip::CCustomTooltip() : bVisible(false), hParent(0) {}

    CCustomTooltip::~CCustomTooltip()
    {
        оffScreenBitmap.release();
        DestroyWindow();
    }

    bool CCustomTooltip::CreateTooltip(HWND hParentWnd)
    {
        hParent = hParentWnd;

        // 1. Регистрируем класс и ПОЛУЧАЕМ имя
        LPCTSTR lpszClassName = 
            AfxRegisterWndClass(
            CS_HREDRAW | CS_VREDRAW,
            ::LoadCursor(NULL, IDC_ARROW)
        );
            //AfxRegisterWndClass(
            //    CS_HREDRAW | CS_VREDRAW,
            //    ::LoadCursor(NULL, IDC_ARROW),
            //    (HBRUSH)GetStockObject(NULL_BRUSH),  // ★ ПРОЗРАЧНЫЙ фон ★
            //    NULL);

        // 2. Используем зарегистрированное имя
        return CreateEx(
            WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
            lpszClassName,  // ← ВОТ ЗДЕСЬ ИМЯ КЛАССА!
            _T("CustomTooltip"),
            WS_POPUP,
            CRect(0, 0, 1, 1),
            CWnd::FromHandle(hParentWnd),
            0
        ) != NULL;
    }

    int CCustomTooltip::OnCreate(LPCREATESTRUCT lpCreateStruct)
    {
        if (CWnd::OnCreate(lpCreateStruct) == -1) return -1;
        //SetLayeredWindowAttributes(RGB(0, 0, 0), 0, LWA_COLORKEY);
        //SetLayeredWindowAttributes(RGB(0, 0, 0), 0, LWA_COLORKEY);
        //SetLayeredWindowAttributes(0, 255, LWA_ALPHA);  // НЕ LWA_COLORKEY!
        return 0;
    }

    BOOL CCustomTooltip::OnEraseBkgnd(CDC*)
    {
        return TRUE;  // ✅ Отключаем стирание фона
    }

    void CCustomTooltip::OnSize(UINT nType, int cx, int cy)
    {
        CWnd::OnSize(nType, cx, cy);

        // Принудительная перерисовка
        //Invalidate(TRUE); // TRUE = стираем фон
        //UpdateWindow();
    }

    void CCustomTooltip::ShowForControl(const CRect& controlBorderScreen, const std::wstring& text)
    {
        currentText = text;
        //ShowWindow(SW_SHOWNOACTIVATE);
        //Invalidate(FALSE);

        CalculateSizeAndPositionForControl(controlBorderScreen);

        ShowWindow(SW_SHOWNOACTIVATE);
        Invalidate(true);
        UpdateWindow();

        bVisible = true;
    }

    void CCustomTooltip::Hide()
    {
        bVisible = false;
        ShowWindow(SW_HIDE);
    }

    void CCustomTooltip::UpdateForControl(const CRect& controlBorderScreen, const std::wstring& text)
    {
        if (!bVisible) return;

        currentText = text;
        CalculateSizeAndPositionForControl(controlBorderScreen);
        Invalidate(false);
        UpdateWindow();
    }

    int width_ = 0;
    int height_ = 0;

    void CCustomTooltip::CalculateSizeAndPositionForControl(const CRect& controlBorderScreen)
    {
        CWnd* pParent = CWnd::FromHandlePermanent(hParent);
        CRect rcParent;
        pParent->GetClientRect(&rcParent);
        pParent->ClientToScreen(&rcParent);

        // Размер по тексту с Gdiplus
        Graphics graphics(GetSafeHwnd());
        FontFamily fontFamily(L"Segoe UI");
        Font font(&fontFamily, 10, FontStyleRegular, UnitPoint);
        RectF textBounds;
        PointF origin(0, 0);

        std::wstring str = currentText.length() > 0 ? currentText.c_str() : L"";
        graphics.MeasureString(str.c_str(), -1, &font, origin, &textBounds);

        int len = currentText.length();
        int charW = textBounds.Width / len;
        int width = textBounds.Width + charW * 2;
        int height = (int)textBounds.Height * 1.5f;
        if(width==0)
        {
            width = 1;
        }
        if (height == 0)
        {
            height = 1;
        }

        //int width = (textBounds.Width / len) * (len + 2);
        //int height = (int)textBounds.Height * 1.5f;

        int x = controlBorderScreen.left + (controlBorderScreen.Width() - width) / 2;
        int y = controlBorderScreen.bottom + 5;

        if (x + width > rcParent.right - 10) x = rcParent.right - width - 10;
        if (x < rcParent.left + 10) x = rcParent.left + 10;

        if (y + height > rcParent.bottom - 10)
        {
            y = controlBorderScreen.top - height - 5;
        }

        if (y < rcParent.top + 10)
        {
            y = controlBorderScreen.bottom + 5;
        }

        //ShowWindow(SW_HIDE);  // 1. Скрыть (стирает старую область)
        //Invalidate(false);              // 4. Принудительно перерисовать себя

        CRect Border(x, y, x + width - 1, y + height - 1);
        SetBorder(Border);
        //SetWindowPos(&wndTopMost, x, y, width, height,
        //    SWP_NOACTIVATE | SWP_SHOWWINDOW | SWP_NOZORDER);
        width_ = width;
        height_ = height;

        //ShowWindow(SW_SHOWNOACTIVATE);  // 3. Показать новую
        //Invalidate(false);              // 4. Принудительно перерисовать себя
    }

    void CCustomTooltip::SetBorder(const CRect& border_)
    {
        if (Border != border_)
        {
            bool hideShowWindow = Border.Width() != border_.Width() || Border.Height() != border_.Height();

            TRACE(_T("Old border LT={%d,%d}-WH{%d,%d} New border LT={%d,%d}-WH{%d,%d}"),
                Border.left, Border.top, Border.Width(), Border.Height(),
                border_.left, border_.top, border_.Width(), border_.Height());

            if (hideShowWindow)
            {
                auto parent = GetParentOwner();
                parent->InvalidateRect(Border);
                parent->UpdateWindow();
            }

            if (Border.Width() != border_.Width() || Border.Height() != border_.Height())
            {
                if (оffScreenBitmap)
                {
                    оffScreenBitmap.reset();
                }
                оffScreenBitmap = std::make_unique<Gdiplus::Bitmap>(border_.Width(), border_.Height(), PixelFormat32bppARGB);
            }
            //if (hideShowWindow)
            //{
            //    //ShowWindow(SW_HIDE);
            //}

            SetWindowPos(&wndTopMost, border_.left, border_.top, border_.Width(), border_.Height(),
                SWP_NOACTIVATE | SWP_SHOWWINDOW | SWP_NOZORDER);

            Border = border_;
            if (hideShowWindow)
            {
                Invalidate(false);
                UpdateWindow();
            }
        }
    }

    void CCustomTooltip::OnPaint()
    {
        CPaintDC dc(this);
        CRect rc;
        GetClientRect(&rc);

        Graphics graphics(оffScreenBitmap.get());
        graphics.SetSmoothingMode(SmoothingModeAntiAlias);
        graphics.SetTextRenderingHint(TextRenderingHintClearTypeGridFit);

        graphics.Clear(Color::Transparent);

        float width = static_cast<float>(rc.Width());
        float height = static_cast<float>(rc.Height());
        float radius = 6.0f;
        float frameThickness = 2;

        // 1. ФОН - темно-синий градиент
        LinearGradientBrush bgGradient(
            RectF(0.0f, 0.0f, width - 1, height - 1),
            Color(255, 42, 48, 53),    // #1E2A44
            Color(255, 39, 44, 49),    // #0F1B2A
            LinearGradientModeVertical
        );
        GraphicsPath bgPath;
        bgPath.AddArc(0.0f, 0.0f, radius * 2.0f, radius * 2.0f, 180.0f, 90.0f);                    // Левый верхний
        bgPath.AddLine(radius, 0.0f, width - radius - 1.0f, 0.0f);                             // Верх (до правого угла)
        bgPath.AddArc(width - radius * 2.0f - 1.0f, 0.0f, radius * 2.0f, radius * 2.0f, 270.0f, 90.0f); // Правый верхний
        bgPath.AddLine(width - 1.0f, radius, width - 1.0f, height - radius - 1.0f); // Право
        bgPath.AddArc(width - radius * 2.0f - 1.0f, height - radius * 2.0f - 1.0f, radius * 2.0f, radius * 2.0f, 0.0f, 90.0f); // Правый нижний
        bgPath.AddLine(width - radius - 1.0f, height - 1.0f, radius, height - 1.0f); // Низ
        bgPath.AddArc(0.0f, height - radius * 2.0f - 1.0f, radius * 2.0f, radius * 2.0f, 90.0f, 90.0f); // Левый нижний        bgPath.CloseFigure();  // ✅ КРИТИЧНО!
        bgPath.CloseFigure();

        Pen framePen(Color(255, 0, 216, 255), 2.f);
        //graphics.DrawPath(&framePen, &bgPath);
        graphics.FillPath(&bgGradient, &bgPath);

        graphics.DrawArc(&framePen, frameThickness / 2.f, frameThickness / 2.f, radius * 2.f, radius * 2.f, 180.0f, 90.0f);
        graphics.DrawArc(&framePen, width - radius * 2.f - frameThickness / 2.f - 1, frameThickness / 2.f, radius * 2.f, radius * 2.f, 270.0f, 90.0f);
        graphics.DrawArc(&framePen, width - radius * 2.f - frameThickness / 2.f - 1, height - frameThickness / 2.f - radius * 2.0f - 1, radius * 2.f, radius * 2.f, 0.0f, 90.0f);
        graphics.DrawArc(&framePen, frameThickness / 2.f, height - radius * 2.f - frameThickness / 2.f - 1, radius * 2.f, radius * 2.f, 90.0f, 90.0f);
        
        // 4. ТЕКСТ
        FontFamily fontFamily(L"Segoe UI");
        Font titleFont(&fontFamily, 10.0f, FontStyleRegular, UnitPoint);
        SolidBrush whiteBrush(Color(255, 0, 216, 255));

        StringFormat stringFormat;
        stringFormat.SetAlignment(StringAlignmentCenter);
        stringFormat.SetLineAlignment(StringAlignmentCenter);
        stringFormat.SetTrimming(StringTrimmingEllipsisCharacter);

        RectF textBounds;
        PointF origin(0, 0);
        graphics.MeasureString(currentText.c_str(), -1, &titleFont, origin, &textBounds);

        //RectF textRect(16.0f, 8.0f, width - 32.0f, height - 16.0f);
        RectF textRect(
            (width - textBounds.Width) * 0.5f,        // X = центр по горизонтали
            (height - textBounds.Height) * 0.5f,      // Y = центр по вертикали  
            textBounds.Width,                         // Ширина текста
            textBounds.Height                         // Высота текста
        );
        // Белый текст поверх
        graphics.DrawString(currentText.c_str(), -1, &titleFont, textRect, &stringFormat, &whiteBrush);

        // 4. Копируем готовый bitmap на экран
        DrawBitmap(dc, оffScreenBitmap.get(), rc, true);
    }

    void CCustomTooltip::OnDestroy()
    {
        Hide();
        CWnd::OnDestroy();
    }
}