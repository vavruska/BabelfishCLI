#line 22 "/host/convert/babelStuff.c"
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

#pragma noroot

#include <types.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <locator.h>
#include <gsos.h>
#include <orca.h>

#include "babelfish.h"

const char *translatorKinds[] = {"Unknown", "Text", "Graphic-PixelMap", "Graphic-True Color Image",
    "Graphic-QuickDraw II Picture", "Font", "Sound" };

void showTranslatorTypes(void) {
    for (int x = 0; x < sizeof(translatorKinds) / sizeof(char *); x++) {
        printf("%3d  %s\r", x, translatorKinds[x]);
    }
}
const char* babelErrorStr(int id) {
    switch (id) {
    case bfNoErr:
        return "No Error";
    case bfNotStarted:
        return "Babelfish not started";
    case bfBFBusy:
        return "Babelfish busy";
    case bfMissingTools:
        return "Missing Tools";
    case bfNoTransErr:
        return "Translator not found";
    case bfTransBusy:
        return "Translator busy";
    case bfNotSupported:
        return "Not supported";
    case bfSupportNotFound:
        return "Support not found";
    case bfBadUserID:
        return "Bad user ID";
    case bfBadFileErr:
        return "Bad File Error";
    case bfReadErr:
        return "Media error reading from the disk";
    case bfWriteErr:
        return "Media error writing to the disk";
    case bfMemErr: 
        return "Memory error";
    default:
        return "Unknown Error";
    }
}

bool babelfishStart(void) {
    BFStartUpIn dataIn;
    BFStartUpOut dataOut;
    bool status = true;

    dataIn.userID = MMStartUp();
    SendRequest(BFStartUp, stopAfterOne + sendToName,
                        (Long)&NAME_OF_BABELFISH, (Long)&dataIn, (Ptr)&dataOut);
    if (dataOut.recvCount == 0) {
        printf("Babelfish not currently active or functioning correctly\r");
        status = false;
    } else if (dataOut.bfResult != bfNoErr) {
        printf("Babelfish startup returned error $%04x: %s\r", 
               dataOut.bfResult, babelErrorStr(dataOut.bfResult));
        status = false;
    }

    return status;
}

void babelfishShutdown(void) {
    BFShutDownIn dataIn;
    BFShutDownOut dataOut;

    dataIn.userID = MMStartUp();
    SendRequest(BFShutDown, stopAfterOne + sendToName,
                        (Long)&NAME_OF_BABELFISH, (Long)&dataIn, (Ptr)&dataOut);
}

static void getNum2Name(int transId, char *name) {
    BFTransNum2NameIn dataIn;
    BFTransNum2NameOut dataOut;
    BFXferRec xfer;
    
    dataIn.xferRecPtr = &xfer;
    memset(&xfer, 0, sizeof(BFXferRec));
    xfer.status = bfContinue;
    xfer.transNum = transId;
    SendRequest(BFTransNum2Name, stopAfterOne + sendToName,
                (Long)&NAME_OF_BABELFISH, (Long)&dataIn, (Ptr)&dataOut);
    if ((dataOut.recvCount != 0) && (dataOut.bfResult == bfNoErr)) {
        char *pTrans = *(dataOut.trNameHndl);
        size_t sz = pTrans[0];
        strncpy(name, pTrans + 1, sz);
        name[sz] = 0;
        DisposeHandle(dataOut.trNameHndl);
    } else {
        name[0] = 0;
    }
}

void babelfishNumToName(int transId, char *name) {
    if (babelfishStart()) {
        getNum2Name(transId, name);
        babelfishShutdown();
    }
}

static int getNameToNum(const char *name, bool exporting) {
    BFTransName2NumIn dataIn;
    BFTransName2NumOut dataOut;
    BFXferRec xfer;
    char pName[256];

    memset(&xfer, 0, sizeof(BFXferRec));
    strcpy(pName + 1, name);
    pName[0] = strlen(name);
    dataIn.namePtr = pName;
    dataIn.xferRecPtr = &xfer;
    xfer.status = bfContinue;
    xfer.miscFlags = exporting ? bffExporting : bffImporting;
    xfer.dataKinds.flag1 = 0;
    xfer.dataKinds.flag2 = 1;
    xfer.dataKinds.flag3 = 2;
    xfer.dataKinds.flag4 = 3;
    xfer.dataKinds.flag5 = 4;
    xfer.dataKinds.flag6 = 5;
    xfer.dataKinds.flag7 = 6;

    SendRequest(BFTransName2Num, stopAfterOne + sendToName,
                (Long)&NAME_OF_BABELFISH, (Long)&dataIn, (Ptr)&dataOut);

    if (dataOut.bfResult) {
        printf("Error getting translator ID. Error: %04x\r", dataOut.bfResult);
    }
    return xfer.transNum;
}

