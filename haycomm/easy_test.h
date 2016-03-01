#pragma once

// from https://www.zhihu.com/question/22608939/answer/21963056
#include <iostream>
#include <string>

#define TEST_OF(name) \
    void TEST_FUNC_##name(); \
    class TEST_CLASS_##name { \
    public: \
        TEST_CLASS_##name() { \
            std::cout << "test-case[" << #name << "] begin... " << std::endl; \
            TEST_FUNC_##name(); \
            std::cout << "test-case[" << #name << "] end." << std::endl; \
        } \
    } TEST_INSTANCE_##name; \
    void TEST_FUNC_##name()

#define TEST_ASSERT(cond) do { \
        if (!(cond)) {std::cout << "fail line[" << __LINE__ << "]" << std::endl; throw 0;} \
    } while (0);

#define TEST_PRINT(msg) std::cout << (msg) << std::endl;

