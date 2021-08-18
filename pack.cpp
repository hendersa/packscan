/****************************************************************
 * PackScan: An SFC memory pack dump analysis tool.
 *
 * Written by Andrew Henderson (hendersa@icculus.org).
 *
 * This code is open source and licensed under the GPLv3. Please
 * review the LICENSE file for the details if you would like to
 * use this source code in your own projects.
 ***************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fstream>
#include <streambuf>
#include <sstream>
#include <iostream>
#include <iomanip>
#include "pack.h"
#include "shiftjis_conv.h"

Pack::Pack(const char *filename) : 
    mIsLoaded(false), mPackSize(INVALID) 
{
    struct stat fileStat;
    int retVal = 0;

    mPackData.empty();
    retVal = stat(filename, &fileStat);

    /* Can we stat() the file? */
    if (retVal == -1) {
        switch(errno) {
            case EACCES:
                std::cout << "Unable to access file '" << filename;
                std::cout << "': Access denied\n";
	        return;

	    case ENOENT:
                std::cout << "Unable to access file '" << filename;
                std::cout << "': Path doesn't exist\n";
	        return;

            default:
                std::cout << "Unable to access file '" << filename;
                std::cout << "': Error opening file\n";
	        return;
        } /* End case */
    } /* End if */

    /* Is the file an actual file? */
    if ((fileStat.st_mode & S_IFMT) != S_IFREG) 
    {
        std::cout << "Unable to access file '" << filename;
        std::cout << "': Not a file\n";
	return;
    }

    /* Is the file the right size for a valid dump? */
    switch(fileStat.st_size) 
    {
        case SIZE_8M:
        case SIZE_32M:
            mPackSize = static_cast<Pack::PackSize_t>(fileStat.st_size);
	    break;

        default:
            std::cout <<"Dump '" << filename << "' is invalid size (";
            std::cout << fileStat.st_size << " bytes)\n";
	    return;
    }

    /* Load file data into mPackData */
    mFilename = std::string(filename);
    std::ifstream str(filename);
    mPackData.reserve(mPackSize);
    mPackData.assign((std::istreambuf_iterator<char>(str)), 
        std::istreambuf_iterator<char>());

    /* Done! */
    mIsLoaded = true;
}

Pack::~Pack()
{
    if (mIsLoaded) mPackData.empty();
}

void Pack::analyze(void) 
{
    uint32_t i = 0;
    uint32_t totalBlocks = 0;
    Header_t header;

    if (!mIsLoaded) {
        mBlockHeader.empty();	    
        return;
    }

    switch (mPackSize) {
        case SIZE_8M:
            totalBlocks = 8;
            break;

        case SIZE_32M:
            totalBlocks = 32;
            break;

        default:
            totalBlocks = 0;
    } /* End switch */

    for (i=0; i < totalBlocks; i++) 
    {
        /* Check block for a header */
        if ( validHeader(i, true, &header) )
            mBlockHeader.push_back(header);
        else if ( validHeader(i, false, &header) )
            mBlockHeader.push_back(header);
	
    } /* End for */
}

