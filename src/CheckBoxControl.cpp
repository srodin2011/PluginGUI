//#include "pch.h"
#include "PluginGUI\include\CheckBoxControl.h"
#include "PluginGUI\include\PluginView.h"
#include "PluginGUI\include\Animation.h"

using namespace Gdiplus;

namespace PluginGUI
{
    CheckBoxControl::CheckBoxControl(const CRect& Border) :
        Control(Border),
        m_ClickAnimationProgress(0.0f)
    {
        Init();
    }

    CheckBoxControl::~CheckBoxControl()
    {
    }

    void CheckBoxControl::Init()
    {
        m_OnColor = Color(255, 255, 255, 255);
        m_OffColor = Color(255, 0, 0, 0);
        m_TextColor = Color(255, 255, 255, 255);
        m_Text = L"Control";

        m_ClickAnimation = std::make_unique<Animation>(
            [&]() { // Start
            },
            [&](float progrees) { // Update
                m_ClickAnimationProgress = progrees;
            },
            [&]() { // Stop
                SetOn(!m_On);
            });

        // Задаем функцию инвалидации по умолчанию
        m_ClickAnimation->SetInvalidateCallback([&]() {
            Invalidate();
            });
    }

    // Вспомогательная функция для добавления окружности с направлением обхода
    void AddCircle(GraphicsPath& path, float cx, float cy, float r, bool clockwise) {
        if (clockwise)
            path.AddArc(cx - r, cy - r, 2 * r, 2 * r, 0, 360);
        else
            path.AddArc(cx - r, cy - r, 2 * r, 2 * r, 360, -360);
    }

    // Нарисовать красную кнопку, похожую на фото
    void DrawRedStopButton(Graphics& graphics, int centerX, int centerY, int radius)
    {
        graphics.SetSmoothingMode(SmoothingModeAntiAlias);

        // 1. Радиальный градиент: светлый красный вверху, чуть темнее по краям
        GraphicsPath path;
        path.AddEllipse(centerX - radius, centerY - radius, radius * 2, radius * 2);

        PathGradientBrush brush(&path);
        // Светлый центр ближе к верхнему краю (имитация блика)
        brush.SetCenterPoint(PointF((REAL)(centerX - radius * 0.3), (REAL)(centerY - radius * 0.4)));
        brush.SetCenterColor(Color(255, 255, 90, 90)); // Светло-красный
        Color surroundColors[] = { Color(255, 160, 30, 30) }; // Тёмно-красный по краям
        INT count = 1;
        brush.SetSurroundColors(surroundColors, &count);

        graphics.FillPath(&brush, &path);

        // 2. Блик — мягкий эллипс, очень прозрачный, смещён вверх-влево
        int hlWidth = radius * 1.1;
        int hlHeight = radius * 0.5;
        SolidBrush highlightBrush(Color(60, 255, 255, 255)); // Очень прозрачный белый
        graphics.FillEllipse(
            &highlightBrush,
            centerX - radius * 0.7,
            centerY - radius * 0.8,
            hlWidth,
            hlHeight
        );

        // 3. Лёгкая тёмная обводка
        Pen borderPen(Color(200, 100, 20, 20), 2);
        graphics.DrawEllipse(&borderPen, centerX - radius, centerY - radius, radius * 2, radius * 2);
    }

    void DrawIndentedRoundButton(Graphics& graphics, int centerX, int centerY, int radius)
    {
        graphics.SetSmoothingMode(SmoothingModeAntiAlias);

        // 1. Основной круг — насыщенно-красный
        SolidBrush redBrush(Color(255, 200, 40, 40));
        graphics.FillEllipse(&redBrush, centerX - radius, centerY - radius, radius * 2, radius * 2);

        // 2. Мягкая тень по краю (размытый ободок)
        int shadowRadius = radius + 4;
        SolidBrush shadowBrush(Color(80, 60, 0, 0)); // Тёмно-красный с прозрачностью
        graphics.FillEllipse(&shadowBrush, centerX - shadowRadius, centerY - shadowRadius, shadowRadius * 2, shadowRadius * 2);

        // 3. Блик — вытянутый эллипс, смещён вверх
        int hlWidth = radius * 1.2;
        int hlHeight = radius * 0.5;
        SolidBrush highlightBrush(Color(120, 255, 255, 255)); // Полупрозрачный белый
        graphics.FillEllipse(
            &highlightBrush,
            centerX - hlWidth / 2,
            centerY - radius * 0.7,
            hlWidth,
            hlHeight
        );

        // 4. Обводка кнопки
        Pen borderPen(Color(180, 60, 0, 0), 2);
        graphics.DrawEllipse(&borderPen, centerX - radius, centerY - radius, radius * 2, radius * 2);
    }