int babelfishNameToNum(const char *name, bool exporting) {
    int id = 0;
    if (babelfishStart()) {
        id = getNameToNum(name, exporting);
        babelfishShutdown();
    }
    return id;
}

void listTranslators(int transTypeId) {
    if (((transTypeId & 0xff) < 0) || ((transTypeId & 0xFF) > 6)) {
        printf("Bad tranlator Id\r");
    } else if (babelfishStart()) {
        BFMatchKindsIn dataIn;
        BFMatchKindsOut dataOut;
        BFXferRec xfer;

        printf("Listing %s %s Babelfish Translators\r", translatorKinds[transTypeId],
               transTypeId & 0x8000 ? "Export" : "Import");

        dataIn.xferRecPtr = &xfer;
        memset(&xfer, 0, sizeof(BFXferRec));
        xfer.status = bfContinue;
        xfer.miscFlags = transTypeId & 0x8000 ? bffExporting : bffImporting;
        xfer.dataKinds.flag1 = transTypeId & 0xff;
        SendRequest(BFMatchKinds, stopAfterOne + sendToName,
                    (Long)&NAME_OF_BABELFISH, (Long)&dataIn, (Ptr)&dataOut);

        if (dataOut.recvCount) {
            BFTransListKindsHndl listHandle = dataOut.transListHndl;
            if ((*listHandle)->transCount == 0) {
                printf("No Translators found for the type %d\r", xfer.dataKinds.flag1);
            } else {
                for (int x = 0; x < (*listHandle)->transCount; x++) {
                    char trans[256];
                    int id;

                    getNum2Name((*listHandle)->transArray[x], trans);
                    if (x == 0) {
                        printf(" ID  Name\r");
                        printf("---  --------------------------------\r");
                    }
                    printf("%3d  %s\r", (*listHandle)->transArray[x], trans);
                }
            }
        }
        babelfishShutdown();
    }
}

static int convert(BFReadInPtr importData, BFWriteInPtr exportData, bool verbose) {
    int status = bfContinue;
    BFResultOut outData;

    while (status == bfContinue) {
        SendRequest(BFRead, stopAfterOne + sendToName,
            (Long)&NAME_OF_BABELFISH, (Long)importData , (Ptr)&outData);
        status = importData->xferRecPtr->status;
        if ((status == bfContinue) || (status == bfDone)) {
            exportData->xferRecPtr->dataRecordPtr = importData->xferRecPtr->dataRecordPtr;
            exportData->xferRecPtr->status = status;
            SendRequest(BFWrite, stopAfterOne + sendToName,
                        (Long)&NAME_OF_BABELFISH, (Long)exportData , (Ptr)&outData);
            if (status != bfDone) {
                //status = exportData->xferRecPtr->status;
            }
        }
    }

    return status;
}
#pragma debug 0

int checkPath(GSString255Ptr path) {
    int error = 0;

    if ((path->text[0] != ':') && (path->text[0] != '/')) {
        PrefixRecGS prefixRec;
        ResultBuf255 prefix;

        prefix.bufSize = 255;
        prefixRec.pCount = 2;
        prefixRec.prefixNum = 8;
        prefixRec.buffer.getPrefix = &prefix;
        GetPrefixGS(&prefixRec);
        if (toolerror() == 0) {
            char *curr;
            prefix.bufString.text[prefix.bufString.length] = 0;
            strcat(prefix.bufString.text, path->text);
            strcpy(path->text, prefix.bufString.text);
            path->length = strlen(path->text);
            while (curr = strchr(path->text, ':')) {
                *curr = '/';
            }
        } else {
            printf("%s:%d toolerror %d\r", __FILE__, __LINE__, toolerror());
            error = toolerror();
        }
    }
    if (!error) {
        FileInfoRecGS info = { 5, path, 0 };
        GetFileInfoGS(&info);
        if (toolerror()) {
            error = toolerror();
            if (error != fileNotFound) {
                printf("%s:%d toolerror %d\r", __FILE__, __LINE__, error);
            }
        } else {
            if (info.storageType == directoryFile) {
                error = 2;
            }
        }
    }
    return error;
}

