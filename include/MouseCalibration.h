#pragma once

#include <vector>

namespace PluginGUI
{
    class MouseCalibration
    {
    private:
        std::vector<float> ratios;
        size_t sampleCount;
        size_t updateCalibrationIndex = 0; 
        size_t skipedCount = 0;
        const int requiredSamples = 100;
        const int calibrationTargetPixels = 400;
        const int requiredPixelsPerSample = 5;
        const int ftameTime = 100;

        MouseCalibration() :
            calibrating(false),
            calibrated(false),
            totalRawTicks(0),
            totalScreenPixels(0),
            lastScreenY(0),
            sampleRawTicks(0),
            sampleScreenPixels(0),
            sampleTime(0),
            lastEventTime(0)
        {
            ratios.resize(requiredSamples, 0.0);
            sampleCount = 0;
        }

        bool calibrating;
        bool calibrated;
        int totalRawTicks;
        int totalScreenPixels;

        int sampleRawTicks;
        int sampleScreenPixels;
        long sampleTime;
        int lastScreenY;
        float ticksPerPixel = NAN;
        long lastEventTime;

    public:
        static MouseCalibration& getInstance();
        bool IsCalibrating() const 
        { 
            return calibrating; 
        }        
        float getTicksPerPixel() const;
        void updateCalibration(long dy, long eventTime);

    private:
        void startCalibration(long eventTime);
        void finishCalibration(int finalScreenPixels);

        float processStatistics();
        void markOutliers3SigmaInPlace();
        float winsorizedMeanFromNonZero() const;
        size_t countValid() const
        {
            size_t count = 0;
            for (double r : ratios) if (r > 0.01) count++;
            return count;
        }
    };
}