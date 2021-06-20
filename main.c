/* 
The MIT License (MIT) 
 
Copyright (c) 2021 Chris Vavruska

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#define theToolsLength 0x0014
#include <types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <locator.h>
#include <memory.h>
#include <quickdraw.h>
#include <orca.h>

#include <babelstuff.h>

#define VERSION_STR "0.1"

//globals
word programID;
Ref startStopAddr;
struct StartStopRecord tStart = {
    0,             /* flags */
    0xC080,             /* video mode*/
    0,             /* resFilelD */
    0,             /* dPageHandle */
    0x0011,    /* numTools */
    {
    {1, 0x0300},   /* Tool Locator */
    {2, 0x0300},   /* Memory Manager */
    {3, 0x0300},   /* Miscellaneous Tools */
    {4, 0x0301},   /* QuickDraw II */
    {5, 0x0302},   /* Desk Manager */
    {6, 0x0300},   /* Event Manager */
    {14, 0x0301},   /* Window Manager */
    {15, 0x0301},   /* Menu Manager */
    {16, 0x0301},   /* Control Manager */
    {18, 0x0301},   /* QuickDraw II Aux. */
    {20, 0x0301},   /* LineEdit Tools */
    {21, 0x0301},   /* Dialog Manager */
    {22, 0x0300},   /* Scrap Manager */
    {27, 0x0301},   /* Font Manager */
    {28, 0x0301},   /* List Manager */
    {30, 0x0100},   /* Resource Manager */
    {34, 0x0101},   /* TextEdit Manager */
    }
};

//getopt externs
extern int getopt(int nargc, const char **nargv, const char *ostr);
extern char *optarg;
extern int optind;

bool toolStartup(void) {
    extern Ref startStopAddr;
    StartStopRecordPtr ss = (StartStopRecordPtr) startStopAddr;
    int i = 0;

    startStopAddr = StartUpTools(programID, noResourceMgr, (Ref) &tStart);
    ss = (StartStopRecordPtr) startStopAddr;
    if (toolerror()) {
        printf("Error starting Tools %4x\r", toolerror());
        GrafOff();
        return false;
    }
    GrafOff();
    return true;
}

void toolShutDown(void) {
    ShutDownTools(0, startStopAddr);
}

void usage(char *cmd) {
    printf("Usage:\r");
    printf("%s [options] 'source file' [output file] \r\r", cmd);
    printf("  Use babelfish to convert files. Input file must be specified. If\r");
    printf("  destination file is not specified then 'outfile' will be used.\r\r");
    printf("  -i id             Source Translator Id\r");
    printf("  -I name           Source Translator Name\r");
    printf("  -o id             Output Translator Id\r");
    printf("  -O name           Output Translator Name\r");
    printf("  -l type           List input translator IDs for type\r");
    printf("  -L type           List output translators IDs for type\r");
    printf("  -F                Delete output file (if exists) without permission\r");
    printf("  -t                List Translator Type IDs\r");
    printf("  -v                Version Information\r");
    printf("  -V                Verbose output\r");
    printf("  -?, -h            This message\r");
    printf("\r");
}

void showTranslatorIDs(void) {
    printf("Translator Type IDs:\r");
    showTranslatorTypes();
    printf("\r");
}

char* getTransName(int transID) {
    char *name = NULL;

    name = (char *)malloc(255);
    if (name != NULL) {
        babelfishNumToName(transID, name);
    }
    return name;
}

int main(int argc, char *argv[]) {
    int c;
    char *inputFile = NULL, *outputFile = "outfile";
    int inputTransId = 0, outputTransId = 0; 
    char *inputTransName = NULL, *outputTransName = NULL;
    char *inputName = NULL, *outputName = NULL;
    int listType = 0;
    bool done = false;
    int status = 0;
    bool verbose = false, autoRemove = false;

    programID = MMStartUp();

    if (toolStartup()) {
        if (argc > 1) {
            while ((c = getopt(argc, argv, "i:I:o:O:l:L:h?vVFt")) != -1) {
                switch (c) {
                case 'i':
                    inputTransId = atoi(optarg);
                    break;
                case 'I':
                    inputTransName = optarg;
                    break;
                case 'o':
                    outputTransId = atoi(optarg);
                    break;
                case 'O':
                    outputTransName = optarg;
                    break;
                case 'l':
                case 'L':
                    listType = atoi(optarg);
                    if (listType) {
                        if (c == 'L') {
                            listType |= 0x8000;
                        }
                        break;
                    }
                    printf("list translator type must be a positive integer value\r");
                    status = 1;
                case 'h':
                case '?':
                    done = true;
                    usage(argv[0]);
                    break;
                case 'v':
                    printf("%s - A bablefish converter v%s\r\r", argv[0], VERSION_STR);
                    done = true;
                    break;
                case 'V':
                    verbose = true;
                    break;
                case 'F':
                    autoRemove = true;
                    break;
                case 't':
                    showTranslatorIDs();
                    done = true;
                    break;
                }
            }
            if (!done) {
                if (listType) {
                    if (argc > 3) {
                        printf("List must not be used with any other options\r");
                        status = 1;
                    } else {
                        listTranslators(listType);
                    }
                } else {
                    if (optind >= argc) {
                        printf("No input file specified\r");
                        status = 1;
                        usage(argv[0]);
                    } else {
                        inputFile = argv[optind++];
                        if (optind < argc) {
                            outputFile = argv[optind];
                        }
                        if (inputTransName == NULL) {
                            inputName = inputTransName = getTransName(inputTransId);
                        } else {
                            inputTransId = babelfishNameToNum(inputTransName, false);
                        }
                        if (inputTransId && inputTransName && strlen(inputTransName)) {
                            if (verbose) {
                                printf("Input File        : %s\r", inputFile);
                                printf("Input Trans ID    : %d\r", inputTransId);
                                printf("Input Trans Name  : %s\r", inputTransName);
                            }
                        } else {
                            if (inputTransId) {
                                printf("Input translator id %d not found\r", inputTransId);
                            } else {
                                printf("Input translator %s not found\r", inputTransName);
                            }
                        }
                        if (outputTransName == NULL) {
                            outputName = outputTransName = getTransName(outputTransId);
                        } else {
                            outputTransId = babelfishNameToNum(outputTransName, false);
                        }
                        if (outputTransId && outputTransName && strlen(outputTransName)) {
                            if (verbose) {
                                printf("Output File       : %s\r", outputFile);
                                printf("output Trans ID   : %d\r", outputTransId);
                                printf("output Trans Name : %s\r", outputTransName);
                            }
                        } else {
                            if (outputTransId) {
                                printf("output translator id %d not found\r", outputTransId);
                            } else {
                                printf("output translator %s not found\r", outputTransName);
                            }
                        }
                        if (inputTransId && inputTransName && strlen(inputTransName)
                            && outputTransId && outputTransName && strlen(outputTransName)) {
                            babelConvert(inputFile, inputTransId,
                                         outputFile, outputTransId, verbose, autoRemove);
                        }
                        if (inputName) {
                            free(inputName);
                        }
                        if (outputName) {
                            free(outputName);
                        }
                    }
                }
            }
        } else {
            usage(argv[0]);
            status = 1;
        }
    }

    toolShutDown();
    return status;
}

