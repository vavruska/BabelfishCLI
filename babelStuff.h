#ifndef __BABELSTUFF_H__
#define __BABELSTUFF_H__

void showTranslatorTypes(void);
void babelfishNumToName(int transId, char *name);
int babelfishNameToNum(const char *name, bool exporting);
void listTranslators(int transTypeId);
void babelConvert(const char *inputFile, int inputTransID, 
                  const char *outputFile, int outputTransID, bool verbose, bool autoRemove);
#endif

