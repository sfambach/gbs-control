#include "measure_period.h"

namespace MeasurePeriod {

volatile uint32_t stopTime;
volatile uint32_t startTime;
volatile uint32_t armed;

uint8_t debugInterruptPin()
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
