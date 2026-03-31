#include "PluginGUI\include\LableControl.h"
#include "PluginGUI\include\PluginView.h"
#include "PluginGUI\include\Utils.h"

using namespace Gdiplus;

namespace PluginGUI
{
    PLUGINGUI_PROPERTY_TABLE_INSTANTIATE(LableControl)

    LableControl::LableControl(const CRect& Border, const std::wstring& text) :
        Control(Border)
    {
        Text = text;
        Init();
    }

    LableControl::~LableControl()
    {
    }

    void LableControl::Init()
    {
    }

    void LableControl::OnMouseMove(UINT nFlags, CPoint p)
    {
        bool wasHover = m_bHover;
        m_bHover = HitTest(p.x, p.y);

        if (m_bHover != wasHover)
        {
            Invalidate();
        }
        Control::OnMouseMove(nFlags, p);
    }

    void LableControl::OnMouseLeave()
    {
        if (m_bHover)
        {
            m_bHover = false;
            Invalidate();
        }
        Control::OnMouseLeave();
    }

    void LableControl::DrawLable(
        Graphics& g,               // Контекст GDI+
        int width, int height,     // Общий размер области
        const WCHAR* text)
    {
        RectF rect(0, 0, Border.Width() - 1, Border.Height() - 1);

        // Фон
        GraphicsPath path;
        SolidBrush backBrush(m_bHover? HoverBackColor : BackColor);
        CreateRoundedRectPath(path, rect, СornerRoundRadius, CornerStyle);
        g.FillPath(&backBrush, &path);

        // Текс
        FontFamily fontFamily(FontName.c_str());
        Gdiplus::Font font(&fontFamily, FontSize, FontStyleRegular, UnitPoint);
        SolidBrush textBrush(m_bHover ? HoverColor : TextColor); // Белый

        // Настройка выравнивания
        StringFormat stringFormat;
        stringFormat.SetTrimming(StringTrimming::StringTrimmingWord);  // Обрезает слова

        switch (TextAlignmentH)
        {
            case TextAlignmentStyle::Near:
                stringFormat.SetAlignment(StringAlignmentNear);
                break;

            case TextAlignmentStyle::Center:
                stringFormat.SetAlignment(StringAlignmentCenter);
                break;

            case TextAlignmentStyle::Far:
                stringFormat.SetAlignment(StringAlignmentFar);
                break;
        }
        switch (TextAlignmentV)
        {
            case TextAlignmentStyle::Near:
                stringFormat.SetLineAlignment(StringAlignmentNear);
                break;

            case TextAlignmentStyle::Center:
                stringFormat.SetLineAlignment(StringAlignmentCenter);
                break;

            case TextAlignmentStyle::Far:
                stringFormat.SetLineAlignment(StringAlignmentFar);
                break;
        }

        g.DrawString(text, -1, &font, rect, &stringFormat, &textBrush);
    }

    void LableControl::Draw(bool hasFocus, bool drawSelected)
    {
        if (m_pContainer)
        {
            Graphics g(оffScreenBitmap.get());
            g.Clear(Color::Transparent);
            g.SetSmoothingMode(SmoothingModeHighQuality/*SmoothingModeAntiAlias*/);

            if (Selected && !drawSelected) return;

            //std::wstring wstr = std::wstring(m_Text.begin(), m_Text.end());
            //const WCHAR* wchars = Text.c_str();  // Готово!

            DrawLable(g, Border.Width(), Border.Height(), Text.c_str());
            DrawBorder2(g);
        }
    }
}
