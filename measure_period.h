#ifndef MEASURE_PERIOD_H_
#define MEASURE_PERIOD_H_

#include "config.h"
#include "platform_gbs.h"

namespace MeasurePeriod {
volatile uint32_t stopTime, startTime;
volatile uint32_t armed;

void _risingEdgeISR_prepare();
void _risingEdgeISR_measure();

static inline uint8_t debugInterruptPin()
{
    return digitalPinToInterrupt(DEBUG_IN_PIN);
}

void start()
{
    startTime = 0;
    stopTime = 0;
    armed = 0;
    attachInterrupt(debugInterruptPin(), _risingEdgeISR_prepare, RISING);
}

GBS_IRAM_ATTR void _risingEdgeISR_prepare()
{
    noInterrupts();
    startTime = gbs_cycle_count();
    detachInterrupt(debugInterruptPin());
    armed = 1;
    attachInterrupt(debugInterruptPin(), _risingEdgeISR_measure, RISING);
    interrupts();
}

GBS_IRAM_ATTR void _risingEdgeISR_measure()
{
    noInterrupts();
    stopTime = gbs_cycle_count();
    detachInterrupt(debugInterruptPin());
    interrupts();
}
} // namespace MeasurePeriod

#endif
