//
// Copyright (C) 2009-2015 Oleg Polivets. All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
// 
// 1. Redistributions of source code must retain the above copyright 
//    notice, this list of conditions and the following disclaimer.  
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.  
// 3. Neither the name of the project nor the names of its contributors
//    may be used to endorse or promote products derived from this software
//    without specific prior written permission. 
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
// 

#pragma once

#include <string>
#include <stack>
#include <cstring>
#include <cmath>
#include "debug.hpp"

namespace op {

class eval {
public:
    enum {
        eval_ok = 0,
        eval_unbalanced,
        eval_invalidoperator,
        eval_invalidoperand,
        eval_evalerr,

        // binary
        operator_mul = 1, // *
        operator_div,     // a/b 
        operator_idiv,    // a\b 
        operator_mod,     // %
        operator_shl,     // <<
        operator_shr,     // >>
        operator_sub,     // -
        operator_add,     // +
        operator_xor,     // ^
        operator_band,    // &
        operator_bor,     // |
        operator_nor,     // !|
        operator_nand,    // !&
        operator_land,    // &&
        operator_lor,     // ||
        operator_iseq,    // ==
        operator_lt,      // <
        operator_gt,      // >
        operator_gte,     // >=
        operator_ne,      // !=
        operator_lte,     // <=
        operator_pow,     // **
        operator_begin = operator_mul,
        operator_end   = operator_pow,

        // functions
        function_sin,
        function_cos,
        function_tan,
        function_asin,
        function_acos,
        function_atan,
        function_sqrt,
        function_exp,
        function_lb,
        function_lg,
        function_ln,
        function_begin = function_sin,
        function_end   = function_ln,
    };

    static int calcInt(const std::string & expr, int * err = NULL) {
        return calc<int>(expr, err);
    }
    static long calcLong(const std::string & expr, int * err = NULL) {
        return calc<long>(expr, err);
    }
    static double calcDouble(const std::string & expr, int * err = NULL) {
        return calc<double>(expr, err);
    }

    template <typename T>
    static T calc(const std::string & expr, int * err = NULL) {
        int error = eval_evalerr;
        T r = 0;
        LOG("EXPR: '%s'", expr.c_str());
        try {
            std::string rpn;
            error = toRPN(expr, rpn);
            if (error == eval_ok) {
                LOG("RPN: '%s'", rpn.c_str());
                error = evaluateRPN(rpn, r);
            }
        } catch (...) {
            error = eval_evalerr;
        }
        if (err) *err = error;
        return r;
    }

    static int toRPN(const std::string & exp, std::string & rpn) {
        std::stack<FuncToken> ft;
        std::stack<std::string> st;
        std::string token, topToken;
        unsigned    tokenLen, topPrecedence, precedence;
        static const std::string SEP(" ");
        static const std::string EMPTY("");
        int skb = 0, fc = 0;

        rpn.clear();
        for (unsigned i = 0; i < exp.length(); ++i) {
            char token1 = exp[i];

            // skip white space
            if (isspace(token1)) continue;

            // push left parenthesis
            if (token1 == '(') {
                ++skb;
                st.push(EMPTY+token1);
                continue;
            }

            // flush all stack till matching the left-parenthesis
            if (token1 == ')') {
                --skb;
                for (;;) {
                    // could not match left-parenthesis
                    if (st.empty()) return eval_unbalanced;
                    topToken = st.top(); st.pop();
                    if (topToken == "(") break;
                    rpn.append(SEP+topToken);
                }
                if (fc > 0) {
                    if (ft.empty()) return eval_unbalanced;
                    if (ft.top().mSkb == skb) {
                        rpn.append(SEP+ft.top().mFunc);
                        ft.pop();
                    }
                    --fc;
                }
                continue;
            }

            precedence = 0;
            if (!isOperator(exp.c_str() + i, &precedence, &tokenLen)) {
                if (isFunction(exp.c_str() + i, &tokenLen)) {
                    // a function
                    ++fc;
                    token = exp.substr(i, tokenLen);
                    ft.push(FuncToken(skb, token));
                } else {
                    // an operand
                    tokenLen = getToken(exp.c_str() + i);
                    if (tokenLen == 0) return eval_invalidoperand;
                    token = exp.substr(i, tokenLen);
                    rpn.append(SEP + token);
                }
                i += tokenLen - 1;
                continue;
            }

            // is an operator
            // expression is empty or last operand an operator
            if (rpn.empty() || isOperator(token.c_str())) {
                rpn.append(SEP + "0");
            }

            // get current operator 
            topPrecedence = 0;
            token = exp.substr(i, tokenLen);
            i += tokenLen - 1;
            for (;;) {
                // get top's precedence
                if (!st.empty()) {
                    topToken = st.top();
                    if (!isOperator(topToken.c_str(), &topPrecedence))
                        topPrecedence = 1; // give a low priority if operator not ok!
                }

                if (st.empty() || st.top() == "(" || precedence > topPrecedence) {
                    st.push(token);
                    break;
                }

                // operator has lower precedence then pop it
                st.pop();
                rpn.append(SEP + topToken);
            }
        }

        while (!st.empty()) {
            topToken = st.top(); st.pop();
            if (topToken == "(") return eval_unbalanced;
            rpn.append(SEP + topToken);
        }

        return eval_ok;
    }

