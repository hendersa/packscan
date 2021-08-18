
/* The code in this file was originally part of the "apollo-ps3" 
 * project, licensed under GPLv3. You can find the original code 
 * for these functions in the "psv_resign.c" file of the project:
 *
 * https://github.com/bucanero/apollo-ps3/blob/master/source/psv_resign.c
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <stdint.h>
#include "shiftjis_table.h"

const char SJIS_REPLACEMENT_TABLE[] = 
    " ,.,..:;?!\"*'`*^"
    "-_????????*---/\\"
    "~||--''\"\"()()[]{"
    "}<><>[][][]+-+X?"
    "-==<><>????*'\"CY"
    "$c&%#&*@S*******"
    "*******T><^_'='";


//Convert Shift-JIS characters to ASCII equivalent
void sjis2ascii(char* bData)
{
	uint16_t ch;
	int i, j = 0;
	int len = strlen(bData);
	
	for (i = 0; i < len; i += 2)
	{
		ch = (bData[i]<<8) | bData[i+1];

		// 'A' .. 'Z'
		// '0' .. '9'
		if ((ch >= 0x8260 && ch <= 0x8279) || (ch >= 0x824F && ch <= 0x8258))
		{
			bData[j++] = (ch & 0xFF) - 0x1F;
			continue;
		}

		// 'a' .. 'z'
		if (ch >= 0x8281 && ch <= 0x829A)
		{
			bData[j++] = (ch & 0xFF) - 0x20;
			continue;
		}

		if (ch >= 0x8140 && ch <= 0x81AC)
		{
			bData[j++] = SJIS_REPLACEMENT_TABLE[(ch & 0xFF) - 0x40];
			continue;
		}

		if (ch == 0x0000)
		{
			//End of the string
			bData[j] = 0;
			return;
		}

		// Character not found
		bData[j++] = bData[i];
		bData[j++] = bData[i+1];
	}

	bData[j] = 0;
	return;
}

// PSV files (PS1/PS2) savegame titles are stored in Shift-JIS
char* sjis2utf8(char* input)
{
    // Simplify the input and decode standard ASCII characters
    sjis2ascii(input);

    int len = strlen(input);
    char* output = (char *)malloc(3 * len); //ShiftJis won't give 4byte UTF8, so max. 3 byte per input char are needed
    size_t indexInput = 0, indexOutput = 0;

    while(indexInput < len)
    {
        char arraySection = ((uint8_t)input[indexInput]) >> 4;

        size_t arrayOffset;
        if(arraySection == 0x8) arrayOffset = 0x100; //these are two-byte shiftjis
        else if(arraySection == 0x9) arrayOffset = 0x1100;
        else if(arraySection == 0xE) arrayOffset = 0x2100;
        else arrayOffset = 0; //this is one byte shiftjis

        //determining real array offset
        if(arrayOffset)
        {
            arrayOffset += (((uint8_t)input[indexInput]) & 0xf) << 8;
            indexInput++;
            if(indexInput >= len) break;
        }
        arrayOffset += (uint8_t)input[indexInput++];
        arrayOffset <<= 1;

        //unicode number is...
        uint16_t unicodeValue = (shiftJIS_convTable[arrayOffset] << 8) | shiftJIS_convTable[arrayOffset + 1];

        //converting to UTF8
        if(unicodeValue < 0x80)
        {
            output[indexOutput++] = unicodeValue;
        }
        else if(unicodeValue < 0x800)
        {
            output[indexOutput++] = 0xC0 | (unicodeValue >> 6);
            output[indexOutput++] = 0x80 | (unicodeValue & 0x3f);
        }
        else
        {
            output[indexOutput++] = 0xE0 | (unicodeValue >> 12);
            output[indexOutput++] = 0x80 | ((unicodeValue & 0xfff) >> 6);
            output[indexOutput++] = 0x80 | (unicodeValue & 0x3f);
        }
    }

	//remove the unnecessary bytes
    output[indexOutput] = 0;
    return output;
}

