#include "cachelab.h"
#include <getopt.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

const int hitResultMask1 = 0x1;
const int hitResultMask2 = 0x10;
const int missResultMask = 0x100;
const int evictResultMask = 0x1000;

struct setvalue {
    int validbits;
    int tag;
    int ts;
};

struct sets {
    int setno;
    struct setvalue *values;
};

int getaddr(char *);

int process(char itype, struct sets *ss, int e, int soffset, int tnum, int* tscounter) {
    struct sets theset = ss[soffset];
    int i;
    int cacheresult = 0;
    for (i = 0; i < e; i++) {
        if (theset.values[i].validbits == 1 && theset.values[i].tag == tnum) {
            theset.values[i].ts = *tscounter;
            *tscounter += 1;
            cacheresult |= hitResultMask1;
            if (itype == 'M') {
                cacheresult |= hitResultMask2;
            }
            return cacheresult;
        }
    }
    int evictIndex = 0;
    for (i = 0; i < e; i++) {
        if (theset.values[i].validbits != 1) {
            evictIndex = i;
            break;
        } else if (theset.values[i].ts < theset.values[evictIndex].ts) {
            evictIndex = i;
        }
    }
    cacheresult |= missResultMask;
    if (theset.values[evictIndex].validbits == 1) {
        cacheresult |= evictResultMask;
    }
    theset.values[evictIndex].validbits = 1;
    theset.values[evictIndex].tag = tnum;
    theset.values[evictIndex].ts = *tscounter;
    *tscounter += 1;
    if (itype == 'M') {
        cacheresult |= hitResultMask2;
    }
    return cacheresult;
}

int main(int argc, char *argv[])
{
    int isverbose = 0;
    int opt = 0;
    int s = 0, e = 0, b = 0;
    int tscounter= 0;
    char *tpath;

    // parse arguments
    while ((opt = getopt(argc, argv, "hvs:E:b:t:")) != -1) {
        switch (opt) {
        case 'h':
            break;
        case 'v':
            isverbose = 1;
            break;
        case 's':
            s = atoi(optarg);
            break;
        case 'E':
            e = atoi(optarg);
            break;
        case 'b':
            b = atoi(optarg);
            break;
        case 't':
            tpath = optarg;
            break;
        }
    }
    printf("%d %d %d %d %s\n", isverbose, s, e, b, tpath);

    // init sets
    int setsize = 1 << s;
    struct sets *ss = malloc(setsize * sizeof(struct sets));
    int i, j;
    for (i = 0; i < setsize; i++) {
        ss[i].setno = i;
        ss[i].values = malloc(e * sizeof(struct setvalue));
        for (j = 0; j < e; j++) {
            ss[i].values[j].validbits = 0;
            ss[i].values[j].tag = 0;
            ss[i].values[j].ts = 0;
        }
    }
    FILE *fp;
    fp = fopen(tpath, "r");
    if (fp == NULL) {
        exit(EXIT_FAILURE);
    }

    int hitsresult = 0, missresult = 0, evictresult = 0;
    const size_t line_size = 300;
    char* line = malloc(line_size);

    // int boffset = 0;
    int soffset = 0, tnum = 0;
    const int bmask = (1<<b) - 1;
    const int smask = (1<<(s+b)) - bmask - 1;
    const unsigned long tmask = ~0 - smask - bmask;

    while (fgets(line, line_size, fp) != NULL)  {
        int size = strlen(line);
        if (size > 1 && line[0] == ' ') {
            char itype = line[1];
            int address = getaddr(line);
            soffset = (address & smask) >> b;
            tnum = (address & tmask) >> (b+s);
            int result = process(itype, ss, e, soffset, tnum, &tscounter);
            // parse result
            if (isverbose == 1) {
                line[size-1] = '\0';
                char* lr = line + 1;
                printf(lr);
            }
            if ((result & missResultMask) != 0) {
                missresult += 1;
                if (isverbose == 1) {
                    printf(" miss");
                }
            }
            if ((result & evictResultMask) != 0) {
                evictresult += 1;
                if (isverbose == 1) {
                    printf(" eviction");
                }
            }
            if ((result & hitResultMask1) != 0) {
                hitsresult += 1;
                if (isverbose == 1) {
                    printf(" hit");
                }
            }
            if ((result & hitResultMask2) != 0) {
                hitsresult += 1;
                if (isverbose == 1) {
                    printf(" hit");
                }
            }
            if (isverbose == 1) {
                printf("\n");
            }
        }
    }

    fclose(fp);
    if (line) {
        free(line);
    }
    for (i = 0; i < setsize; i++) {
        free(ss[i].values);
    }
    free(ss);

    printSummary(hitsresult, missresult, evictresult);
    return 0;
}

int getaddr(char *line) {
    int i;
    char *r = malloc(strlen(line) + 1);
    char *tmp = r;
    for (i = 3; i < strlen(line); i++) {
        if (line[i] == ',') {
            break;
        }
        *tmp = line[i];
        tmp++;
    }
    *tmp = '\0';
    return (int)strtol(r, NULL, 16);
}