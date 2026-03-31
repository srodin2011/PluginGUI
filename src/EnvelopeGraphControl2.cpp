#define NOMINMAX
#include "PluginGUI\include\EnvelopeGraphControl2.h"
#include "PluginGUI\include\PluginView.h"
#include <algorithm>
#include <numbers>
#include <fstream>
#include <filesystem>  

namespace PluginGUI
{
    //using namespace Gdiplus;

    EnvelopeGraphControl2::EnvelopeGraphControl2(const CRect& border)
        : Control(border)
    {
        // Начальные точки по умолчанию (как ADSR)
        //m_allPoints = {
        //    {0.0, 1., true},
        //    //{0.0, .99, false},
        //    {1.0, 0., true}
        //};
        m_allPoints = {
            {0.0f, 0.0f, true},
            {0.25f, 1.0f, true},
            {0.5f, 1.0f, true},
            {0.75f, 0.0f, true},
            {1.0f, 0.0f, true}
        };
        RecalculatePoints();
    }

    /// <summary>
    /// Нужно обрабатывать следующие кейсы:
    /// 1.Если удалили главную точку, то нужно удалить соседние промежуточные точки
    /// 2.Если между двумя главными точками нет промежуточной, ее необходимо добавить
    /// </summary>
    void EnvelopeGraphControl2::RecalculatePoints()
    {
        std::vector<Point> newPoints;
        newPoints.reserve(m_allPoints.size());

        size_t i = 0;
        while (i < m_allPoints.size())
        {
            if (m_allPoints[i].isMain)
            {
                newPoints.push_back(m_allPoints[i]);  // Сохраняем главную
                ++i;

                // ★ ПРОВЕРЯЕМ промежуточные до следующей главной ★
                size_t interCount = 0;
                while (i < m_allPoints.size() && !m_allPoints[i].isMain)
                {
                    ++interCount;
                    ++i;
                }

                // Если было РОВНО 1 промежуточную - возвращаем её
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


        // ★ ПРОХОД 2: Добавляем недостающие промежуточные ★
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
                    mid.x = (m_allPoints[i].x + m_allPoints[i + 1].x) * 0.5f;
                    mid.y = (m_allPoints[i].y + m_allPoints[i + 1].y) * 0.5f;
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
    void EnvelopeGraphControl2::MovePoint(int index, Point p, bool recursion)
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
            pNew.y = std::clamp(p.y, 0., 1.);

            if (index == 0) // Левая главная точка
            {
                // x не двигается
                pNew.x = 0;
                if (synchronizeEdgeValues​ && !recursion)
                {
                    Point& lastMainPoint = m_allPoints.back();
                    int lastMainIndex = (int)m_allPoints.size() - 1;
                    MovePoint(lastMainIndex, { lastMainPoint.x, pNew.y, true }, true);
                }
            }
            else if (index == m_allPoints.size() - 1) // Правая главная точка
            {
                if (!fixedRightPoint)
                {
                    // x ограничен [x левой главной точки,1] 
                    pNew.x = std::clamp(p.x, m_allPoints[prevMainIndex].x, 1.);
                }
                else
                {
                    pNew.x = pMoved.x;
                }

                if (synchronizeEdgeValues​ && !recursion)
                {
                    Point& firstMainPoint = m_allPoints.at(0);
                    int firstMainIndex = 0;
                    MovePoint(firstMainIndex, { firstMainPoint.x, pNew.y, true }, true);
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
                UpdateMidpoint(pMid, pPrevMain, pMoved, pNew);
            }

            if (nextMidIndex != -1)
            {
                const Point& pNextMain = m_allPoints[nextMainIndex];
                Point& pMid = m_allPoints[nextMidIndex];
                UpdateMidpoint(pMid, pNextMain, pMoved, pNew);
            }

            pMoved = pNew;
        }
        else
        {
            Point pPrevMain = m_allPoints[index - 1];
            Point pNextMain = m_allPoints[index + 1];

            double minY = std::min(pPrevMain.y, pNextMain.y);
            double maxY = std::max(pPrevMain.y, pNextMain.y);

            pMoved.y = std::clamp(p.y, minY, maxY);
        }
    }

    /// <summary>
    /// Обновить у промежуточной точки
    /// </summary>
    /// <param name="pMid">Промежуточная точка</param>
    /// <param name="pNeighborMain">Соседння главная точка</param>
    /// <param name="pMain">Главная точка</param>
    /// <param name="pNewMain">Новая главная точка, после перемещения</param>
    void EnvelopeGraphControl2::UpdateMidpoint(Point& pMid, const Point& pNeighborMain, const Point& pMain, const Point& pNewMain)
    {
        double dy1 = pMid.y - pNeighborMain.y;
        double dy2 = pMain.y - pNeighborMain.y;
        double t = (std::abs(dy2) > 1e-6f) ? dy1 / dy2 : 0.5f;

        // Применяем новый t
        pMid.y = pNeighborMain.y + t * (pNewMain.y - pNeighborMain.y);
        pMid.x = (pNewMain.x + pNeighborMain.x) * 0.5f;
    }

    bool EnvelopeGraphControl2::HitTest(int x, int y)
    {
        return (x >= border.left && y >= border.top && x <= border.right && y <= border.bottom);    

        //auto [gx, gy] = ToGraph((double)x, (double)y);
        //int idx = FindNearestPoint(gx, gy);
        //return idx != -1;
    }

    void EnvelopeGraphControl2::Draw(const CPaintDC& dc, bool hasFocus)
    {
        Gdiplus::Graphics g(оffScreenBitmap.get());
        g.Clear(Gdiplus::Color::Transparent);
        g.SetSmoothingMode(Gdiplus::SmoothingModeHighQuality);

        // Фон
        Gdiplus::SolidBrush bgBrush(m_BackgroundColor);
        g.FillRectangle(&bgBrush, Gdiplus::Rect(0, 0, border.Width(), border.Height()));

        // Сетка
        if (m_ShowGrid)
        {
            DrawGrid(g);
        }

        // Кривая
        DrawCurve(g);

        // Точки
        DrawPoints(g);

        if (m_DrawBorder)
        {
            DrawBorder2(g);
        }
    }

    EnvelopeGraphControl2::Point FindCubicThroughPoints(EnvelopeGraphControl2::Point p0, EnvelopeGraphControl2::Point p1, EnvelopeGraphControl2::Point p2, double t = 0.5f)
    {
        EnvelopeGraphControl2::Point newP1 = {
            (p1.x - 0.25f * p0.x - 0.25f * p2.x) / 0.5f,  // 0.625
            (p1.y - 0.25f * p0.y - 0.25f * p2.y) / 0.5f   // 1.500
        };
        return newP1;
    }

    EnvelopeGraphControl2::Point FindDiagonalPoints(EnvelopeGraphControl2::Point p0, EnvelopeGraphControl2::Point p1, EnvelopeGraphControl2::Point p2)
    {
        double t = (p1.y - p2.y) / (p0.y - p2.y);

        return {
            p0.x + t * (p2.x - p0.x),  // линейная по X
            p2.y + t * (p0.y - p2.y)   // линейная по Y
        };
    }

    double lerp(double a, double b, double f)
    {
        return a + f * (b - a);
    }

    double clamp(double v, double minv, double maxv)
    {
        return v < minv ? minv : (v > maxv ? maxv : v);
    }

    double smoothstep(double edge0, double edge1, double x)
    {
        x = clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
        return x * x * (3.0f - 2.0f * x);
    }
    double smoothpow(double x, double e)
    {
        return x > 0.0f ? pow(x, e) : 0.0f;
    }

    EnvelopeGraphControl2::Point EnvelopeGraphControl2::SerumEnvPoint(double t, Point p0, Point p1, Point p2)
    {
        double x = p0.x + t * (p2.x - p0.x);

        double u = 1.0f - t;

        // РЕЗКАЯ Serum: плато→обрыв (непрерывно!)
        double hold = 1.0f - smoothpow(t - 0.7f, 3.0f);
        double curve = hold * (p1.y * 2.0f - 1.0f);

        double y = p0.y * u + p2.y * t + curve * 0.6f;

        return { x, y, false };
    }

    EnvelopeGraphControl2::Point EnvelopeGraphControl2::hermite_spline(double t, Point p0, Point p1, Point p2)
    {
        // t в [0,1] - параметр от p0 к p1
        double t2 = t * t;
        double t3 = t2 * t;

        double m0 = (p1.y - p0.y) / (p1.x - p0.x);
        double m1 = (p2.y - p1.y) / (p2.x - p1.x);

        double mMax = 20;
        m0 = std::clamp(m0, -mMax, mMax);
        m1 = std::clamp(m1, -mMax, mMax);

        // Кубический Hermite сплайн - ВАША формула
        double y = (2. * t3 - 3. * t2 + 1.) * p0.y +
            (t3 - 2. * t2 + t) * m0 +
            (-2. * t3 + 3. * t2) * p2.y +
            (t3 - t2) * m1;

        // X интерполируем линейно
        double x = p0.x + t * (p2.x - p0.x);

        return { (double)x, (double)y, false };
    }

    double distributionFunction(double x, double n)
    {
        //// Логарифмическое отображение: 0.5 -> 1, симметрично относительно центра
        //double exponent = 1.0 - 2.0 * x;  // [1, -1] для x из [0,1]
        //return std::pow(n, exponent);
        return std::pow(n, -x);
    }

    double invertDistributionFunction(double y, double n)
    {
        return -std::log(y) / std::log(n); // Считаем x
    }

    void EnvelopeGraphControl2::crossSegment(Point A, Point B, Point& A1, Point& B1)
    {
        A1 = { A.x, B.y, A.isMain };
        B1 = { B.x, A.y, B.isMain };
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
    EnvelopeGraphControl2::Point EnvelopeGraphControl2::reflectBySegment(Point A, Point B, Point P)
    {
        Point A1, B1;
        crossSegment(A, B, A1, B1);

        double dx = B1.x - A1.x;  // направляющий
        double dy = B1.y - A1.y;

        double a = dy;      // нормаль: поворот на 90°
        double b = -dx;     // (dy, -dx)
        double c = A1.x * B1.y - B1.x * A1.y;

        double denom = a * a + b * b;
        double d = a * P.x + b * P.y + c;

        return {
            P.x - 2 * a * d / denom,
            P.y - 2 * b * d / denom,
            P.isMain
        };
    }

    EnvelopeGraphControl2::Point EnvelopeGraphControl2::reflectByCenter(Point A, Point B, Point P)
    {
        Point C = { (A.x + B.x) / 2., (A.y + B.y) / 2. };
        return { 2.0f * C.x - P.x, 2.0f * C.y - P.y, P.isMain };
    }

    /// <summary>
    /// 
    /// </summary>
    /// <param name="t"></param>
    /// <param name="p0"></param>
    /// <param name="p1"></param>
    /// <param name="p2"></param>
    /// <returns></returns>
    EnvelopeGraphControl2::Point EnvelopeGraphControl2::CalculateСurvePoint(double t, Point p0, Point p1, Point p2)
    {
        double x = p0.x + (p2.x - p0.x) * t;

        if (p0.y == p2.y)
        {
            return { x, p0.y, false };
        }

        double center = (p0.y + p2.y) / 2;
        double halfHeight = std::abs(p2.y - p0.y) / 2;
        double diff = (p1.y - center) / halfHeight;

        // Коррекция diff ----
        const double minDiff = -0.9;   // 0.0001% от высоты
        const double maxDiff = 0.9;   // 0.0001% от высоты

        double diff1 = std::clamp(diff, minDiff, maxDiff);
        p1.y = center + diff * halfHeight;
        // Коррекция diff ----

        //double power = distributionFunction(-diff1, distributionFunctionBase);
        double power = distributionFunction(-std::abs(diff), distributionFunctionBase);

        double y1;
        double yNormPowered;

        if (diff > 0 && p2.y < p0.y || diff < 0 && p2.y > p0.y)
        {
            yNormPowered = std::pow(t, power); // Возрастает в нормализованном диапазоне [0;1] 
            y1 = p0.y + (p2.y - p0.y) * yNormPowered; // от p0.y → p2.y (рост)
        }
        else
        {
            yNormPowered = std::pow(1 - t, power); // Убывает в нормализованном диапазоне [0;1] 
            y1 = p2.y - (p2.y - p0.y) * yNormPowered; // от p0.y → p2.y (рост)
        }

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

        return { x, (double)y1, false };
    }

    EnvelopeGraphControl2::Point EnvelopeGraphControl2::CalculatePowerPoint(double t, Point p0, Point p1, Point p2)
    {
        double center = (p0.y + p2.y) / 2;
        double halfHeight = std::abs(p2.y - p0.y) / 2;
        double diff = (p1.y - center) / halfHeight;

        Point pCurve;
        if (diff > 0)
        {
            Point p1Symmetrical = symmetricByCenter(p0, p2, p1); 
            Point pCurveSymmetrical = CalculateСurvePoint(t, p0, p1Symmetrical, p2);
            pCurve = reflectByCenter(p0, p2, pCurveSymmetrical);
        }
        else
        {
            pCurve = CalculateСurvePoint(t, p0, p1, p2);
        }

        return pCurve;
    }

    double slog(double x)
    {
        return std::log(x > 0 ? x : 1e-20f);
    }

    EnvelopeGraphControl2::Point EnvelopeGraphControl2::RestoreP1ByCurvePoint(double t, Point p0, Point pCurve, Point p2)
    {
        //double cross = (p2.x - p0.x) * (pCurve.y - p0.y) - (p2.y - p0.y) * (pCurve.x - p0.x);

        Point p;
        double center = (p0.y + p2.y) / 2;
        double halfHeight = std::abs(p2.y - p0.y) / 2;

        //if (pCurve.y == p0.y)
        //{
        //    return { 0, p0.y, false };
        //}
        //if (pCurve.y == p2.y)
        //{
        //    return { 0, p2.y, false };
        //}

        //if (pCurve.y > center)
        ////if (cross > 0.0f) // pCurve выше [p0;p2]
        //{
        //    p = reflectBySegment(p0, p2, pCurve);
        //}
        //else
        //{
        //    p = pCurve;
        //}

        p = pCurve;
        //double y = p0.y + (p2.y - p0.y) * t; // Диапазон [p0.y;p2.y]
        //double yNorm = (y - p0.y) / (p2.y - p0.y); // Нормализованный диапазон [0;1]
        //double yNormPowered = (p2.y - p.y) / (p2.y - p0.y);
        //double y1 = p0.y + (p2.y - p0.y) * yNormPowered;

        //double y1 = p0.y + (p2.y - p0.y) * yNormPowered;
        //double y1 = p2.y - (p2.y - p0.y) * yNormPowered;
        //double y1 = p0.y + (p2.y - p0.y) * yNormPowered;

        //double yNormPowered = (p.y - p0.y) / (p2.y - p0.y);
        //double direction = (p2.y >= p0.y) ? 1.0f : -1.0f;
        //double yNormPowered = (p.y - p0.y) / (std::abs(p2.y - p0.y) * direction);
        //double yNormPowered = (p.y - p0.y) / (p2.y - p0.y);

        double y1;
        double yNormPowered;

        double power;
        const double minDiff = 1e-20;   // 0.0001% от высоты
        const double maxDiff = 1- 1e-20;   // 0.0001% от высоты

        if (pCurve.y > center && p2.y < p0.y || pCurve.y < center && p2.y > p0.y)
        {
            yNormPowered = (p.y - p0.y) / (p2.y - p0.y);
            yNormPowered = std::clamp(yNormPowered, minDiff, maxDiff);
            power = slog(yNormPowered) / slog(t);
        }
        else
        {
            yNormPowered = (p2.y - p.y) / (p2.y - p0.y);
            yNormPowered = std::clamp(yNormPowered, minDiff, maxDiff);
            power = slog(yNormPowered) / slog(1 - t);
        }



        //yNormPowered = std::clamp(yNormPowered, 1e-12, 1.0 - 1e-12);

        //if (pCurve.y < center)
        //{
        //    if (p2.y > p0.y) // Возрастает
        //    {
        //        yNormPowered = (p.y - p0.y) / (p2.y - p0.y);
        //        power = slog(yNormPowered) / slog(t);
        //    }
        //    else // Убывает
        //    {

        //        yNormPowered = (p2.y - p.y) / (p2.y - p0.y);
        //        power = slog(yNormPowered) / slog(1 - t);
        //    }
        //}
        //else
        //{
        //    if (p2.y > p0.y) // Возрастает
        //    {
        //        yNormPowered = (p2.y - p.y) / (p2.y - p0.y);
        //        power = slog(yNormPowered) / slog(1 - t);
        //    }
        //    else // Убывает
        //    {
        //        yNormPowered = (p.y - p0.y) / (p2.y - p0.y);
        //        power = slog(yNormPowered) / slog(t);
        //    }
        //}

        //double center = (p0.y + p2.y) / 2;
        //double halfHeight = std::abs(p2.y - p0.y) / 2;
        //double diff = (p1.y - center) / halfHeight;

        //double power = distributionFunction(-std::abs(diff), distributionFunctionBase);

        double diffAbs = -invertDistributionFunction(power, distributionFunctionBase);
        if (diffAbs > 0.9)
        {
            diffAbs = 0.9;
        }

        //double center = (p0.y + p2.y) / 2;

        double p1_y = (pCurve.y > center ? diffAbs : -diffAbs) * halfHeight + center;

        return { t, p1_y, false };
    }

    EnvelopeGraphControl2::Point EnvelopeGraphControl2::symmetricByCenter(Point A, Point B, Point P)
    {
        double centerY = (A.y + B.y) / 2;
        return { P.x, P.y - 2. * (P.y - centerY), P.isMain };
    }


    EnvelopeGraphControl2::Point EnvelopeGraphControl2::CalculateIntermediatePoint(double t, Point p0, Point p1, Point p2)
    {
        Point pCurve;
        double center = (p0.y + p2.y) / 2;
        if (p1.y > center)
        {
            Point pSymmetrical = symmetricByCenter(p0, p2, p1); // Построить точку, симметричную p1 относительно центра [p0;p2]
            Point p = CalculateСurvePoint(0.5f, p0, pSymmetrical, p2); // Вычислить точку кривой для симметричной точки
            pCurve = symmetricByCenter(p0, p2, p); // Построить точку, симметричную p относительно центра [p0;p2] 
        }
        else
        {
            pCurve = CalculateСurvePoint(0.5f, p0, p1, p2);
        }
        return pCurve;
    }

    EnvelopeGraphControl2::Point EnvelopeGraphControl2::RestoreP1ByIntemediatePoint(double t, Point p0, Point pCurve, Point p2)
    {
        double center = (p2.y + p0.y) / 2;
        //double cross = (p2.x - p0.x) * (pCurve.y - p0.y) - (p2.y - p0.y) * (pCurve.x - p0.x);

        Point p1;

        if (pCurve.y > center) // pCurve выше [p0;p2]
        {
            Point pCurveSymmetrical = symmetricByCenter(p0, p2, pCurve); // Построить точку, симметричную p1 относительно центра [p0;p2]
            Point p1Symmetrical = RestoreP1ByCurvePoint(0.5f, p0, pCurveSymmetrical, p2);
            p1 = symmetricByCenter(p0, p2, p1Symmetrical); // Построить точку, симметричную p относительно центра [p0;p2] 
        }
        else
        {
            p1 = RestoreP1ByCurvePoint(0.5f, p0, pCurve, p2);
        }
        return p1;
    }


    EnvelopeGraphControl2::Point interpolate(double t, EnvelopeGraphControl2::Point p0, EnvelopeGraphControl2::Point p1, EnvelopeGraphControl2::Point p2)
    {
        // 1. Обычная квадратичная Безье (или линейная между сегментами)
        double mid_y = 0.5f * (p0.y + p2.y);
        double bez_y = (1 - t) * (1 - t) * p0.y + 2 * t * (1 - t) * p1.y + t * t * p2.y;

        // 2. Отклонение от прямой линии p0-p2
        double linear_y = p0.y + t * (p2.y - p0.y);
        double d = bez_y - linear_y;  // отклонение от прямой [-0.5,0.5]

        // 3. КВАДРАТИЧНОЕ УСИЛЕНИЕ отклонения
        double abs_d = fabs(d);
        //double amplified_d = 2.0f * abs_d * abs_d * (d >= 0 ? 1.0f : -1.0f);

        double amplified_d = (exp(4.0f * abs_d) - 1.0f) / (exp(2.0f) - 1.0f) * (d >= 0 ? 1.0f : -1.0f);

        // 4. Применяем усиленное отклонение обратно
        double result_y = linear_y + 0.5f * amplified_d;

        return { t, result_y };
    }

    EnvelopeGraphControl2::Point interpolate3(double t, EnvelopeGraphControl2::Point p0, EnvelopeGraphControl2::Point p1, EnvelopeGraphControl2::Point p2)
    {
        // 1. Обычная квадратичная Безье (или линейная между сегментами)
        double mid_y = 0.5f * (p0.y + p2.y);
        double bez_y = (1 - t) * (1 - t) * p0.y + 2 * t * (1 - t) * p1.y + t * t * p2.y;

        // 2. Отклонение от прямой линии p0-p2
        double linear_y = p0.y + t * (p2.y - p0.y);
        double d = bez_y - linear_y;  // отклонение от прямой [-0.5,0.5]

        // 3. КВАДРАТИЧНОЕ УСИЛЕНИЕ отклонения
        double abs_d = fabs(d);
        //double amplified_d = 2.0f * abs_d * abs_d * (d >= 0 ? 1.0f : -1.0f);

        double amplified_d = (exp(4.0f * abs_d) - 1.0f) / (exp(2.0f) - 1.0f) * (d >= 0 ? 1.0f : -1.0f);

        // 4. Применяем усиленное отклонение обратно
        double result_y = linear_y + 0.5f * amplified_d;

        return { t, result_y };
    }

    void EnvelopeGraphControl2::DrawCurve(Gdiplus::Graphics& g)
    {
        if (m_allPoints.empty()) return;

        Gdiplus::Pen linePen(LineColor, 2.0f);

        // ★ ОДИН ЦИКЛ: [P0, M01, P1, M12, P2...] ★
        Point p0 = {0, 0, true};
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
                    if (p0.x != 0. || p0.y != 0.)
                    {
                        boredPath.AddLine(ToScreenPointF(0., 0.), ToScreenPointF(p0.x, p0.y));
                    }
                }
                else
                {
                    // Предыдущая = промежуточная M01, текущая = P1
                    Point p1 = m_allPoints[i - 1];  // ★ ПРЯМО предыдущая! ★
                    const Point& p2 = pt;

                    // ★ КРИВАЯ БЕЗЬЕ ★
                    //Point p1_test = { 0.5, .99, false };
                    //Point pCurvTest = CalculateСurvePoint(0.5f, p0, p1_test, p2);
                    //Point p1_test_2 = RestoreP1ByCurvePoint(0.5f, p0, pCurvTest, p2);

                    //Point pCurvTest2 = { 0.5, 0.99, false };
                    //Point p1_test_3 = RestoreP1ByCurvePoint(0.5f, p0, pCurvTest2, p2);

                    //Point p1_test = RestoreP1ByIntemediatePoint(0.5f, p0, p1, p2);
                    //Point pCurvTest = CalculateСurvePoint2(0.5f, p0, p1_test, p2);

                    static bool output = true;
                    std::ofstream file;
                    if (output)
                    {
                        file.open("Out.txt");
                        file << std::fixed << std::setprecision(6);
                    }

                    //Point p1_curve = RestoreP1ByIntemediatePoint(0.5f, p0, p1, p2);
                    Point p1_curve = RestoreP1ByCurvePoint(0.5f, p0, p1, p2);
                    Point p1_rest = CalculateСurvePoint(0.5f, p0, p1_curve, p2);
                    //Point p1_curve = p1;
                    int N = 100;
                    for (int seg = 1; seg <= N; ++seg)
                    {
                        double t = static_cast<double>(seg - 1) / (N - 1);
                        Point p = CalculateСurvePoint(t, p0, p1_curve, p2);
                        //Point p = CalculateСurvePoint2(t, p0, p1_curve, p2);
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

        if (p0.x != 1. || p0.y != 0.)
        {
            boredPath.AddLine(prevPoint, ToScreenPointF(p0.x, 0.));
        }
        //boredPath.AddLine(ToScreenPointF(1., 0.), Gdiplus::PointF(0., 0.));
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
    Gdiplus::PointF EnvelopeGraphControl2::ToScreenPointF(double x, double y) const
    {
        int w = border.Width();
        int h = border.Height();
        return Gdiplus::PointF(
            (Gdiplus::REAL)(x * w),
            (Gdiplus::REAL)((1.0 - y) * h)
        );
    }

    void EnvelopeGraphControl2::DrawPoints(Gdiplus::Graphics& g)
    {
        Gdiplus::Pen mainPen(MainPointColor, 1.);
        Gdiplus::Pen interPen(MainPointColor, 1.);
        Gdiplus::SolidBrush mainBrush(MainPointColor);
        Gdiplus::SolidBrush interBrush(InterPointColor);
        Gdiplus::SolidBrush focusBrush(FocusColor);
        const double pointRadius = 3;

        for (size_t i = 0; i < m_allPoints.size(); ++i)
        {
            const Point& pt = m_allPoints[i];
            CPoint screenPt = ToScreen(pt.x, pt.y);

            // ★ Фокус для dragged точки ★
            if (m_dragActive && m_draggedAllPointsIndex != -1 && m_draggedAllPointsIndex == i || m_hoveredPointIndex == i)
            {
                Gdiplus::RectF focus((Gdiplus::REAL)(screenPt.x - pointRadius * 3), (Gdiplus::REAL)(screenPt.y - pointRadius * 3),
                    (Gdiplus::REAL)(pointRadius * 6), (Gdiplus::REAL)(pointRadius * 6));
                g.FillEllipse(&focusBrush, focus);
            }

            if (pt.isMain)
            {
                Gdiplus::RectF circle((Gdiplus::REAL)(screenPt.x - pointRadius), (Gdiplus::REAL)(screenPt.y - pointRadius),
                    (Gdiplus::REAL)(pointRadius * 2), (Gdiplus::REAL)(pointRadius * 2));
                g.FillEllipse(&mainBrush, circle);
                g.DrawEllipse(&mainPen, circle);
            }
            else
            {
                Gdiplus::RectF circle((Gdiplus::REAL)(screenPt.x - pointRadius / 1.5), (Gdiplus::REAL)(screenPt.y - pointRadius / 1.5), (Gdiplus::REAL)(pointRadius * 1.5), (Gdiplus::REAL)(pointRadius * 1.5));
                if (m_dragActive && i == m_draggedAllPointsIndex)
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

    void EnvelopeGraphControl2::DrawGrid(Gdiplus::Graphics& g)
    {
        Gdiplus::Pen gridPen(m_GridColor, 0.8f);
        //Gdiplus::Pen gridPen2(m_GridColor, 4.);
        Gdiplus::Color gridColor2 = Gdiplus::Color::MakeARGB(/*m_GridColor.GetAlpha() * 2*/255,
            m_GridColor.GetR(), m_GridColor.GetG(), m_GridColor.GetB());
        Gdiplus::Pen gridPen2(gridColor2, 1.5f);


        int w = border.Width();
        int h = border.Height();

        // Вертикальные линии
        for (int i = 0; i <= m_HorizontalDivisions; ++i)
        {
            Gdiplus::Pen* pen;
            if(i % (m_HorizontalDivisions / 2 ))
            { 
                pen = &gridPen;
            }
            else
            {
                pen = &gridPen2;
            }
            double x = (double)i / m_HorizontalDivisions * w;
            g.DrawLine(pen, (int)x, 0, (int)x, h);
        }

        // Горизонтальные линии
        for (int i = 0; i <= m_VerticalDivisions; ++i)
        {
            Gdiplus::Pen* pen;
            if (i % (m_VerticalDivisions / 2))
            {
                pen = &gridPen;
            }
            else
            {
                pen = &gridPen2;
            }
            double y = (double)i / m_VerticalDivisions * h;
            g.DrawLine(pen, 0, (int)y, w, (int)y);
        }
    }

    CPoint EnvelopeGraphControl2::ToScreen(double x, double y) const
    {
        int w = border.Width();
        int h = border.Height();
        return CPoint(
            (int)(x * w),              // X: 0→w
            (int)((1.0f - y) * h)      // Y: 1→0 (сверху), 0→h (снизу) ✅
        );
    }


    std::pair<double, double> EnvelopeGraphControl2::ToGraph(double screenX, double screenY) const
    {
        //double localX = screenX - border.left;
        //double localY = screenY - border.top;

        //int w = border.Width();
        //int h = border.Height();

        //return {
        //    localX / w,              // X: 0.0 (слева) → 1.0 (справа)
        //    1.0f - (localY / h)      // Y: 1.0 (сверху) → 0.0 (снизу) ✅
        //};
        double localX = screenX - border.left;
        double localY = screenY - border.top;

        int w = border.Width();
        int h = border.Height();

        // ★ ЛИНЕЙНЫЙ graph → пиксели ★
        double linearX = static_cast<double>(localX / w);
        double linearY = 1 - static_cast<double>(localY / h);

        // ★ НЕЛИНЕЙНЫЙ mapping: края медленнее ★
        //double graphX = SmoothMap(linearX, Axis::X);
        //double graphY = SmoothMap(linearY, Axis::Y);  // Инвертируем Y
        double graphX = linearX;
        double graphY = linearY;  // Инвертируем Y

        return { graphX, graphY };
    }

    double EnvelopeGraphControl2::SmoothMap(double t, Axis axis) const
    {
        if (axis == Axis::Y)
        {
            const double k = 0.3;

            // ★ ЕДИНЫЙ clamp по оси ★
            double clampZonePx = 2.0;
            double size = (axis == Axis::X) ? border.Width() : border.Height();
            double clampZone = clampZonePx / size;

            if (t < clampZone) return 0.0;
            if (t > 1.0 - clampZone) return 1.0;

            //double power = 1.0 / (1.0 + k * (1.0 - t * 2.0));
            double power = 1.0 / (1.0 + k * (t * 2.0 - 1.0));
            return std::pow(t, power);
        }
        else
        {
            return t;
        }
    }

    float EnvelopeGraphControl2::GetAdaptiveThreshold(Axis axis, float pointPos) const
    {
        float baseFraction = 0.04f;

        // ★ СИММЕТРИЯ: расстояние ОТ ЦЕНТРА! ★
        float distanceFromCenter = std::abs(pointPos - 0.5f);  // 0.1↔0.9=0.4
        float sens = static_cast<float>(SmoothMap(distanceFromCenter, axis));

        return baseFraction * (1.0f + 1.5f * sens);
    }

    float EnvelopeGraphControl2::GetPointScore(float pointX, float pointY, float graphX, float graphY) const
    {
        float threshX = GetAdaptiveThreshold(Axis::X, pointX);
        float threshY = GetAdaptiveThreshold(Axis::Y, pointY);

        float dx = fabsf(pointX - graphX);
        float dy = fabsf(pointY - graphY);

        if ((dx * dx) / (threshX * threshX) + (dy * dy) / (threshY * threshY) > 1.0f)
            return FLT_MAX;

        float dxPx = (pointX - graphX) * static_cast<float>(border.Width());
        float dyPx = (pointY - graphY) * static_cast<float>(border.Height());

        return dxPx * dxPx + dyPx * dyPx;
    }

    int EnvelopeGraphControl2::FindNearestPoint(float graphX, float graphY) const
    {
        int nearest = -1;
        float bestScore = FLT_MAX;

        for (int i = 0; i < (int)m_allPoints.size(); ++i)
        {
            float score = GetPointScore(m_allPoints[i].x, m_allPoints[i].y, graphX, graphY);
            if (score < bestScore)
            {
                bestScore = score;
                nearest = i;
            }
        }
        return nearest;
    }

    //int EnvelopeGraphControl::FindNearestPoint(double x, double y, double threshold)
    //{
    //    double minDist = threshold * threshold;
    //    int nearest = -1;

    //    for (int i = 0; i < (int)m_allPoints.size(); ++i)
    //    {
    //        double dx;
    //        double dy;

    //        if ((i + 1) % 2 == 0) // Промежуточная точка
    //        {
    //            // Найти соседние главные точки
    //            Point p0 = m_allPoints[i - 1];
    //            Point p1 = m_allPoints[i];
    //            Point p2 = m_allPoints[i + 1];

    //            /* Новая контрольная точка -------------------------
    //            Point p = CalculateIntermediatePoint(0.5f, p0, p1, p2); // Новая промежуточная точка

    //            dx = p.x - x;
    //            dy = p.y - y;
    //            ------------------------------------- */
    //            dx = p1.x - x;
    //            dy = p1.y - y;
    //        }
    //        else // Основная точка
    //        {
    //            dx = m_allPoints[i].x - x;
    //            dy = m_allPoints[i].y - y;
    //        }

    //        double distSq = dx * dx + dy * dy;

    //        if (distSq < minDist)
    //        {
    //            minDist = distSq;
    //            nearest = i;
    //        }
    //    }
    //    return nearest;
    //}

    void EnvelopeGraphControl2::OnLButtonDown(UINT nFlags, CPoint p)
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
            else
            {
                m_draggedAllPointsIndex = idx;           // ★ ЗАПОМИНАЕМ ИНДЕКС ★
                m_draggedPoint = &m_allPoints[idx];
                m_dragActive = true;
                SetValueChangeMode(true);
                if (m_pPluginView)
                {
                    m_pPluginView->SetCapture();
                }
            }
        }
    }

    BOOL EnvelopeGraphControl2::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
    {
        return TRUE;
    }

    void EnvelopeGraphControl2::OnMouseLeave()
    {
        if (m_hoveredPointIndex != -1)
        {
            m_hoveredPointIndex = -1;
            Invalidate();
        }
        Control::OnMouseLeave();
    }

    void EnvelopeGraphControl2::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
    {
    }

    void EnvelopeGraphControl2::OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags)
    {
    }

    void EnvelopeGraphControl2::OnSysKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
    {
        bool isAltPressed = (nChar == VK_MENU);
        if (isAltPressed)
        {
            if (m_dragActive)
            {
                POINT cursorPos;
                GetCursorPos(&cursorPos);           // Экранные координаты
                m_pPluginView->ScreenToClient(&cursorPos);         // Переводим в локальные

                auto [gx, gy] = ToGraph(cursorPos.x, cursorPos.y);
                auto [gridX, gridY] = SnapToGrid(gx, gy);
                Point p = { gridX, gridY, m_draggedPoint->isMain };
                MovePoint(m_draggedAllPointsIndex, p);
                Invalidate();

                //if (m_lastMouseX != -1 && m_lastMouseY != 0)
                //{
                //}
            }
        }
    }

    void EnvelopeGraphControl2::OnSysKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags)
    {
        bool isAltPressed = (nChar == VK_MENU);
        if (isAltPressed)
        {
            if (m_dragActive)
            {
                POINT cursorPos;
                GetCursorPos(&cursorPos);           // Экранные координаты
                m_pPluginView->ScreenToClient(&cursorPos);         // Переводим в локальные

                auto [gx, gy] = ToGraph(cursorPos.x, cursorPos.y);
                Point p = { gx, gy, m_draggedPoint->isMain };
                MovePoint(m_draggedAllPointsIndex, p);
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

    void EnvelopeGraphControl2::OnMouseMove(UINT nFlags, CPoint p)
    {
        Control::OnMouseMove(nFlags, p);    

        m_lastMouseX = p.x;
        m_lastMouseY = p.y;

        //if (!HitTest(p.x, p.y))
        //{
        //    return;
        //}

        auto [gx, gy] = ToGraph(p.x, p.y);
        int idx = FindNearestPoint(gx, gy);  // Ищем ближайшую точку

        // Обработка курсора
        if (idx != -1)
        {
            Point p_ = m_allPoints[idx];
            if (p_.isMain) // Основная точка
            {
                if (idx == 0 || idx == m_allPoints.size() - 1 && fixedRightPoint)
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
                SetCursor(LoadCursor(NULL, IDC_SIZENS));
            }
        }
        else
        {
            SetCursor(LoadCursor(NULL, IDC_ARROW));
        }

        if (m_dragActive && m_draggedAllPointsIndex != -1 && m_draggedPoint->x >= 0)
        {
            auto [gx, gy] = ToGraph(p.x, p.y);

            if (m_draggedPoint->isMain)
            {
                bool isAltPressed = (GetKeyState(VK_MENU) & 0x8000) != 0;
                if ((GetKeyState(VK_MENU) & 0x8000) != 0) // Alt нажата
                {
                    auto [gridX, gridY] = SnapToGrid(gx, gy);
                    gx = gridX;
                    gy = gridY;
                }
            }

            Point pNew = { gx, gy, m_draggedPoint->isMain };
            MovePoint(m_draggedAllPointsIndex, pNew);

            Invalidate();
        }
        else
        {
            // Подсветка точки
            if (idx != m_hoveredPointIndex)
            {
                m_hoveredPointIndex = idx;  // Изменился hover
                Invalidate();  // Перерисовка только при смене!
            }
        }
    }

    std::pair<double, double> EnvelopeGraphControl2::SnapToGrid(double x, double y)
    {
        double cellWidth = 1.0f / HorizontalDivisions;
        double cellHeight = 1.0f / VerticalDivisions;

        // Математическое округление к ближайшему узлу сетки
        int gridX = static_cast<int>(std::floor(x / cellWidth + 0.5f));
        int gridY = static_cast<int>(std::floor(y / cellHeight + 0.5f));

        // Координаты узла сетки
        double snappedX = gridX * cellWidth;
        double snappedY = gridY * cellHeight;

        // Ограничение в [0,1]
        snappedX = std::clamp(snappedX, 0.0, 1.0);
        snappedY = std::clamp(snappedY, 0.0, 1.0);

        return {snappedX, snappedY};
    }

    void EnvelopeGraphControl2::OnLButtonUp(UINT nFlags, CPoint p)
    {
        if (m_dragActive)
        {
            m_dragActive = false;
            m_draggedAllPointsIndex = -1;
            SetValueChangeMode(false);
            if (m_pPluginView) ReleaseCapture();
            Invalidate();
        }
        Control::OnLButtonUp(nFlags, p);
    }

    void EnvelopeGraphControl2::OnLButtonDblClk(UINT nFlags, CPoint point)
    {
        Control::OnLButtonDblClk(nFlags, point);

        auto [gx, gy] = ToGraph(point.x, point.y);
        int idx = FindNearestPoint(gx, gy);

        if (idx != -1 && m_allPoints[idx].isMain)
        {
            // Двойной клик по основной точке - удаляем её (кроме первой и последней)
            if (idx > 0 && idx < (int)m_allPoints.size() - 1)
            {  // Только средние основные точки
                m_allPoints.erase(m_allPoints.begin() + idx);
                RecalculatePoints();
                Invalidate();
            }
        }
        if (idx == -1)
        {
            int prevMainIndex = FindPrevMainPoint(gx);
            if (prevMainIndex != -1)
            {
                // Удалить промежуточную точку справа
                m_allPoints.erase(m_allPoints.begin() + prevMainIndex + 1);

                // Создать ноиую главную точку по координате [gx,gy]
                Point newPoint = { gx, gy, true };
                m_allPoints.insert(m_allPoints.begin() + prevMainIndex + 1, newPoint);

                RecalculatePoints();
                Invalidate();
            }
        }
    }

    int EnvelopeGraphControl2::FindPrevMainPoint(double x) const
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

    void EnvelopeGraphControl2::Invalidate()
    {
        if (m_pPluginView)
        {
            m_pPluginView->InvalidateRect(Border, false);
            //m_pPluginView->InvalidateRect(CRect(m_StartPoint.x, m_StartPoint.y, m_StartPoint.x + m_Size, m_StartPoint.y + m_Size), false);
        }
    }

}