/*******************************************************************************
 *   (c) 2018 Zondax GmbH
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

#include <json/json_parser.h>
#include <parser.h>
#include <parser_validate.h>

#include "gtest/gtest.h"
#include "utils/common.h"

namespace {
TEST(TxValidationTest, CorrectFormat) {
    auto transaction =
        R"({"account_number":"0","chain_id":"test-chain-1","fee":{"amount":[{"amount":"5","denom":"photon"}],"gas":"10000"},"memo":"testmemo","msgs":[{"inputs":[{"address":"cosmosaccaddr1d9h8qat5e4ehc5","coins":[{"amount":"10","denom":"atom"}]}],"outputs":[{"address":"cosmosaccaddr1da6hgur4wse3jx32","coins":[{"amount":"10","denom":"atom"}]}]}],"sequence":"1"})";

    parsed_json_t json;
    parser_error_t err;

    err = JSON_PARSE(&json, transaction);
    ASSERT_EQ(err, parser_ok);

    err = parser_json_validate(&json);
    EXPECT_EQ(err, parser_ok) << "Validation failed, error: " << parser_getErrorDescription(err);
}

TEST(TxValidationTest, MissingAccountNumber) {
    auto transaction =
        R"({"chain_id":"test-chain-1","fee":{"amount":[{"amount":"5","denom":"photon"}],"gas":"10000"},"memo":"testmemo","msgs":[{"inputs":[{"address":"cosmosaccaddr1d9h8qat5e4ehc5","coins":[{"amount":"10","denom":"atom"}]}],"outputs":[{"address":"cosmosaccaddr1da6hgur4wse3jx32","coins":[{"amount":"10","denom":"atom"}]}]}],"sequence":"1"})";

    parsed_json_t json;
    parser_error_t err;

    err = JSON_PARSE(&json, transaction);
    ASSERT_EQ(err, parser_ok);

    err = parser_json_validate(&json);

    EXPECT_EQ(err, parser_json_missing_account_number) << "Validation failed, error: " << parser_getErrorDescription(err);
}

TEST(TxValidationTest, MissingChainId) {
    auto transaction =
        R"({"account_number":"0","fee":{"amount":[{"amount":"5","denom":"photon"}],"gas":"10000"},"memo":"testmemo","msgs":[{"inputs":[{"address":"cosmosaccaddr1d9h8qat5e4ehc5","coins":[{"amount":"10","denom":"atom"}]}],"outputs":[{"address":"cosmosaccaddr1da6hgur4wse3jx32","coins":[{"amount":"10","denom":"atom"}]}]}],"sequence":"1"})";

    parsed_json_t json;
    parser_error_t err;

    err = JSON_PARSE(&json, transaction);
    ASSERT_EQ(err, parser_ok);

    err = parser_json_validate(&json);
    EXPECT_EQ(err, parser_json_missing_chain_id) << "Validation failed, error: " << parser_getErrorDescription(err);
}

TEST(TxValidationTest, MissingFee) {
    auto transaction =
        R"({"account_number":"0","chain_id":"test-chain-1","fees":{"amount":[{"amount":"5","denom":"photon"}],"gas":"10000"},"memo":"testmemo","msgs":[{"inputs":[{"address":"cosmosaccaddr1d9h8qat5e4ehc5","coins":[{"amount":"10","denom":"atom"}]}],"outputs":[{"address":"cosmosaccaddr1da6hgur4wse3jx32","coins":[{"amount":"10","denom":"atom"}]}]}],"sequence":"1"})";

    parsed_json_t json;
    parser_error_t err;

    err = JSON_PARSE(&json, transaction);
    ASSERT_EQ(err, parser_ok);

    err = parser_json_validate(&json);
    EXPECT_EQ(err, parser_json_missing_fee) << "Validation failed, error: " << parser_getErrorDescription(err);
}

TEST(TxValidationTest, MissingMsgs) {
    auto transaction =
        R"({"account_number":"0","chain_id":"test-chain-1","fee":{"amount":[{"amount":"5","denom":"photon"}],"gas":"10000"},"memo":"testmemo","msgsble":[{"inputs":[{"address":"cosmosaccaddr1d9h8qat5e4ehc5","coins":[{"amount":"10","denom":"atom"}]}],"outputs":[{"address":"cosmosaccaddr1da6hgur4wse3jx32","coins":[{"amount":"10","denom":"atom"}]}]}],"sequence":"1"})";

    parsed_json_t json;
    parser_error_t err;

    err = JSON_PARSE(&json, transaction);
    ASSERT_EQ(err, parser_ok);

    err = parser_json_validate(&json);
    EXPECT_EQ(err, parser_json_missing_msgs) << "Validation failed, error: " << parser_getErrorDescription(err);
}

TEST(TxValidationTest, MissingSequence) {
    auto transaction =
        R"({"account_number":"0","chain_id":"test-chain-1","fee":{"amount":[{"amount":"5","denom":"photon"}],"gas":"10000"},"memo":"testmemo","msgs":[{"inputs":[{"address":"cosmosaccaddr1d9h8qat5e4ehc5","coins":[{"amount":"10","denom":"atom"}]}],"outputs":[{"address":"cosmosaccaddr1da6hgur4wse3jx32","coins":[{"amount":"10","denom":"atom"}]}]}]})";

    parsed_json_t json;
    parser_error_t err;

    err = JSON_PARSE(&json, transaction);
    ASSERT_EQ(err, parser_ok);

    err = parser_json_validate(&json);
    EXPECT_EQ(err, parser_json_missing_sequence) << "Validation failed, error: " << parser_getErrorDescription(err);
}

TEST(TxValidationTest, Spaces_InTheMiddle) {
    auto transaction =
        R"({"account_number":"0","chain_id":"test-chain-1", "fee":{"amount":[{"amount":"5","denom":"photon"}],"gas":"10000"},"memo":"testmemo","msgs":[{"inputs":[{"address":"cosmosaccaddr1d9h8qat5e4ehc5","coins":[{"amount":"10","denom":"atom"}]}],"outputs":[{"address":"cosmosaccaddr1da6hgur4wse3jx32","coins":[{"amount":"10","denom":"atom"}]}]}],"sequence":"1"})";

    parsed_json_t json;
    parser_error_t err;

    err = JSON_PARSE(&json, transaction);
    ASSERT_EQ(err, parser_ok);

    err = parser_json_validate(&json);
    EXPECT_EQ(err, parser_json_contains_whitespace) << "Validation failed, error: " << parser_getErrorDescription(err);
}

TEST(TxValidationTest, Spaces_AtTheFront) {
    auto transaction =
        R"({  "account_number":"0","chain_id":"test-chain-1","fee":{"amount":[{"amount":"5","denom":"photon"}],"gas":"10000"},"memo":"testmemo","msgs":[{"inputs":[{"address":"cosmosaccaddr1d9h8qat5e4ehc5","coins":[{"amount":"10","denom":"atom"}]}],"outputs":[{"address":"cosmosaccaddr1da6hgur4wse3jx32","coins":[{"amount":"10","denom":"atom"}]}]}],"sequence":"1"})";

    parsed_json_t json;
    parser_error_t err;

    err = JSON_PARSE(&json, transaction);
    ASSERT_EQ(err, parser_ok);

    err = parser_json_validate(&json);
    EXPECT_EQ(err, parser_json_contains_whitespace) << "Validation failed, error: " << parser_getErrorDescription(err);
}

TEST(TxValidationTest, Spaces_AtTheEnd) {
    auto transaction =
        R"({"account_number":"0","chain_id":"test-chain-1","fee":{"amount":[{"amount":"5","denom":"photon"}],"gas":"10000"},"memo":"testmemo","msgs":[{"inputs":[{"address":"cosmosaccaddr1d9h8qat5e4ehc5","coins":[{"amount":"10","denom":"atom"}]}],"outputs":[{"address":"cosmosaccaddr1da6hgur4wse3jx32","coins":[{"amount":"10","denom":"atom"}]}]}],"sequence":"1"  })";

    parsed_json_t json;
    parser_error_t err;

    err = JSON_PARSE(&json, transaction);
    ASSERT_EQ(err, parser_ok);

    err = parser_json_validate(&json);
    EXPECT_EQ(err, parser_json_contains_whitespace) << "Validation failed, error: " << parser_getErrorDescription(err);
}

TEST(TxValidationTest, Spaces_Lots) {
    auto transaction =
        R"({"account_number":"0",   "chain_id":"test-chain-1",    "fee":{"amount":    [{"amount":"5","denom":"photon"}],"gas":"10000"},"memo":"testmemo","msgs":[{"inputs":[{"address":"cosmosaccaddr1d9h8qat5e4ehc5","coins":[{"amount":"10","denom":"atom"}]}],"outputs":[{"address":"cosmosaccaddr1da6hgur4wse3jx32","coins":[{"amount":"10","denom":"atom"}]}]}],"sequence":"1"})";

    parsed_json_t json;
    parser_error_t err;

    err = JSON_PARSE(&json, transaction);
    ASSERT_EQ(err, parser_ok);

    err = parser_json_validate(&json);
    EXPECT_EQ(err, parser_json_contains_whitespace) << "Validation failed, error: " << parser_getErrorDescription(err);
}

TEST(TxValidationTest, AllowSpacesInString) {
    auto transaction =
        R"({"account_number":"0","chain_id":"    test-chain-1    ","fee":{"amount":[{"amount":"5","denom":"    photon"}],"gas":"10000"},"memo":"testmemo","msgs":[{"inputs":[{"address":"cosmosaccaddr1d9h8qat5e4ehc5","coins":[{"amount":"10","denom":"atom"}]}],"outputs":[{"address":"cosmosaccaddr1da6hgur4wse3jx32","coins":[{"amount":"10","denom":"atom"}]}]}],"sequence":"1"})";

    parsed_json_t json;
    parser_error_t err;

    err = JSON_PARSE(&json, transaction);
    ASSERT_EQ(err, parser_ok);

    err = parser_json_validate(&json);
    EXPECT_EQ(err, parser_ok) << "Validation failed, error: " << parser_getErrorDescription(err);
}

TEST(TxValidationTest, SortedDictionary) {
    auto transaction =
        R"({"account_number":"0","chain_id":"test-chain-1","fee":{"amount":[{"amount":"5","denom":"photon"}],"gas":"10000"},"memo":"testmemo","msgs":[{"inputs":[{"address":"cosmosaccaddr1d9h8qat5e4ehc5","coins":[{"amount":"10","denom":"atom"}]}],"outputs":[{"address":"cosmosaccaddr1da6hgur4wse3jx32","coins":[{"amount":"10","denom":"atom"}]}]}],"sequence":"1"})";

    parsed_json_t json;
    parser_error_t err;

    err = JSON_PARSE(&json, transaction);
    ASSERT_EQ(err, parser_ok);

    err = parser_json_validate(&json);
    EXPECT_EQ(err, parser_ok) << "Validation failed, error: " << parser_getErrorDescription(err);
}

TEST(TxValidationTest, NotSortedDictionary_FirstElement) {
    auto transaction =
        R"({"chain_id":"test-chain-1","account_number":"0","fee":{"amount":[{"amount":"5","denom":"photon"}],"gas":"10000"},"memo":"testmemo","msgs":[{"inputs":[{"address":"cosmosaccaddr1d9h8qat5e4ehc5","coins":[{"amount":"10","denom":"atom"}]}],"outputs":[{"address":"cosmosaccaddr1da6hgur4wse3jx32","coins":[{"amount":"10","denom":"atom"}]}]}],"sequence":"1"})";

    parsed_json_t json;
    parser_error_t err;

    err = JSON_PARSE(&json, transaction);
    ASSERT_EQ(err, parser_ok);

    err = parser_json_validate(&json);
    EXPECT_EQ(err, parser_json_is_not_sorted) << "Validation failed, error: " << parser_getErrorDescription(err);
}

TEST(TxValidationTest, NotSortedDictionary_MiddleElement) {
    auto transaction =
        R"({"account_number":"0","chain_id":"test-chain-1","memo":"testmemo","fee":{"amount":[{"amount":"5","denom":"photon"}],"gas":"10000"},"msgs":[{"inputs":[{"address":"cosmosaccaddr1d9h8qat5e4ehc5","coins":[{"amount":"10","denom":"atom"}]}],"outputs":[{"address":"cosmosaccaddr1da6hgur4wse3jx32","coins":[{"amount":"10","denom":"atom"}]}]}],"sequence":"1"})";

    parsed_json_t json;
    parser_error_t err;

    err = JSON_PARSE(&json, transaction);
    ASSERT_EQ(err, parser_ok);

    err = parser_json_validate(&json);
    EXPECT_EQ(err, parser_json_is_not_sorted) << "Validation failed, error: " << parser_getErrorDescription(err);
}

TEST(TxValidationTest, NotSortedDictionary_LastElement) {
    auto transaction =
        R"({"account_number":"0","chain_id":"test-chain-1","fee":{"amount":[{"amount":"5","denom":"photon"}],"gas":"10000"},"memo":"testmemo","sequence":"1","msgs":[{"inputs":[{"address":"cosmosaccaddr1d9h8qat5e4ehc5","coins":[{"amount":"10","denom":"atom"}]}],"outputs":[{"address":"cosmosaccaddr1da6hgur4wse3jx32","coins":[{"amount":"10","denom":"atom"}]}]}]})";

    parsed_json_t json;
    parser_error_t err;

    err = JSON_PARSE(&json, transaction);
    ASSERT_EQ(err, parser_ok);

    err = parser_json_validate(&json);
    EXPECT_EQ(err, parser_json_is_not_sorted) << "Validation failed, error: " << parser_getErrorDescription(err);
}
}  // namespace
