#include <afxwin.h>

#include "PluginGUI\include\MouseCalibration.h"
#include "PluginGUI\include\Utils.h"
#include <cmath>
#include <algorithm>

namespace PluginGUI
{
    /// <summary>
    /// Синглтон класс
    /// </summary>
    /// <returns></returns>
    MouseCalibration& MouseCalibration::getInstance()
    {
        static MouseCalibration instance;
        return instance;
    }

    /// <summary>
    /// Вернуть отношение тиков мышки на пиксель. 
    /// Пока калибровка не завершена — возвращаем значение по умолчанию
    /// </summary>
    /// <returns></returns>
    float MouseCalibration::getTicksPerPixel() const
    {
        static const float defaultTicksPerPixel =  200.f / 145.f;

        return calibrating ? defaultTicksPerPixel : ticksPerPixel;
    }

    /// <summary>
    /// Вызвать продолжение калибровки
    /// </summary>
    /// <param name="dy"></param>
    void MouseCalibration::updateCalibration(long dy, long eventTime)
    {
        // Проверка калибровки
        if (calibrated)
        {
            return;
        }
        // Автозапуск калибровки если не запущена
        if (!calibrating)
        {
            startCalibration(eventTime);
            return;
        }

        POINT currentPt;
        GetCursorPos(&currentPt);

        sampleRawTicks += abs(dy);
        sampleScreenPixels += abs(currentPt.y - lastScreenY);

        lastScreenY = currentPt.y;

        if (eventTime - lastEventTime >= ftameTime)
        {
            // Прошел 1 фрейм времени. Проверяем, что изменилось

            if (sampleScreenPixels >= requiredPixelsPerSample) // Накопили требуемое перемещение
            {
                sampleTime += eventTime - lastEventTime;

                // Звершить семпл
                float ratio = (float)sampleRawTicks / sampleScreenPixels;
                ratios[sampleCount++] = ratio;

                //TRACE("Sample: sampleScreenPixels = %d sampleRawTicks = %d ratio = %.3f sampleTime = %d\n", sampleScreenPixels, sampleRawTicks, ratio, sampleTime);

                sampleRawTicks = 0;
                sampleScreenPixels = 0;
                sampleTime = 0;
            }

            if (sampleCount >= requiredSamples)
            {
                finishCalibration(totalScreenPixels);
            }

            lastEventTime = eventTime;
        }

        //if (deltaScreenPixels >= 1)
        //{

        //    TRACE("1 iteration: deltaScreenPixels = %d rawTicksSinceLastScreenUpdate = %d\n", deltaScreenPixels, rawTicksSinceLastScreenUpdate);

        //    rawTicksSinceLastScreenUpdate = 0; // Сбрасываем счетчик тиков мышки

        //    if (sampleScreenPixels >= requiredPixelsPerSample) // Накопили требуемое перемещение
        //    {
        //        //TRACE("1 sample: sampleScreenPixels = %d sampleRawTicks = %d\n", sampleScreenPixels, sampleRawTicks);

        //        // Звершить семпл
        //        float ratio = (float)sampleRawTicks / sampleScreenPixels;
        //        ratios[sampleCount++] = ratio;
        //        sampleRawTicks = 0;
        //        sampleScreenPixels = 0;
        //    }

        //    if (sampleCount >= requiredSamples)
        //    {
        //        finishCalibration(totalScreenPixels);
        //    }
        //}


        // Игнорируем большие скачки в тиках и нулевое перемещение в пикселях
        //if (deltaRawTicks < 3 || deltaRawTicks > 8 || deltaScreenPixels < 2 || deltaScreenPixels > 6)
        //{
        //    return;
        //}

        // Убираем только шум
        //if (deltaRawTicks < 1 || deltaScreenPixels < 1)
        //{
        //    skipedCount++;
        //    return;
        //}

        updateCalibrationIndex++;



        //float ratio = (float)deltaRawTicks / deltaScreenPixels;


        ////if (ratio >= 0.8 && ratio <= 1.4)
        //{
        //    //TRACE("Calibrating: %d ticks; %d pixels %.3f\n", deltaRawTicks, deltaScreenPixels, (float)deltaRawTicks / deltaScreenPixels);

        //    // Суммируем абсолютные тики Raw Input
        //    totalRawTicks += deltaRawTicks;
        //    totalScreenPixels += deltaScreenPixels;

        //    if (sampleCount < requiredSamples)
        //    {
        //        ratios[sampleCount++] = ratio;
        //    }
        //    // Калибровка завершена
        //    //if (totalScreenPixels >= calibrationTargetPixels)
        //    //{
        //    //    finishCalibration(totalScreenPixels);
        //    //}
        //    if (sampleCount >= requiredSamples)
        //    {
        //        finishCalibration(totalScreenPixels);
        //    }
        //}
    }

