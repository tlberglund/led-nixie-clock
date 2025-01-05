
#ifndef __HSL_TO_RGB_H__
#define __HSL_TO_RGB_H__

#ifdef cplusplus
extern "C" {
#endif


typedef struct {
    uint16_t hue;
    uint16_t saturation;
    uint16_t lightness;
} HSL;

typedef struct 
{
    uint8_t red;
    uint8_t green;
    uint8_t blue;
} RGB;

void hsl_to_rgb(HSL *hsl, RGB *rgb);

#ifdef cplusplus
}
#endif

#endif
