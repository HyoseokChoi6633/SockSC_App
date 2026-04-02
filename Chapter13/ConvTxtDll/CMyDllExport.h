#pragma once

#include "pch.h"

#ifdef CONVTXT_EXPORTS
#define CONVTXT_API __declspec(dllexport)
#else
#define CONVTXT_API __declspec(dllimport)
#endif
