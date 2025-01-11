#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pico/stdlib.h"

// Base64 decoding table
static const unsigned char d64[256] = {
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 62, 64, 64, 64, 63,
    52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 64, 64, 64, 64, 64, 64,
    64,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
    15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 64, 64, 64, 64, 64,
    64, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
    41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64
};

/**
 * Decode a base64-encoded string.
 * 
 * @param input The base64-encoded string to decode, represented as an array of uint8_t characters.
 * @param output A pointer to a uint8_t buffer that will hold the decoded result. It must be at least 
 *               output_length in size.
 * @param output_length A pointer to a size_t variable that holds the size of the output buffer on input,
 *                      and the length of the decoded data on output.
 * @returns A pointer to the output buffer, or NULL if the decoding failed.
 */
uint8_t *base64_decode(const uint8_t *input, uint8_t *output, size_t *output_length) {
    size_t input_length = strlen(input);
    
    // Calculate padding
    int padding = 0;
    if(input_length > 0) {
        if(input[input_length - 1] == '=') padding++;
        if(input_length > 1 && input[input_length - 2] == '=') padding++;
    }
    
    // Calculate output size, and fail if the output won't fit in the buffer
    size_t predicted_length = (input_length * 3) / 4 - padding;
    if(predicted_length > *output_length) {
        return NULL;
    }
    *output_length = predicted_length;
   
    // Process input in blocks of 4 characters
    size_t i = 0, j = 0;
    uint32_t block = 0;
    int block_size = 0;
    
    while(i < input_length) {
        uint8_t c = input[i++];
        if(c == '=') break; // End of input
        
        unsigned char val = d64[c];
        if(val == 64) continue; // Skip invalid characters
        
        block = (block << 6) | val;
        block_size++;
        
        if(block_size == 4) {
            output[j++] = (block >> 16) & 0xff;
            output[j++] = (block >> 8) & 0xff;
            output[j++] = block & 0xff;
            block = 0;
            block_size = 0;
        }
    }
    
    // Handle remaining bytes
    if(block_size > 1) {
        block <<= (6 * (4 - block_size));
        if(block_size == 3) {
            output[j++] = (block >> 16) & 0xff;
            output[j++] = (block >> 8) & 0xff;
        } 
        else if(block_size == 2) {
            output[j++] = (block >> 16) & 0xff;
        }
    }
    
    return output;
}
