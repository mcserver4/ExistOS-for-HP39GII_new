#pragma once
#include <stdint.h>

void mkNCB(uint32_t block);
void mkDBBT(uint32_t block);
void mkLDLB(uint32_t block, uint32_t fwPageOffset, uint32_t fwPageTotal, uint32_t DBBT1_page, uint32_t DBBT2_page);
void RestoreLDLB2(int block);
