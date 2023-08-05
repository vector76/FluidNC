#include "TicToc.h"
#include "Driver/delay_usecs.h"
#include "esp32/clk.h"


int32_t tic() {
    return getCpuTicks();
}

int32_t toc_us(int32_t tic) {
    int32_t elapsed = getCpuTicks() - tic;
    return (elapsed + ticks_per_us/2)/ticks_per_us;
}

int32_t toc_us_max(int32_t tic, int32_t prev_max_us) {
    int32_t tmp_us = toc_us(tic);
    return (tmp_us > prev_max_us) ? tmp_us : prev_max_us;
}