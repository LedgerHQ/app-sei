/*******************************************************************************
 *   (c) 2018 - 2024 Zondax AG
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 ********************************************************************************/

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <sigutils.h>
#include <stdbool.h>

#include "coin.h"
#include "zxerror.h"

extern uint32_t hdPath[HDPATH_LEN_DEFAULT];
extern uint32_t hdPath_len;

zxerr_t crypto_fillAddress(uint8_t *buffer, uint16_t bufferLen, uint16_t *addrResponseLen);

zxerr_t crypto_sign(uint8_t *signature, uint16_t signatureMaxlen, uint16_t *sigSize);

zxerr_t crypto_sha256(const uint8_t *input, uint16_t inputLen, uint8_t *output, uint16_t outputLen);

zxerr_t ripemd160_32(uint8_t *out, uint8_t *in);

#ifdef __cplusplus
}
#endif
