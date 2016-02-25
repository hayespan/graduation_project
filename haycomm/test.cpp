#include <iostream>
#include <unistd.h>

#include "easy_test.h"
#include "daemonize.h"
#include "haylog.h"
#include <stdio.h>

using namespace std;

/*
TEST_OF(daemonize) {
    int iRet = 0;
    TEST_ASSERT((iRet=HayComm::Daemonize()) == 0);
    sleep(10);
}
*/

TEST_OF(haylog) {
    int x = 1;
    string s = "hello world";

    TEST_ASSERT(HayLog(LOG_INFO, "hayespan test %d %s", 
                x, s.c_str()) == 0);
    TEST_ASSERT(HayLog(LOG_DBG, "hayespan test DBG") < 0);
    HayComm::g_Logger.SetLogLevel(LOG_DBG);
    TEST_ASSERT(HayLog(LOG_DBG, "hayespan test DBG") == 0);
    HayComm::g_Logger.SetLogLevel(LOG_ERR);
    TEST_ASSERT(HayLog(LOG_DBG, "hayespan test DBG") < 0);

}

int main() {
    return 0;
}
