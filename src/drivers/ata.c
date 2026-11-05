#include "drivers/ata.h"
#include "drivers/port_io.h"

bool ata_read_sectors(uint32_t lba, uint8_t count, void* buffer)
{
    if(count == 0 || buffer == 0 || lba > 0x0FFFFFFFu) return false;

    int iter = 0;
    while ((inb(ATA_PORT_STATUS) & ATA_SR_BSY) != 0)
    {
        iter++;
        if(iter > 100000) return false;
    }

    outb(ATA_PORT_DRIVE_HEAD, (uint8_t)(0xE0u | ((lba >> 24) & 0x0Fu)));
    outb(ATA_PORT_SECT_COUNT, count);
    outb(ATA_PORT_LBA_LOW,    (uint8_t)(lba & 0xFFu));
    outb(ATA_PORT_LBA_MID,    (uint8_t)((lba >> 8) & 0xFFu));
    outb(ATA_PORT_LBA_HIGH,   (uint8_t)((lba >> 16) & 0xFFu));

    outb(ATA_PORT_COMMAND, 0x20); // 0x20 = ATA_CMD_READ_PIO

    for(uint8_t s = 0; s < count; ++s)
    {
        inb(ATA_PORT_STATUS); inb(ATA_PORT_STATUS);
        inb(ATA_PORT_STATUS); inb(ATA_PORT_STATUS);

        iter = 0;
        while(1)
        {
            uint8_t status = inb(ATA_PORT_STATUS);
            if((status & ATA_SR_BSY) == 0)
            {
                if((status & (ATA_SR_ERR | ATA_SR_DF)) != 0) return false;
                if((status & ATA_SR_DRQ) != 0) break;
            }
            if(++iter > 100000) return false;
        }

        if((inb(ATA_PORT_STATUS) & (ATA_SR_ERR | ATA_SR_DF)) != 0) return false;

        uint16_t* out_words = (uint16_t*)((uint8_t*)buffer + (s * 512u));
        for (int w = 0; w < 256; ++w)
        {
            out_words[w] = inw(ATA_PORT_DATA);
        }
    }

    return true;
}