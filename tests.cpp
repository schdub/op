#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <gtest/gtest.h>
#include <cassert>

#include "debug.hpp"
#include "url.hpp"
#include "eval.hpp"

// Eval //////////////////////////////////////////////////////// //

TEST(Eval, calculations) {

    struct Expections {
        std::string str_;
        double exp_;
    };
    std::vector<Expections> to_exec = {
        { "2 + 2",       4 },
        { "2 + 2 * 2",   6 },
        { "(2 + 2) * 2", 8 },
        { "(2 - 2) + 2", 2 },
        { "1 << 8",    256 },
        { "64/8",        8 },
        { "sqrt(4)-2", 0.0 },
        { "cos(60)",   0.5 },
        { "sin(60)", 0.8660254037844386 },
        { "tan(60)", 1.7320508075688767 },
    };
    for (size_t i = 0; i < to_exec.size(); ++i) {
        int error = 0;
        const auto & ref_original = to_exec[i];
        double answer = op::eval::calcDouble(ref_original.str_, &error);
        ASSERT_DOUBLE_EQ(answer, ref_original.exp_);
        ASSERT_EQ(error, 0);
    }
}

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
