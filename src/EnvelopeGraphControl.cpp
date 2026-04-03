#define NOMINMAX
#include "PluginGUI\include\EnvelopeGraphControl.h"
#include "PluginGUI\include\PluginView.h"
#include "PluginGUI\include\commands.h"
#include "PluginGUI\include\Utils.h"
#include "resource.h"
#include <algorithm>
#include <numbers>
#include <fstream>
#include <filesystem>  
#include <ranges>

namespace PluginGUI
{
    EnvelopeGraphControl::EnvelopeGraphControl(const CRect& Border)
        : Control(Border)
    {
        // Начальные точки по умолчанию (как ADSR)
        //m_allPoints = {
        //    {0.0f, 1.f, true},
        //    {1.0f, 0.0f, true}
        //};
        //m_allPoints = {
        //    {0.0f, 0.0f, true},
        //    {0.25f, 0.5f, false, 0.f, 0.f, CurveType::Bezier},
        //    {0.5f, 1.0f, true},
        //    {0.75f, 0.5f, false, 0.f, 0.f, CurveType::Power},
        //    {1.0f, 0.f, true}
        //};
        m_allPoints = {
            Point(0.0f, 0.0f),
            Point(0.25f, 0.5f, 0.f, 0.f, CurveType::Power),
            Point(0.5f, 1.0f),
            Point(0.75f, 0.5f, 0.f, 0.f, CurveType::Power),
            Point(1.0f, 0.f)
        };

        //m_allPoints = {
        //    {0.0f, 0.0f, true},
        //    {0.25f, 1.0f, true},
        //    {0.5f, 1.0f, true},
        //    {0.75f, 0.0f, true},
        //    {1.0f, 0.0f, true}
        //};
        RecalculatePoints();
    }

    void EnvelopeGraphControl::SetPoints(std::vector<Point> points)  // by value
    {
        m_allPoints = std::move(points);  
        RecalculatePoints();              
        Invalidate();                     
    }

    /// <summary>
    /// Нужно обрабатывать следующие кейсы:
    /// 1.Если удалили главную точку, то нужно удалить соседние промежуточные точки
    /// 2.Если между двумя главными точками нет промежуточной, ее необходимо добавить
    /// </summary>
    void EnvelopeGraphControl::RecalculatePoints()
    {
        // 1. Убираем лишние промежуточные точки
        std::vector<Point> newPoints;
        newPoints.reserve(m_allPoints.size());

        size_t i = 0;
        while (i < m_allPoints.size())
        {
            if (m_allPoints[i].isMain)
            {
                newPoints.push_back(m_allPoints[i]);  // Сохраняем главную
                ++i;

                // Проверяем промежуточные до следующей главной
                size_t interCount = 0;
                while (i < m_allPoints.size() && !m_allPoints[i].isMain)
                {
                    ++interCount;
                    ++i;
                }

                // Если была 1 промежуточная точка, возвращаем её
                if (interCount == 1)
                {
                    newPoints.push_back(m_allPoints[i - 1]);  // Последняя промежуточная
                }
                // 0 или 2+ → ничего не добавляем (все удалены)
            }
            else
            {
                // Изолированная промежуточная - удаляем
                ++i;
            }
        }

        m_allPoints.swap(newPoints);

        // 2. Добавляем недостающие промежуточные
        for (size_t i = 0; i + 1 < m_allPoints.size(); )
        {
            if (m_allPoints[i].isMain && m_allPoints[i + 1].isMain)
            {
                // Две основные подряд - нужна промежуточная
                size_t nextNonMain = i + 1;
                while (nextNonMain < m_allPoints.size() && !m_allPoints[nextNonMain].isMain)
                {
                    ++nextNonMain;
                }

                if (nextNonMain == i + 1)
                {
                    // Нет промежуточной - добавляем середину
                    Point mid;
                    RestoreP1ByRelative(mid, m_allPoints[i], m_allPoints[i + 1]);
                    mid.isMain = false;

                    m_allPoints.insert(m_allPoints.begin() + i + 1, mid);
                }
                i = nextNonMain;
            }
            else
            {
                ++i;
            }
        }
    }

    /// <summary>
    /// Передвинуть точку
    /// </summary>
    /// <param name="index">Индекс передвигаемой точки</param>
    /// <param name="p">Новая точка, в которую перемещаемся</param>
    //void EnvelopeGraphControl::MovePoint(int index, Point p, bool recursion)
    void EnvelopeGraphControl::MovePoint(int index, const Point& p, bool recursion)
    {
        ASSERT(index >= 0 && index < m_allPoints.size());

        Point& pMoved = m_allPoints[index];

        if (pMoved.isMain)
        {
            int prevMainIndex = index - 2 >= 0 ? index - 2 : -1;
            int nextMainIndex = index + 2 < (int)m_allPoints.size() ? index + 2 : -1;
            int prevMidIndex = index - 2 >= 0 ? index -1 : -1;
            int nextMidIndex = index + 2 < (int)m_allPoints.size() ? index + 1 : -1;

            Point pNew = p;
            // y ограничен [0,1] для всех главных точек
            pNew.y = std::clamp(p.y, 0.f, m_graphH);

            if (index == 0) // Левая главная точка
            {
                // x не двигается
                pNew.x = 0;
                if (SynchronizeEdgeValues && !recursion)
                {
                    const Point& lastMainPoint = m_allPoints.back();
                    int lastMainIndex = (int)m_allPoints.size() - 1;
                    MovePoint(lastMainIndex, Point(lastMainPoint.x, pNew.y), true);
                }
            }
            else if (index == m_allPoints.size() - 1) // Правая главная точка
            {
                if (!FixedRightPoint)
                {
                    // x ограничен [x левой главной точки,1] 
                    pNew.x = std::clamp(p.x, m_allPoints[prevMainIndex].x, m_graphW);
                }
                else
                {
                    pNew.x = pMoved.x;
                }

                if (SynchronizeEdgeValues && !recursion)
                {
                    const Point& firstMainPoint = m_allPoints.at(0);
                    int firstMainIndex = 0;
                    MovePoint(firstMainIndex, Point(firstMainPoint.x, pNew.y), true);
                }
            }
            else // Промежуточная главная точка
            {
                if (prevMainIndex != -1 && nextMainIndex != -1)
                {
                    // x ограничен [x левой главной точки,x правой главной точки]
                    pNew.x = std::clamp(p.x, m_allPoints[prevMainIndex].x, m_allPoints[nextMainIndex].x);
                }
            }

            if (prevMainIndex != -1)
            {
                const Point& pPrevMain = m_allPoints[prevMainIndex];
                Point& pMid = m_allPoints[prevMidIndex];
                RestoreP1ByRelative(pMid, pPrevMain, pNew);
            }

            if (nextMidIndex != -1)
            {
                const Point& pNextMain = m_allPoints[nextMainIndex];
                Point& pMid = m_allPoints[nextMidIndex];
                RestoreP1ByRelative(pMid, pNew, pNextMain);
            }

            // Меняем только координаты, но не состояние
            pMoved.x = pNew.x;
            pMoved.y = pNew.y;
        }
        else
        {
            const Point& pPrevMain = m_allPoints[index - 1];
            const Point& pNextMain = m_allPoints[index + 1];

            float minX = std::min(pPrevMain.x, pNextMain.x);
            float maxX = std::max(pPrevMain.x, pNextMain.x);
            pMoved.x = std::clamp(p.x, minX, maxX);

            if (pMoved.curve == CurveType::Power)
            {
                float minY = std::min(pPrevMain.y, pNextMain.y);
                float maxY = std::max(pPrevMain.y, pNextMain.y);
                pMoved.y = std::clamp(p.y, minY, maxY);
            }
            else
            {
                pMoved.y = std::clamp(p.y, 0.f, m_graphH);
            }
            UpdateRelative(pMoved, pPrevMain, pNextMain);
        }
    }

