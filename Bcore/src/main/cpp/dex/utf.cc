/*
 * Copyright (C) 2011 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "utf.h"

#include <android-base/logging.h>
#include <android-base/stringprintf.h>
#include <android-base/strings.h>

#include "base/casts.h"
#include "utf-inl.h"

namespace art_lkchan {

using android_lkchan::base::StringAppendF;
using android_lkchan::base::StringPrintf;

// This is used only from debugger and test code.
size_t CountModifiedUtf8Chars(const char* utf8) {
  return CountModifiedUtf8Chars(utf8, strlen(utf8));
}

/*
 * This does not validate UTF8 rules (nor did older code). But it gets the right answer
 * for valid UTF-8 and that's fine because it's used only to size a buffer for later
 * conversion.
 *
 * Modified UTF-8 consists of a series of bytes up to 21 bit Unicode code points as follows:
 * U+0001  - U+007F   0xxxxxxx
 * U+0080  - U+07FF   110xxxxx 10xxxxxx
 * U+0800  - U+FFFF   1110xxxx 10xxxxxx 10xxxxxx
 * U+10000 - U+1FFFFF 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
 *
 * U+0000 is encoded using the 2nd form to avoid nulls inside strings (this differs from
 * standard UTF-8).
 * The four byte encoding converts to two utf16 characters.
 */
size_t CountModifiedUtf8Chars(const char* utf8, size_t byte_count) {
  DCHECK_LE(byte_count, strlen(utf8));
  size_t len = 0;
  const char* end = utf8 + byte_count;
  for (; utf8 < end; ++utf8) {
    int ic = *utf8;
    len++;
    if (LIKELY((ic & 0x80) == 0)) {
      // One-byte encoding.
      continue;
    }
    // Two- or three-byte encoding.
    utf8++;
    if ((ic & 0x20) == 0) {
      // Two-byte encoding.
      continue;
    }
    utf8++;
    if ((ic & 0x10) == 0) {
      // Three-byte encoding.
      continue;
    }

    // Four-byte encoding: needs to be converted into a surrogate
    // pair.
    utf8++;
    len++;
  }
  return len;
}

// This is used only from debugger and test code.
void ConvertModifiedUtf8ToUtf16(uint16_t* utf16_data_out, const char* utf8_data_in) {
  while (*utf8_data_in != '\0') {
    const uint32_t ch = GetUtf16FromUtf8(&utf8_data_in);
    const uint16_t leading = GetLeadingUtf16Char(ch);
    const uint16_t trailing = GetTrailingUtf16Char(ch);

    *utf16_data_out++ = leading;
    if (trailing != 0) {
      *utf16_data_out++ = trailing;
    }
  }
}

void ConvertModifiedUtf8ToUtf16(uint16_t* utf16_data_out, size_t out_chars,
                                const char* utf8_data_in, size_t in_bytes) {
  const char *in_start = utf8_data_in;
  const char *in_end = utf8_data_in + in_bytes;
  uint16_t *out_p = utf16_data_out;

  if (LIKELY(out_chars == in_bytes)) {
    // Common case where all characters are ASCII.
    for (const char *p = in_start; p < in_end;) {
      // Safe even if char is signed because ASCII characters always have
      // the high bit cleared.
      *out_p++ = dchecked_integral_cast<uint16_t>(*p++);
    }
    return;
  }

  // String contains non-ASCII characters.
  for (const char *p = in_start; p < in_end;) {
    const uint32_t ch = GetUtf16FromUtf8(&p);
    const uint16_t leading = GetLeadingUtf16Char(ch);
    const uint16_t trailing = GetTrailingUtf16Char(ch);

    *out_p++ = leading;
    if (trailing != 0) {
      *out_p++ = trailing;
    }
  }
}

void ConvertUtf16ToModifiedUtf8(char* utf8_out, size_t byte_count,
                                const uint16_t* utf16_in, size_t char_count) {
  if (LIKELY(byte_count == char_count)) {
    // Common case where all characters are ASCII.
    const uint16_t *utf16_end = utf16_in + char_count;
    for (const uint16_t *p = utf16_in; p < utf16_end;) {
      *utf8_out++ = dchecked_integral_cast<char>(*p++);
    }
    return;
  }

  // String contains non-ASCII characters.
  while (char_count--) {
    const uint16_t ch = *utf16_in++;
    if (ch > 0 && ch <= 0x7f) {
      *utf8_out++ = ch;
    } else {
      // Char_count == 0 here implies we've encountered an unpaired
      // surrogate and we have no choice but to encode it as 3-byte UTF
      // sequence. Note that unpaired surrogates can occur as a part of
      // "normal" operation.
      if ((ch >= 0xd800 && ch <= 0xdbff) && (char_count > 0)) {
        const uint16_t ch2 = *utf16_in;

        // Check if the other half of the pair is within the expected
        // range. If it isn't, we will have to emit both "halves" as
        // separate 3 byte sequences.
        if (ch2 >= 0xdc00 && ch2 <= 0xdfff) {
          utf16_in++;
          char_count--;
          const uint32_t code_point = (ch << 10) + ch2 - 0x035fdc00;
          *utf8_out++ = (code_point >> 18) | 0xf0;
          *utf8_out++ = ((code_point >> 12) & 0x3f) | 0x80;
          *utf8_out++ = ((code_point >> 6) & 0x3f) | 0x80;
          *utf8_out++ = (code_point & 0x3f) | 0x80;
          continue;
        }
      }

      if (ch > 0x07ff) {
        // Three byte encoding.
        *utf8_out++ = (ch >> 12) | 0xe0;
        *utf8_out++ = ((ch >> 6) & 0x3f) | 0x80;
        *utf8_out++ = (ch & 0x3f) | 0x80;
      } else /*(ch > 0x7f || ch == 0)*/ {
        // Two byte encoding.
        *utf8_out++ = (ch >> 6) | 0xc0;
        *utf8_out++ = (ch & 0x3f) | 0x80;
      }
    }
  }
}