    // Теперь функция принимает толщины окружностей как параметры
    void DrawButton(Graphics& g, float x, float y, float width, float height, bool isActive) 
    {
        float outerThickness = 1.5f;
        float innerThickness = 2.f;
        float circleBorderThickness = 1.f;

        float size = min(width, height);
        float cx = x + width / 2.0f;
        float cy = y + height / 2.0f;

        g.SetSmoothingMode(SmoothingModeAntiAlias);

        // --- Внешнее светло-серое кольцо ---
        float outerR1 = size / 2.0f;
        float outerR2 = outerR1 - outerThickness;
        GraphicsPath outerRing;
        AddCircle(outerRing, cx, cy, outerR1, true);    // Внешняя окружность по часовой
        AddCircle(outerRing, cx, cy, outerR2, false);   // Внутренняя окружность против часовой
        outerRing.SetFillMode(FillModeAlternate);
        SolidBrush lightBrush(Color(255, 130, 130, 130));
        g.FillPath(&lightBrush, &outerRing);

        // --- Внутреннее темно-серое кольцо ---
        float innerR1 = outerR2;
        float innerR2 = innerR1 - innerThickness;
        GraphicsPath innerRing;
        AddCircle(innerRing, cx, cy, innerR1, false);   // Внешняя окружность против часовой
        AddCircle(innerRing, cx, cy, innerR2, true);    // Внутренняя окружность по часовой
        innerRing.SetFillMode(FillModeAlternate);
        SolidBrush darkBrush(Color(255, 50, 50, 50));
        g.FillPath(&darkBrush, &innerRing);

        // --- Внутренний круг с вертикальным градиентом ---
        float fillR = innerR2;
        float fillX = cx - fillR;
        float fillY = cy - fillR;
        float fillSize = 2.0f * fillR;
        GraphicsPath fillPath;
        fillPath.AddEllipse(fillX, fillY, fillSize, fillSize);

        Color gradTop = isActive ? Color(255, 255, 210, 170) : Color(255, 140, 140, 140);
        Color gradBottom = isActive ? Color(255, 210, 90, 40) : Color(255, 70, 70, 70);

        LinearGradientBrush brush(
            PointF(fillX, fillY),
            PointF(fillX, fillY + fillSize),
            gradTop,
            gradBottom
        );
        g.FillPath(&brush, &fillPath);

        //float size = min(width, height);
        //float cx = x + width / 2.0f;
        //float cy = y + height / 2.0f;

        //g.SetSmoothingMode(SmoothingModeAntiAlias);

        //// --- Внешнее светло-серое кольцо ---
        //float outerR1 = size / 2.0f;
        //float outerR2 = outerR1 - outerThickness;
        //GraphicsPath outerRing;
        //AddCircle(outerRing, cx, cy, outerR1, true);
        //AddCircle(outerRing, cx, cy, outerR2, false);
        //outerRing.SetFillMode(FillModeAlternate);
        //SolidBrush lightBrush(Color(255, 130, 130, 130));
        ////g.FillPath(&lightBrush, &outerRing);

        //// --- Внутреннее темно-серое кольцо ---
        //float innerR1 = outerR2;
        //float innerR2 = innerR1 - innerThickness;
        //GraphicsPath innerRing;
        //AddCircle(innerRing, cx, cy, innerR1, false);
        //AddCircle(innerRing, cx, cy, innerR2, true);
        //innerRing.SetFillMode(FillModeAlternate);
        //SolidBrush darkBrush(Color(255, 50, 50, 50));
        ////g.FillPath(&darkBrush, &innerRing);

        //DrawRedStopButton(g, cx, cy, innerR2);


        /*
        // --- Внутренний круг с градиентной границей и разнонаправленными градиентами ---
        float borderR1 = innerR2;
        float borderR2 = borderR1 - circleBorderThickness;

        // Градиент для границы (например, сверху вниз)
        GraphicsPath circleBorderPath;
        AddCircle(circleBorderPath, cx, cy, borderR1, true);
        AddCircle(circleBorderPath, cx, cy, borderR2, false);
        circleBorderPath.SetFillMode(FillModeAlternate);

        Color borderGradBottom = isActive ? Color(255, 255, 220, 180) : Color(255, 170, 170, 170);
        Color borderGradTop = isActive ? Color(255, 255, 140, 80) : Color(255, 100, 100, 100);

        LinearGradientBrush borderBrush(
            PointF(cx, cy - borderR1),
            PointF(cx, cy + borderR1),
            borderGradTop,
            borderGradBottom
        );
        g.FillPath(&borderBrush, &circleBorderPath);

        // Градиент для внутренней части круга (например, снизу вверх)
        float fillR = borderR2;
        float fillX = cx - fillR;
        float fillY = cy - fillR;
        float fillSize = 2.0f * fillR;
        GraphicsPath fillPath;
        fillPath.AddEllipse(fillX, fillY, fillSize, fillSize);

        Color fillGradTop = isActive ? Color(255, 255, 220, 180) : Color(255, 170, 170, 170);
        Color fillGradBottom = isActive ? Color(255, 255, 140, 80) : Color(255, 100, 100, 100);

        // Градиент снизу вверх (меняем местами top и bottom)
        LinearGradientBrush fillBrush(
            PointF(cx, cy + fillR),
            PointF(cx, cy - fillR),
            borderGradBottom,
            borderGradTop
        );
        g.FillPath(&fillBrush, &fillPath);
        */
    }

