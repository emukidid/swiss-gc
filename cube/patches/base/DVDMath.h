// Copyright 2017 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

#pragma once

#include "common.h"

double CalculatePhysicalDiscPosition(u32 offset);
double CalculateSeekTime(u32 offset_from, u32 offset_to);
double CalculateRotationalLatency(u32 offset, double time);
double CalculateRawDiscReadTime(u32 offset, u32 length);
