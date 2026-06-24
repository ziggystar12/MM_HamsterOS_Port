#pragma once
#include <stdint.h>

/*
 * rle.h -- MM1 RLE stream decompressor
 *
 * Two algorithms:
 *   rle_decode_stream  -- column-major->row-major 2bpp stream (WALLPIX/MONPIX)
 *   rle_decompress     -- simple byte-level RLE (SCREEN files)
 */

/*
 * Decode a 2bpp MM1 stream starting at data[offset].
 * Writes cols*height bytes into raw_out (already allocated by caller).
 * width must be divisible by 4.
 * Returns the new read position (bytes consumed up to).
 */
int rle_decode_stream(const uint8_t *data, int data_len, int offset,
                      uint8_t *raw_out, int width, int height);

/*
 * Simple RLE decompress (used for SCREEN files).
 * Reads from in[0..in_len), writes up to target_size bytes to out.
 * Returns number of bytes written.
 */
int rle_decompress(const uint8_t *in, int in_len, uint8_t *out, int target_size);
