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

#include "parser_print.h"

#include <zxformat.h>
#include <zxmacros.h>

#include "app_mode.h"
#include "coin.h"
#include "parser_common.h"
#include "parser_impl.h"
#include "utf8.h"

#define NUM_REQUIRED_ROOT_PAGES 7

const char *get_required_root_item(root_item_e i) {
    switch (i) {
        case root_item_chain_id:
            return "chain_id";
        case root_item_account_number:
            return "account_number";
        case root_item_sequence:
            return "sequence";
        case root_item_fee:
            return "fee";
        case root_item_memo:
            return "memo";
        case root_item_msgs:
            return "msgs";
        default:
            return "?";
    }
}

__Z_INLINE uint8_t get_root_max_level(root_item_e i) {
    switch (i) {
        case root_item_chain_id:
            return 2;
        case root_item_account_number:
            return 2;
        case root_item_sequence:
            return 2;
        case root_item_fee:
            return 1;
        case root_item_memo:
            return 2;
        case root_item_msgs:
            return extraDepthLevel ? 3 : 2;
        default:
            return 0;
    }
}

display_cache_t display_cache;

__Z_INLINE parser_error_t calculate_is_default_chainid() {
    display_cache.is_default_chain = false;

    // get chain_id
    char outKey[2];
    char outVal[COIN_MAX_CHAINID_LEN];
    uint8_t pageCount;
    INIT_QUERY_CONTEXT(outKey, sizeof(outKey), outVal, sizeof(outVal), 0, get_root_max_level(root_item_chain_id))
    parser_tx_obj.query.item_index = 0;
    parser_tx_obj.query._item_index_current = 0;

    uint16_t ret_value_token_index = 0;
    CHECK_ERROR_CLEAN_QUERY(
        parser_traverse_find(display_cache.root_item_start_token_idx[root_item_chain_id], &ret_value_token_index))

    CHECK_ERROR_CLEAN_QUERY(parser_getToken(ret_value_token_index, outVal, sizeof(outVal), 0, &pageCount))

    zemu_log_stack(outVal);
    zemu_log_stack(COIN_DEFAULT_CHAINID);

    if (strcmp(outVal, COIN_DEFAULT_CHAINID) == 0) {
        // If we don't match the default chainid, switch to expert mode
        display_cache.is_default_chain = true;
    } else if ((outVal[0] == 0x30 || outVal[0] == 0x31) && strlen(outVal) == 1) {
        CLEAN_QUERY();
        return parser_unexpected_chain;
    }

    CLEAN_QUERY();
    return parser_ok;
}

__Z_INLINE bool address_matches_own(char *addr) {
    if (parser_tx_obj.own_addr == NULL || addr == NULL) {
        return false;
    }
    if (strcmp(parser_tx_obj.own_addr, addr) != 0) {
        return false;
    }
    return true;
}

