/****************************************************************
 * PackScan: An SFC memory pack dump analysis tool.
 *
 * Written by Andrew Henderson (hendersa@icculus.org).
 *
 * This code is open source and licensed under the GPLv3. Please
 * review the LICENSE file for the details if you would like to
 * use this source code in your own projects.
 ***************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <iostream>
#include "version.h"
#include "pack.h"

static void showVersion(void)
{
    std::cout << "PackScan: An SFC memory pack dump analyzer (v";
    std::cout << VERSION << ")\n";
    std::cout << "Written by Andrew Henderson (hendersa@icculus.org)\n";
    std::cout << "Source available: https://www.beaglesatella.org\n\n";
}

static void showHelp(const char *programName) 
{
    std::cout << "Usage:\n\n  " << programName;
    std::cout << " [options] [Memory pack dump filename]\n";
    std::cout << "\nOptions:\n";
    std::cout << "  -n   No color codes in report\n";
    std::cout << "  -v   Display version\n";
    std::cout << "  -h   Display this help\n";
}

int main(int argc, char *argv[]) 
{
    Pack *pack = NULL;
    int fileIdx = 0;
    bool useColor = true;
    int opt = 0;

    /* Parse command line options */
    while ((opt = getopt(argc, argv, "nvh")) != -1)
    {
        switch(opt)
        {
            case 'n':
                useColor = false;
                break;

            case 'v':
                showVersion();
                return 0;

            case 'h':
                showVersion();
                showHelp(argv[0]);
                return 0;

            case '?':
                showVersion();
		std::cout << "\nUnknown option: '";
		std::cout << optopt << "'\n\n";
                showHelp(argv[0]);
                return 0;

            default:
                break;		
        }
    } /* End while */

    /* Parse memory pack dump filename */
    if ( optind == (argc - 1) )
    {
        fileIdx = optind;
    }
    else
    {
        /* Filename wasn't specified */
        showVersion();
	std::cout << "No memory pack file specified.\n\n";
        showHelp(argv[0]);
        return 0;
    }

    /* Load the pack data */
    pack = new Pack(argv[fileIdx]);
    if (!pack->isLoaded())
    {
        delete pack;
	return 0;
    }

    /* Analyze the pack and generate a report */
    pack->analyze();
    showVersion();
    std::cout << pack->generateReport(useColor);

    /* Delete the pack data and exit */
    delete pack;
    return 0;
}

