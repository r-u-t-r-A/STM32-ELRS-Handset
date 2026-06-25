/*
 * OLED text helper for 128px-wide display (8px per character -> 16 chars per row).
 * Strings longer than max_chars are hard-cut (no ellipsis).
 */

#ifndef OLED_TEXT_H
#define OLED_TEXT_H

#include <stdint.h>

#define OLED_ROW_CHARS 16
#define OLED_CHAR_WIDTH_PX 8

/* bad/good packet stats on Packet Rate row: "BBB/GGG" */
#define OLED_PKT_STATS_CHARS 7

/*
 * chop_chars
 *   Hard-limit src to max_chars and return the result.
 *
 *   src       - input string (NULL treated as "")
 *   max_chars - max characters to keep (e.g. OLED_ROW_CHARS or 15 when ^/v shown)
 *
 *   Returns a pointer to an internal static buffer; valid until the next chop_chars()
 *   call. Safe for: oled.print(chop_chars(text, 16));
 */
const char *chop_chars(const char *src, uint8_t max_chars);

#endif