parser_error_t parser_indexRootFields() {
    if (parser_tx_obj.flags.cache_valid) {
        return parser_ok;
    }

    // Clear cache
    MEMZERO(&display_cache, sizeof(display_cache_t));

    char tmp_key[INDEXING_TMP_KEYSIZE];
    char tmp_val[INDEXING_TMP_VALUESIZE];
    MEMZERO(&tmp_key, sizeof(tmp_key));
    MEMZERO(&tmp_val, sizeof(tmp_val));

    // Grouping references
    char reference_msg_type[INDEXING_GROUPING_REF_TYPE_SIZE];
    char reference_msg_from[INDEXING_GROUPING_REF_FROM_SIZE];
    MEMZERO(&reference_msg_type, sizeof(reference_msg_type));
    MEMZERO(&reference_msg_from, sizeof(reference_msg_from));

    parser_tx_obj.filter_msg_type_count = 0;
    parser_tx_obj.filter_msg_from_count = 0;
    parser_tx_obj.flags.msg_type_grouping = 1;
    parser_tx_obj.flags.msg_from_grouping = 1;

    // Look for all expected root items in the JSON tree
    // mark them as found/valid,

    for (root_item_e root_item_idx = 0; root_item_idx < NUM_REQUIRED_ROOT_PAGES; root_item_idx++) {
        uint16_t req_root_item_key_token_idx = 0;

        const char *required_root_item_key = get_required_root_item(root_item_idx);

        parser_error_t err =
            object_get_value(&parser_tx_obj.json, ROOT_TOKEN_INDEX, required_root_item_key, &req_root_item_key_token_idx);

        if (err == parser_no_data) {
            continue;
        }
        CHECK_ERROR(err)

        // Remember root item start token
        display_cache.root_item_start_token_valid[root_item_idx] = true;
        display_cache.root_item_start_token_idx[root_item_idx] = req_root_item_key_token_idx;

        // Now count how many items can be found in this root item
        int16_t current_item_idx = 0;
        while (err == parser_ok) {
            INIT_QUERY_CONTEXT(tmp_key, sizeof(tmp_key), tmp_val, sizeof(tmp_val), 0, get_root_max_level(root_item_idx))

            parser_tx_obj.query.item_index = current_item_idx;
            strncpy_s(parser_tx_obj.query.out_key, required_root_item_key, parser_tx_obj.query.out_key_len);

            uint16_t ret_value_token_index;
            err = parser_traverse_find(display_cache.root_item_start_token_idx[root_item_idx], &ret_value_token_index);
            if (err != parser_ok) {
                continue;
            }

            uint8_t pageCount;
            CHECK_ERROR_CLEAN_QUERY(parser_getToken(ret_value_token_index, parser_tx_obj.query.out_val,
                                                    parser_tx_obj.query.out_val_len, 0, &pageCount))

            switch (root_item_idx) {
                case root_item_memo: {
                    if (strlen(parser_tx_obj.query.out_val) == 0) {
                        err = parser_query_no_results;
                        continue;
                    }
                    break;
                }
                case root_item_msgs: {
                    // Note: if we are dealing with the message field, Ledger has requested that we group.
                    // This means that if all messages share the same time, we should only count the type field once
                    // This is indicated by `parser_tx_obj.flags.msg_type_grouping`

                    // GROUPING: Message Type
                    if (parser_tx_obj.flags.msg_type_grouping && is_msg_type_field(tmp_key)) {
                        // First message, initialize expected type
                        if (parser_tx_obj.filter_msg_type_count == 0) {
                            if (strlen(tmp_val) >= sizeof(reference_msg_type)) {
                                CLEAN_QUERY()
                                return parser_unexpected_type;
                            }

                            snprintf(reference_msg_type, sizeof(reference_msg_type), "%s", tmp_val);
                            parser_tx_obj.filter_msg_type_valid_idx = current_item_idx;
                        }

                        if (strcmp(reference_msg_type, tmp_val) != 0) {
                            // different values, so disable grouping
                            parser_tx_obj.flags.msg_type_grouping = 0;
                            parser_tx_obj.filter_msg_type_count = 0;
                        }

                        parser_tx_obj.filter_msg_type_count++;
                    }

                    // GROUPING: Message From
                    if (parser_tx_obj.flags.msg_from_grouping && is_msg_from_field(tmp_key)) {
                        // First message, initialize expected from
                        if (parser_tx_obj.filter_msg_from_count == 0) {
                            snprintf(reference_msg_from, sizeof(reference_msg_from), "%s", tmp_val);
                            parser_tx_obj.filter_msg_from_valid_idx = current_item_idx;
                        }

                        if (strcmp(reference_msg_from, tmp_val) != 0) {
                            // different values, so disable grouping
                            parser_tx_obj.flags.msg_from_grouping = 0;
                            parser_tx_obj.filter_msg_from_count = 0;
                        }

                        parser_tx_obj.filter_msg_from_count++;
                    }

                    break;
                }
                default:
                    break;
            }

            display_cache.root_item_number_subitems[root_item_idx]++;
            current_item_idx++;
        }

        if (err != parser_query_no_results && err != parser_no_data) {
            CLEAN_QUERY()
            return err;
        }

        display_cache.total_item_count += display_cache.root_item_number_subitems[root_item_idx];
    }

    parser_tx_obj.flags.cache_valid = 1;

    CHECK_ERROR_CLEAN_QUERY(calculate_is_default_chainid())

    // turn off grouping if we are not in expert mode
    bool is_expert_or_default = false;
    CHECK_ERROR_CLEAN_QUERY(parser_is_expert_mode_or_not_default_chainid(&is_expert_or_default))
    if (is_expert_or_default) {
        parser_tx_obj.flags.msg_from_grouping = 0;
    }

    // check if from reference value matches the device address that will be signing
    parser_tx_obj.flags.msg_from_grouping_hide_all = 0;
    if (address_matches_own(reference_msg_from)) {
        parser_tx_obj.flags.msg_from_grouping_hide_all = 1;
    }
    CLEAN_QUERY()
    return parser_ok;
}

