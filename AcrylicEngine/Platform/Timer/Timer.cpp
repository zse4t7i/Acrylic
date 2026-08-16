#include "Timer.hpp"
#include "Log.hpp"

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

// All Measured in seconds.
F64 FrameTime{0.0F};
F64 TotalTime{0.0F};

U64 FrameCounter{0};
//==============================================================================
// Internal Function
//==============================================================================

} // namespace
#pragma endregion

#pragma region External
namespace Acrylic::Engine::Timer
{
void Init()
{
    QueryPerformanceFrequency(&CounterFrequency);
    QueryPerformanceCounter(&TimeStampStart);
    TimeStampCurrent  = TimeStampStart;
    TimeStampPrevious = TimeStampStart;

    LOG_INFO("Acrylic::Engine::Timer::Init() succeeded.");
}

void Update()
{
    TimeStampPrevious = TimeStampCurrent;
    QueryPerformanceCounter(&TimeStampCurrent);

    FrameCounter++;
    FrameTime = static_cast<F64>(TimeStampCurrent.QuadPart -
                                 TimeStampPrevious.QuadPart) /
                static_cast<F64>(CounterFrequency.QuadPart);
    TotalTime =
        static_cast<F64>(TimeStampCurrent.QuadPart - TimeStampStart.QuadPart) /
        static_cast<F64>(CounterFrequency.QuadPart);
}
//==============================================================================
// Accessors
//==============================================================================
auto GetFrameTime() -> F64
{
    return FrameTime;
}
auto GetTotalTime() -> F64
{
    return TotalTime;
}
auto GetFPS() -> F32
{
    return FrameTime > 0.0F ? static_cast<F32>(1.0F / FrameTime) : 0.0F;
}
auto GetFrameCount() -> U64
{
    return FrameCounter;
}
} // namespace Acrylic::Engine::Timer
#pragma endregion