    // Нарисовать контрол
    void EnvelopeGraphControl::Draw(bool hasFocus, bool drawSelected)
    {
        if (m_pContainer)
        {
            Gdiplus::Graphics g(оffScreenBitmap.get());
            g.Clear(Gdiplus::Color::Transparent);
            g.SetSmoothingMode(Gdiplus::SmoothingModeHighQuality);

            if (Selected && !drawSelected) return;

            // Фон
            Gdiplus::SolidBrush bgBrush(m_BackgroundColor);
            g.FillRectangle(&bgBrush, Gdiplus::Rect(0, 0, Border.Width(), Border.Height()));

            // Сетка
            if (ShowGrid)
            {
                DrawGrid(g);
            }

            // Кривая
            DrawCurve(g);

            // Точки
            DrawPoints(g);

            // Перекрестье
            if (ShowTrackingCross​ && !m_dragActive && !m_selectActive)
            {
                DrawCross(g);
            }

            if (m_selectActive)
            {
                DrawSelection(g);
            }

            DrawBorder2(g);
        }
    }

    EnvelopeGraphControl::Point EnvelopeGraphControl::hermite_spline(Point p0, Point p1, Point p2, float t)
    {
        // t в [0,1] - параметр от p0 к p1
        float t2 = t * t;
        float t3 = t2 * t;

        float m0 = (p1.y - p0.y) / (p1.x - p0.x);
        float m1 = (p2.y - p1.y) / (p2.x - p1.x);

        float mMax = 20;
        m0 = std::clamp(m0, -mMax, mMax);
        m1 = std::clamp(m1, -mMax, mMax);

        // Кубический Hermite сплайн - ВАША формула
        float y = (2.f * t3 - 3.f * t2 + 1.f) * p0.y +
            (t3 - 2.f * t2 + t) * m0 +
            (-2.f * t3 + 3.f * t2) * p2.y +
            (t3 - t2) * m1;

        // X интерполируем линейно
        float x = p0.x + t * (p2.x - p0.x);

        return Point( x, y, false);
    }

    /// <summary>
    /// Логарифмическое отображение [-1;1] -> [n;1/n]
    /// </summary>
    /// <param name="x"></param>
    /// <param name="n"></param>
    /// <returns></returns>
    double EnvelopeGraphControl::distributionFunction(double x, double n) const
    {
        return std::pow(n, -x);
    }

    double EnvelopeGraphControl::invertDistributionFunction(double y, double n) const
    {
        return -std::logf(y) / std::logf(n); // Считаем x
    }

    void EnvelopeGraphControl::crossSegment(Point A, Point B, Point& A1, Point& B1)
    {
        A1 = Point(A.x, B.y, A.isMain);
        B1 = Point(B.x, A.y, B.isMain);
    }

    /// <summary>
    /// Отражение точки относительно прямой, заданной двумя точками
    /// a = y1 - y2
    /// b = x2 - x1
    /// c = x1 * y2 - x2 * y1
    /// x' = x - 2*a*(a*x + b*y + c)/(a^2 + b^2)
    /// y' = y - 2*b*(a*x + b*y + c)/(a^2 + b^2)
    /// </summary>
    /// <param name="A">Точка прямой 1</param>
    /// <param name="B">Точка прямой 2</param>
    /// <param name="P">Точка, которую нужно отразить, относительно прямой AB</param>
    /// <returns>Отраженная точка</returns>
    EnvelopeGraphControl::Point EnvelopeGraphControl::reflectBySegment(Point A, Point B, Point P)
    {
        Point A1, B1;
        crossSegment(A, B, A1, B1);

        float dx = B1.x - A1.x;  // направляющий
        float dy = B1.y - A1.y;

        float a = dy;      // нормаль: поворот на 90°
        float b = -dx;     // (dy, -dx)
        float c = A1.x * B1.y - B1.x * A1.y;

        float denom = a * a + b * b;
        float d = a * P.x + b * P.y + c;

        return Point(
            P.x - 2 * a * d / denom,
            P.y - 2 * b * d / denom,
            P.isMain);
    }

    EnvelopeGraphControl::Point EnvelopeGraphControl::reflectByCenter(Point A, Point B, Point P)
    {
        Point C = Point((A.x + B.x) / 2.f, (A.y + B.y) / 2.f);
        return Point(2.0f * C.x - P.x, 2.0f * C.y - P.y, P.isMain);
    }

    /// <summary>
    /// Определить точку на кривой по индексу промежуточной точки
    /// </summary>
    /// <param name="p1Index"></param>
    /// <param name="t"></param>
    /// <returns></returns>
    EnvelopeGraphControl::Point EnvelopeGraphControl::CalculateСurvePoint(int p1Index, float t) const
    {
        ASSERT(p1Index > 0 && p1Index < m_allPoints.size() - 1);

        int prevMainIndex = p1Index - 1;
        int nextMainIndex = p1Index + 1;

        const Point& p0 = m_allPoints[prevMainIndex];
        const Point& p1 = m_allPoints[p1Index];
        const Point& p2 = m_allPoints[nextMainIndex];

        Point pCurve = CalculateСurvePoint(p0, p1, p2, t);
        return pCurve;
    }

    /// <summary>
    /// Обновить значение p1.relativeY по точкам p0, p2
    /// </summary>
    /// <param name="p1"></param>
    /// <param name="p0"></param>
    /// <param name="p2"></param>
    void EnvelopeGraphControl::UpdateRelative(Point& p1, const Point& p0, const Point& p2)
    {
        float centerY = (p0.y + p2.y) / 2;
        float halfHeightY = std::abs(p2.y - p0.y) / 2;
        float centerX = (p0.x + p2.x) / 2;
        float halfHeightX = std::abs(p2.x - p0.x) / 2;
        p1.relativeX = std::clamp((p1.x - centerX) / halfHeightX, -1.0f, 1.0f);
        p1.relativeY = std::clamp((p1.y - centerY) / halfHeightY, -1.0f, 1.0f);
        p1.onAscendingCurve = p0.y <= p2.y;
    }

    /// <summary>
    /// Восстановить p1.y по p1.relativeY и точкам p0, p2
    /// </summary>
    /// <param name="p1"></param>
    /// <param name="p0"></param>
    /// <param name="p2"></param>
    void EnvelopeGraphControl::RestoreP1ByRelative(Point& p1, const Point& p0, const Point& p2)
    {
        if (p1.curve == CurveType::Power)
        {
            p1.x = (p0.x + p2.x) * 0.5f;
        }
        else
        {
            float centerX = (p0.x + p2.x) / 2;
            float halfHeightX = std::abs(p2.x - p0.x) / 2;
            p1.x = centerX + p1.relativeX * halfHeightX;
        }

        bool increse = p0.y <= p2.y;
        int sign = p1.onAscendingCurve == increse ? 1 : -1;

        float centerY = (p0.y + p2.y) / 2;
        float halfHeightY = std::abs(p2.y - p0.y) / 2;
        p1.y = centerY + p1.relativeY * halfHeightY * sign;
    }