__Z_INLINE parser_error_t is_default_chainid(bool *is_default) {
    if (is_default == NULL) {
        return parser_unexpected_value;
    }

    CHECK_ERROR(parser_indexRootFields())
    *is_default = display_cache.is_default_chain;

    return parser_ok;
}

parser_error_t parser_is_expert_mode_or_not_default_chainid(bool *expert_or_default) {
    if (expert_or_default == NULL) {
        return parser_unexpected_value;
    }

    bool is_default = false;
    CHECK_ERROR(is_default_chainid(&is_default))
    *expert_or_default = app_mode_expert() || !is_default;

    return parser_ok;
}

__Z_INLINE parser_error_t get_subitem_count(root_item_e root_item, uint8_t *num_items) {
    if (num_items == NULL) {
        return parser_unexpected_value;
    }

    CHECK_ERROR(parser_indexRootFields())
    if (display_cache.total_item_count == 0) {
        *num_items = 0;
        return parser_ok;
    }

    int32_t tmp_num_items = display_cache.root_item_number_subitems[root_item];

    switch (root_item) {
        case root_item_chain_id:
            break;
        case root_item_sequence:
        case root_item_account_number:
            if (!app_mode_expert()) {
                tmp_num_items = 0;
            }
            break;
        case root_item_msgs: {
            // Remove grouped items from list
            if (parser_tx_obj.flags.msg_type_grouping && parser_tx_obj.filter_msg_type_count > 0) {
                tmp_num_items += 1;  // we leave main type
                tmp_num_items -= parser_tx_obj.filter_msg_type_count;
            }
            if (parser_tx_obj.flags.msg_from_grouping && parser_tx_obj.filter_msg_from_count > 0) {
                if (!parser_tx_obj.flags.msg_from_grouping_hide_all) {
                    tmp_num_items += 1;  // we leave main from
                }
                tmp_num_items -= parser_tx_obj.filter_msg_from_count;
            }
            break;
        }
        case root_item_memo:
            break;
        case root_item_fee:
            if (!app_mode_expert()) {
                tmp_num_items = 1;
            }
            break;
        case root_item_tip:
            tmp_num_items += 0;
            break;
        default:
            break;
    }
    *num_items = tmp_num_items;

    return parser_ok;
}

__Z_INLINE parser_error_t retrieve_tree_indexes(uint8_t display_index, root_item_e *root_item, uint8_t *subitem_index) {
    if (root_item == NULL || subitem_index == NULL) {
        return parser_unexpected_value;
    }

    // Find root index | display_index idx -> item_index
    // consume indexed subpages until we get the item index in the subpage
    *root_item = 0;
    *subitem_index = 0;
    uint8_t num_items;

    CHECK_ERROR(get_subitem_count(*root_item, &num_items));
    while (num_items == 0) {
        (*root_item)++;
        CHECK_ERROR(get_subitem_count(*root_item, &num_items));
    }

    for (uint16_t i = 0; i < display_index; i++) {
        (*subitem_index)++;
        uint8_t subitem_count = 0;
        CHECK_ERROR(get_subitem_count(*root_item, &subitem_count));
        if (*subitem_index >= subitem_count) {
            // Advance root index and skip empty items
            *subitem_index = 0;
            (*root_item)++;

            uint8_t num_items_2 = 0;
            CHECK_ERROR(get_subitem_count(*root_item, &num_items_2));
            while (num_items_2 == 0) {
                (*root_item)++;
                CHECK_ERROR(get_subitem_count(*root_item, &num_items_2));
            }
        }
    }

    if (*root_item > NUM_REQUIRED_ROOT_PAGES) {
        return parser_no_data;
    }

    return parser_ok;
}

