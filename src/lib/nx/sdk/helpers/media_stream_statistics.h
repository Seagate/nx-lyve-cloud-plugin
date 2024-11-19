// Copyright 2018-present Network Optix, Inc. Licensed under MPL 2.0: www.mozilla.org/MPL/2.0/

#pragma once

// Temporary fix to prevent crashes on startup of Network Optix Server
// Issue causes by compiling with MSVC 14.40 which has a code change incompatible
// with MSVC 14.38 which the Network Optix server is compiled with.
#if defined(_WIN32)
#ifndef _DISABLE_CONSTEXPR_MUTEX_CONSTRUCTOR
#define _DISABLE_CONSTEXPR_MUTEX_CONSTRUCTOR
#endif
#endif

#include <chrono>
#include <deque>
#include <limits>
#include <mutex>

namespace nx::sdk {

/**
 * This class calculates media stream bitrate, average frame rate and GOP size.
 */

class MediaStreamStatistics
{
public:
    MediaStreamStatistics(
        std::chrono::microseconds windowSize = std::chrono::seconds(2),
        int maxDurationInFrames = 0);

    void setWindowSize(std::chrono::microseconds windowSize);
    void setMaxDurationInFrames(int maxDurationInFrames);

    void reset();
    void onData(std::chrono::microseconds timestamp, size_t dataSize, bool isKeyFrame);
    int64_t bitrateBitsPerSecond() const;
    float getFrameRate() const;
    float getAverageGopSize() const;
    bool hasMediaData() const;

private:
    std::chrono::microseconds m_windowSize {};
    int m_maxDurationInFrames = 0;

    struct Data
    {
        Data() = default;
        Data(std::chrono::microseconds timestamp, size_t size, bool isKeyFrame):
            timestamp(timestamp), size(size), isKeyFrame(isKeyFrame)
        {
        }

        std::chrono::microseconds timestamp{};
        size_t size = 0;
        bool isKeyFrame = false;

        bool operator<(std::chrono::microseconds value) const { return timestamp < value; }
    };

    std::chrono::microseconds intervalUnsafe() const;

    mutable std::mutex m_mutex;
    std::deque<Data> m_data;
    int64_t m_totalSizeBytes = 0;
    //nx::utils::ElapsedTimer m_lastDataTimer;
    std::chrono::steady_clock::time_point m_lastDataTimer;
};

} // namespace nx::sdk
