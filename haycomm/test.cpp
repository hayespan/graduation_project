#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <cstring>

#include "easy_test.h"
#include "daemonize.h"
#include "haylog.h"
#include "config.h"
#include "file.h"

using namespace std;
using namespace HayComm;

/*
TEST_OF(daemonize) {
    int iRet = 0;
    TEST_ASSERT((iRet=HayComm::Daemonize()) == 0);
}
*/

TEST_OF(haylog) {
    int x = 1;
    string s = "hello world";

    TEST_ASSERT(HayLog(LOG_INFO, "hayespan test %d %s", x, s.c_str()) == 0);
    TEST_ASSERT(HayLog(LOG_DBG, "hayespan test DBG") < 0);
    HayComm::g_Logger.SetLogLevel(LOG_DBG);
    TEST_ASSERT(HayLog(LOG_DBG, "hayespan test DBG") == 0);
    HayComm::g_Logger.SetLogLevel(LOG_ERR);
    TEST_ASSERT(HayLog(LOG_DBG, "hayespan test DBG") < 0);
}

TEST_OF(config) {
    const char * pszPath = "./haytmp.ini";
    FILE *tmpFile = fopen(pszPath, "w+");
    const char * pszContent = 
        "#this is comment 1\n"
        "[section1] # comment2\n"
        "k1=v1\n"
        "k2=v2\n"
        "k3=\n"
        "\n"
        "[section2]\n"
        "k1=v1\n"
        "#comment3\n"
        "k3=v3\n"
        "[section3]\n"
        "[section4]";
    fwrite(pszContent, strlen(pszContent), 1, tmpFile);
    fflush(tmpFile);
    fclose(tmpFile);

    Config conf;
    TEST_ASSERT(0 == conf.InitConfigFile(pszPath)); 
    string val;
    TEST_ASSERT(0 == conf.GetValue("section1", "k1", val));
    TEST_ASSERT("v1" == val);
    TEST_ASSERT(0 == conf.GetValue("section1", "k2", val));
    TEST_ASSERT("v2" == val);
    TEST_ASSERT(0 == conf.GetValue("section1", "k3", val));
    TEST_ASSERT("" == val);

    TEST_ASSERT(0 == conf.GetValue("section2", "k1", val));
    TEST_ASSERT("v1" == val);
    TEST_ASSERT(0 == conf.GetValue("section2", "k3", val));
    TEST_ASSERT("v3" == val);

    TEST_ASSERT(conf.GetConfigBody()[2].second.empty());
    TEST_ASSERT(conf.GetConfigBody()[3].second.empty());

    Body & body = conf.GetConfigBody();
    body[3].first = "section4.1";
    body[3].second.push_back(make_pair("k4", "v4"));
    TEST_ASSERT(0 == conf.UpdateConfigFile());
    unlink(pszPath);
}


int main() {
    return 0;
}