parser_error_t parser_display_numItems(uint8_t *num_items) {
    if (num_items == NULL) {
        return parser_unexpected_value;
    }
    *num_items = 0;
    CHECK_ERROR(parser_indexRootFields())

    *num_items = 0;
    uint8_t n_items = 0;
    for (root_item_e root_item = 0; root_item < NUM_REQUIRED_ROOT_PAGES; root_item++) {
        CHECK_ERROR(get_subitem_count(root_item, &n_items))
        *num_items += n_items;
    }

    return parser_ok;
}

// This function assumes that the tx_ctx has been set properly
parser_error_t parser_display_query(uint16_t displayIdx, char *outKey, uint16_t outKeyLen, char *outVal, uint16_t outValLen,
                                    uint16_t *ret_value_token_index) {
    if (outKey == NULL || ret_value_token_index == NULL) {
        return parser_unexpected_value;
    }
    CHECK_ERROR(parser_indexRootFields())

    uint8_t num_items;
    CHECK_ERROR(parser_display_numItems(&num_items))

    if (displayIdx >= num_items) {
        return parser_display_idx_out_of_range;
    }

    root_item_e root_index = 0;
    uint8_t subitem_index = 0;
    CHECK_ERROR(retrieve_tree_indexes(displayIdx, &root_index, &subitem_index))

    // Prepare query
    INIT_QUERY_CONTEXT(outKey, outKeyLen, outVal, sizeof(outValLen), 0, get_root_max_level(root_index))
    parser_tx_obj.query.item_index = subitem_index;
    parser_tx_obj.query._item_index_current = 0;

    strncpy_s(outKey, get_required_root_item(root_index), outKeyLen);

    if (!display_cache.root_item_start_token_valid[root_index]) {
        CLEAN_QUERY()
        return parser_no_data;
    }

    CHECK_ERROR_CLEAN_QUERY(parser_traverse_find(display_cache.root_item_start_token_idx[root_index], ret_value_token_index))
    return parser_ok;
}

static const key_subst_t key_substitutions[] = {
    {"chain_id", "Chain ID"},
    {"account_number", "Account number"},
    {"sequence", "Sequence"},
    {"memo", "Memo"},
    {"fee/amount", "Fee"},
    {"fee/gas", "Gas"},
    {"fee/gas_limit", "Gas Limit"},
    {"msgs/type", "Type"},

    {"msgs/value/from_address", "From address"},
    {"msgs/value/to_address", "To address"},
    {"msgs/value/amount", "Amount"},
    {"msgs/value/delegator_address", "Delegator address"},
    {"msgs/value/validator_address", "Validator address"},
    {"msgs/value/validator_dst_address", "Validator dest"},
    {"msgs/value/validator_src_address", "Validator source"},
    {"msgs/value/inputs", "Inputs"},
    {"msgs/value/outputs", "Outputs"},
    {"msgs/value/contract", "Contract address"},
    {"msgs/value/funds", "Funds"},
    {"msgs/value/msg", "Msg"},
    {"msgs/value/sender", "Sender address"},

    {"msgs/value/inputs/address", "Src Address"},
    {"msgs/value/inputs/coins", "Src Coins"},
    {"msgs/value/outputs/address", "Dest Address"},
    {"msgs/value/outputs/coins", "Dest Coins"},
};

parser_error_t parser_display_make_friendly() {
    if (!parser_tx_obj.flags.cache_valid) {
        return parser_unexpected_value;
    }

    // post process keys
    for (size_t i = 0; i < array_length(key_substitutions); i++) {
        const char *str1 = (const char *)PIC(key_substitutions[i].str1);
        const char *str2 = (const char *)PIC(key_substitutions[i].str2);
        const uint16_t str1Len = strlen(str1);
        const uint16_t str2Len = strlen(str2);

        const uint16_t outKeyLen = strnlen(parser_tx_obj.query.out_key, parser_tx_obj.query.out_key_len);
        if ((outKeyLen == str1Len && strncmp(parser_tx_obj.query.out_key, str1, str1Len) == 0) &&
            parser_tx_obj.query.out_key_len >= str2Len) {
            MEMZERO(parser_tx_obj.query.out_key, parser_tx_obj.query.out_key_len);
            MEMCPY(parser_tx_obj.query.out_key, str2, str2Len);
            break;
        }
    }
    return parser_ok;
}

