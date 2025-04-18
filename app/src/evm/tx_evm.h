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

#include "coin.h"
#include "os.h"
#include "zxerror.h"

/// Parse message stored in transaction buffer
/// This function should be called as soon as full buffer data is loaded.
/// \return It returns NULL if data is valid or error message otherwise.
const char *tx_parse_eth(uint8_t *error_code);

/// Return the number of items in the transaction
zxerr_t tx_getNumItemsEth(uint8_t *num_items);

/// Gets an specific item from the transaction (including paging)
zxerr_t tx_getItemEth(int8_t displayIdx, char *outKey, uint16_t outKeyLen, char *outValue, uint16_t outValueLen,
                      uint8_t pageIdx, uint8_t *pageCount);

zxerr_t tx_compute_eth_v(unsigned int info, uint8_t *v, bool is_personal_message);
