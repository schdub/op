#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <gtest/gtest.h>

#include "url.hpp"

// URL ///////////////////////////////////////////////////////// //

TEST(URL, Parse) {
    {
        std::string str_url("protocol://host:port/path/on/server?query");
        op::URL url = op::URL::Parse(str_url);
        ASSERT_EQ(url.Protocol, "protocol");
        ASSERT_EQ(url.Host, "host");
        ASSERT_EQ(url.Port, "port");
        ASSERT_EQ(url.Path, "path/on/server");
        ASSERT_EQ(url.QueryString, "query");
    }
    {
        std::string str_url("https://mail.google.com/mail/u/0/?inbox");
        op::URL url = op::URL::Parse(str_url);
        ASSERT_EQ(url.Protocol, "https");
        ASSERT_EQ(url.Host, "mail.google.com");
        ASSERT_EQ(url.Port, "");
        ASSERT_EQ(url.Path, "mail/u/0/");
        ASSERT_EQ(url.QueryString, "inbox");
    }
}

TEST(URL, Encode) {
    {
        std::string str_query("'One two three'");
        std::string str_expected("%27One%20two%20three%27");
        std::string encoded = op::URL::Encode(str_query);
        ASSERT_EQ(encoded, str_expected);
    }
}

TEST(URL, Decode) {
    {
        std::string str_query("%27One%20two%20three%27");
        std::string str_expected("'One two three'");
        std::string decoded = op::URL::Decode(str_query);
        ASSERT_EQ(decoded, str_expected);
    }
    {
        std::string str_query("+One+two+three+");
        std::string str_expected(" One two three ");
        std::string decoded = op::URL::Decode(str_query);
        ASSERT_EQ(decoded, str_expected);
    }
}

int main(int argc, char ** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
