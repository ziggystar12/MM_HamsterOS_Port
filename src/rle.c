#include "rle.h"
#include "mm_util.h"

/*
 * rle_decode_stream
 *
 * Ported directly from cga.py decode_2bpp_stream_to_bytes().
 *
 * The stream is written column-by-column into a row-major output buffer.
 * cols = width / 4  (because each byte encodes 4 2bpp pixels)
 * img_size = cols * height
 */
int rle_decode_stream(const uint8_t *data, int data_len, int offset,
                      uint8_t *raw_out, int width, int height)
{
    int cols     = width / 4;
    int img_size = cols * height;

    mm_memset(raw_out, 0, (uint32_t)img_size);

    int dest  = 0;
    int index = 0;
    int x     = 0;
    int i     = offset;

    while (x < cols && i < data_len) {
        uint8_t v     = data[i++];
        int     count = 1;

        if (v == 0x7B) {
            if (i >= data_len) break;
            count = (int)data[i++] + 1;
            if (i >= data_len) break;
            v = data[i++];
        }

        for (int k = 0; k < count; k++) {
            int pos = dest + index;
            if (pos < img_size) {
                raw_out[pos] = v;
            }
            index += cols;
            if (index >= img_size) {
                index = 0;
                dest++;
                x++;
            }
            if (x >= cols) break;
        }
    }

    return i;
}

/*
 * rle_decompress
 *
 * Simple RLE used by SCREEN files.
 * Format: each run is either:
 *   - a literal byte (value != 0x7B) -- emit once
 *   - 0x7B <count-1> <value>         -- emit value (count) times
 *
 * Stops when target_size bytes have been written or input is exhausted.
 */
int rle_decompress(const uint8_t *in, int in_len, uint8_t *out, int target_size)
{
    int r = 0;
    int w = 0;

    while (r < in_len && w < target_size) {
        uint8_t b = in[r++];

        if (b == 0x7B) {
            if (r + 1 >= in_len) break;
            int     count = (int)in[r++] + 1;
            uint8_t val   = in[r++];
            for (int k = 0; k < count && w < target_size; k++) {
                out[w++] = val;
            }
        } else {
            out[w++] = b;
        }
    }

    return w;
}
