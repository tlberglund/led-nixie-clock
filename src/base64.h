
#ifndef __BASE64_H__
#define __BASE64_H__

#ifdef __cplusplus
extern "C" {
#endif


uint8_t *base64_decode(const uint8_t *input, uint8_t *output, size_t *output_length);


#ifdef __cplusplus
}
#endif

#endif
