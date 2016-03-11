#pragma once

#include <signal.h>

int AddSigHandler(int iSig, sighandler_t pfSigHandler);

int AddToEpoll(int iEpFd, int iFd, int iEv, bool bNonBlock=true);

int DelFromEpoll(int iEpFd, int iFd);

int ModInEpoll(int iEpFd, int iFd, int iEv, bool bNonBlock=true);

