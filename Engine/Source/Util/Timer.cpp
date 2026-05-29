#include "Timer.hpp"

#pragma region Internal
namespace
{
//==============================================================================
// Internal Variable
//==============================================================================
LARGE_INTEGER CounterFrequency;
LARGE_INTEGER TimeStampStart;
LARGE_INTEGER TimeStampCurrent;
LARGE_INTEGER TimeStampPrevious;

// Measured in seconds.
FP64 TimeTotal{0.0};
// Measured in milliseconds.
FP64 TimeDelta{0.0};

U64 FrameCounter{0};
//==============================================================================
// Internal Function
//==============================================================================

} // namespace
#pragma endregion

#pragma region External
namespace Acrylic::Timer
{
void Init()
{
    QueryPerformanceFrequency(&CounterFrequency);
    QueryPerformanceCounter(&TimeStampStart);
    TimeStampCurrent = TimeStampStart;
    TimeStampPrevious = TimeStampStart;
}
void Update()
{
    TimeStampPrevious = TimeStampCurrent;
    QueryPerformanceCounter(&TimeStampCurrent);

    FrameCounter++;
    TimeDelta = static_cast<FP64>(TimeStampCurrent.QuadPart -
                                  TimeStampPrevious.QuadPart) /
                static_cast<FP64>(CounterFrequency.QuadPart) * 1000;
    TimeTotal =
        static_cast<FP64>(TimeStampCurrent.QuadPart - TimeStampStart.QuadPart) /
        static_cast<FP64>(CounterFrequency.QuadPart);
}
//==============================================================================
// Accessors
//==============================================================================
auto GetDeltaTime() -> FP64
{
    return TimeDelta;
}
auto GetTotalTime() -> FP64
{
    return TimeTotal;
}
auto GetFPS() -> FP32
{
    return TimeDelta > 0.0 ? static_cast<FP32>(1.0F / TimeDelta) : 0.0F;
}
auto GetFrameCount() -> U64
{
    return FrameCounter;
}
} // namespace Acrylic::Timer
#pragma endregion