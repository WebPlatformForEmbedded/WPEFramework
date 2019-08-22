#include <functional>
#include <sstream>
#include <gtest/gtest.h>
#include "JSON.h"

using namespace WPEFramework::Core;

TEST(JSONStringSerialization, EscapeCharacters)
{
    JSON::String json_string;
    json_string = "\"\\\b\f\n\r\t/";
    string jsonified_string;

    json_string.ToString(jsonified_string);

    EXPECT_EQ(jsonified_string, "\"\\\"\\\\\\b\\f\\n\\r\\t\\/\"");
}

TEST(JSONStringSerialization, EscapeNonPrintable) {
    string example = {static_cast<char>(1), static_cast<char>(127)};
    string serialized;

    JSON::String json;
    json = example;

    json.ToString(serialized);

    EXPECT_EQ(serialized, "\"\\u0001\\u007f\"");
}

TEST(JSONStringSerialization, SerializationIsReversable) {
    const string example = "{\"test\":123,\"test2\":\"123\x02\"}";
    string serialized;

    JSON::String json;
    json = example;

    json.ToString(serialized);
    json.FromString(serialized);

    EXPECT_EQ(json.Value(), example); 
}

TEST(JSONStringSerialization, MultipleSerializationIsReversable) {
    const string example = "{\"test\":123,\"test2\":\"123\x02\"}";
    string serialized;
    string unserialized;

    JSON::String json;
    json = example;

    json.ToString(serialized);
    json = serialized;
    json.ToString(serialized);

    json.FromString(serialized);
    unserialized = json.Value();
    json.FromString(unserialized);
    unserialized = json.Value();

    EXPECT_EQ(unserialized, example); 
}