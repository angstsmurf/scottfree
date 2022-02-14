//
//  detectgame.c
//  scott
//
//  Created by Administrator on 2022-01-10.
//

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "scott.h"

#include "detectgame.h"
#include "gameinfo.h"
#include "decompresstext.h"

#include "parser.h"
#include "load_TI99_4a.h"


extern const char *sysdict_zx[MAX_SYSMESS];
extern int header[];

struct dictionaryKey {
    dictionary_type dict;
    char *signature;
};

struct dictionaryKey dictKeys[] = {
    { FOUR_LETTER_UNCOMPRESSED, "AUTO\0GO\0" },
    { THREE_LETTER_UNCOMPRESSED, "AUT\0GO\0" },
    { FIVE_LETTER_UNCOMPRESSED, "AUTO\0\0GO" },
    { FOUR_LETTER_COMPRESSED, "aUTOgO\0" },
    { GERMAN, "\xc7" "EHENSTEIGE" },
    { FIVE_LETTER_COMPRESSED, "gEHENSTEIGE"}, // Gremlins C64
    { SPANISH, "ANDAENTRAVAN" },
    { FIVE_LETTER_UNCOMPRESSED, "*CROSS*RUN\0\0" } // Claymorgue
};


int FindCode(char *x, int base)
{
    unsigned const char *p = entire_file + base;
    int len = strlen(x);
    if (len < 7) len = 7;
    while(p < entire_file + file_length - len) {
        if(memcmp(p, x, len) == 0) {
            return p - entire_file;
        }
        p++;
    }
    return -1;
}

dictionary_type getId(size_t *offset) {
    for (int i = 0; i < 8; i++) {
        *offset = FindCode(dictKeys[i].signature , 0);
        if (*offset != -1) {
            if (i == 4 || i == 5) // GERMAN
                *offset -= 5;
            else if (i == 6) // SPANISH
                *offset -= 8;
            else if (i == 7) // Claymorgue
                *offset -= 11;
            return dictKeys[i].dict;
        }
    }

    return NOT_A_GAME;
}

void read_header(uint8_t *ptr) {
    int i,value;
    for (i = 0; i < 24; i++) {
        value = *ptr + 256 * *(ptr + 1);
        header[i] = value;
        ptr += 2;
    }
}

int sanity_check_header(void) {
    int16_t v = GameHeader.NumItems;
    if (v < 10 || v > 500)
        return 0;
    v = GameHeader.NumActions;
    if (v < 100 || v > 500)
        return 0;
    v = GameHeader.NumWords; // word pairs
    if (v < 50 || v > 190)
        return 0;
    v = GameHeader.NumRooms;  // Number of rooms
    if (v < 10 || v > 100)
        return 0;

    return 1;
}

uint8_t *read_dictionary(struct GameInfo info, uint8_t **pointer, int loud) {
    uint8_t *ptr = *pointer;
    char dictword[info.word_length + 2];
    char c = 0;
    int wordnum = 0;
    int charindex = 0;

    int nv = info.number_of_verbs;
    int nn = info.number_of_nouns;

    do {
        for (int i = 0; i < info.word_length; i++)
        {
            c = *(ptr++);

            if (info.dictionary == FOUR_LETTER_COMPRESSED || info.dictionary == FIVE_LETTER_COMPRESSED) {
                if (charindex == 0) {
                    if (c >= 'a')
                    {
                        c = toupper(c);
                    } else if (c != '.' && c != 0) {
                        dictword[charindex++] = '*';
                    }
                }
                dictword[charindex++] = c;
            } else if (info.subtype == LOCALIZED) {
                if (charindex == 0) {
                    if (c & 0x80)
                    {
                        c = c & 0x7f;
                    } else if (c != '.') {
                        dictword[charindex++] = '*';
                    }
                }
                dictword[charindex++] = c;
            } else {
                if (c == 0)
                {
                    if (charindex == 0)
                    {
                        c=*(ptr++);
                    }
                }
                dictword[charindex] = c;
                if (c == '*')
                    i--;
                charindex++;
            }
        }
        dictword[charindex] = 0;
        if (wordnum < nv)
        {
            Verbs[wordnum] = MemAlloc(charindex+1);
            memcpy((char *)Verbs[wordnum], dictword, charindex+1);
            if (loud)
                fprintf(stderr, "Verb %d: \"%s\"\n", wordnum, Verbs[wordnum]);
        } else {
            Nouns[wordnum - nv] = MemAlloc(charindex+1);
            memcpy((char *)Nouns[wordnum - nv], dictword, charindex+1);
            if (loud)
                fprintf(stderr, "Noun %d: \"%s\"\n", wordnum - nv, Nouns[wordnum - nv]);
        }
        wordnum++;

        if (c != 0 && !isascii(c))
            return ptr;
        
        charindex = 0;
    } while (wordnum <= nv + nn);

    int nw = GameHeader.NumWords;

    for (int i = 0; i <= MAX(nn, nw) - nv; i++) {
        Verbs[nv + i] = ".\0";
    }

    for (int i = 0; i <= MAX(nn, nw) - nn; i++) {
        Nouns[nn + i] = ".\0";
    }

    return ptr;
}

