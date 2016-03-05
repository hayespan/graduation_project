#pragma once

#include "haycomm/log.h"

#define HayLog HayComm::g_Logger.log
#define LOG_INFO HayComm::LogLevel::LOG_INFO
#define LOG_WARN HayComm::LogLevel::LOG_WARN
#define LOG_ERR HayComm::LogLevel::LOG_ERR
#define LOG_DBG HayComm::LogLevel::LOG_DBG
#define LOG_FATAL HayComm::LogLevel::LOG_FATAL

