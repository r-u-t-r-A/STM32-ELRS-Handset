#pragma once
/*
 * Protocol-independent RC value ranges.
 * Included by crsf.h (protocol layer) and boards_pins.h (input layer)
 * so neither has to depend on the other.
 */

/* CRSF channel range */
#define RC_CHANNEL_MIN 172
#define RC_CHANNEL_MID 991
#define RC_CHANNEL_MAX 1811

/* AUX / digital channel range */
#define CRSF_DIGITAL_CHANNEL_MIN 172
#define CRSF_DIGITAL_CHANNEL_MAX 1811

/* ADC full-scale, matches analogReadResolution(12) in setup() */
#define ADC_FULL_SCALE 4096

/* PPM trainer range in microseconds.
 * Currently unused: PPM.c is not in the build and PPM_MIN/PPM_MAX are
 * undefined project-wide, so define these only when you re-enable PPM. */
/* #define PPM_MIN 1000 */
/* #define PPM_MAX 2000 */