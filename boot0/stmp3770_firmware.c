#include "loader.h"
#include "board.h"
#include <string.h>
#include "stmp3770_firmware.h"

typedef struct _NAND_Control_Block
{
    uint32_t m_u32Fingerprint1;
    struct
    {
        uint8_t m_u8DataSetup;
        uint8_t m_u8DataHold;
        uint8_t m_u8AddressSetup;
        uint8_t m_u8DSAMPLE_TIME;
    } NAND_Timing;
    uint32_t m_u32DataPageSize;      //!< 2048 for 2K pages, 4096 for 4K pages.
    uint32_t m_u32TotalPageSize;     //!< 2112 for 2K pages, 4314 for 4K pages.
    uint32_t m_u32SectorsPerBlock;   //!< Number of 2K sections per block.
    uint32_t m_u32SectorInPageMask;  //!< Mask for handling pages > 2K.
    uint32_t m_u32SectorToPageShift; //!< Address shift for handling pages > 2K.
    uint32_t m_u32NumberOfNANDs;     //!< Total Number of NANDs - not used by ROM.

    uint32_t Reserve1[3];

    uint32_t m_u32Fingerprint2; // @ word offset 10

    uint32_t m_u32NumRowBytes;          //!< Number of row bytes in read/write transactions.
    uint32_t m_u32NumColumnBytes;       //!< Number of row bytes in read/write transactions.
    uint32_t m_u32TotalInternalDie;     //!< Number of separate chips in this NAND.
    uint32_t m_u32InternalPlanesPerDie; //!< Number of internal planes -
    // !<treat like separate chips.
    uint32_t m_u32CellType;    //!< MLC or SLC.
    uint32_t m_u32ECCType;     //!< 4 symbol or 8 symbol ECC?
    uint32_t m_u32Read1stCode; //!< First value sent to initiate a NAND
    //!< Read sequence.
    uint32_t m_u32Read2ndCode; //!< Second value sent to initiate a NAND
    //!< Read sequence.

    uint32_t Reserve[12];

    uint32_t m_u32Fingerprint3; // @ word offset 20
} NAND_Control_Block;

typedef struct _LogicalDriveLayoutBlock
{
    uint32_t m_u32Fingerprint1;
    struct
    {
        uint16_t m_u16Major;
        uint16_t m_u16Minor;
        uint16_t m_u16Sub;
    } LDLB_Version;

    uint16_t Reserve1;
    uint32_t Reserve2[8];

    uint32_t m_u32Fingerprint2;

    uint32_t m_u32Firmware_startingNAND;
    uint32_t m_u32Firmware_startingSector;
    uint32_t m_u32Firmware_sectorStride;
    uint32_t m_u32SectorsInFirmware;
    uint32_t m_u32Firmware_StartingNAND2;
    uint32_t m_u32Firmware_StartingSector2;
    uint32_t m_u32Firmware_SectorStride2;
    uint32_t m_u32SectorsInFirmware2;
    struct
    {
        uint16_t m_u16Major;
        uint16_t m_u16Minor;
        uint16_t m_u16Sub;
    } FirmwareVersion;

    uint32_t m_u32DiscoveredBBTableSector;
    uint32_t m_u32DiscoveredBBTableSector2;
    uint32_t Rsvd[8];

    uint32_t m_u32Fingerprint3;
    // uint32_t RSVD[100]; // Region configuration used by SDK only.

} LogicalDriveLayoutBlock_t;

typedef struct _DiscoveredBadBlockTable
{
    uint32_t m_u32Fingerprint1;
    uint32_t m_u32NumberBB_NAND0;
    uint32_t m_u32NumberBB_NAND1;
    uint32_t m_u32NumberBB_NAND2;
    uint32_t m_u32NumberBB_NAND3;
    uint32_t m_u32Number2KPagesBB_NAND0;
    uint32_t m_u32Number2KPagesBB_NAND1;
    uint32_t m_u32Number2KPagesBB_NAND2;
    uint32_t m_u32Number2KPagesBB_NAND3;
    uint32_t RSVD[2];
    uint32_t m_u32Fingerprint2;
    uint32_t RSVD2[20];
    uint32_t m_u32Fingerprint3;
} DiscoveredBadBlockTable_t;

