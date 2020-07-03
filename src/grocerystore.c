/**
 * Questo è lo starting point del programma
 */

#include "grocerystore.h"
#include "stdio.h"
#include "signal.h"
#include "string.h"

#define DEFAULT_CONFIG_FILE "config.txt"

char* parseConfigFile(int argc, char* args[]);

//grocerystore -c pathtoconfigfile
int main(int argc, char** args) {
    char* configFile = parseConfigFile(argc, args);
    printf("%s", configFile);
    return 0;
}

char* parseConfigFile(int argc, char* args[])
{
    int i = 0;
    while(i < argc && strcmp(args[i], "-c") != 0) {
        i++;
    }
    i++;
    return i < argc ? args[i]:DEFAULT_CONFIG_FILE;
}