    void CheckBoxControl::DrawCustomCheckBox(
        Graphics& g,               // Контекст GDI+
        int width, int height,     // Общий размер области
        const WCHAR* text,         // Текст
        Color highlightColor       // Цвет подсветки кнопки
    )
    {
        // Размеры "кнопки"
        int buttonWidth = 10;
        int buttonHeight = 25;//18;
        int buttonY = (height - buttonHeight) / 2;
        int buttonX = 4;

        //// Рисуем фон (тёмно-серый)
        //SolidBrush bgBrush(Color(255, 62, 71, 82)); // #3E4752
        //g.FillRectangle(&bgBrush, x, y, width, height);

        //DrawButton(g, buttonX, buttonY, min(buttonHeight, buttonHeight), min(buttonHeight, buttonHeight), true);
        
        // Рисуем кнопку с подсветкой
        SolidBrush buttonBrush(highlightColor);
        g.FillRectangle(&buttonBrush, buttonX, buttonY, buttonWidth, buttonHeight);

        // Чёрная рамка вокруг кнопки
        Pen buttonBorderPen(Color(255, 0, 0, 0), 1);
        g.DrawRectangle(&buttonBorderPen, buttonX, buttonY, buttonWidth, buttonHeight);
        
        // Рисуем текст справа от кнопки
        FontFamily fontFamily(L"Segoe UI");
        Gdiplus::Font font(&fontFamily, 12, FontStyleRegular, UnitPixel);
        SolidBrush textBrush(Color(255, 255, 255, 255)); // Белый
        int textX = buttonX + buttonWidth + 8;
        int textY = (height - 16) / 2; // Вертикальное выравнивание

        g.DrawString(text, -1, &font, PointF((REAL)textX, (REAL)textY), &textBrush);
    }

    void CheckBoxControl::Draw(bool hasFocus, bool drawSelected)
    {
        if (m_pContainer)
        {
            Graphics g(оffScreenBitmap.get());
            g.Clear(Color::Transparent);
            g.SetSmoothingMode(SmoothingModeHighQuality);

            if (Selected && !drawSelected) return;

            std::wstring wstr = std::wstring(m_Text.begin(), m_Text.end());
            const WCHAR* wchars = wstr.c_str();  // Готово!

            if (m_ClickAnimation->AnimationStarted())
            {
                Color startColor = m_ClickAnimation->AnimatingForward ? m_OffColor : m_OnColor;
                Color endColor = m_ClickAnimation->AnimatingForward ? m_OnColor : m_OffColor;

                if (m_ClickAnimationProgress * 10 > 5)
                {
                    Color col0 = InterpolateColors(Color(255, 0, 0, 0), Color(255, 255, 255, 255), 0.f);
                    Color col1 = InterpolateColors(Color(255, 0, 0, 0), Color(255, 255, 255, 255), 0.25f);
                    Color col2 = InterpolateColors(Color(255, 0, 0, 0), Color(255, 255, 255, 255), 0.5f);
                    Color col3 = InterpolateColors(Color(255, 0, 0, 0), Color(255, 255, 255, 255), 0.75f);
                    Color col4 = InterpolateColors(Color(255, 0, 0, 0), Color(255, 255, 255, 255), 1.f);
                }
                Color color = InterpolateColors(startColor, endColor, m_ClickAnimation->AnimatingForward ? m_ClickAnimationProgress : 1 - m_ClickAnimationProgress);
                TRACE("startColor ARGB: A=%u, R=%u, G=%u, B=%u\n", startColor.GetA(), startColor.GetR(), startColor.GetG(), startColor.GetB());
                TRACE("m_AnimatingForward: %u; m_AnimationProgress: %f; Color ARGB: A=%u, R=%u, G=%u, B=%u\n", m_ClickAnimation->AnimatingForward, m_ClickAnimationProgress, color.GetA(), color.GetR(), color.GetG(), color.GetB());
                DrawCustomCheckBox(g, Border.Width(), Border.Height(), wchars, color);
            }
            else
            {
                DrawCustomCheckBox(g, Border.Width(), Border.Height(), wchars, m_On ? m_OnColor : m_OffColor);
            }
            DrawBorder2(g);
        }
    }

    void CheckBoxControl::OnLButtonDown(UINT, CPoint p)
    {
        m_ClickAnimation->StartAnimation(!m_On);

        //Invalidate();
    }

    //void CheckBoxControl::OnStopAnimation()
    //{
    //    SetOn(!m_On);
    //}
}