__Z_INLINE bool parser_areEqual(uint16_t tokenIdx, const char *expected) {
    if (parser_tx_obj.json.tokens[tokenIdx].type != JSMN_STRING) {
        return false;
    }

    int32_t len = parser_tx_obj.json.tokens[tokenIdx].end - parser_tx_obj.json.tokens[tokenIdx].start;
    if (len < 0) {
        return false;
    }

    if (strlen(expected) != (size_t)len) {
        return false;
    }

    const char *p = parser_tx_obj.tx + parser_tx_obj.json.tokens[tokenIdx].start;
    for (int32_t i = 0; i < len; i++) {
        if (expected[i] != *(p + i)) {
            return false;
        }
    }

    return true;
}

bool parser_isAmount(char *key) {
    if (key == NULL) {
        return false;
    }
    if (strcmp(key, "fee/amount") == 0) {
        return true;
    }

    if (strcmp(key, "msgs/value/amount") == 0) {
        return true;
    }

    if (strcmp(key, "tip/amount") == 0) {
        return true;
    }

    if (strcmp(key, "msgs/value/inputs/coins") == 0) {
        return true;
    }

    if (strcmp(key, "msgs/value/outputs/coins") == 0) {
        return true;
    }

    return false;
}

__Z_INLINE parser_error_t is_default_denom_base(const char *denom, uint8_t denom_len, bool *is_default) {
    if (denom == NULL || denom_len == 0) {
        return parser_unexpected_value;
    }
    if (is_default == NULL) {
        return parser_unexpected_value;
    }

    if (strlen(COIN_DEFAULT_DENOM_BASE) != denom_len) {
        *is_default = false;
        return parser_ok;
    }

    if (memcmp(denom, COIN_DEFAULT_DENOM_BASE, denom_len) == 0) {
        *is_default = true;
        return parser_ok;
    }

    return parser_ok;
}

void remove_fraction(char *s) {
    size_t len = strlen(s);

    // Find the decimal point
    char *decimal_point = strchr(s, '.');
    if (decimal_point == NULL) {
        // No decimal point found, nothing to remove
        return;
    }

    // Find the end of the string up to the decimal point
    size_t end_index = decimal_point - s;

    // Find the first non-zero digit after the decimal point
    size_t non_zero_index = end_index + 1;
    while (s[non_zero_index] == '0') {
        non_zero_index++;
    }

    // Check if there is a non-zero digit after the decimal point
    if (non_zero_index >= len) {
        // There is no non-zero digit after the decimal point
        // Remove the decimal point and trailing zeros
        s[end_index] = '\0';
    }
}

