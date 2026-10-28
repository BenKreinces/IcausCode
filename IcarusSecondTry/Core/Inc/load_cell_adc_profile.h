/**
 * @file load_cell_adc_profile.h
 * @brief Provisional board profile for the AD7124-8 load-cell ADC.
 *
 * This is a software-complete profile recovered from the legacy project, not
 * an electrically verified board fact. It must be checked against the board
 * schematic and then verified on hardware before using its samples for any
 * control or safety decision.
 */
#ifndef LOAD_CELL_ADC_PROFILE_H
#define LOAD_CELL_ADC_PROFILE_H

#include "ad7124.h"

/*
 * AIN2-AIN3 differential, setup 0, REFIN1 configuration, SINC4 filter with
 * FS=384, continuous-conversion ADC_CONTROL reset value. The driver reads
 * DATA transactionally; it does not enable continuous-read mode.
 */
static const ad7124_single_channel_config_t load_cell_adc_profile = {
    .adc_control = 0x0000U,
    .config_0 = 0x0860U,
    .filter_0 = 0x060180U,
    .setup = 0U,
    .input_positive = 2U,
    .input_negative = 3U};

#endif /* LOAD_CELL_ADC_PROFILE_H */