bool Pack::validHeader(const uint32_t block, const bool LoROM, Pack::Header_t *header) 
{
    uint32_t offset = 0;
    uint32_t i = 0;

    memset(header, 0, sizeof(Pack::Header_t));

    /* Check for a header */
    if (LoROM) offset = (block * 0x10000) + 0x7FB0;
    else offset = (block * 0x10000) + 0xFFB0;

    /* Copy header address */
    header->address = offset;

    /* Copy licensee */
    for (i=0; i < 2; i++)
        header->licensee[i] = mPackData[offset + i + 0x00];

    /* Copy program type */
    for (i=0; i < 4; i++)
        header->programType[i] = mPackData[offset + i + 0x02];

    /* Check for valid program type */
    if ( (header->programType[0] != 0) ||
        (header->programType[1] != 0) ||
        (header->programType[3] != 0) ||
        (header->programType[2] > 2) )
        return false;

    /* Copy title */
    for (i=0; i < 16; i++)
        header->title[i] = mPackData[offset + i + 0x10];
    header->title[16] = '\0';

    /* Copy block allocation flags */
    for (i=0; i < 4; i++)
        header->blockAlloc[i] = mPackData[offset + i + 0x20];
  
    /* Check for valid block allocation */
    if (mPackData.size() != SIZE_8M)
    {
        if ( (header->blockAlloc[3] != 0) ||
            (header->blockAlloc[2] != 0) ||
            (header->blockAlloc[1] != 0) )
            /* Allocating blocks beyond an 8M datapack */
            return false;
    }

    /* Copy limited starts */
    for (i=0; i < 2; i++)
        header->starts[i] = mPackData[offset + i + 0x24];

    /* Copy month/day */
    header->dateMonth = mPackData[offset + 0x26];
    header->dateDay = mPackData[offset + 0x27];

    /* Copy map mode */
    header->speedMap = mPackData[offset + 0x28];

    /* Copy file type */
    header->fileType = mPackData[offset + 0x29];

    header->maker = mPackData[offset + 0x2A];

    /* Check for valid version number */
    if (mPackData[offset + 0x2A] != 0x33)
        return false;

    /* Copy version */
    header->version = mPackData[offset + 0x2B];

    /* Copy inverse checksum */
    header->invChksum = (uint8_t)mPackData[offset + 0x2C];
    header->invChksum += ((uint8_t)mPackData[offset + 0x2D]) << 8;

    /* Copy checksum */
    header->chksum = (uint8_t)mPackData[offset + 0x2E];
    header->chksum += ((uint8_t)mPackData[offset + 0x2F]) << 8;
    return true;    
}

std::string Pack::generateReport(const bool color) 
{
    std::stringstream report;
    uint32_t i = 0, x = 0;
    uint32_t temp = 0;
    uint16_t tempCRC = 0;

    std::string colorReset = "";
    std::string colorLabel = "";
    std::string colorGood = "";
    std::string colorBad = "";

    /* If no memory pack is loaded, we're done */
    if (!mIsLoaded) 
    {
        report << "No memory pack is loaded, exiting...\n";
	return report.str();
    }

    /* If the report is in color, set up the terminal color codes */
    if (color)
    {
        colorReset = "\u001b[0m";
        colorLabel = "\u001b[33m"; /* Yellow */
        colorGood = "\u001b[32m";  /* Green */
	colorBad = "\u001b[31m";   /* Red */
    }

    report << colorLabel << "MEMORY PACK FILENAME: " << colorReset;
    report << mFilename << "\n";
    report << colorLabel << "MEMORY PACK SIZE:     " << colorReset;
    report << mPackSize << " bytes\n";

    for(i=0; i < mBlockHeader.size(); i++) 
    {
        report << std::dec << colorLabel << "\nHEADER #" << (i + 1);
        report << " (offset 0x" << std::uppercase << std::setfill('0');
        report << std::setw(5) << std::hex << (int)mBlockHeader[i].address;
        report << "):\n" << std::dec << std::setw(1);

        report << "    TITLE:" << colorReset << "                [";
        report << sjis2utf8((char *)mBlockHeader[i].title) << "]\n";

	report << colorLabel << "    DATE:" << colorReset;
	report << "                 ";
        report << (mBlockHeader[i].dateMonth >> 4) << "/";
        report << (mBlockHeader[i].dateDay >> 3) << '\n';

	report << colorLabel << "    REPORTED CRC/INVERSE:" << colorReset;
        report << " 0x" << std::uppercase << std::setfill('0') << std::setw(4);
        report << std::hex << (int)mBlockHeader[i].chksum;
        report << "/0x";
        report << std::uppercase << std::setfill('0') << std::setw(4);
        report << std::hex << (int)mBlockHeader[i].invChksum;
	if ((mBlockHeader[i].chksum + mBlockHeader[i].invChksum) == 0xFFFF) 
            report << colorGood << " [LOOKS OK!]\n";
	else
            report << colorBad << " [LOOKS BAD]\n";

	report << colorLabel << "    CALCULATED CRC:" << colorReset;
	report << "       0x";
        tempCRC = calcCRC(&(mBlockHeader[i]));
        report << std::uppercase << std::setfill('0') << std::setw(4);
        report << std::hex << tempCRC << std::dec;
        if (tempCRC == mBlockHeader[i].chksum)
            report << colorGood << " [MATCHES REPORTED]";
        else
            report << colorBad << " [DOES NOT MATCH REPORTED]";
	report << "\n";

        report << colorLabel << "    BS-X MENU VISIBILITY: " << colorReset;
	if ( ((mBlockHeader[i].chksum == 0) && 
            (mBlockHeader[i].invChksum == 0)) || 
            (mBlockHeader[i].maker != 0x33) ||
            (mBlockHeader[i].starts[1] == 0x80) )
            report << "No" << colorBad << " [NOT SHOWN IN MENU]\n";
	else
            report << "Yes" << colorGood << " [SHOWS IN MENU]\n";

	report << std::dec << colorLabel << "    PROGRAM TYPE:";
        report << colorReset << "         ";
	switch (mBlockHeader[i].programType[2]) {
            case 0x00:
                report << "65C816 code\n";
                break;

            case 0x01:
                report << "BS-X bytecode\n";
                break;

            case 0x02:
                report << "SA-1 code\n";
                break;

            default:
                report << colorBad << "UNKNOWN\n";
        } /* End switch */

	report << colorLabel << "    BOOTS REMAINING:";
        report << colorReset << "      ";
        if ( (mBlockHeader[i].starts[1]) & 0x80 )
        {
            report << ( (mBlockHeader[i].starts[1] >> 2) & 0x1F ); 
            report << "\n"; 
        }
        else
            report << "Unlimited\n";

        report << colorLabel << "    ROM MAPPING/SPEED:    " << colorReset;
        if ( mBlockHeader[i].speedMap & 1 )
            report << "HiROM,";
        else
            report << "LoROM,";

	if ( (mBlockHeader[i].speedMap >> 4) > 2 )
            report << "FastROM (120 ns)\n";
        else
            report << "SlowROM (200 ns)\n";

	report << colorLabel << "    EXECUTION:            " << colorReset;
	if (mBlockHeader[i].fileType & 0x20)
            report << "PSRAM,";
        else
            report << "FLASH,";
	if (mBlockHeader[i].fileType & 0x10)
            report << "Soundlink MUTED,";
	else
            report << "Soundlink UNMUTED,";
        if (mBlockHeader[i].fileType & 0x80)
            report << "NO St. GIGA intro\n";
        else
            report << "St. GIGA intro\n";

        report << colorLabel << "    BLOCK ALLOCATION:" << colorReset;
	report << "     [";
        temp = (mBlockHeader[i].blockAlloc[3] << 24);
        temp |= (mBlockHeader[i].blockAlloc[2] << 16);
	temp |= (mBlockHeader[i].blockAlloc[1] << 8);
	temp |= (mBlockHeader[i].blockAlloc[0] << 0);
	for (x=0; x < (mPackSize / (128 * 1024)); x++)
        {
            if ( (temp >> x) & 0x1 )
                report << "X";
            else
                report << ".";
        } /* End for */
        
       report << "]\n";	
    }

    return report.str();
}