uint8_t *seek_to_pos(uint8_t *buf, int offset) {
    if (offset > file_length)
        return 0;
    return buf + offset;
}

int seek_if_needed(int expected_start, int *offset, uint8_t **ptr) {
    if (expected_start != FOLLOWS) {
        *offset = expected_start + file_baseline_offset;
        uint8_t *ptrbefore = *ptr;
        *ptr = seek_to_pos(entire_file, *offset);
        if (*ptr == ptrbefore)
            fprintf(stderr, "Seek unnecessary, could have been set to FOLLOWS.\n");
        if (*ptr == 0)
            return 0;
    }
    return 1;
}

int parse_header(int *h, header_type type, int *ni, int *na, int *nw, int *nr, int *mc, int *pr, int *tr, int *wl, int *lt, int *mn, int *trm) {
    switch (type) {
        case NO_HEADER:
            return 0;
        case EARLY:
            *ni = h[1];
            *na = h[2];
            *nw = h[3];
            *nr = h[4];
            *mc = h[5];
            *pr = h[6];
            *tr = h[7];
            *wl = h[8];
            *lt = h[9];
            *mn = h[10];
            *trm = h[11];
            break;
        case LATE:
            *ni = h[1];
            *na = h[2];
            *nw = h[3];
            *nr = h[4];
            *mc = h[5];
            *pr = 1;
            *tr = 0;
            *wl = h[6];
            *lt = -1;
            *mn = h[7];
            *trm = 0;
            break;
        case HULK_HEADER:
            *ni = h[3];
            *na = h[2];
            *nw = h[1];
            *nr = h[5];
            *mc = h[6];
            *pr = h[7];
            *tr = h[8];
            *wl = h[0];
            *lt = h[9];
            *mn = h[4];
            *trm = h[10];
            break;
        case SAVAGE_ISLAND_C64_HEADER:
            *ni = h[1];
            *na = h[2];
            *nw = h[3];
            *nr = h[4];
            *mc = h[5];
            *pr = h[6];
            *tr = 0;
            *wl = h[8];
            *lt = -1;
            *mn = h[10];
            *trm = 0;
            break;
        case ROBIN_C64_HEADER:
            *ni = h[1];
            *na = h[2];
            *nw = h[6];
            *nr = h[4];
            *mc = h[5];
            *pr = 1;
            *tr = 0;
            *wl = h[7];
            *lt = -1;
            *mn = h[3];
            *trm = 0;
            break;
        case GREMLINS_C64_HEADER:
            *ni = h[1];
            *na = h[2];
            *nw = h[5];
            *nr = h[3];
            *mc = h[6];
            *pr = h[8];
            *tr = 0;
            *wl = h[7];
            *lt = -1;
            *mn = 98;
            *trm = 0;
            break;
        case SUPERGRAN_C64_HEADER:
            *ni = h[3];
            *na = h[1];
            *nw = h[2];
            *nr = h[4];
            *mc = h[8];
            *pr = 1;
            *tr = 0;
            *wl = h[6];
            *lt = -1;
            *mn = h[5];
            *trm = 0;
            break;
        case SEAS_OF_BLOOD_C64_HEADER:
            *ni = h[0];
            *na = h[1];
            *nw = 134;
            *nr = h[3];
            *mc = h[4];
            *pr = 1;
            *tr = 0;
            *wl = h[6];
            *lt = -1;
            *mn = h[2];
            *trm = 0;
            break;
        default:
            fprintf(stderr, "Unhandled header type!\n");
            return 0;
            break;
    }
    return 1;
}

