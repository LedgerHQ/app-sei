/*******************************************************************************
 *  (c) 2018 - 2024 Zondax AG
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

#include <stdio.h>
#include <zxmacros.h>

#include "parser_common.h"
#include "parser_impl_evm.h"
#include "rlp.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DECIMAL_BASE 10
typedef enum RlpError {
    rlp_ok = 0,
    rlp_no_data,
    rlp_invalid_data,
} rlp_error_t;

#define SEI_TOKEN_SYMBOL " SEI"

// Add two numbers returning UINT64_MAX if overflows
uint64_t saturating_add(uint64_t a, uint64_t b);

// Add two numbers returning UINT32_MAX if overflows
uint32_t saturating_add_u32(uint32_t a, uint32_t b);

/// Returns the number of bytes read and the number of bytes to read
// Gets the number of bytes read and the number of bytes to read
//
// Returns false if there is a error in the rlp encoded data, true otherwise.
rlp_error_t get_tx_rlp_len(const uint8_t *buffer, uint32_t len, uint64_t *read, uint64_t *to_read);

// Use to decode rlp data pointed by data.
// sets itemOffset to point to encoded data like item = data[itemOffset], and sets its len.
// indicates amount of bytes read through read ptr
rlp_error_t parse_rlp_item(const uint8_t *data, uint32_t dataLen, uint32_t *read, uint32_t *item_len);

// converts a big endian stream of bytes to an u64 number.
// returns 0 on success, a negative number otherwise
parser_error_t be_bytes_to_u64(const uint8_t *bytes, uint8_t len, uint64_t *num);

parser_error_t printRLPNumber(const rlp_t *num, char *outVal, uint16_t outValLen, uint8_t pageIdx, uint8_t *pageCount);

parser_error_t printEVMAddress(const rlp_t *address, char *outVal, uint16_t outValLen, uint8_t pageIdx, uint8_t *pageCount);

parser_error_t printBigIntFixedPoint(const uint8_t *number, uint16_t number_len, char *outVal, uint16_t outValLen,
                                     uint8_t pageIdx, uint8_t *pageCount, uint16_t decimals);

parser_error_t printEVMMaxFees(const eth_tx_t *ethObj, char *outVal, uint16_t outValLen, uint8_t pageIdx,
                               uint8_t *pageCount);
#ifdef __cplusplus
}
#endif