    void MouseCalibration::startCalibration(long eventTime)
    {
        calibrating = true;
        totalRawTicks = 0;
        totalScreenPixels = 0;
        sampleRawTicks = 0;
        sampleScreenPixels = 0;
        lastEventTime = eventTime;
        sampleTime = 0;

        POINT pt;
        GetCursorPos(&pt);
        lastScreenY = pt.y;
    }

    void MouseCalibration::finishCalibration(int finalScreenPixels)
    {
        // Соотношение тиков/пикселей
        ticksPerPixel = (float)totalRawTicks / finalScreenPixels;
        calibrating = false;
        calibrated = true;
        float ticksPerPixel_ = 0.f;
        auto time_ms = measureExecutionTime([&]()
            {
                ticksPerPixel_ = processStatistics();
            });
        TRACE("Время выполнения \"processStatistics\" = %.3f\n", time_ms);

        //float ticksPerPixel_ = processStatistics();

#ifdef _DEBUG
        TRACE("Calibration complete: %d ticks / %d px = %.10f Statistic ticks/pixel = %.10f\n",
            totalRawTicks, finalScreenPixels, ticksPerPixel, ticksPerPixel_);
#endif
        ticksPerPixel = ticksPerPixel_;
    }

    float MouseCalibration::processStatistics()
    {
        markOutliers3SigmaInPlace();

        size_t validCount = countValid();
        if (validCount < 30) return 1.0;

        // ✅ ПРОСТО возвращаем результат!
        return winsorizedMeanFromNonZero();
    }

    void MouseCalibration::markOutliers3SigmaInPlace()
    {
        size_t n = ratios.size();

        // Среднее по НЕ-нулевым
        double sum = 0.0;
        int count = 0;
        for (double r : ratios)
        {
            if (r > 0.01)
            {
                sum += r;
                count++;
            }
        }
        if (count < 3) return;
        double mean = sum / count;

        // Дисперсия по НЕ-нулевым
        double variance = 0.0;
        for (double r : ratios)
        {
            if (r > 0.01)
            {
                double diff = r - mean;
                variance += diff * diff;
            }
        }
        double stdDev = sqrt(variance / count);

        // ✅ ЗАНУЛЯЕМ выбросы ПРЯМО В МЕСТЕ!
        for (size_t i = 0; i < n; ++i)
        {
            if (ratios[i] > 0.01 && fabs(ratios[i] - mean) > 3.0 * stdDev)
            {
                ratios[i] = 0.0;  // 🎯 ЗАНУЛЕНО!
            }
        }
    }

    float MouseCalibration::winsorizedMeanFromNonZero() const
    {
        std::vector<float> good;
        good.reserve(ratios.size());

        for (float r : ratios)
        {
            if (r > 0.01) good.push_back(r);
        }

        if (good.size() < 3) return 1.0;

        std::sort(good.begin(), good.end());
        size_t n = good.size();
        size_t trim = (int)(n * 0.1);

        float sum = 0.0;
        for (size_t i = trim; i < n - trim; ++i)
        {
            sum += good[i];
        }
        return sum / (n - 2 * trim);
    }
}