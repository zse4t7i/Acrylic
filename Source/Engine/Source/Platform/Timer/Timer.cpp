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

// All Measured in seconds.
F64 TimeTotal{0.0};
F64 TimeDelta{0.0};

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
}
void Update()
{
    TimeStampPrevious = TimeStampCurrent;
    QueryPerformanceCounter(&TimeStampCurrent);

    FrameCounter++;
    TimeDelta = static_cast<F64>(TimeStampCurrent.QuadPart -
                                 TimeStampPrevious.QuadPart) /
                static_cast<F64>(CounterFrequency.QuadPart);
    TimeTotal =
        static_cast<F64>(TimeStampCurrent.QuadPart - TimeStampStart.QuadPart) /
        static_cast<F64>(CounterFrequency.QuadPart);
}
//==============================================================================
// Accessors
//==============================================================================
auto GetDeltaTime() -> F64
{
    return TimeDelta;
}
auto GetTotalTime() -> F64
{
    return TimeTotal;
}
auto GetFPS() -> F32
{
    return TimeDelta > 0.0 ? static_cast<F32>(1.0F / TimeDelta) : 0.0F;
}
auto GetFrameCount() -> U64
{
    return FrameCounter;
}
} // namespace Acrylic::Engine::Timer
#pragma endregion