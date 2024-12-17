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

#include <zxmacros.h>

#include "parser_common.h"
#include "parser_txdef.h"
#include "zxtypes.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_RECURSION_DEPTH 6
extern bool extraDepthLevel;

#define INIT_QUERY_CONTEXT(_KEY, _KEY_LEN, _VAL, _VAL_LEN, _PAGE_IDX, _MAX_LEVEL) \
    parser_tx_obj.query._item_index_current = 0;                                  \
    parser_tx_obj.query.max_depth = MAX_RECURSION_DEPTH;                          \
    parser_tx_obj.query.max_level = _MAX_LEVEL;                                   \
                                                                                  \
    parser_tx_obj.query.item_index = 0;                                           \
    parser_tx_obj.query.page_index = (_PAGE_IDX);                                 \
                                                                                  \
    MEMZERO(_KEY, (_KEY_LEN));                                                    \
    MEMZERO(_VAL, (_VAL_LEN));                                                    \
    parser_tx_obj.query.out_key = _KEY;                                           \
    parser_tx_obj.query.out_val = _VAL;                                           \
    parser_tx_obj.query.out_key_len = (_KEY_LEN);                                 \
    parser_tx_obj.query.out_val_len = (_VAL_LEN);

#define CLEAN_QUERY()                    \
    parser_tx_obj.query.out_key = NULL;  \
    parser_tx_obj.query.out_val = NULL;  \
    parser_tx_obj.query.out_key_len = 0; \
    parser_tx_obj.query.out_val_len = 0;

#define CHECK_ERROR_CLEAN_QUERY(__CALL) \
    {                                   \
        parser_error_t __err = __CALL;  \
        CHECK_APP_CANARY()              \
        if (__err != parser_ok) {       \
            CLEAN_QUERY()               \
            return __err;               \
        }                               \
    }

typedef struct {
    const uint8_t *buffer;
    uint16_t bufferLen;
    uint16_t offset;
    parser_tx_t *tx_obj;
} parser_context_t;

typedef struct {
    const char *str1;
    const char *str2;
} key_subst_t;

typedef struct {
    char ascii_code;
    char str;
} ascii_subst_t;

extern parser_tx_t parser_tx_obj;

parser_error_t _read(parser_context_t *c);

parser_error_t parser_traverse_find(uint16_t root_token_index, uint16_t *ret_value_token_index);

// Retrieves the value for the corresponding token index. If the value goes beyond val_len, the chunk_idx will be used
parser_error_t parser_getToken(uint16_t token_index, char *out_val, uint16_t out_val_len, uint8_t pageIdx,
                               uint8_t *pageCount);

bool is_msg_type_field(char *field_name);
bool is_msg_from_field(char *field_name);

#ifdef __cplusplus
}
#endif
