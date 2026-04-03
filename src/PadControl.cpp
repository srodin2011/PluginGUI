#include "PluginGUI\include\PadControl.h"
#include "PluginGUI\include\PluginView.h"
#include "PluginGUI\include\Utils.h"

using namespace Gdiplus;

namespace PluginGUI
{
    /// <summary>
    /// Конструктор класса
    /// </summary>
    /// <param name="pluginView">Ссылка плагин</param>
    /// <param name="border">Границы элемента управления</param>
    PadControl::PadControl(const CRect& Border) :
        Control(Border)
    {
    }

    static void DrawPad(Graphics& g, const RectF& rect, const Color& color)
    {
        Color borderColor(255, 30, 60, 90);
        Color shadowColor(100, 0, 0, 0);
        Color textColor(255, 255, 255, 255);

        int radius = 10;
        GraphicsPath path;
        CreateRoundedRectPath(path, rect, radius);

        // Создаем градиент для 3D эффекта
        Color topColor = Color(255, 120, 170, 220);
        Color bottomColor = Color(255, 70, 130, 180);

        SolidBrush brush(color);
        //LinearGradientBrush brush(rect, topColor, bottomColor, LinearGradientModeVertical);

        // Заполняем кнопку градиентом
        g.FillPath(&brush, &path);
    }

    void PadControl::Draw(bool hasFocus, bool drawSelected)
    {
        if (m_pContainer)
        {
            Graphics g(оffScreenBitmap.get());
            g.Clear(Color::Transparent);
            g.SetSmoothingMode(SmoothingModeHighQuality);

            if (Selected && !drawSelected) return;

            RectF border_(0, 0, Border.Width() - 1, Border.Height() - 1);
            DrawPad(g, border_, GetBaseColor());
            DrawBorder2(g);
        }
    }
}