int32_t ComputeUtf16HashFromModifiedUtf8(const char* utf8, size_t utf16_length) {
  uint32_t hash = 0;
  while (utf16_length != 0u) {
    const uint32_t pair = GetUtf16FromUtf8(&utf8);
    const uint16_t first = GetLeadingUtf16Char(pair);
    hash = hash * 31 + first;
    --utf16_length;
    const uint16_t second = GetTrailingUtf16Char(pair);
    if (second != 0) {
      hash = hash * 31 + second;
      DCHECK_NE(utf16_length, 0u);
      --utf16_length;
    }
  }
  return static_cast<int32_t>(hash);
}

uint32_t ComputeModifiedUtf8Hash(const char* chars) {
  uint32_t hash = 0;
  while (*chars != '\0') {
    hash = hash * 31 + *chars++;
  }
  return static_cast<int32_t>(hash);
}

int CompareModifiedUtf8ToUtf16AsCodePointValues(const char* utf8, const uint16_t* utf16,
                                                size_t utf16_length) {
  for (;;) {
    if (*utf8 == '\0') {
      return (utf16_length == 0) ? 0 : -1;
    } else if (utf16_length == 0) {
      return 1;
    }

    const uint32_t pair = GetUtf16FromUtf8(&utf8);

    // First compare the leading utf16 char.
    const uint16_t lhs = GetLeadingUtf16Char(pair);
    const uint16_t rhs = *utf16++;
    --utf16_length;
    if (lhs != rhs) {
      return lhs > rhs ? 1 : -1;
    }

    // Then compare the trailing utf16 char. First check if there
    // are any characters left to consume.
    const uint16_t lhs2 = GetTrailingUtf16Char(pair);
    if (lhs2 != 0) {
      if (utf16_length == 0) {
        return 1;
      }

      const uint16_t rhs2 = *utf16++;
      --utf16_length;
      if (lhs2 != rhs2) {
        return lhs2 > rhs2 ? 1 : -1;
      }
    }
  }
}

size_t CountUtf8Bytes(const uint16_t* chars, size_t char_count) {
  size_t result = 0;
  const uint16_t *end = chars + char_count;
  while (chars < end) {
    const uint16_t ch = *chars++;
    if (LIKELY(ch != 0 && ch < 0x80)) {
      result++;
      continue;
    }
    if (ch < 0x800) {
      result += 2;
      continue;
    }
    if (ch >= 0xd800 && ch < 0xdc00) {
      if (chars < end) {
        const uint16_t ch2 = *chars;
        // If we find a properly paired surrogate, we emit it as a 4 byte
        // UTF sequence. If we find an unpaired leading or trailing surrogate,
        // we emit it as a 3 byte sequence like would have done earlier.
        if (ch2 >= 0xdc00 && ch2 < 0xe000) {
          chars++;
          result += 4;
          continue;
        }
      }
    }
    result += 3;
  }
  return result;
}

static inline constexpr bool NeedsEscaping(uint16_t ch) {
  return (ch < ' ' || ch > '~');
}

std::string PrintableChar(uint16_t ch) {
  std::string result;
  result += '\'';
  if (NeedsEscaping(ch)) {
    StringAppendF(&result, "\\u%04x", ch);
  } else {
    result += static_cast<std::string::value_type>(ch);
  }
  result += '\'';
  return result;
}

std::string PrintableString(const char* utf) {
  std::string result;
  result += '"';
  const char* p = utf;
  size_t char_count = CountModifiedUtf8Chars(p);
  for (size_t i = 0; i < char_count; ++i) {
    uint32_t ch = GetUtf16FromUtf8(&p);
    if (ch == '\\') {
      result += "\\\\";
    } else if (ch == '\n') {
      result += "\\n";
    } else if (ch == '\r') {
      result += "\\r";
    } else if (ch == '\t') {
      result += "\\t";
    } else {
      const uint16_t leading = GetLeadingUtf16Char(ch);

      if (NeedsEscaping(leading)) {
        StringAppendF(&result, "\\u%04x", leading);
      } else {
        result += static_cast<std::string::value_type>(leading);
      }

      const uint32_t trailing = GetTrailingUtf16Char(ch);
      if (trailing != 0) {
        // All high surrogates will need escaping.
        StringAppendF(&result, "\\u%04x", trailing);
      }
    }
  }
  result += '"';
  return result;
}

}  // namespace art_lkchan