void mkNCB(uint32_t block)
{
    uint8_t pg[2048 + 19];
    memset(pg, 0xff, sizeof(pg));

    NAND_Control_Block *NCB = (NAND_Control_Block *)pg;

    NCB->m_u32Fingerprint1 = 0x504D5453;
    NCB->NAND_Timing.m_u8DataSetup = 0x10;
    NCB->NAND_Timing.m_u8DataHold = 0x0C;
    NCB->NAND_Timing.m_u8AddressSetup = 0x14;
    NCB->NAND_Timing.m_u8DSAMPLE_TIME = 0x06;
    NCB->m_u32DataPageSize = 0x800;
    NCB->m_u32TotalPageSize = 0x840;
    NCB->m_u32SectorsPerBlock = 0x40;
    NCB->m_u32SectorInPageMask = 0;
    NCB->m_u32SectorToPageShift = 0;
    NCB->m_u32NumberOfNANDs = 1;

    NCB->m_u32Fingerprint2 = 0x2042434E;

    NCB->m_u32NumRowBytes = 2;
    NCB->m_u32NumColumnBytes = 2;
    NCB->m_u32TotalInternalDie = 1;
    NCB->m_u32InternalPlanesPerDie = 1;
    NCB->m_u32CellType = 2;
    NCB->m_u32ECCType = 0;
    NCB->m_u32Read1stCode = 0x00;
    NCB->m_u32Read2ndCode = 0x30;
    NCB->m_u32Fingerprint3 = 0x4E494252;

    pg[2048 + 2] = 0x42;
    pg[2048 + 3] = 0x43;
    pg[2048 + 4] = 0x42;
    pg[2048 + 5] = 0x20;

    bsp_nand_erase_block(block);
    bsp_nand_write_page(block * 64, pg);
}

void mkDBBT(uint32_t block)
{
    uint8_t pg[2048 + 19];
    memset(pg, 0, 2048);
    memset(&pg[2048], 0xff, 19);

    DiscoveredBadBlockTable_t *DBBT = (DiscoveredBadBlockTable_t *)pg;

    DBBT->m_u32Fingerprint1 = 0x504D5453;
    DBBT->m_u32Number2KPagesBB_NAND0 = 1;
    DBBT->m_u32Number2KPagesBB_NAND1 = 1;
    DBBT->m_u32Number2KPagesBB_NAND2 = 1;
    DBBT->m_u32Number2KPagesBB_NAND3 = 1;

    DBBT->m_u32Fingerprint2 = 0x54424244;
    DBBT->m_u32Fingerprint3 = 0x44494252;

    memset(&pg[2048 + 2], 0, 6);

    bsp_nand_erase_block(block);
    bsp_nand_write_page(block * 64, pg);
}

void mkLDLB(uint32_t block, uint32_t fwPageOffset, uint32_t fwPageTotal, uint32_t DBBT1_page, uint32_t DBBT2_page)
{

    uint8_t pg[2048 + 19];
    memset(pg, 0xff, sizeof(pg));

    LogicalDriveLayoutBlock_t *LDLB = (LogicalDriveLayoutBlock_t *)pg;

    LDLB->m_u32Fingerprint1 = 0x504D5453;
    LDLB->LDLB_Version.m_u16Major = 1;
    LDLB->LDLB_Version.m_u16Minor = 0;
    LDLB->LDLB_Version.m_u16Sub = 0;

    LDLB->m_u32Fingerprint2 = 0x424C444C;
    LDLB->m_u32Firmware_startingNAND = 0;
    LDLB->m_u32Firmware_startingSector = fwPageOffset;
    LDLB->m_u32Firmware_sectorStride = 0;
    LDLB->m_u32SectorsInFirmware = fwPageTotal;

    LDLB->m_u32Firmware_StartingNAND2 = 0;
    LDLB->m_u32Firmware_StartingSector2 = fwPageOffset;
    LDLB->m_u32Firmware_SectorStride2 = 0;
    LDLB->m_u32SectorsInFirmware2 = fwPageTotal;

    LDLB->FirmwareVersion.m_u16Major = 1;
    LDLB->FirmwareVersion.m_u16Minor = 0;
    LDLB->FirmwareVersion.m_u16Sub = 0;

    LDLB->m_u32DiscoveredBBTableSector = DBBT1_page;
    LDLB->m_u32DiscoveredBBTableSector2 = DBBT2_page;

    LDLB->m_u32Fingerprint3 = 0x4C494252;

    pg[2048 + 2] = 0x42; // B
    pg[2048 + 3] = 0x43; // C
    pg[2048 + 4] = 0x42; // B
    pg[2048 + 5] = 0x20; //' '

    bsp_nand_erase_block(block);
    bsp_nand_write_page(block * 64, pg);
}




