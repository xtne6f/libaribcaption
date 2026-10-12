// Calculate MD5 ​​for multiple DRCS variations from a DRCS bitmap file output by TVTest using GetDRCSDigest().
// `g++ -Wall -Wextra hash_drcs_bmp.cpp ../../src/base/md5.c -o hash_drcs_bmp`
#include <cstdio>
#include <cstdint>
#include <string>
#include <vector>
#include "../../src/base/md5.h"

namespace
{

// Copied from "include/aribcaption/caption.hpp"

/**
 * Structure contains DRCS data and related information.
 */
struct DRCS {
    int width = 0;
    int height = 0;
    int depth = 0;
    int depth_bits = 0;
    std::vector<uint8_t> pixels;
    std::string md5;                ///< MD5 digest for the pixels buffer
    std::string alternative_text;   ///< Appears only if alternative Unicode codepoint exists
    uint32_t alternative_ucs4 = 0;  ///< Will be non-zero only if alternative Unicode codepoint exists
public:
    DRCS() = default;
    DRCS(const DRCS&) = default;
    DRCS(DRCS&&) noexcept = default;
    DRCS& operator=(const DRCS&) = default;
    DRCS& operator=(DRCS&&) noexcept = default;
};

// Copied from "src/base/md5_helper.hpp"

std::string GetDigest(const uint8_t* buffer, size_t length) {
    const uint8_t* ptr = buffer;
    aribcc_md5_ctx_t ctx;
    aribcc_md5_init(&ctx);

    while (length) {
        if (length > 64) {
            aribcc_md5_update(&ctx, ptr, 64);
            length -= 64;
            ptr += 64;
        } else {
            aribcc_md5_update(&ctx, ptr, static_cast<unsigned long>(length));
            length -= length;
            ptr += length;
        }
    }

    std::vector<uint8_t> digest(16, 0);
    aribcc_md5_final(&digest[0], &ctx);

    std::string digest_str(32, '\0');
    for (size_t i = 0; i < 16; i++) {
        snprintf(&digest_str[i * 2], 16 * 2, "%02x", static_cast<uint32_t>(digest[i]));
    }

    return digest_str;
}

// Copied from "src/decoder/decoder_impl.cpp"

std::string GetDRCSDigest(const DRCS& drcs, int variation)
{
    if (variation == 0) {
        return GetDigest(drcs.pixels.data(), drcs.pixels.size());
    } else if (variation >= 1 && variation <= 3 &&
               drcs.depth == 2 &&
               drcs.width == 16 && drcs.height == 18) {
        // For TR-B14 C-profile
        // variation 1: Twice the size, then place on the left side of 36x36 image
        // variation 2: Twice the size, then place on the center
        // variation 3: Twice the size, then place on the right
        uint8_t buffer[36 * 36 / 4] = {};
        for (int y = 0; y < 18; y++) {
            for (int x = 0, i = 36 * y + variation - 1; x < 16; x++, i++) {
                uint8_t pixel = (drcs.pixels[2 * y + (x >> 3)] >> (7 - (x & 7))) & 1;
                int pos_bytes = i >> 1;
                if ((i & 1) == 0) {
                    buffer[pos_bytes] = (pixel * 15) << 4;
                } else {
                    buffer[pos_bytes] |= pixel * 15;
                }
                buffer[9 + pos_bytes] = buffer[pos_bytes];
            }
        }
        return GetDigest(buffer, sizeof(buffer));
    } else if (variation >= 1 && variation <= 2 &&
               drcs.depth == 4 &&
               drcs.width >= 4 && drcs.width <= 36 && drcs.width % 2 == 0 &&
               drcs.height >= 1 && drcs.height <= 36) {
        // For TR-B14 A-profile
        // variation 1: Move 2 pixels to the right, or return variation 0 (rare case)
        // variation 2: Move 2 pixels to the left
        int w = drcs.width / 2;
        int n = w * drcs.height;
        uint8_t buffer[36 * 36 / 4];
        buffer[0] = 0;
        buffer[(n - 1) / 2] = 0;
        for (int i = 0, j = (variation == 1 ? 1 : -1); i < n; i++, j++) {
            uint8_t two_pixels = (drcs.pixels[i >> 1] >> (4 - (i & 1) * 4)) & 15;
            if (variation == 1) {
                if (j % w == 0) {
                    if (two_pixels != 0) {
                        // The right edge is not black
                        return GetDRCSDigest(drcs, 0);
                    }
                    if (j == n) {
                        continue;
                    }
                }
            } else {
                if (i % w == 0) {
                    if (two_pixels != 0) {
                        // The left edge is not black
                        return "";
                    }
                    if (i == 0) {
                        continue;
                    }
                }
            }
            int pos_bytes = j >> 1;
            if ((j & 1) == 0) {
                buffer[pos_bytes] = two_pixels << 4;
            } else {
                buffer[pos_bytes] |= two_pixels;
            }
        }
        return GetDigest(buffer, (n + 1) / 2);
    }

    return "";
}

}

int main(int argc, char** argv)
{
    if (argc < 2) {
        return 2;
    }
    FILE* fp = fopen(argv[1], "rb");
    if (!fp) {
        return 1;
    }
    uint8_t buffer[2374 + 1];
    size_t n = fread(buffer, 1, sizeof(buffer), fp);
    fclose(fp);

    DRCS drcs;
    if (n == 2374) {
        // 36x36,8bits->2bits
        drcs.depth = 4;
        drcs.width = 36;
        drcs.height = 36;
        for (int i = n - 36; i >= 1078; i -= 36) {
            for (int j = 0; j < drcs.width; j += 4) {
                drcs.pixels.push_back(
                    (buffer[i + j] / 85) << 6 |
                    (buffer[i + j + 1] / 85) << 4 |
                    (buffer[i + j + 2] / 85) << 2 |
                    (buffer[i + j + 3] / 85));
            }
        }
    } else if (n == 1798 || n == 2038) {
        // 18x36||30x30,8bits->2bits
        drcs.depth = 4;
        int stride;
        if (n == 1798) {
            drcs.width = 18;
            drcs.height = 36;
            stride = 20;
        } else {
            drcs.width = 30;
            drcs.height = 30;
            stride = 32;
        }
        for (int i = n - stride * 2; i >= 1078; i -= stride * 2) {
            for (int j = stride; j < stride * 2; j += 4) {
                drcs.pixels.push_back(
                    (buffer[i + j] / 85) << 6 |
                    (buffer[i + j + 1] / 85) << 4 |
                    (buffer[i + j + 2] / 85) << 2 |
                    (buffer[i + j + 3] / 85));
            }
            drcs.pixels.back() &= 0xf0;
            drcs.pixels.back() |= (buffer[i] / 85) << 2 | (buffer[i + 1] / 85);
            for (int j = 2; j < stride - 2; j += 4) {
                drcs.pixels.push_back(
                    (buffer[i + j] / 85) << 6 |
                    (buffer[i + j + 1] / 85) << 4 |
                    (buffer[i + j + 2] / 85) << 2 |
                    (buffer[i + j + 3] / 85));
            }
        }
    } else if (n == 134) {
        // 16x18,1bit
        drcs.depth = 2;
        drcs.width = 16;
        drcs.height = 18;
        for (int i = n - 4; i >= 62; i -= 4) {
            drcs.pixels.push_back(buffer[i]);
            drcs.pixels.push_back(buffer[i + 1]);
        }
    } else {
        return 1;
    }

    int variation = 0;
    for (std::string md5; !(md5 = GetDRCSDigest(drcs, variation)).empty(); variation++) {
        printf("%s\n", md5.c_str());
    }
    return 0;
}