void print_header_info(int *h, int ni, int na, int nw, int nr, int mc, int pr, int tr, int wl, int lt, int mn, int trm) {
    uint16_t value;
    for (int i = 0; i < 13; i++) {
        value=h[i];
        fprintf(stderr, "b $%X %d: ", 0x494d + 0x3FE5 + i * 2, i);
        fprintf(stderr, "\t%d\n",value);
    }

    fprintf(stderr, "Number of items =\t%d\n", ni);
    fprintf(stderr, "Number of actions =\t%d\n", na);
    fprintf(stderr, "Number of words =\t%d\n",nw);
    fprintf(stderr, "Number of rooms =\t%d\n",nr);
    fprintf(stderr, "Max carried items =\t%d\n",mc);
    fprintf(stderr, "Word length =\t%d\n",wl);
    fprintf(stderr, "Number of messages =\t%d\n",mn);
    fprintf(stderr, "Player start location: %d\n", pr);
    fprintf(stderr, "Treasure room: %d\n", tr);
    fprintf(stderr, "Lightsource time left: %d\n", lt);
    fprintf(stderr, "Number of treasures: %d\n", tr);
}

int try_loading_old(struct GameInfo info, int dict_start) {

    int ni,na,nw,nr,mc,pr,tr,wl,lt,mn,trm;
    int ct;

    Action *ap;
    Room *rp;
    Item *ip;
    /* Load the header */

    uint8_t *ptr = entire_file;
    file_baseline_offset = dict_start - info.start_of_dictionary;

    int offset = info.start_of_header + file_baseline_offset;

    ptr = seek_to_pos(entire_file, offset);
    if (ptr == 0)
        return 0;

    read_header(ptr);

    if(!parse_header(header, info.header_style, &ni, &na, &nw, &nr, &mc, &pr, &tr, &wl, &lt, &mn, &trm))
        return 0;

    if (ni != info.number_of_items || na != info.number_of_actions || nw != info.number_of_words || nr != info.number_of_rooms || mc != info.max_carried) {
        fprintf(stderr, "Non-matching header\n");
        return 0;
    }

    GameHeader.NumItems=ni;
    Items=(Item *)MemAlloc(sizeof(Item)*(ni+1));
    GameHeader.NumActions=na;
    Actions=(Action *)MemAlloc(sizeof(Action)*(na+1));
    GameHeader.NumWords=nw;
    GameHeader.WordLength=wl;
    Verbs=MemAlloc(sizeof(char *)*(nw+2));
    Nouns=MemAlloc(sizeof(char *)*(nw+2));
    GameHeader.NumRooms=nr;
    Rooms=(Room *)MemAlloc(sizeof(Room)*(nr+1));
    GameHeader.MaxCarry=mc;
    GameHeader.PlayerRoom=pr;
    GameHeader.Treasures=tr;
    GameHeader.LightTime=lt;
    LightRefill=lt;
    GameHeader.NumMessages=mn;
    Messages=MemAlloc(sizeof(char *)*(mn+1));
    GameHeader.TreasureRoom=trm;

#pragma mark actions

    if (seek_if_needed(info.start_of_actions, &offset, &ptr) == 0)
        return 0;

    ct=0;

    ap=Actions;

    uint16_t value, cond, comm;

    while(ct<na+1)
    {
        value = *(ptr++);
        value += *(ptr++) * 0x100; /* verb/noun */
        ap->Vocab = value;


        cond = 5;
        comm = 2;

        for (int j = 0; j < 5; j++)
        {
            if (j < cond) {
                value = *(ptr++);
                value += *(ptr++) * 0x100;
            } else value = 0;
            ap->Condition[j] = value;
        }
        for (int j = 0; j < 2; j++)
        {
            if (j < comm) {
                value = *(ptr++) ;
                value += *(ptr++) * 0x100;
            } else value = 0;
            ap->Action[j] = value;
        }
        ap++;
        ct++;
    }

#pragma mark room connections

    if (seek_if_needed(info.start_of_room_connections, &offset, &ptr) == 0)
        return 0;

    ct=0;
    rp=Rooms;

    while(ct<nr+1)
    {
        for (int j= 0; j < 6; j++) {
            rp->Exits[j] = *(ptr++);
        }
        ct++;
        rp++;
    }

#pragma mark item locations

    if (seek_if_needed(info.start_of_item_locations, &offset, &ptr) == 0)
        return 0;
    
    ct=0;
    ip=Items;
    while(ct<ni+1) {
        ip->Location=*(ptr++);
        ip->InitialLoc=ip->Location;
        ip++;
        ct++;
    }

#pragma mark dictionary

    if (seek_if_needed(info.start_of_dictionary, &offset, &ptr) == 0)
        return 0;

    ptr = read_dictionary(info, &ptr, 0);

#pragma mark rooms

    if (seek_if_needed(info.start_of_room_descriptions, &offset, &ptr) == 0)
        return 0;

    if (info.start_of_room_descriptions == FOLLOWS)
        ptr++;

    ct=0;
    rp=Rooms;

    char text[256];
    char c=0;
    int charindex = 0;

    do {
        c = *(ptr++);
        text[charindex] = c;
        if (c == 0) {
            rp->Text = MemAlloc(charindex + 1);
            strcpy(rp->Text, text);
            rp->Image = 255;
            ct++;
            rp++;
            charindex = 0;
        } else {
            charindex++;
        }
        if (c != 0 && !isascii(c))
            return 0;
    } while (ct<nr+1);

#pragma mark messages

    if (seek_if_needed(info.start_of_messages, &offset, &ptr) == 0)
        return 0;

    ct=0;
    charindex = 0;

    while(ct<mn+1)
    {
        c = *(ptr++);
        text[charindex] = c;
        if (c == 0) {
            Messages[ct] = MemAlloc(charindex + 1);
            strcpy((char *)Messages[ct], text);
            ct++;
            charindex = 0;
        } else {
            charindex++;
        }
    }

#pragma mark items

    if (seek_if_needed(info.start_of_item_descriptions, &offset, &ptr) == 0)
        return 0;

    ct=0;
    ip=Items;
    charindex = 0;


    do {
        c = *(ptr++);
        text[charindex] = c;
        if (c == 0) {
            ip->Text = MemAlloc(charindex + 1);
            strcpy(ip->Text, text);
            ip->AutoGet=strchr(ip->Text,'/');
            /* Some games use // to mean no auto get/drop word! */
            if(ip->AutoGet && strcmp(ip->AutoGet,"//") && strcmp(ip->AutoGet,"/*"))
            {
                char *t;
                *ip->AutoGet++=0;
                t=strchr(ip->AutoGet,'/');
                if(t!=NULL)
                    *t=0;
            }
            ct++;
            ip++;
            charindex = 0;
        } else {
            charindex++;
        }
    } while(ct<ni+1);

#pragma mark System messages

    ct=0;
    if (seek_if_needed(info.start_of_system_messages, &offset, &ptr) == 0)
        return 0;

    charindex = 0;

    do {
        c=*(ptr++);
        text[charindex] = c;
        if (c == 0 || c == 0x0d) {
            if (charindex > 0) {
                if (c == 0x0d)
                    charindex++;
                system_messages[ct] = MemAlloc(charindex + 1);
                strncpy(system_messages[ct], text, charindex + 1);
                system_messages[ct][charindex] = '\0';
                ct++;
                charindex = 0;
            }
        } else {
            charindex++;
        }

        if (c != 0 && c != 0x0d && c != '\x83' && !isascii(c))
            break;
    } while (ct<40);

    charindex = 0;

    if (seek_if_needed(info.start_of_directions, &offset, &ptr) == 0)
        return 0;

    ct = 0;
    do {
        c=*(ptr++);
        text[charindex] = c;
        if (c == 0 || c == 0x0d) {
            if (charindex > 0) {
                sys[ct] = MemAlloc(charindex + 2);
                strcpy(sys[ct], text);
                if (c == 0x0d)
                    charindex++;
                sys[ct][charindex] = '\0';
                ct++;
                charindex = 0;
            }
        } else {
            charindex++;
        }

        if (c != 0 && c != 0x0d && !isascii(c))
            break;
    } while (ct<6);

    return 1;
}