__Z_INLINE parser_error_t parser_formatAmountItem(uint16_t amountToken, char *outVal, uint16_t outValLen, uint8_t pageIdx,
                                                  uint8_t *pageCount) {
    if (outVal == NULL || outValLen == 0 || pageCount == NULL) {
        return parser_unexpected_error;
    }
    *pageCount = 0;

    uint16_t numElements;
    CHECK_ERROR(array_get_element_count(&parser_tx_obj.json, amountToken, &numElements))

    if (numElements == 0) {
        *pageCount = 1;
        snprintf(outVal, outValLen, "Empty");
        return parser_ok;
    }

    if (numElements != 4) {
        return parser_unexpected_field;
    }

    if (parser_tx_obj.json.tokens[amountToken].type != JSMN_OBJECT) {
        return parser_unexpected_field;
    }

    if (!parser_areEqual(amountToken + 1u, "amount")) {
        return parser_unexpected_field;
    }

    if (!parser_areEqual(amountToken + 3u, "denom")) {
        return parser_unexpected_field;
    }

    char bufferUI[100];
    char tmpDenom[COIN_DENOM_MAXSIZE];
    char tmpAmount[COIN_AMOUNT_MAXSIZE];
    MEMZERO(tmpDenom, sizeof tmpDenom);
    MEMZERO(tmpAmount, sizeof(tmpAmount));
    MEMZERO(outVal, outValLen);
    MEMZERO(bufferUI, sizeof(bufferUI));

    if (parser_tx_obj.json.tokens[amountToken + 2].start < 0 || parser_tx_obj.json.tokens[amountToken + 4].start < 0) {
        return parser_unexpected_buffer_end;
    }
    const char *amountPtr = parser_tx_obj.tx + parser_tx_obj.json.tokens[amountToken + 2].start;

    const int32_t amountLen =
        parser_tx_obj.json.tokens[amountToken + 2].end - parser_tx_obj.json.tokens[amountToken + 2].start;
    const char *denomPtr = parser_tx_obj.tx + parser_tx_obj.json.tokens[amountToken + 4].start;
    const int32_t denomLen =
        parser_tx_obj.json.tokens[amountToken + 4].end - parser_tx_obj.json.tokens[amountToken + 4].start;

    if (denomLen <= 0 || denomLen >= COIN_DENOM_MAXSIZE) {
        return parser_unexpected_error;
    }
    if (amountLen <= 0 || amountLen >= COIN_AMOUNT_MAXSIZE) {
        return parser_unexpected_error;
    }

    const size_t totalLen = amountLen + denomLen + 2;
    if (sizeof(bufferUI) < totalLen) {
        return parser_unexpected_buffer_end;
    }

    // Extract amount and denomination
    MEMCPY(tmpDenom, denomPtr, denomLen);
    MEMCPY(tmpAmount, amountPtr, amountLen);

    snprintf(bufferUI, sizeof(bufferUI), "%s ", tmpAmount);
    // If denomination has been recognized format and replace
    bool is_default = false;
    CHECK_ERROR(is_default_denom_base(denomPtr, denomLen, &is_default))

    if (is_default) {
        if (fpstr_to_str(bufferUI, sizeof(bufferUI), tmpAmount, COIN_DEFAULT_DENOM_FACTOR) != 0) {
            return parser_unexpected_error;
        }
        number_inplace_trimming(bufferUI, 1);
        remove_fraction(bufferUI);
        snprintf(tmpDenom, sizeof(tmpDenom), " %s", COIN_DEFAULT_DENOM_REPR);
    }

    z_str3join(bufferUI, sizeof(bufferUI), "", tmpDenom);
    pageString(outVal, outValLen, bufferUI, pageIdx, pageCount);

    return parser_ok;
}

parser_error_t parser_formatAmount(uint16_t amountToken, char *outVal, uint16_t outValLen, uint8_t pageIdx,
                                   uint8_t *pageCount) {
    if (outVal == NULL || pageCount == NULL) {
        return parser_unexpected_error;
    }
    *pageCount = 0;
    if (parser_tx_obj.json.tokens[amountToken].type != JSMN_ARRAY) {
        return parser_formatAmountItem(amountToken, outVal, outValLen, pageIdx, pageCount);
    }

    uint8_t totalPages = 0;
    uint8_t showItemSet = 0;
    uint8_t showPageIdx = pageIdx;
    uint16_t showItemTokenIdx = 0;

    uint16_t numberAmounts;
    CHECK_ERROR(array_get_element_count(&parser_tx_obj.json, amountToken, &numberAmounts))

    // Count total subpagesCount and calculate correct page and TokenIdx
    for (uint16_t i = 0; i < numberAmounts; i++) {
        uint16_t itemTokenIdx;
        uint8_t subpagesCount;

        CHECK_ERROR(array_get_nth_element(&parser_tx_obj.json, amountToken, i, &itemTokenIdx));
        CHECK_ERROR(parser_formatAmountItem(itemTokenIdx, outVal, outValLen, 0, &subpagesCount));
        // Check for overflow before accumulating pages
        if (totalPages > UINT8_MAX - subpagesCount) {
            return parser_value_out_of_range;
        }
        totalPages += subpagesCount;

        if (!showItemSet) {
            if (showPageIdx < subpagesCount) {
                showItemSet = 1;
                showItemTokenIdx = itemTokenIdx;
            } else {
                if (showPageIdx < subpagesCount) {
                    return parser_unexpected_value;
                }
                showPageIdx -= subpagesCount;
            }
        }
    }
    *pageCount = totalPages;
    if (pageIdx > totalPages) {
        return parser_unexpected_value;
    }

    if (totalPages == 0) {
        *pageCount = 1;
        snprintf(outVal, outValLen, "Empty");
        return parser_ok;
    }

    uint8_t dummy;
    return parser_formatAmountItem(showItemTokenIdx, outVal, outValLen, showPageIdx, &dummy);
}
