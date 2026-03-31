#include "PluginGUI\include\Utils.h"

namespace PluginGUI
{
	/// <summary>
	/// Округлить действительное число до заданного знака после запятой
	/// </summary>
	/// <param name="value">Дейстрительное число</param>
	/// <param name="decimals">Позиций полсе запятой</param>
	/// <returns></returns>
    float roundToDecimals(float value, int decimals)
	{
		_ASSERT(decimals >= 0);

        float factor = std::pow(10.0, decimals);
		return std::round(value * factor) / factor;
	}

    /// <summary>
    /// Вывод bitmap на экран. 
    /// Есть два режима:
    /// 1.С использованием AlphaBlend. useLayered = false.
    /// - SetLayeredWindowAttributes(RGB(0, 0, 0), 0, LWA_COLORKEY); черный цвет объявляется цветом прозрачности
    /// - Очистка окна "прозрачным" цветом: graphics.Clear(Color(0, 0, 0));
    /// 2.С использованием UpdateLayeredWindow. useLayered = true
    /// - SetLayeredWindowAttributes - убрать
    /// - Очистку окна делать явно не нужно
    /// </summary>
    /// <param name="dc">Графический контекст окна отрисовки</param>
    /// <param name="bitmap">Bitmap для отрисовки</param>
    /// <param name="border">Границы отрисовки в координатах окна</param>
    /// <param name="useLayered"></param>
    void DrawBitmap(CPaintDC& dc, Gdiplus::Bitmap* bitmap, const CRect& Border, bool useLayered)
    {
        if (!bitmap)
        {
            return;
        }

        UINT width = bitmap->GetWidth();
        UINT height = bitmap->GetHeight();

        BITMAPINFO bmi = { 0 };
        bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth = width;
        bmi.bmiHeader.biHeight = -(LONG)height;
        bmi.bmiHeader.biPlanes = 1;
        bmi.bmiHeader.biBitCount = 32;
        bmi.bmiHeader.biCompression = BI_RGB;

        void* pBits = nullptr;

        CDC memDC;
        memDC.CreateCompatibleDC(&dc);

        HBITMAP hDIB = CreateDIBSection(memDC.GetSafeHdc(), &bmi, DIB_RGB_COLORS, &pBits, NULL, 0);
        if (!hDIB || !pBits)
        {
            return;
        }

        HBITMAP hOldBitmap = (HBITMAP)memDC.SelectObject(hDIB);

        // Копируем данные из Gdiplus::Bitmap в DIBSection (БЕЗ ИЗМЕНЕНИЙ!)
        Gdiplus::BitmapData bitmapData;
        Gdiplus::Rect rect(0, 0, width, height);

        if (bitmap->LockBits(&rect, Gdiplus::ImageLockModeRead, PixelFormat32bppARGB, &bitmapData) == Gdiplus::Ok)
        {
            BYTE* srcPixels = (BYTE*)bitmapData.Scan0;
            BYTE* dstPixels = (BYTE*)pBits;
            int srcStride = bitmapData.Stride;
            int dstStride = width * 4;

            for (UINT y = 0; y < height; y++)
            {
                BYTE* srcRow = srcPixels + y * srcStride;
                BYTE* dstRow = dstPixels + y * dstStride;

                for (UINT x = 0; x < width; x++)
                {
                    BYTE B = srcRow[x * 4 + 0];
                    BYTE G = srcRow[x * 4 + 1];
                    BYTE R = srcRow[x * 4 + 2];
                    BYTE A = srcRow[x * 4 + 3];

                    float alpha = A / 255.0f;
                    BYTE Rp = static_cast<BYTE>(R * alpha);
                    BYTE Gp = static_cast<BYTE>(G * alpha);
                    BYTE Bp = static_cast<BYTE>(B * alpha);

                    dstRow[x * 4 + 0] = Bp; // Blue
                    dstRow[x * 4 + 1] = Gp; // Green
                    dstRow[x * 4 + 2] = Rp; // Red
                    dstRow[x * 4 + 3] = A;  // Alpha
                }
            }
            bitmap->UnlockBits(&bitmapData);
        }
        else
        {
            memset(pBits, 0, width * height * 4);
        }

        if (!useLayered)
        {
            // ✅ AlphaBlend (как раньше)
            BLENDFUNCTION blend = { 0 };
            blend.BlendOp = AC_SRC_OVER;
            blend.SourceConstantAlpha = 255;
            blend.AlphaFormat = AC_SRC_ALPHA;

            dc.AlphaBlend(Border.left, Border.top, Border.Width(), Border.Height(), &memDC, 0, 0, width, height, blend);
        }
        else
        {
            // ✅ UpdateLayeredWindow
            HWND hWnd = WindowFromDC(dc.GetSafeHdc());
            CWnd* pWnd = CWnd::FromHandle(hWnd);
            HDC hdcScreen = GetDC(NULL);

            CPoint ptScreen = Border.TopLeft();
            pWnd->ClientToScreen(&ptScreen);

            POINT ptZero = { 0, 0 };
            POINT ptDest = { ptScreen.x, ptScreen.y };
            SIZE size = { (LONG)width, (LONG)height };
            BLENDFUNCTION blend = { 0 };
            blend.BlendOp = AC_SRC_OVER;
            blend.SourceConstantAlpha = 255;
            blend.AlphaFormat = AC_SRC_ALPHA;

            UpdateLayeredWindow(hWnd, hdcScreen, &ptDest, &size,
                memDC.GetSafeHdc(), &ptZero, 0, &blend, ULW_ALPHA);

            ReleaseDC(NULL, hdcScreen);
        }

        TRACE("Bitmap width=%d, height=%d; Border width=%d, height=%d\n", width, height, Border.Width(), Border.Height());

        memDC.SelectObject(hOldBitmap);
        DeleteObject(hDIB);
    }

