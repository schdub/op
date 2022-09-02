#pragma once

#include <chrono>
#include <string>
#include <iostream>

namespace op {

class benchmark {
private:
    std::string msg_;
    const std::ostream & stream_;
    std::chrono::high_resolution_clock::time_point t1, t2;
public:
    benchmark(std::string msg, std::ostream & stream = std::cerr)
        : msg_(std::move(msg))
        , stream_(stream)
    {
        start();
    }
    ~benchmark() {
        stop();
        if (!msg_.empty()) stream_ << msg_ << " ";
        stream_ << msec_elapsed() << std::endl;
    }
    inline void start() {
        t1 = std::chrono::high_resolution_clock::now();
    }
    inline void stop() {
        t2 = std::chrono::high_resolution_clock::now();
    }
    inline unsigned msec_elapsed() {
        return std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();
    }
};

#define OP_BENCHMARK op::benchmark __op_bench##COUNTER(__FUNCTION__, std::cerr)

};