uint16_t Pack::calcCRC(const Pack::Header_t *header)
{
    uint16_t crc = 0;
    uint32_t i = 0;
    uint32_t x = 0;
    uint32_t bitmask = 0;
    uint32_t totalBlocks = mPackSize / (128 * 1024);
    
    bitmask =  (header->blockAlloc[3] << 24);
    bitmask |= (header->blockAlloc[2] << 16);
    bitmask |= (header->blockAlloc[1] << 8);
    bitmask |= (header->blockAlloc[0] << 0);

    for (x = 0; x < totalBlocks; x++)
    {
        /* Is the current block in use by this header? */
        if ( (bitmask >> x) & 0x1 )
        {
            /* Does the current block contain the header? */
            if ( ((x * 0x20000) < header->address) &&
                 (((x+1) * 0x20000) > header->address) )
            {
                /* Calculate CRC before header location */
                for (i = (x * 0x20000); i < header->address; i++)
                    crc += (uint8_t)mPackData[i];

                /*  Calculate CRC after header location */
                for (i = (header->address + 0x30); i < ((x+1) * 0x20000); i++ )
                    crc += (uint8_t)mPackData[i];
            }
            else
            {
                /* Calculate CRC of entire block */
                for (i = (x * 0x20000); i < ((x+1) * 0x20000); i++)
                    crc += (uint8_t)mPackData[i];
            }
        } /* End if */
    } /* End for */

    /* Done! */
    return crc;
}

