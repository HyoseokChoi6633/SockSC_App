#pragma once

#include "pch.h"

#ifdef CMYCHATWND_EXPORTS
#define CMYCHATWND_API __declspec(dllexport)
#else
#define CMYCHATWND_API __declspec(dllimport)
#endif