    /// <summary>
    /// Расчитать точку на кривой по типу вспомогательной точки
    /// </summary>
    /// <param name="p0"></param>
    /// <param name="p1"></param>
    /// <param name="p2"></param>
    /// <param name="t"></param>
    /// <returns></returns>
    EnvelopeGraphControl::Point EnvelopeGraphControl::CalculateСurvePoint(const Point& p0, const Point& p1, const Point& p2, float t) const
    {
        switch (p1.curve)
        {
            case CurveType::Power:
                return CalculatePowerPoint(p0, p1, p2, t);

            case CurveType::Bezier:
                return CalculateBezierPoint(p0, p1, p2, t);
        }
        return Point();
    }

    /// <summary>
    /// Расчитать точку на степенной кривой
    /// </summary>
    /// <param name="p0"></param>
    /// <param name="p1"></param>
    /// <param name="p2"></param>
    /// <param name="t"></param>
    /// <returns></returns>
    EnvelopeGraphControl::Point EnvelopeGraphControl::CalculatePowerPoint(const Point& p0, const Point& p1, const Point& p2, float t) const
    {
        float x = p0.x + (p2.x - p0.x) * t;

        if (p0.y == p2.y)
        {
            return Point(x, p0.y, false);
        }

        float power = distributionFunction(-std::abs(p1.relativeY), distributionFunctionBase);

        float y1;
        float yNormPowered;
        if ((p1.relativeY < 0) == (p2.y > p0.y))  // Направления совпадают
        {
            yNormPowered = std::pow(t, power);
            y1 = p0.y + (p2.y - p0.y) * yNormPowered;
        }
        else  // Направления противоположны
        {
            yNormPowered = std::pow(1 - t, power);
            y1 = p2.y - (p2.y - p0.y) * yNormPowered;
        }

        return Point(x, y1, false);
    }

    /* Старый вариант
    EnvelopeGraphControl::Point EnvelopeGraphControl::CalculatePowerPoint(const Point& p0, const Point& p1, const Point& p2, float t) const
    {
        float x = p0.x + (p2.x - p0.x) * t;

        if (p0.y == p2.y)
        {
            return Point(x, p0.y, false);
        }

        //float center = (p0.y + p2.y) / 2;
        //float halfHeight = std::abs(p2.y - p0.y) / 2;
        //float diff = (p1.y - center) / halfHeight;

        float diff = p1.relativeY;

        float power = distributionFunction(-std::abs(diff), distributionFunctionBase);

        float y1;
        float yNormPowered;
        //if (diff < 0)
        //{
        //    if (p2.y > p0.y) // Возрастает
        //    {
        //        yNormPowered = std::pow(t, power); // Возрастает в нормализованном диапазоне [0;1] 
        //        y1 = p0.y + (p2.y - p0.y) * yNormPowered; // от p0.y → p2.y (рост)
        //    }
        //    else // Убывает
        //    {
        //        yNormPowered = std::pow(1 - t, power); // Убывает в нормализованном диапазоне [0;1] 
        //        y1 = p2.y - (p2.y - p0.y) * yNormPowered; // от p2.y → p0.y (убывание)
        //    }
        //}
        //else
        //{
        //    if (p2.y > p0.y) // Возрастает
        //    {
        //        yNormPowered = std::pow(1 - t, power); // Убывает в нормализованном диапазоне [0;1] 
        //        y1 = p2.y - (p2.y - p0.y) * yNormPowered; // от p0.y → p2.y (рост)
        //    }
        //    else // Убывает
        //    {
        //        yNormPowered = std::pow(t, power); // Возрастает в нормализованном диапазоне [0;1]
        //        y1 = p0.y + (p2.y - p0.y) * yNormPowered; // от p2.y → p0.y (убывание)
        //    }
        //}

        if ((diff < 0) == (p2.y > p0.y))  // Направления совпадают
        {
            yNormPowered = std::pow(t, power);
            y1 = p0.y + (p2.y - p0.y) * yNormPowered;
        }
        else  // Направления противоположны
        {
            yNormPowered = std::pow(1 - t, power);
            y1 = p2.y - (p2.y - p0.y) * yNormPowered;
        }


        return Point(x, (float)y1, false);
    }
    */

    /// <summary>
    /// Расчитать точку на квадратичной кривой Безье
    /// </summary>
    /// <param name="p0"></param>
    /// <param name="p1"></param>
    /// <param name="p2"></param>
    /// <param name="t"></param>
    /// <returns></returns>
    EnvelopeGraphControl::Point EnvelopeGraphControl::CalculateBezierPoint(const Point& p0, const Point& p1, const Point& p2, float t) const
    {
        // Безье: B(t) = (1-t)²p0 + 2t(1-t)p1 + t²p2 
        float u = 1.0f - t; // u = 1-t
        float u2 = u * u;   // u²
        float t2 = t * t;   // t²

        float x = u2 * p0.x + 2 * t * u * p1.x + t2 * p2.x;
        float y = u2 * p0.y + 2 * t * u * p1.y + t2 * p2.y;

        return Point(x, y, false);
    }

    EnvelopeGraphControl::Point EnvelopeGraphControl::symmetricByCenter(Point A, Point B, Point P)
    {
        float centerY = (A.y + B.y) / 2;
        return Point(P.x, P.y - 2.f * (P.y - centerY), P.isMain);
    }

    void EnvelopeGraphControl::DrawCurve(Gdiplus::Graphics& g)
    {
        if (m_allPoints.empty()) return;

        Gdiplus::Pen linePen(LineColor, 2.0f);

        // ★ ОДИН ЦИКЛ: [P0, M01, P1, M12, P2...] ★
        Point p0 = Point(0, 0);
        bool haveP0 = false;
        size_t i = 0;

        Gdiplus::GraphicsPath boredPath;
        boredPath.StartFigure();

        Gdiplus::PointF prevPoint;

        while (i < m_allPoints.size())
        {
            const Point& pt = m_allPoints[i];

            if (pt.isMain)
            {
                if (!haveP0)
                {
                    // Первая основная
                    p0 = pt;
                    haveP0 = true;
                    if (p0.x != 0.f || p0.y != 0.f)
                    {
                        boredPath.AddLine(ToScreenPointF(0.f, 0.f), ToScreenPointF(p0.x, p0.y));
                    }
                }
                else
                {
                    // Предыдущая = промежуточная M01, текущая = P1
                    const Point& p1 = m_allPoints[i - 1];  // ★ ПРЯМО предыдущая! ★
                    const Point& p2 = pt;

                    static bool output = true;
                    std::ofstream file;
                    if (output)
                    {
                        file.open("Out.txt");
                        file << std::fixed << std::setprecision(6);
                    }

                    int N = 100;
                    for (int seg = 1; seg <= N; ++seg)
                    {
                        float t = static_cast<float>(seg - 1) / (N - 1);
                        Point p = CalculateСurvePoint(p0, p1, p2, t);
                        if (output)
                        {
                            file << p.x << "; " << p.y << "\n";
                        }

                        Gdiplus::PointF currPoint = ToScreenPointF(p.x, p.y);
                        if (seg > 1)
                        {
                            g.DrawLine(&linePen, prevPoint, currPoint);
                            boredPath.AddLine(prevPoint, currPoint);
                        }
                        prevPoint = currPoint;
                    }

                    if (output)
                    {
                        file.close();
                        //output = false;
                    }

                    p0 = p2;
                }
                ++i;
            }
            else
            {
                ++i;  // Пропускаем промежуточные (будут использованы как p1)
            }
        }

        if (p0.x != m_graphW || p0.y != 0.f)
        {
            boredPath.AddLine(prevPoint, ToScreenPointF(p0.x, 0.f));
        }
        boredPath.CloseFigure();

        Gdiplus::RectF bounds;
        boredPath.GetBounds(&bounds);  // Получаем границы

        //Gdiplus::REAL topY = bounds.GetTop();      // Верхняя Y координата
        //Gdiplus::REAL bottomY = bounds.GetBottom(); // Нижняя Y координата
        //Gdiplus::REAL width = bounds.Width();      // Ширина для нормализации X
        //Gdiplus::REAL width = bounds.Height();      // Ширина для нормализации X

        Gdiplus::Color underCunveTopColor = Gdiplus::Color::MakeARGB(50,
            UnderCurveColor.GetR(), UnderCurveColor.GetG(), UnderCurveColor.GetB());
        Gdiplus::Color underCunveBottomColor = Gdiplus::Color::MakeARGB(0,
            UnderCurveColor.GetR(), UnderCurveColor.GetG(), UnderCurveColor.GetB());

        Gdiplus::LinearGradientBrush underCirveBrush(
            bounds,
            underCunveTopColor,
            underCunveBottomColor,
            Gdiplus::LinearGradientModeVertical);
        g.FillPath(&underCirveBrush, &boredPath);
        Gdiplus::Pen pen_(Gdiplus::Color(255, 125, 0, 0), 4.0f);
        //g.DrawPath(&pen_, &boredPath);
    }