int try_loading(struct GameInfo info, int dict_start, int loud) {

    if (info.type == TEXT_ONLY)
        return try_loading_old(info, dict_start);
    
    int ni,na,nw,nr,mc,pr,tr,wl,lt,mn,trm;
    int ct;

    Action *ap;
    Room *rp;
    Item *ip;
    /* Load the header */

    uint8_t *ptr = entire_file;

    if (loud) {
        fprintf(stderr, "dict_start:%x\n", dict_start);
        fprintf(stderr, " info.start_of_dictionary:%x\n",  info.start_of_dictionary);
    }
    file_baseline_offset = dict_start - info.start_of_dictionary;

    if (loud)
        fprintf(stderr, "file_baseline_offset:%x (%d)\n", file_baseline_offset, file_baseline_offset);

    int offset = info.start_of_header + file_baseline_offset;

    ptr = seek_to_pos(entire_file, offset);
    if (ptr == 0)
        return 0;

    read_header(ptr);

    if (!parse_header(header, info.header_style, &ni, &na, &nw, &nr, &mc, &pr, &tr, &wl, &lt, &mn, &trm))
        return 0;

    if (loud)
        print_header_info(header, ni, na, nw, nr, mc, pr, tr, wl, lt, mn, trm);

    GameHeader.NumItems=ni;
    GameHeader.NumActions=na;
    GameHeader.NumWords=nw;
    GameHeader.WordLength=wl;
    GameHeader.NumRooms=nr;
    GameHeader.MaxCarry=mc;
    GameHeader.PlayerRoom=pr;
    GameHeader.Treasures=tr;
    GameHeader.LightTime=lt;
    LightRefill=lt;
    GameHeader.NumMessages=mn;
    GameHeader.TreasureRoom=trm;

    if (sanity_check_header() == 0) {
        return 0;
    }

    if (loud) {
        fprintf(stderr, "Found a valid header at position 0x%x\n", offset);
        fprintf(stderr, "Expected: 0x%x\n", info.start_of_header + file_baseline_offset);
    }

    print_header_info(header, ni, na, nw, nr, mc, pr, tr, wl, lt, mn, trm);

    if (ni != info.number_of_items || na != info.number_of_actions || nw != info.number_of_words || nr != info.number_of_rooms || mc != info.max_carried) {
        fprintf(stderr, "Non-matching header\n");
        return 0;
    }

    if (info.gameID == SAVAGE_ISLAND2)
        GameHeader.PlayerRoom = 30;

    Items=(Item *)MemAlloc(sizeof(Item)*(ni+1));
    Actions=(Action *)MemAlloc(sizeof(Action)*(na+1));
    Verbs=MemAlloc(sizeof(char *)*(nw+2));
    Nouns=MemAlloc(sizeof(char *)*(nw+2));
    Rooms=(Room *)MemAlloc(sizeof(Room)*(nr+1));
    Messages=MemAlloc(sizeof(char *)*(mn+1));

    int compressed = (info.dictionary == FOUR_LETTER_COMPRESSED);

#pragma mark room images

    if (info.start_of_room_image_list != 0) {
        if (seek_if_needed(info.start_of_room_image_list, &offset, &ptr) == 0)
            return 0;

        rp=Rooms;

        for (ct = 0; ct <= GameHeader.NumRooms; ct++) {
            rp->Image = *(ptr++);
            rp++;
        }
    }
#pragma mark Item flags

    if (info.start_of_item_flags != 0) {

        if (seek_if_needed(info.start_of_item_flags, &offset, &ptr) == 0)
            return 0;

        ip=Items;

        for (ct = 0; ct <= GameHeader.NumItems; ct++) {
            ip->Flag = *(ptr++);
            ip++;
        }
    }

#pragma mark item images

    if (info.start_of_item_image_list != 0) {

        if (seek_if_needed(info.start_of_item_image_list, &offset, &ptr) == 0)
            return 0;

        ip=Items;

        for (ct = 0; ct <= GameHeader.NumItems; ct++) {
            ip->Image = *(ptr++);
            ip++;
        }
    }

    if (loud)
        fprintf(stderr, "Offset after reading item images: %lx\n", ptr - entire_file - file_baseline_offset);

#pragma mark actions

    if (seek_if_needed(info.start_of_actions, &offset, &ptr) == 0)
        return 0;

    ct=0;

    ap=Actions;

    uint16_t value, cond, comm;

    while(ct<na+1)
    {
        value = *(ptr++);
        value += *(ptr++) * 0x100; /* verb/noun */
        ap->Vocab = value;

        if (info.actions_style == COMPRESSED) {
            value = *(ptr++); /* count of actions/conditions */
            cond = value & 0x1f;
            if (cond > 5) {
                fprintf(stderr, "Condition error at action %d!\n", ct);
                cond = 5;
            }
            comm = (value & 0xe0) >> 5;
            if (comm > 2) {
                fprintf(stderr, "Command error at action %d!\n", ct);
                comm = 2;
            }
        } else {
            cond = 5;
            comm = 2;
        }
        for (int j = 0; j < 5; j++)
        {
            if (j < cond) {
                value = *(ptr++);
                value += *(ptr++) * 0x100;
            } else value = 0;
            ap->Condition[j] = value;
        }
        for (int j = 0; j < 2; j++)
        {
            if (j < comm) {
                value = *(ptr++) ;
                value += *(ptr++) * 0x100;
            } else value = 0;
            ap->Action[j] = value;
        }

        ap++;
        ct++;
    }

#pragma mark dictionary

    if (seek_if_needed(info.start_of_dictionary, &offset, &ptr) == 0)
        return 0;

    ptr = read_dictionary(info, &ptr, loud);

    if (loud)
        fprintf(stderr, "Offset after reading dictionary: %lx\n", ptr - entire_file - file_baseline_offset);

#pragma mark rooms

    if (info.start_of_room_descriptions != 0) {
        if (seek_if_needed(info.start_of_room_descriptions, &offset, &ptr) == 0)
            return 0;

        ct=0;
        rp=Rooms;

        char text[256];
        char c=0;
        int charindex = 0;

        if (!compressed) {
            do {
                c = *(ptr++);
                text[charindex] = c;
                if (c == 0) {
                    rp->Text = MemAlloc(charindex + 1);
                    strcpy(rp->Text, text);
                    if (loud)
                        fprintf(stderr, "Room %d: %s\n", ct, rp->Text);
                    ct++;
                    rp++;
                    charindex = 0;
                } else {
                    charindex++;
                }
                if (c != 0 && !isascii(c))
                    return 0;
            } while (ct<nr+1);
        } else {
            do {
                rp->Text = decompress_text(ptr, ct);
                if (rp->Text == NULL)
                    return 0;
                *(rp->Text) = tolower(*(rp->Text));
                ct++;
                rp++;
            } while (ct<nr);
        }
    }

#pragma mark room connections

    if (seek_if_needed(info.start_of_room_connections, &offset, &ptr) == 0)
        return 0;

    ct=0;
    rp=Rooms;

    while(ct<nr+1)
    {
        for (int j= 0; j < 6; j++) {
            rp->Exits[j] = *(ptr++);
        }

        ct++;
        rp++;
    }

#pragma mark messages

    if (seek_if_needed(info.start_of_messages, &offset, &ptr) == 0)
        return 0;

    ct=0;
    char text[256];
    char c=0;
    int charindex = 0;

    if (compressed) {
        while(ct<mn+1) {
            Messages[ct] = decompress_text(ptr, ct);
            if (Messages[ct] == NULL)
                return 0;
            ct++;
        }
    } else {
        while(ct<mn+1) {
            c = *(ptr++);
            text[charindex] = c;
            if (c == 0) {
                Messages[ct] = MemAlloc(charindex + 1);
                strcpy((char *)Messages[ct], text);
                ct++;
                charindex = 0;
            } else {
                charindex++;
            }
        }
    }

#pragma mark items

    if (seek_if_needed(info.start_of_item_descriptions, &offset, &ptr) == 0)
        return 0;

    ct=0;
    ip=Items;
    charindex = 0;

    if (compressed) {
        do {
            ip->Text = decompress_text(ptr, ct);
            ip->AutoGet = NULL;
            if (ip->Text != NULL && ip->Text[0] != '.') {
                if (loud)
                    fprintf(stderr, "Item %d: %s\n", ct, ip->Text);
                ip->AutoGet=strchr(ip->Text,'.');
                if (ip->AutoGet) {
                    char *t;
                    *ip->AutoGet++=0;
                    ip->AutoGet++;
                    t=strchr(ip->AutoGet,'.');
                    if(t!=NULL)
                        *t=0;
                    for (int i = 1; i < GameHeader.WordLength; i++)
                        *(ip->AutoGet+i) = toupper(*(ip->AutoGet+i));
                }
            }
            ct++;
            ip++;
        } while(ct<ni+1);
    } else {
        do {
            c = *(ptr++);
            text[charindex] = c;
            if (c == 0) {
                ip->Text = MemAlloc(charindex + 1);
                strcpy(ip->Text, text);
                if (loud)
                    fprintf(stderr, "Item %d: %s\n", ct, ip->Text);
                ip->AutoGet=strchr(ip->Text,'/');
                /* Some games use // to mean no auto get/drop word! */
                if(ip->AutoGet && strcmp(ip->AutoGet,"//") && strcmp(ip->AutoGet,"/*")) {
                    char *t;
                    *ip->AutoGet++=0;
                    t=strchr(ip->AutoGet,'/');
                    if(t!=NULL)
                        *t=0;
                }
                ct++;
                ip++;
                charindex = 0;
            } else {
                charindex++;
            }
        } while(ct<ni+1);
    }

#pragma mark item locations

    if (seek_if_needed(info.start_of_item_locations, &offset, &ptr) == 0)
        return 0;

    ct=0;
    ip=Items;
    while(ct<ni+1)
    {
        ip->Location=*(ptr++);
        ip->InitialLoc=ip->Location;
        ip++;
        ct++;
    }

#pragma mark System messages

    if (seek_if_needed(info.start_of_system_messages, &offset, &ptr) == 0)
        return 1;

jumpSysMess:
    ptr = seek_to_pos(entire_file, offset);

    ct=0;
    charindex = 0;

    do {
        c=*(ptr++);
        if (ptr - entire_file > file_length || ptr < entire_file) {
            fprintf(stderr, "Read out of bounds!\n");
            return 0;
        }
        if (charindex > 255)
            charindex = 0;
        text[charindex] = c;
        if (c == 0 || c == 0x0d) {
            if (charindex > 0) {
                if (c == 0x0d)
                    charindex++;
                if (ct == 0 && (info.subtype & (C64 | ENGLISH) ) == (C64 | ENGLISH) && memcmp(text, "NORTH", 5) != 0) {
                    offset--;
                    goto jumpSysMess;
                }
                system_messages[ct] = MemAlloc(charindex + 1);
                strncpy(system_messages[ct], text, charindex + 1);
                system_messages[ct][charindex] = '\0';
                if (loud)
                    fprintf(stderr, "system_messages %d: \"%s\"\n", ct, system_messages[ct]);
                ct++;
                charindex = 0;
            }
        } else {
            charindex++;
        }
    } while (ct < 45);

    if (loud)
        fprintf(stderr, "Offset after reading system messages: %lx\n", ptr - entire_file);

    if ((info.subtype & (C64 | ENGLISH)) == (C64 | ENGLISH)) {
        return 1;
    }

    if (seek_if_needed(info.start_of_directions, &offset, &ptr) == 0)
        return 0;

    charindex = 0;

    ct = 0;
    do {
        c=*(ptr++);
        text[charindex] = c;
        if (c == 0 || c == 0x0d) {
            if (charindex > 0) {
                sys[ct] = MemAlloc(charindex + 2);
                strcpy(sys[ct], text);
                if (c == 0x0d)
                    charindex++;
                sys[ct][charindex] = '\0';
                ct++;
                charindex = 0;
            }
        } else {
            charindex++;
        }

        if (c != 0 && c != 0x0d && !isascii(c))
            break;
    } while (ct<6);

    return 1;
}

