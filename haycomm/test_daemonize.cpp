#include <iostream>
#include <unistd.h>

#include "daemonize.h"

using namespace std;

int main() {
    int iRet = HayUtils::Daemonize();
    cout << iRet << endl;
    sleep(10);
    return 0;
}
