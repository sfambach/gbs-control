#ifndef MEASURE_PERIOD_H_
#define MEASURE_PERIOD_H_

#include "config.h"
#include "platform_gbs.h"

namespace MeasurePeriod {
extern volatile uint32_t stopTime;
extern volatile uint32_t startTime;
extern volatile uint32_t armed;

void _risingEdgeISR_prepare();
void _risingEdgeISR_measure();

uint8_t debugInterruptPin();
void start();
} // namespace MeasurePeriod

#endif
