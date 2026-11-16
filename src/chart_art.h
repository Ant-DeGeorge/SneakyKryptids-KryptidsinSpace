#ifndef CHART_ART_H
#define CHART_ART_H
#include <gb/gb.h>
#include <stdint.h>
BANKREF_EXTERN(chart_art)
#define CHART_NT 190
#define CHART_NPAL 7
extern const uint8_t chart_tiles[], chart_map[], chart_attr[];
extern const uint16_t chart_pals[];
// where the ship parks for each mission (sprite x, y), next to its stop on the chart
#define CHART_STOPS {{12, 58}, {42, 70}, {78, 34}, {150, 74}, {90, 100}, {92, 128}}
#endif
