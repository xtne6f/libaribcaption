/*
 * Copyright (C) 2021 magicxqq <xqq@xqq.im>. All rights reserved.
 *
 * This file is part of libaribcaption.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef ARIBCAPTION_B24_DECODER_HPP
#define ARIBCAPTION_B24_DECODER_HPP

#include <cstdint>
#include <functional>
#include <memory>
#include "context.hpp"

namespace aribcaption {

enum B24Type : uint8_t {
    kB24TypeCaption = 0x80,
    kB24TypeSuperimpose = 0x81,
    kB24TypeDefault = kB24TypeCaption
};

enum B24Profile {
    kB24ProfileA = 0x0008,
    kB24ProfileC = 0x0012,
    kB24ProfileDefault = kB24ProfileA
};

enum B24LanguageId : uint8_t {
    kB24LanguageIdFirst = 1,
    kB24LanguageIdSecond = 2,
    kB24LanguageIdDefault = kB24LanguageIdFirst,
    kB24LanguageIdMax = kB24LanguageIdSecond
};

struct Caption;
class DecoderImpl;

class Decoder {
public:
    using OutputCB = std::function<void(std::unique_ptr<Caption>)>;

    enum DecodeStatus {
        kDecodeStatusError = 0,
        kDecodeStatusOK = 1,
        kDecodeStatusGotCaption = 2
    };
public:
    explicit Decoder(Context& context);
    ~Decoder();
public:
    bool Initialize(B24Type type = kB24TypeDefault,
                    B24Profile profile = kB24ProfileDefault,
                    B24LanguageId language_id = kB24LanguageIdDefault);
    void SetType(B24Type type);
    void SetProfile(B24Profile profile);
    void SetLanguageId(B24LanguageId language_id);
    uint32_t QueryISO639LanguageCode(B24LanguageId language_id);
    Decoder::DecodeStatus Decode(const uint8_t* pes_data, size_t length, int64_t pts,
                                 const Decoder::OutputCB& output_cb);
    bool Flush();
public:
    Decoder(const Decoder&) = delete;
    Decoder& operator=(const Decoder&) = delete;
private:
    std::unique_ptr<DecoderImpl> pimpl_;
};

}  // namespace aribcaption

#endif  // ARIBCAPTION_B24_DECODER_HPP