GameIDType detect_game(const char *file_name) {

     FILE *f = fopen(file_name, "r");
        if(f==NULL)
            Fatal("Cannot open game");

    for (int i = 0; i < NUMBER_OF_DIRECTIONS; i++)
        Directions[i] = EnglishDirections[i];
    for (int i = 0; i < NUMBER_OF_SKIPPABLE_WORDS; i++)
        SkipList[i] = EnglishSkipList[i];
    for (int i = 0; i < NUMBER_OF_DELIMITERS; i++)
        DelimiterList[i] = EnglishDelimiterList[i];
    for (int i = 0; i < NUMBER_OF_EXTRA_NOUNS; i++)
        ExtraNouns[i] = EnglishExtraNouns[i];

    // Check if the original ScottFree LoadDatabase() function can read the file.
    if (LoadDatabase(f, 1)) {
        fclose(f);
        GameInfo = MemAlloc(sizeof(GameInfo));
        GameInfo->gameID = SCOTTFREE;
        return SCOTTFREE;
    }

    file_length = GetFileLength(f);

    if (file_length > MAX_GAMEFILE_SIZE)  {
        fprintf(stderr, "File too large to be a vaild game file (%zu, max is %d)\n", file_length, MAX_GAMEFILE_SIZE);
        return 0;
    }

    entire_file = MemAlloc(file_length);
    fseek(f, 0, SEEK_SET);
    size_t result = fread(entire_file, 1, file_length, f);
    fclose(f);
    if (result == 0)
        Fatal("File empty or read error!");

    GameIDType TI994A_id = UNKNOWN_GAME;

        TI994A_id = DetectTI994A(&entire_file, &file_length);
        if (TI994A_id) {
            GameInfo = MemAlloc(sizeof(GameInfo));
            GameInfo->gameID = SCOTTFREE;
            return SCOTTFREE;
        }


    if (!TI994A_id) {
        size_t offset;

        dictionary_type dict_type = getId(&offset);

        if (dict_type == NOT_A_GAME)
            return UNKNOWN_GAME;

        for (int i = 0; i < NUMGAMES; i++) {
            if (games[i].dictionary == dict_type) {
//                fprintf(stderr, "The game might be %s\n", games[i].Title);
                if (try_loading(games[i], offset, 0)) {
                    GameInfo = &games[i];
                    break;
                }
//                else
//                    fprintf(stderr, "It was not.\n");
            }
        }

        if (GameInfo == NULL)
            return 0;
    }

    /* Copy ZX Spectrum style system messages as base */
    for (int i = 6; i < MAX_SYSMESS && sysdict_zx[i] != NULL; i++) {
        sys[i] = (char *)sysdict_zx[i];
    }

    return CurrentGame;
}