    // Вспомогательная функция
    Gdiplus::PointF EnvelopeGraphControl::ToScreenPointF(float x, float y) const
    {
        int w = Border.Width();
        int h = Border.Height();
        return Gdiplus::PointF(
            x * w,
            (1.0f - y) * h
        );
    }

    void EnvelopeGraphControl::DrawPoints(Gdiplus::Graphics& g)
    {
        const float pointRadius = 3;

        Gdiplus::Pen mainPen(MainPointColor, 1.f);
        Gdiplus::Pen interPen(MainPointColor, 1.f);
        Gdiplus::Pen selectedPen(SelectedPointColor, 1.f);

        Gdiplus::SolidBrush mainBrush(MainPointColor);
        Gdiplus::SolidBrush selectedBrush(SelectedPointColor);
        Gdiplus::SolidBrush interBrush(InterPointColor);
        Gdiplus::SolidBrush hoverBrush(HoverColor);

        for (int i = 0; i < m_allPoints.size(); ++i)
        {
            Point pt = m_allPoints[i];
            if (!pt.isMain)
            {
                //pt.y = CalculateСurvePoint(i).y;
                pt = CalculateСurvePoint(i);
            }
            
            CPoint screenPt = ToScreen(pt.x, pt.y);


            // ★ Фокус для dragged точки ★
            //if (m_dragActive && m_draggedPointIndex != -1 && m_draggedPointIndex == i || m_hoveredPointIndex == i)
            if(m_hoveredPointIndex == i)
            {
                const Point& p = m_allPoints[m_hoveredPointIndex];
                Gdiplus::RectF hover(screenPt.x - pointRadius * 3, screenPt.y - pointRadius * 3,
                    pointRadius * 6, pointRadius * 6);
                g.FillEllipse(&hoverBrush, hover);
                if (p.selected)
                {
                    g.DrawEllipse(&selectedPen, hover);
                }
            }

            if (pt.isMain)
            {
                bool hovered = m_hoveredPointIndex == i;
                Gdiplus::RectF circle(screenPt.x - pointRadius, screenPt.y - pointRadius,
                    pointRadius * 2, pointRadius * 2);
                g.FillEllipse(pt.selected && !hovered? &selectedBrush : &mainBrush, circle);
            }
            else
            {
                Gdiplus::RectF circle(screenPt.x - pointRadius / 1.5f, screenPt.y - pointRadius / 1.5f, pointRadius * 1.5f, pointRadius * 1.5f);
                if (m_dragActive && i == m_draggedPointIndex)
                {
                    Gdiplus::SolidBrush interBrush(Gdiplus::Color(255, 0, 255, 0));
                    g.FillEllipse(&interBrush, circle);
                }
                else
                {
                    g.DrawEllipse(&interPen, circle);
                }
            }
        }
    }

    void EnvelopeGraphControl::DrawGrid(Gdiplus::Graphics& g)
    {
        Gdiplus::Pen gridPen(m_GridColor, 0.8f);
        //Gdiplus::Pen gridPen2(m_GridColor, 4.f);
        Gdiplus::Color gridColor2 = Gdiplus::Color::MakeARGB(/*m_GridColor.GetAlpha() * 2*/255,
            m_GridColor.GetR(), m_GridColor.GetG(), m_GridColor.GetB());
        Gdiplus::Pen gridPen2(gridColor2, 1.5f);

        int w = Border.Width();
        int h = Border.Height();

        // Вертикальные линии
        for (int i = 0; i <= HorizontalDivisions; ++i)
        {
            Gdiplus::Pen* pen;
            if(i % (HorizontalDivisions / 2 ))
            { 
                pen = &gridPen;
            }
            else
            {
                pen = &gridPen2;
            }
            float x = (float)i / HorizontalDivisions * w;
            g.DrawLine(pen, (int)x, 0, (int)x, h);
        }

        // Горизонтальные линии
        for (int i = 0; i <= VerticalDivisions; ++i)
        {
            Gdiplus::Pen* pen;
            if (i % (VerticalDivisions / 2))
            {
                pen = &gridPen;
            }
            else
            {
                pen = &gridPen2;
            }
            float y = (float)i / VerticalDivisions * h;
            g.DrawLine(pen, 0, (int)y, w, (int)y);
        }
    }

    //void EnvelopeGraphControl::DrawSelection(Gdiplus::Graphics& g)
    //{
    //    Gdiplus::Pen pen(SelectColor, 1.f);

    //    int w = Border.Width();
    //    int h = Border.Height();

    //    POINT cursorPos;
    //    cursorPos.x = m_lastMouseX;
    //    cursorPos.y = m_lastMouseY;

    //    if (Border.PtInRect(cursorPos))
    //    {
    //        // Координаты ОТНОСИТЕЛЬНО border (0,0)
    //        float x1 = static_cast<float>(m_selectX - Border.left);
    //        float y1 = static_cast<float>(m_selectY - Border.top);

    //        float x2 = static_cast<float>(cursorPos.x - Border.left);
    //        float y2 = static_cast<float>(cursorPos.y - Border.top);

    //        g.DrawRectangle(&pen, x1, y1, x2 - x1 + 1, y2 - y1 + 1);
    //    }
    //}

    //void EnvelopeGraphControl::DrawSelection(Gdiplus::Graphics& g)
    //{
    //    Gdiplus::Pen pen(SelectColor, 1.f);

    //    // ✅ m_lastMouseX/Y — уже клиентские координаты
    //    float x1 = static_cast<float>(m_selectX - Border.left);
    //    float y1 = static_cast<float>(m_selectY - Border.top);
    //    float x2 = static_cast<float>(m_lastMouseX - Border.left);  // ← Текущая позиция
    //    float y2 = static_cast<float>(m_lastMouseY - Border.top);   // ← Текущая позиция

    //    // Min/Max прямоугольник
    //    float left = std::min(x1, x2);
    //    float top = std::min(y1, y2);
    //    float width = fabs(x2 - x1) + 1.f;
    //    float height = std::fabs(y2 - y1) + 1.f;