    template <typename T>
    static int evaluateRPN(const std::string & rpn, T & result) {
        std::stack<T> st;
        T r, op1, op2;
        unsigned tokenLen = 0;

        for (unsigned i = 0; i < rpn.length(); ++i) {
            if (isspace(rpn[i])) continue;

            int token = isOperator(rpn.c_str()+i, NULL, &tokenLen);
            if (!token) {
                double d;
                token = isFunction(rpn.c_str()+i, &tokenLen);
                if (!token) {
                    // operatorand
                    tokenLen = getToken(rpn.c_str() + i);
                    char *errstr;
                    double d = strtod(rpn.substr(i, tokenLen).c_str(), &errstr);
                    if (*errstr) return eval_invalidoperand;
                    i += tokenLen - 1;
                    r = d;
                    st.push(r);
                } else {
                    // function
                    if (st.empty()) return eval_unbalanced;
                    d = st.top(); st.pop();
                    i += tokenLen - 1;
                    switch (token) {
                    case function_sin:  d = sin(d*(M_PI/180.0));  break;
                    case function_cos:  d = cos(d*(M_PI/180.0));  break;
                    case function_tan:  d = tan(d*(M_PI/180.0));  break;
                    case function_asin: d = asin(d)*(180.0/M_PI); break;
                    case function_acos: d = acos(d)*(180.0/M_PI); break;
                    case function_atan: d = atan(d)*(180.0/M_PI); break;
                    case function_sqrt: d = sqrt(d);              break;
                    case function_exp:  d = exp(d);               break;
                    case function_lb:   d = log2(d);              break;
                    case function_lg:   d = log10(d);             break;
                    case function_ln:   d = log(d);               break;
                    default: continue;
                    }
                    r = d;
                    st.push(r);
                }
                continue;
            }

            // an operator

            op2 = st.top(); st.pop();
            if (st.empty()) return eval_unbalanced;
            op1 = st.top(); st.pop();

            i += tokenLen - 1;

            switch (token) {
            case operator_mul:  r = op1 * op2;                break;
            case operator_idiv: r = (long) (op1 / op2);       break;
            case operator_div:  r = op1 / op2;                break;
            case operator_mod:  r = (long)op1 % (long)op2;    break;
            case operator_add:  r = op1 + op2;                break;
            case operator_sub:  r = op1 - op2;                break;
            case operator_land: r = op1 && op2;               break;
            case operator_band: r = (long)op1 & (long)op2;    break;
            case operator_lor:  r = op1 || op2;               break;
            case operator_bor:  r = (long)op1 | (long)op2;    break;
            case operator_xor:  r = (long)op1 ^ (long)op2;    break;
            case operator_nor:  r = ~((long)op1 | (long)op2); break;
            case operator_nand: r = ~((long)op1 & (long)op2); break;
            case operator_iseq: r = op1 == op2;               break;
            case operator_ne:   r = op1 != op2;               break;
            case operator_shl:  r = (long)op1 << (long)op2;   break;
            case operator_shr:  r = (long)op1 >> (long)op2;   break;
            case operator_lt:   r = op1 < op2;                break;
            case operator_lte:  r = op1 <= op2;               break;
            case operator_gt:   r = op1 > op2;                break;
            case operator_gte:  r = op1 >= op2;               break;
            case operator_pow:  r = pow(op1, op2);            break;
            default: continue;
            }
            st.push(r);
        }

        result = st.top(); st.pop();
        if (!st.empty()) return eval_evalerr;
        return eval_ok;
    }

private:
    struct FuncToken {
        int mSkb;
        std::string mFunc;
        FuncToken(int skb, const std::string & func)
            : mSkb(skb), mFunc(func) {}
    };

    // returns 0 if 'ptr' isn't function
    // or functions unique id
    static unsigned isFunction(
        const char *ptr,
        unsigned * tokenLen
    ) {
        const char * funcs[] = {
            "sin",  "cos",  "tan",
            "asin", "acos", "atan",
            "sqrt", "exp",
            "lb",   "lg",   "ln"
        };
        unsigned len = 0;
        for (; isalpha(ptr[len]); ++len);
        if (!len) return 0;
        for (unsigned i = 0; i < sizeof(funcs)/sizeof(*funcs); ++i) {
            if (!memcmp(ptr, funcs[i], len)) {
                *tokenLen = len;
                return (function_begin + i);
            }
        }
        return 0;
    }

    // Returns 0 if 'op' is not an operator
    // Otherwise it returns the index of the operator in the 'operators' array
    static unsigned isOperator(
        const char * op, unsigned * pprec = NULL,
        unsigned * opLen = NULL
    ) {
        static struct operator_t {
            const char *op;
            unsigned precedence;
        } ops[] = {
            {NULL, 0},
            {"*", 19}, {"/", 19}, {"\\",19}, {"%", 18}, {"<<",17},
            {">>",17}, {"-", 16}, {"+", 16}, {"^", 15}, {"&", 15},
            {"|", 15}, {"!|",15}, {"!&",15}, {"&&",14}, {"||",14},
            {"==",13}, {"<", 13}, {">", 13}, {">=",13}, {"!=",13},
            {"<=",13}, {"**",19}
        };
        unsigned oplen = 0;
        static const std::string opchars("<>=*/%+-^&|\\!");
        while (opchars.find(op[oplen]) != opchars.npos) ++oplen;
        if (!oplen) return 0;
        for (unsigned i = 1; i < sizeof(ops)/sizeof(*ops); ++i) {
            if (ops[i].op[oplen] == '\0' && !memcmp(op, ops[i].op, oplen)) {
                if (pprec) *pprec = ops[i].precedence;
                if (opLen) *opLen = oplen;
                return i;
            }
        }
        return 0;
    }

    // returns the operands length.
    // scans as long as the current value is an alphanumeric or a decimal seperator
    static unsigned getToken(const char * str) {
        unsigned i = 0;
        while (isalnum(str[i]) || (str[i]=='.')) ++i;
        return i;
    }

}; // class eval

}; // namespace op