bool removePath(GSString255Ptr path, bool autoRemove) {
    char ans;
    NameRecGS destroy = { 1, path };
    bool removed = false;

    if (!autoRemove) {
        printf("File %s exists. Remove? (y/n) ", path->text);
        ans = getchar();
        if ((ans == 'y') || ans == 'Y') {
            autoRemove = true;
        }
    }
    if (autoRemove) {
        DestroyGS(&destroy);
        if (toolerror()) {
            printf("%s:%d toolerror %d\r", __FILE__, __LINE__, toolerror());
        } else {
            removed = true;
        }
    }
    return removed;
}

void babelConvert(const char *inputFilePath, int inputTransID, 
                  const char *outputFilePath, int outputTransID, bool verbose, bool removeOutput) {
    BFImportThisIn importIn;
    BFImportThisOut importOut;
    BFExportThisIn exportIn;
    BFExportThisOut exportOut;
    BFReadIn dataIn;
    BFReadOut dataOut;
    BFXferRec importXfer, exportXfer;
    char *inputFile, *outputFile;
    GSString255 inputFilePathGS, outputFilePathGS;
    int status = bfContinue;
    bool clear = true;

    if (babelfishStart()) {
        memset(&importXfer, 0, sizeof(BFXferRec));
        importIn.xferRecPtr = &importXfer;
        importXfer.status = bfContinue;
        importXfer.pCount = 12;
        importXfer.dataKinds.flag1 = 0;
        importXfer.dataKinds.flag2 = 1;
        importXfer.dataKinds.flag3 = 2;
        importXfer.dataKinds.flag4 = 3;
        importXfer.dataKinds.flag5 = 4;
        importXfer.dataKinds.flag6 = 5;
        importXfer.dataKinds.flag7 = 6;
        importXfer.transNum = inputTransID;
        strcpy(inputFilePathGS.text, inputFilePath);
        inputFilePathGS.length = strlen(inputFilePath);
        if (!(status = checkPath(&inputFilePathGS))) {
            importXfer.filePathPtr = &inputFilePathGS;
            inputFile = strrchr(inputFilePath, ':');
            if (inputFile == NULL) {
                inputFile = strrchr(inputFilePath, '/');
                if (inputFile == NULL) {
                    inputFile = inputFilePath;
                }
            }
            importXfer.fileNamePtr = inputFile;
            SendRequest(BFImportThis, stopAfterOne + sendToName,
                        (Long)&NAME_OF_BABELFISH, (Long)&importIn, (Ptr)&importOut);

            if ((importOut.recvCount !=0) && (importOut.bfResult == bfNoErr)) {
                memset(&exportXfer, 0, sizeof(BFXferRec));
                exportIn.xferRecPtr = &exportXfer;
                exportXfer.status = bfContinue;
                exportXfer.pCount = 12;
                exportXfer.dataKinds.flag1 = importXfer.dataKinds.flag1;
                exportXfer.transNum = outputTransID;
                strcpy(outputFilePathGS.text, outputFilePath);
                outputFilePathGS.length = strlen(outputFilePath);
                if (!(status = checkPath(&outputFilePathGS))) {
                    clear = removePath(&outputFilePathGS, removeOutput);
                } else {
                    if (status != fileNotFound) {
                        clear = false;
                        if (status == 2) {
                            printf("Unable to write. %s is a folder\r", outputFilePathGS.text);
                        }
                    }
                }
                if (clear) {
                    exportXfer.filePathPtr = &outputFilePathGS;
                    outputFile = strrchr(outputFilePath, ':');
                    if (outputFile == NULL) {
                        outputFile = strrchr(outputFilePath, '/');
                        if (outputFile == NULL) {
                            outputFile = outputFilePath;
                        }
                    }
                    exportXfer.fileNamePtr = outputFile;
                    status = bfNoErr;
                    SendRequest(BFExportThis, stopAfterOne + sendToName,
                                (Long)&NAME_OF_BABELFISH, (Long)&exportIn, (Ptr)&exportOut);
                    if ((exportOut.recvCount != 0) && (exportOut.bfResult == bfNoErr)) {
                        status = convert(&importIn, &exportIn, verbose);
                        if ((status != bfDone) && (status != bfNoErr)) {
                            printf("Error converting: $%04x:%s\r", 
                                   status, babelErrorStr(status));
                        }
                    }
                }
            }
        } else if (status == fileNotFound) {
            printf("Source file does not exists\r");
        }
        babelfishShutdown();
    }
}