    /// <summary>
    /// Нарисовать прямоугольник со скругленными углами
    /// </summary>
    /// <param name="path">GraphicsPath</param>
    /// <param name="rect">Границы прмоугольника</param>
    /// <param name="radius">Радиус скругления угла</param>
    void CreateRoundedRectPath(Gdiplus::GraphicsPath& path, const Gdiplus::RectF& rect, int radius)
    {
        float diameter = radius * 2;
        path.Reset();

        // Верхний левый угол
        path.AddArc(rect.X, rect.Y, diameter, diameter, 180, 90);
        // Верхняя линия
        path.AddLine(rect.X + radius, rect.Y, rect.GetRight() - radius, rect.Y);
        // Верхний правый угол
        path.AddArc(rect.GetRight() - diameter, rect.Y, diameter, diameter, 270, 90);
        // Правая линия
        path.AddLine(rect.GetRight(), rect.Y + radius, rect.GetRight(), rect.GetBottom() - radius);
        // Нижний правый угол
        path.AddArc(rect.GetRight() - diameter, rect.GetBottom() - diameter, diameter, diameter, 0, 90);
        // Нижняя линия
        path.AddLine(rect.GetRight() - radius, rect.GetBottom(), rect.X + radius, rect.GetBottom());
        // Нижний левый угол
        path.AddArc(rect.X, rect.GetBottom() - diameter, diameter, diameter, 90, 90);
        // Левая линия
        path.AddLine(rect.X, rect.GetBottom() - radius, rect.X, rect.Y + radius);

        path.CloseFigure();
    }

    void CreateRoundedRectPath(Gdiplus::GraphicsPath& path, const Gdiplus::RectF& rect, int radius, CornerMask cornerMask)
    {
        if (cornerMask == CornerMask::None)
        {
            path.AddRectangle(rect);
            path.CloseFigure();
            return;
        }

        float diameter = radius * 2;
        path.Reset();

        float x = rect.X, y = rect.Y;
        float right = rect.GetRight(), bottom = rect.GetBottom();

        UINT mask = static_cast<UINT>(cornerMask);
        bool tl = (mask & static_cast<UINT>(CornerMask::TopLeft));
        bool tr = (mask & static_cast<UINT>(CornerMask::TopRight));
        bool br = (mask & static_cast<UINT>(CornerMask::BottomRight));
        bool bl = (mask & static_cast<UINT>(CornerMask::BottomLeft));

        // 1. Верхний левый угол
        if (tl)
        {
            path.AddArc(x, y, diameter, diameter, 180, 90);
        }
        // Иначе ничего - переходим сразу к верхней линии

        // 2. Верхняя линия  
        path.AddLine(
            x + (tl ? radius : 0),
            y,
            right - (tr ? radius : 0),
            y
        );

        // 3. Верхний правый угол
        if (tr)
        {
            path.AddArc(right - diameter, y, diameter, diameter, 270, 90);
        }

        // 4. Правая линия
        path.AddLine(
            right,
            y + (tr ? radius : 0),
            right,
            bottom - (br ? radius : 0)
        );

        // 5. Нижний правый угол
        if (br)
        {
            path.AddArc(right - diameter, bottom - diameter, diameter, diameter, 0, 90);
        }

        // 6. Нижняя линия
        path.AddLine(
            right - (br ? radius : 0),
            bottom,
            x + (bl ? radius : 0),
            bottom
        );

        // 7. Нижний левый угол
        if (bl)
        {
            path.AddArc(x, bottom - diameter, diameter, diameter, 90, 90);
        }

        // 8. Левая линия
        path.AddLine(
            x,
            bottom - (bl ? radius : 0),
            x,
            y + (tl ? radius : 0)
        );

        path.CloseFigure();
    }
}