    //    g.DrawRectangle(&pen, left, top, width, height);
    //}

    void EnvelopeGraphControl::DrawSelection(Gdiplus::Graphics& g)
    {
        Gdiplus::Pen pen(SelectColor, 1.f);
        CRect selRect(m_selectX, m_selectY, m_lastMouseX, m_lastMouseY);

        selRect.NormalizeRect();
        selRect.OffsetRect(-Border.left, -Border.top);
        selRect.IntersectRect(selRect, CRect(0, 0, Border.Width() - 1, Border.Height() - 1));

        g.DrawRectangle(&pen,
            static_cast<float>(selRect.left),
            static_cast<float>(selRect.top),
            static_cast<float>(selRect.Width()),
            static_cast<float>(selRect.Height()));
    }

    void EnvelopeGraphControl::DrawCross(Gdiplus::Graphics& g)
    {
        Gdiplus::Pen crossPen(CrossColor, 0.8f);

        Gdiplus::REAL dashPattern[] = { 10.0f, 5.0f };
        crossPen.SetDashPattern(dashPattern, 2);
        crossPen.SetDashStyle(Gdiplus::DashStyleCustom);

        int w = Border.Width();
        int h = Border.Height();

        POINT cursorPos;
        GetCursorPos(&cursorPos);           // Экранные координаты

        if (Border.PtInRect(cursorPos))
        {
            // Координаты ОТНОСИТЕЛЬНО border (0,0)
            float x = static_cast<float>(cursorPos.x - Border.left);
            float y = static_cast<float>(cursorPos.y - Border.top);

            // Горизонтальная линия
            g.DrawLine(&crossPen, 0.f, y, (Gdiplus::REAL)w, y);

            // Вертикальная линия
            g.DrawLine(&crossPen, x, 0.f, x, (Gdiplus::REAL)h);
        }
    }

    CPoint EnvelopeGraphControl::ToScreen(float x, float y) const
    {
        int w = Border.Width();
        int h = Border.Height();
        return CPoint(
            (int)(x * w),              // X: 0→w
            (int)((1.0f - y) * h)      // Y: 1→0 (сверху), 0→h (снизу) ✅
        );
    }

    std::pair<float, float> EnvelopeGraphControl::ToGraph(int screenX, int screenY) const
    {
        float localX = screenX - (float)Border.left;
        float localY = screenY - (float)Border.top;

        int w = Border.Width();
        int h = Border.Height();

        return {
            localX / w,              // X: 0.0 (слева) → 1.0 (справа)
            1.0f - (localY / h)      // Y: 1.0 (сверху) → 0.0 (снизу) ✅
        };
    }

    int EnvelopeGraphControl::FindNearestPoint(float x, float y, float threshold)
    {
        float minDist = threshold * threshold;
        int nearest = -1;

        for (int i = 0; i < (int)m_allPoints.size(); ++i)
        {
            float dx;
            float dy;

            if (i % 2 == 1) // Промежуточная точка
            {
                // Найти соседние главные точки
                //Point p0 = m_allPoints[i - 1];
                //Point p1 = m_allPoints[i];
                //Point p2 = m_allPoints[i + 1];

                /* Новая контрольная точка -------------------------
                Point p = CalculateIntermediatePoint(0.5f, p0, p1, p2); // Новая промежуточная точка

                dx = p.x - x;
                dy = p.y - y;
                ------------------------------------- */
                Point pCurve = CalculateСurvePoint(i);
                dx = pCurve.x - x;
                dy = pCurve.y - y;
            }
            else // Основная точка
            {
                dx = m_allPoints[i].x - x;
                dy = m_allPoints[i].y - y;
            }

            float distSq = dx * dx + dy * dy;

            if (distSq < minDist)
            {
                minDist = distSq;
                nearest = i;
            }
        }
        return nearest;
    }

    void EnvelopeGraphControl::OnLButtonDown(UINT nFlags, CPoint p)
    {
        //if (!HitTest(p.x, p.y)) return;
        Control::OnLButtonDown(nFlags, p);

        auto [gx, gy] = ToGraph(p.x, p.y);
        int idx = FindNearestPoint(gx, gy);  // Ищем ближайшую точку

        if (idx != -1)
        {
            Point& p = m_allPoints[idx];

            if (!p.isMain && nFlags & MK_CONTROL)
            {
                // Поставить промежуточную точку на середину

                // 1.Удалить промежуточную точку
                m_allPoints.erase(m_allPoints.begin() + idx);
                RecalculatePoints();
                Invalidate();
            }
            else if (!p.isMain && (GetKeyState(VK_MENU) & 0x8000))
            {
                AdaptiveSplineRemapping(idx);
                Invalidate();
            }

            // Включаем режим drag&drop
            m_draggedPointIndex = idx;           
            m_draggedPoint = &m_allPoints[idx];
            m_dragActive = true;
            InitDragRelative();
        }
        else
        {
            // Включаем режим селекции главных точек
            DropPointSelection();
            m_selectX = p.x;
            m_selectY = p.y;
            m_selectActive = true;
        }
    }

    /// <summary>
    /// Адаптация всех сплайнов по 1 сплайну
    /// </summary>
    /// <param name="index"></param>
    void EnvelopeGraphControl::AdaptiveSplineRemapping(int index)
    {
        if (index < 1 || index >= m_allPoints.size() - 1)
            return;

        const Point& p0 = m_allPoints[index - 1];
        const Point& p1 = m_allPoints[index];
        const Point& p2 = m_allPoints[index + 1];

        bool increase1 = p0.y <= p2.y;

        float relativeY = p0.y <= p2.y ? -p1.relativeY : p1.relativeY;

        for (int i = 1; i < m_allPoints.size() - 1; ++i)
        {
            Point& p = m_allPoints[i];

            if (&p == &p1 || p.isMain)
            {
                continue;
            }

            //int prevMainIdx = FindNextPoint(i, -1, true);
            //int nextMainIdx = FindNextPoint(i, +1, true);
            
            //const Point& pPrevMain = m_allPoints[prevMainIdx];
            //const Point& pNextMain = m_allPoints[nextMainIdx];

            const Point& pPrevMain = m_allPoints[i - 1];
            const Point& pNextMain = m_allPoints[i + 1];

            float maxY = std::max(pPrevMain.y, pNextMain.y);
            float minY = std::min(pPrevMain.y, pNextMain.y);

            bool top = std::abs(0.5f - maxY) >= std::abs(0.5f - minY);

            bool increase = pPrevMain.y <= pNextMain.y;
            int sign = p.onAscendingCurve == increase ? 1 : -1;

            p.relativeY = (top ? relativeY : -relativeY ) * sign;
            RestoreP1ByRelative(p, pPrevMain, pNextMain);
        }
    }

    BOOL EnvelopeGraphControl::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
    {
        int idx = -1;
        if (m_dragActive)
        {
            idx = m_draggedPointIndex;
        }
        else
        {
            POINT cursorPos;
            GetCursorPos(&cursorPos);           

            auto [gx, gy] = ToGraph(cursorPos.x, cursorPos.y);

            idx = FindNearestPoint(gx, gy);  // Ищем ближайшую точку
        }

        // Обработка курсора
        if (idx != -1)
        {
            Point p_ = m_allPoints[idx];
            if (p_.isMain) // Основная точка
            {
                if (idx == 0 || idx == m_allPoints.size() - 1 && FixedRightPoint)
                {
                    SetCursor(LoadCursor(NULL, IDC_SIZENS));
                }
                else
                {
                    SetCursor(LoadCursor(NULL, IDC_SIZEALL));
                }
            }
            else // Промежуточная точка
            {
                if (p_.curve == CurveType::Power)
                {
                    SetCursor(LoadCursor(NULL, IDC_SIZENS));
                }
                else
                {
                    SetCursor(LoadCursor(NULL, IDC_SIZEALL));
                }
            }
        }
        else
        {
            SetCursor(LoadCursor(NULL, IDC_CROSS));
        }

        return TRUE;
    }

