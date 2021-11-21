/****************************************************************
 * PackScan: An SFC memory pack dump analysis tool.
 *
 * Written by Andrew Henderson (hendersa@icculus.org).
 *
 * This code is open source and licensed under the GPLv3. Please
 * review the LICENSE file for the details if you would like to
 * use this source code in your own projects.
 ***************************************************************/

#ifndef __PACK_H__
#define __PACK_H__

#include <string>
#include <vector>
#include <cstdint>

class Pack {
public:
    Pack(const char *filename);
    ~Pack();
    bool isLoaded(void) { return mIsLoaded; }
    void analyze(void);
    std::string generateReport(const bool color);

private:

    std::vector<uint8_t> mPackData;
    std::string mFilename;
    bool mIsLoaded;

    enum PackSize_t {
       INVALID = 0,
       SIZE_8M = (1024 * 1024),
       SIZE_32M = (4 * 1024 * 1024)
    } mPackSize;

    typedef struct {
        uint32_t address;       /* Address of header in pack */
        uint8_t licensee[2];    /* xFB0-xFB1 */
        uint8_t programType[4]; /* xFB2-xFB5 */
        uint8_t title[17];      /* xFC0-xFCF (plus NUL) */
        uint8_t blockAlloc[4];  /* xFD0-xFD3 */
        uint8_t starts[2];      /* xFD4-xFD5 */
        uint8_t dateMonth;      /* xFD6 */
        uint8_t dateDay;        /* xFD7 */
        uint8_t speedMap;       /* xFD8 */
        uint8_t fileType;       /* xFD9 */
        uint8_t maker;          /* xFDA */
        uint8_t version;        /* xFDB */
        uint16_t invChksum;   /* xFDC-xFDD */
        uint16_t chksum;      /* xFDE-xFDF */
    } Header_t;

    std::vector<Header_t> mBlockHeader;

    bool validHeader(const uint32_t block, const bool LoROM, Pack::Header_t *header);
    uint16_t calcCRC(const Header_t *header);
};

#endif /* __PACK_H__ */

