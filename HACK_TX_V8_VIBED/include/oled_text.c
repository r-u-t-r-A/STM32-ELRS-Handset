#include "oled_text.h"
#include <string.h>

const char *chop_chars(const char *src, uint8_t max_chars) {
    static char buf[OLED_ROW_CHARS + 1];

    if (max_chars > OLED_ROW_CHARS) {
        max_chars = OLED_ROW_CHARS;
    }

    if (!src) {
        buf[0] = 0;
        return buf;
    }

    const size_t len = strlen(src);
    const size_t copy_len = (len <= max_chars) ? len : max_chars;
    memcpy(buf, src, copy_len);
    buf[copy_len] = 0;
    return buf;
}
