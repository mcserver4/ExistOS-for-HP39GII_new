#pragma once
#include <stdint.h>

void setHCLKDivider(uint32_t div);

void setCPUDivider(uint32_t div);

void setCPUFracDivider(uint32_t div);

void setCPU_HFreqDomain(bool enable);
