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

#include "parser.h"

#include <stdio.h>
#include <zxformat.h>
#include <zxmacros.h>
#include <zxtypes.h>

#include "app_mode.h"
#include "coin.h"
#include "crypto.h"
#include "parser_common.h"
#include "parser_impl.h"
#include "parser_print.h"
#include "parser_validate.h"

parser_error_t parser_init_context(parser_context_t *ctx, const uint8_t *buffer, uint16_t bufferSize) {
    if (ctx == NULL || buffer == NULL) {
        return parser_unexpected_error;
    }
    ctx->offset = 0;
    ctx->buffer = NULL;
    ctx->bufferLen = 0;

    if (bufferSize == 0 || buffer == NULL) {
        // Not available, use defaults
        ctx->buffer = NULL;
        ctx->bufferLen = 0;
        return parser_init_context_empty;
    }

    ctx->buffer = buffer;
    ctx->bufferLen = bufferSize;
    return parser_ok;
}

parser_error_t parser_parse(parser_context_t *ctx, const uint8_t *data, size_t dataLen) {
    if (ctx == NULL || data == NULL) {
        return parser_unexpected_error;
    }
    CHECK_ERROR(parser_init_context(ctx, data, dataLen))
    app_mode_skip_blindsign_ui();
    return _read(ctx);
}

parser_error_t parser_validate(const parser_context_t *ctx) {
    if (ctx->buffer == NULL || ctx->bufferLen == 0) {
        return parser_init_context_empty;
    }

    CHECK_ERROR(parser_json_validate(&parser_tx_obj.json))

    // Iterate through all items to check that all can be shown and are valid
    uint8_t numItems = 0;
    CHECK_ERROR(parser_getNumItems(ctx, &numItems))

    char tmpKey[40] = {0};
    char tmpVal[40] = {0};
    uint8_t pageCount = 0;
    for (uint8_t idx = 0; idx < numItems; idx++) {
        CHECK_ERROR(parser_getItem(ctx, idx, tmpKey, sizeof(tmpKey), tmpVal, sizeof(tmpVal), 0, &pageCount))
    }
    return parser_ok;
}

parser_error_t parser_getNumItems(const parser_context_t *ctx, uint8_t *num_items) {
    if (ctx == NULL || num_items == NULL) {
        return parser_unexpected_error;
    }
    UNUSED(ctx);
    *num_items = 1;
    if (*num_items == 0) {
        return parser_unexpected_number_items;
    }

    return parser_display_numItems(num_items);
}

parser_error_t parser_getItem(const parser_context_t *ctx, uint8_t displayIdx, char *outKey, uint16_t outKeyLen,
                              char *outVal, uint16_t outValLen, uint8_t pageIdx, uint8_t *pageCount) {
    if (ctx == NULL || outKey == NULL || outVal == NULL || pageCount == NULL) {
        return parser_unexpected_error;
    }
    *pageCount = 0;
    char tmpKey[35] = {0};
    char tmpVal[2] = {0};

    MEMZERO(outKey, outKeyLen);
    MEMZERO(outVal, outValLen);

    uint8_t numItems = 0;
    CHECK_ERROR(parser_getNumItems(ctx, &numItems))
    CHECK_APP_CANARY()

    if (numItems == 0) {
        return parser_unexpected_number_items;
    }

    if (displayIdx >= numItems) {
        return parser_display_idx_out_of_range;
    }

    uint16_t ret_value_token_index = 0;
    CHECK_ERROR_CLEAN_QUERY(
        parser_display_query(displayIdx, tmpKey, sizeof(tmpKey), tmpVal, sizeof(tmpVal), &ret_value_token_index))
    CHECK_APP_CANARY()
    snprintf(outKey, outKeyLen, "%s", tmpKey);

    if (parser_isAmount(tmpKey)) {
        CHECK_ERROR_CLEAN_QUERY(parser_formatAmount(ret_value_token_index, outVal, outValLen, pageIdx, pageCount))
    } else {
        CHECK_ERROR_CLEAN_QUERY(parser_getToken(ret_value_token_index, outVal, outValLen, pageIdx, pageCount))
    }
    CHECK_APP_CANARY()

    CHECK_ERROR_CLEAN_QUERY(parser_display_make_friendly())
    CHECK_APP_CANARY()

    snprintf(outKey, outKeyLen, "%s", tmpKey);
    CHECK_APP_CANARY()
    CLEAN_QUERY()
    return parser_ok;
}