    void EnvelopeGraphControl::OnMouseLeave()
    {
        if (m_hoveredPointIndex != -1)
        {
            m_hoveredPointIndex = -1;
            Invalidate();
        }
        Control::OnMouseLeave();
    }

    void EnvelopeGraphControl::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
    {
    }

    void EnvelopeGraphControl::OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags)
    {
    }

    void EnvelopeGraphControl::OnSysKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
    {
        bool isAltPressed = (nChar == VK_MENU);
        m_AltPressed = true;
        TRACE(_T("Alt pressed\n"));
        if (isAltPressed)
        {
            if (m_dragActive)
            {
                POINT cursorPos;
                GetCursorPos(&cursorPos);           

                auto [gx, gy] = ToGraph(cursorPos.x, cursorPos.y);
                auto [gridX, gridY] = SnapToGrid(gx, gy);
                Point p = { gridX, gridY, m_draggedPoint->isMain };
                MovePoint(m_draggedPointIndex, p);
                Invalidate();

                //if (m_lastMouseX != -1 && m_lastMouseY != 0)
                //{
                //}
            }
        }
    }

    void EnvelopeGraphControl::OnSysKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags)
    {
        bool isAltPressed = (nChar == VK_MENU);
        m_AltPressed = false;
        TRACE(_T("Alt released\n"));
        if (isAltPressed)
        {
            if (m_dragActive)
            {
                POINT cursorPos;
                GetCursorPos(&cursorPos);           

                auto [gx, gy] = ToGraph(cursorPos.x, cursorPos.y);
                Point p = { gx, gy, m_draggedPoint->isMain };
                MovePoint(m_draggedPointIndex, p);
                Invalidate();

                //if (m_lastMouseX != -1 && m_lastMouseY != 0)
                //{
                //    auto [gx, gy] = ToGraph(m_lastMouseX, m_lastMouseY);
                //    Point p = { gx, gy, m_draggedPoint->isMain };
                //    MovePoint(m_draggedAllPointsIndex, p);
                //    Invalidate();
                //}
            }
        }
    }

    void EnvelopeGraphControl::OnContextMenu(CPoint point)
    {
        CMenu menu;
        menu.LoadMenu(IDR_ENVELOP_CONTEXT_MENU);
        CMenu* pPopup = menu.GetSubMenu(0);
        pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, AfxGetMainWnd());
    }

    void EnvelopeGraphControl::OnMouseMove(UINT nFlags, CPoint p)
    {
        Control::OnMouseMove(nFlags, p);    

        //if (!HitTest(p.x, p.y))
        //{
        //    m_lastMouseX = p.x;
        //    m_lastMouseY = p.y;

        //    return;
        //}

        auto [gx, gy] = ToGraph(p.x, p.y);

        bool needsRedraw = false;

        if (m_dragActive /*&& m_draggedAllPointsIndex != -1 && m_draggedPoint->x >= 0*/)
        {
            //auto [gx, gy] = ToGraph(p.x, p.y);

            if (m_draggedPoint->isMain) // Основная
            {
                if (m_draggedPoint->selected)
                {
                    //float dx = m_draggedPoint->x - gx;
                    //float dy = m_draggedPoint->y - gy;
                    float dx = gx - m_draggedPoint->x;
                    float dy = gy - m_draggedPoint->y;
                    TRACE(_T("Move (%.3f, %.3f) to (%.3f, %.3f) selected on dx = %.3f, dy = %.3f\n"), m_draggedPoint->x, m_draggedPoint->y, gx, gy, dx, dy);
                    if (dx != 0.f || dy != 0.f)
                    {
                        MoveSelectedPoints(dx, dy);
                        needsRedraw = true;
                    }
                }
                else
                {
                    if (m_draggedPoint->x != gx || m_draggedPoint->y != gx)
                    {
                        if(GetAsyncKeyState(VK_MENU) & 0x8000)
                        {
                            auto [gridX, gridY] = SnapToGrid(gx, gy);
                            gx = gridX;
                            gy = gridY;
                        }

                        Point pNew(gx, gy, m_draggedPoint->isMain);
                        MovePoint(m_draggedPointIndex, pNew);
                        needsRedraw = true;
                    }
                }
            }
            else
            {
                float dx = 0;
                if (m_draggedPoint->curve != CurveType::Power)
                {
                    if (m_lastMouseX != -1 && m_lastMouseX != p.x)
                    {
                        auto [gx1, gy1] = ToGraph(m_lastMouseX, m_lastMouseY);
                        dx = -(gx1 - gx);
                    }
                }

                float dy = 0;
                if (m_lastMouseY != -1 && m_lastMouseY != p.y)
                {
                    auto [gx1, gy1] = ToGraph(m_lastMouseX, m_lastMouseY);
                    dy = -(gy1 - gy);
                }

                const Point& p1 = m_allPoints[m_draggedPointIndex];
                Point pNew(p1.x + dx, p1.y + dy, m_draggedPoint->isMain);
                MovePoint(m_draggedPointIndex, pNew);

                if (/*m_AltPressed*/GetKeyState(VK_MENU) & 0x8000)
                {
                    TRACE(_T("Move mose + ALT Pressed"));
                    AdaptiveSplineRemapping(m_draggedPointIndex);
                }
                else
                {
                    TRACE(_T("Move mose + ALT Released"));
                }

                needsRedraw = true;
            }
        }
        else if (m_selectActive)
        {
            needsRedraw = true;
        }
        else 
        {
            int idx = FindNearestPoint(gx, gy);  // Ищем ближайшую точку

            // Подсветка точки
            if (idx != m_hoveredPointIndex)
            {
                m_hoveredPointIndex = idx;  // Изменился hover
                needsRedraw = true;;  // Перерисовка только при смене!
            }

            if (ShowTrackingCross​ && (p.x != m_lastMouseX || p.y != m_lastMouseY))
            {
                needsRedraw = true;;  // Перерисовка только при смене!
            }
        }

        //if (m_selectActive)
        //{
        //    needsRedraw = true;
        //}

        if (needsRedraw)
        {
            Invalidate();
        }

        m_lastMouseX = p.x;
        m_lastMouseY = p.y;
    }

    std::pair<float, float> EnvelopeGraphControl::SnapToGrid(float x, float y) const
    {
        float cellWidth = 1.0f / HorizontalDivisions;
        float cellHeight = 1.0f / VerticalDivisions;

        // Математическое округление к ближайшему узлу сетки
        int gridX = static_cast<int>(std::floor(x / cellWidth + 0.5f));
        int gridY = static_cast<int>(std::floor(y / cellHeight + 0.5f));

        // Координаты узла сетки
        float snappedX = gridX * cellWidth;
        float snappedY = gridY * cellHeight;

        // Ограничение в [0,1]
        snappedX = std::clamp(snappedX, 0.0f, 1.0f);
        snappedY = std::clamp(snappedY, 0.0f, 1.0f);

        return {snappedX, snappedY};
    }

    void EnvelopeGraphControl::OnLButtonUp(UINT nFlags, CPoint p)
    {
        if (m_dragActive)
        {
            DropDragRelative();
            UpdateCurveDirections();
            m_dragActive = false;
            m_draggedPointIndex = -1;
            Invalidate();
        }
        else if(m_selectActive)
        {
            if (SelectPoints())
            {
                Invalidate();
            }
            m_selectX = -1;
            m_selectY = -1;
            m_selectActive = false;
        }
        Control::OnLButtonUp(nFlags, p);
    }

    bool EnvelopeGraphControl::SelectPoints()
    {
        bool selected = false;

        auto [gx1, gy1] = ToGraph(m_selectX, m_selectY);
        auto [gx2, gy2] = ToGraph(m_lastMouseX, m_lastMouseY);

        float left = std::min(gx1, gx2);
        float right = std::max(gx1, gx2);
        float top = std::min(gy1, gy2);
        float bottom = std::max(gy1, gy2);

        for (Point& p : m_allPoints)
        {
            if (p.isMain &&
                p.x >= left && p.x <= right &&
                p.y >= top && p.y <= bottom)
            {
                p.selected = true; 
                selected = true;
            }
        }

        return selected;
    }

    /// <summary>
    /// Есть ли выбранные точки
    /// </summary>
    /// <returns></returns>
    bool EnvelopeGraphControl::IsSelectedPoints() const
    {
        //for (const Point& p: m_allPoints)
        //{
        //    if (p.isMain && p.selected)
        //    {
        //        return true;
        //    }
        //}

        //return false;
        return std::any_of(m_allPoints.begin(), m_allPoints.end(),
            [](const Point& p) { return p.isMain && p.selected; });
    }


    /// <summary>
    /// Определить для всех выбранных главных точек относительные координаты относительно точки перемещения
    /// </summary>
    void EnvelopeGraphControl::InitDragRelative()
    {
        //for (int i = 0; i < m_allPoints.size(); ++i)
        //{
        //    Point& p = m_allPoints[i];
        //    if (p.isMain && p.selected)
        //    {
        //        p.dragRelativeX = p.x - m_draggedPoint->x;
        //        p.dragRelativeY = p.y - m_draggedPoint->y;
        //    }
        //}

        std::for_each(m_allPoints.begin(), m_allPoints.end(),
            [&](Point& p)
            {
                if (p.isMain && p.selected)
                {
                    p.dragRelativeX = p.x - m_draggedPoint->x;
                    p.dragRelativeY = p.y - m_draggedPoint->y;
                }
            });
    }

    /// <summary>
    /// Обнулить у всех выбранных главных точек относительные координаты относительно точки перемещения
    /// </summary>
    void EnvelopeGraphControl::DropDragRelative()
    {
        //for (int i = 0; i < m_allPoints.size(); ++i)
        //{
        //    Point& p = m_allPoints[i];
        //    if (p.isMain && p.selected)
        //    {
        //        p.dragRelativeX = 0.f;
        //        p.dragRelativeY = 0.f;
        //    }
        //}
    
        std::for_each(m_allPoints.begin(), m_allPoints.end(),
            [](Point& p) 
            {
                if (p.isMain && p.selected)
                {
                    p.dragRelativeX = 0.f;
                    p.dragRelativeY = 0.f;
                }
            });
    }

    void EnvelopeGraphControl::DeleteSelectedPoints()
    {
        for (int i = 0; i < m_allPoints.size(); ++i)
        {
            Point& p = m_allPoints[i];
            if (p.isMain && p.selected)
            {
                m_allPoints.erase(m_allPoints.begin() + i);
            }
        }
        RecalculatePoints();
    }

    void EnvelopeGraphControl::UpdateCurveDirections()
    {
        size_t index = 0;
        std::for_each(m_allPoints.begin(), m_allPoints.end(),
            [this, &index](Point& p)
            {
                if (!p.isMain)
                {  
                    const Point& p0 = m_allPoints[index - 1];
                    const Point& p2 = m_allPoints[index + 1];
                    p.onAscendingCurve = p0.y <= p2.y;
                }
                ++index;
            });
    }

    /// <summary>
    /// Переместить выбранные точки
    /// </summary>
    /// <param name="dx"></param>
    /// <param name="dy"></param>
    /// <returns></returns>
    void EnvelopeGraphControl::MoveSelectedPoints(float dx, float dy)
    {
        ASSERT(m_draggedPoint != nullptr);

        // 1.Перемещаем основную точку на новую позицию
        MovePoint(m_draggedPointIndex, Point(m_draggedPoint->x + dx, m_draggedPoint->y + dy));

        // 2.Перемещаем остальные выделенные точки относительно основной
        for (int i = 0; i < m_allPoints.size(); ++i)
        {
            if (i == m_draggedPointIndex)
            {
                continue;
            }

            const Point& p = m_allPoints[i];
            if (p.isMain && p.selected)
            {
                Point pNew(m_draggedPoint->x + p.dragRelativeX, m_draggedPoint->y + p.dragRelativeY);
                MovePoint(i, pNew);
            }
        }
    }

    /// <summary>
    /// Найти индекс точки, смещенной относительно currentIdx на diffIdx 
    /// </summary>
    /// <param name="currentIdx">Индекс относительно которого ищем</param>
    /// <param name="diffIdx">На сколько позиций нужно сместиться вправо/влево</param>
    /// <param name="isMain">Ищем главную точку или промежуточную</param>
    /// <returns>Индекс найденой точки или -1, если не нашли</returns>
    int EnvelopeGraphControl::FindNextPoint(int currentIdx, int diffIdx, bool isMain)
    {
        if (currentIdx < 0 || currentIdx >= (int)m_allPoints.size())
        {
            return -1;
        }

        if (diffIdx == 0)
        {
            return currentIdx;
        }

        int step = (diffIdx > 0) ? +1 : -1;
        int remaining = abs(diffIdx);
        int idx = currentIdx;

        while (remaining > 0)
        {
            idx += step;

            if (idx < 0 || idx >= (int)m_allPoints.size())
            {
                return -1;
            }

            if (m_allPoints[idx].isMain == isMain)
            {
                remaining--;
            }
        }

        return idx;
    }

    /// <summary>
    /// Сбросить селекцию главных точек
    /// </summary>
    void EnvelopeGraphControl::DropPointSelection()
    {
        std::for_each(m_allPoints.begin(), m_allPoints.end(),
            [](Point& p) { p.selected = false; });
    }

    void EnvelopeGraphControl::OnLButtonDblClk(UINT nFlags, CPoint point)
    {
        Control::OnLButtonDblClk(nFlags, point);

        auto [gx, gy] = ToGraph(point.x, point.y);
        int idx = FindNearestPoint(gx, gy);

        if (idx != -1 && m_allPoints[idx].isMain)
        {
            // Двойной клик по основной точке - удаляем её (кроме первой и последней)
            if (idx > 0 && idx < (int)m_allPoints.size() - 1)
            {  
                m_allPoints.erase(m_allPoints.begin() + idx);
                RecalculatePoints();
                Invalidate();
            }
        }
        else if (idx == -1)
        {
            int prevMainIndex = FindPrevMainPoint(gx);
            if (prevMainIndex == -1)
                return;

            // Удобные индексы
            int idxMainPrev = prevMainIndex;
            int idxP1Prev = prevMainIndex + 1;
            int idxMainNext = prevMainIndex + 2;

            // Проверяем, что индексы в пределах массива
            if (idxMainNext >= (int)m_allPoints.size())
                return;

            // Базовые ссылки
            const Point& pMainPrev = m_allPoints[idxMainPrev];
            Point& p1Prev = m_allPoints[idxP1Prev];
            const Point& pMainNext = m_allPoints[idxMainNext];

            // Новая главная точка и P1‑точка
            Point pNewMain = { gx, gy, true };
            Point p1Next{ 0.0f, 0.0f, p1Prev.relativeX, p1Prev.relativeY, p1Prev.curve };

            // Восстановить P1 относительно новой точки
            RestoreP1ByRelative(p1Prev, pMainPrev, pNewMain);
            RestoreP1ByRelative(p1Next, pNewMain, pMainNext);

            // Вставить точки: сначала pNewMain, потом p1Next
            m_allPoints.insert(m_allPoints.begin() + idxMainNext, pNewMain);
            m_allPoints.insert(m_allPoints.begin() + idxMainNext + 1, p1Next);

            RecalculatePoints();
            Invalidate();
        }
    }

    /// <summary>
    /// Проверка доступности команды 
    /// </summary>
    /// <param name="pCmdUI"></param>
    void EnvelopeGraphControl::OnUpdateCommand(CCmdUI* pCmdUI)
    {
        UINT nID = pCmdUI->m_nID;  // ID текущей команды!

        switch (nID)
        {
            case ID_PLUGINGUI_ENVELOP_FLIP_HORIZONTAL:
                pCmdUI->Enable(TRUE);     // Включить/отключить
                break;

            case ID_PLUGINGUI_ENVELOP_FLIP_VERTICAL:
                pCmdUI->Enable(TRUE);
                break;

            case ID_PLUGINGUI_ENVELOP_FLIP_BY_CENTER:
                pCmdUI->Enable(TRUE);
                break;


            case ID_PLUGINGUI_ENVELOP_REMOVE_SELECTED_POINTS:
                pCmdUI->Enable(IsSelectedPoints());
                break;

            case ID_PLUGINGUI_ENVELOP_CHANGE_CURVE_POWER:
                if (m_hoveredPointIndex != -1)
                {
                    const Point& p = m_allPoints[m_hoveredPointIndex];
                    if (!p.isMain && p.curve != CurveType::Power) 
                    {
                        pCmdUI->Enable(TRUE);
                        break;
                    }
                }
                pCmdUI->Enable(FALSE);
                break;

            case ID_PLUGINGUI_ENVELOP_CHANGE_CURVE_BEZIER:
                if (m_hoveredPointIndex != -1)
                {
                    const Point& p = m_allPoints[m_hoveredPointIndex];
                    if (!p.isMain && p.curve != CurveType::Bezier)
                    {
                        pCmdUI->Enable(TRUE);
                        break;
                    }
                }
                pCmdUI->Enable(FALSE);
                break;

            default:
                pCmdUI->Enable(TRUE);       
        }
    }

    /// <summary>
    /// Выполнить команду
    /// </summary>
    /// <param name="nID"></param>
    void EnvelopeGraphControl::OnCommand(UINT nID)
    {
        switch (nID)
        {
            case ID_PLUGINGUI_ENVELOP_FLIP_HORIZONTAL:
                FlipHorizontal();
                break;

            case ID_PLUGINGUI_ENVELOP_FLIP_VERTICAL:
                FlipVertical();
                break;

            case ID_PLUGINGUI_ENVELOP_FLIP_BY_CENTER:
                FlipByCenter();
                break;

            case ID_PLUGINGUI_ENVELOP_REMOVE_SELECTED_POINTS:
                DeleteSelectedPoints();
                break;

            case ID_PLUGINGUI_ENVELOP_CHANGE_CURVE_POWER:
                if (m_hoveredPointIndex != -1)
                {
                    ChangeCurve(m_hoveredPointIndex, CurveType::Power);
                }
                break;

            case ID_PLUGINGUI_ENVELOP_CHANGE_CURVE_BEZIER:
                if (m_hoveredPointIndex != -1)
                {
                    ChangeCurve(m_hoveredPointIndex, CurveType::Bezier);
                }
                break;
        }
    }

    /// <summary>
    /// Найти предыдущую главную точку по координате х
    /// </summary>
    /// <param name="x"></param>
    /// <returns></returns>
    int EnvelopeGraphControl::FindPrevMainPoint(float x) const
    {
        int lastIndex = -1;
        for (int i = 0; i < static_cast<int>(m_allPoints.size()); ++i)
        {
            const Point& p = m_allPoints[i];
            if (p.x <= x && p.isMain)
            {
                lastIndex = i;
            }
        }
        return lastIndex;
    }

    /// <summary>
    /// Удалить главную точку
    /// </summary>
    /// <param name="index">Индекс точки</param>
    void EnvelopeGraphControl::DeleteMainPoint(int index)
    {
        // Только средние основные точки
        if (index > 0 && index < (int)m_allPoints.size() - 1)
        {
            const Point& p = *(m_allPoints.begin() + index);
            if (p.isMain) // Убеждаемся, что точка главная
            {
                m_allPoints.erase(m_allPoints.begin() + index);
                RecalculatePoints();
                Invalidate();
            }
        }
    }

    /// <summary>
    /// Изменить тип сплайна
    /// </summary>
    /// <param name="index"></param>
    /// <param name="curve"></param>
    void EnvelopeGraphControl::ChangeCurve(int index, CurveType curve)
    {
        // Только средние точки
        if (index > 0 && index < (int)m_allPoints.size() - 1)
        {
            Point& p = m_allPoints[index];
            if (!p.isMain && p.curve != curve) // Убеждаемся, что точка главная
            {
                p.curve = curve;
                Invalidate();
            }
        }
    }

    /// <summary>
    /// Перевернуть огибающую по вертикали
    /// </summary>
    void EnvelopeGraphControl::FlipVertical()
    {
        for (Point& pt : m_allPoints)
        {
            pt.y = 1 - pt.y;
            if (!pt.isMain)
            {
                pt.relativeY *= -1;
                pt.onAscendingCurve = !pt.onAscendingCurve;
            }
        }
        UpdateCurveDirections();
        Invalidate();
    }

    /// <summary>
    /// Перевернуть огибающую по горизонтали
    /// </summary>
    void EnvelopeGraphControl::FlipHorizontal()
    {
        std::vector<Point> newPoints;
        newPoints.reserve(m_allPoints.size());

        for (Point pt : m_allPoints | std::views::reverse)
        {
            pt.x = 1 - pt.x;
            if (!pt.isMain)
            {
                pt.relativeX *= -1;
                pt.onAscendingCurve = !pt.onAscendingCurve;
            }
            newPoints.push_back(std::move(pt));
        }

        m_allPoints.swap(newPoints);
        UpdateCurveDirections();
        Invalidate();
    }

    /// <summary>
    /// Перевернуть огибающую относительно центра
    /// </summary>
    void EnvelopeGraphControl::FlipByCenter()
    {
        std::vector<Point> newPoints;
        newPoints.reserve(m_allPoints.size());

        for (Point pt : m_allPoints | std::views::reverse)
        {
            pt.x = 1 - pt.x;
            pt.y = 1 - pt.y;
            if (!pt.isMain)
            {
                pt.relativeX *= -1;
                pt.relativeY *= -1;
                pt.onAscendingCurve = !pt.onAscendingCurve;
            }
            newPoints.push_back(std::move(pt));
        }

        m_allPoints.swap(newPoints);
        UpdateCurveDirections();
        Invalidate();
    }
}