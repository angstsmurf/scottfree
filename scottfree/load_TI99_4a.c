//
//  load_TI99_4a.c
//  scott
//
//  Created by Administrator on 2022-02-12.
//

#include <string.h>
#include <assert.h>
#include <ctype.h>
#include <stdlib.h>

#include "load_TI99_4a.h"

#include "detectgame.h"
#include "scott.h"


#define PACKED __attribute__((__packed__))

struct DATAHEADER {
    uint8_t    num_objects           PACKED;        /* number of objects */
    uint8_t    num_verbs             PACKED;        /* number of verbs */
    uint8_t    num_nouns             PACKED;        /* number of nouns */
    uint8_t    red_room              PACKED;        /* the red room (dead room) */
    uint8_t    max_items_carried     PACKED;        /* max number of items can be carried */
    uint8_t    begin_locn            PACKED;        /* room to start in */
    uint8_t    num_treasures         PACKED;        /* number of treasures */
    uint8_t    cmd_length            PACKED;        /* number of letters in commands */
    uint16_t    light_turns          PACKED;        /* max number of turns light lasts */
    uint8_t    treasure_locn         PACKED;        /* location of where to store treasures */
    uint8_t    strange               PACKED;        /* !?! not known. */

    uint16_t    p_obj_table          PACKED;        /* pointer to object table */
    uint16_t    p_orig_items         PACKED;        /* pointer to original items */
    uint16_t    p_obj_link           PACKED;        /* pointer to link table from noun to object */
    uint16_t    p_obj_descr          PACKED;        /* pointer to object descriptions */
    uint16_t    p_message            PACKED;        /* pointer to message pointers */
    uint16_t    p_room_exit          PACKED;        /* pointer to room exits table */
    uint16_t    p_room_descr         PACKED;        /* pointer to room descr table */

    uint16_t    p_noun_table         PACKED;        /* pointer to noun table */
    uint16_t    p_verb_table         PACKED;        /* pointer to verb table */

    uint16_t    p_explicit           PACKED;        /* pointer to explicit action table */
    uint16_t    p_implicit           PACKED;        /* pointer to implicit actions */
};

int max_messages;

int max_item_descr;


uint16_t fix_address(uint16_t ina)
{
    return(ina - 0x380 + file_baseline_offset);
}

uint16_t fix_word(uint16_t word)
{
#ifdef WE_ARE_BIG_ENDIAN
    return word;
#else
    return (  ((word&0xFF)<<8) | ((word>>8)&0xFF) );
#endif
}

uint16_t get_word(uint8_t *mem)
{
    uint16_t x;

#ifdef WE_ARE_BIG_ENDIAN
    x=*(uint16_t *)mem;
#else
    x=(*(mem+0)<<8);
    x+=(*(mem+1)&0xFF);

    x=*(uint16_t *)mem;
#endif
    return fix_word(x);
}

void get_max_messages(struct DATAHEADER dh)
{
    uint8_t *msg;
    uint16_t msg1;

    msg = (uint8_t *)entire_file+fix_address(fix_word(dh.p_message));
    msg1 = fix_address(get_word((uint8_t *)msg));
    max_messages = (msg1 - fix_address(fix_word(dh.p_message)))/2;
}

void get_max_items(struct DATAHEADER dh)
{
    uint8_t *msg;
    uint16_t msg1;

    msg = (uint8_t *)entire_file+fix_address(fix_word(dh.p_obj_descr));
    msg1 = fix_address(get_word((uint8_t *)msg));
    max_item_descr = (msg1 - fix_address(fix_word(dh.p_obj_descr)))/2;
}

void print_ti99_header_info(struct DATAHEADER header) {
    fprintf(stderr, "Number of items =\t%d\n", header.num_objects);
    fprintf(stderr, "Number of verbs =\t%d\n",header.num_verbs);
    fprintf(stderr, "Number of nouns =\t%d\n",header.num_nouns);
    fprintf(stderr, "Deadroom =\t%d\n",header.red_room);
    fprintf(stderr, "Max carried items =\t%d\n",header.max_items_carried);
    fprintf(stderr, "Player start location: %d\n", header.begin_locn);
    fprintf(stderr, "Word length =\t%d\n",header.cmd_length);
    fprintf(stderr, "Treasure room: %d\n", header.treasure_locn);
    fprintf(stderr, "Lightsource time left: %d\n", fix_word(header.light_turns));
    fprintf(stderr, "Unknown: %d\n", header.strange);
}

int try_loading_ti994a(struct DATAHEADER dh, int loud);

GameIDType DetectTI994A(uint8_t **sf, size_t *extent) {

    int offset = FindCode("\xe2\x31\x10\x18\x0c\x07\x03\x00\x4c\x90\x20\x40" , 0);
    if (offset == -1)
        return UNKNOWN_GAME;
    else
        fprintf(stderr, "This is a TI99/4A game.\n");

    file_baseline_offset = offset - 0x4c0;
    fprintf(stderr, "file_baseline_offset:%d\n", file_baseline_offset);

    struct DATAHEADER     dh;
    memcpy(&dh, entire_file + 0x8a0 + file_baseline_offset, sizeof(struct DATAHEADER));

    print_ti99_header_info(dh);

    get_max_messages(dh);
    fprintf(stderr, "Maximum number of messages: %d.\n", max_messages);

    get_max_items(dh);
    fprintf(stderr, "Maximum number of item descriptions: %d.\n", max_item_descr);

    /* test some pointers for valid game... */

    assert(file_length >= fix_address(fix_word(dh.p_obj_table)));
    assert(file_length >= fix_address(fix_word(dh.p_orig_items)));
    assert(file_length >= fix_address(fix_word(dh.p_obj_link)));
    assert(file_length >= fix_address(fix_word(dh.p_obj_descr)));
    assert(file_length >= fix_address(fix_word(dh.p_message)));
    assert(file_length >= fix_address(fix_word(dh.p_room_exit)));
    assert(file_length >= fix_address(fix_word(dh.p_room_descr)));
    assert(file_length >= fix_address(fix_word(dh.p_noun_table)));
    assert(file_length >= fix_address(fix_word(dh.p_verb_table)));
    assert(file_length >= fix_address(fix_word(dh.p_explicit)));
    assert(file_length >= fix_address(fix_word(dh.p_implicit)));

    return try_loading_ti994a(dh, 1);
}

uint8_t *get_TI994A_word(uint8_t *string, uint8_t **result, size_t *length)
{
    uint8_t *msg;

    msg=string;
    *length=msg[0];
    msg++;
    *result = MemAlloc(*length);

    strncpy((char*)*result, (char*)msg, *length);
    *(*result + *length) = 0;

    msg += *length;

    return(msg);
}

char *get_TI994A_string(uint16_t table, int table_offset)
{
    uint8_t *msgx, *msgy, *nextword;
    char *result;
    uint16_t msg0, msg1, msg2;
    uint8_t buffer[1024];
    size_t length, total_length = 0;

    uint8_t *game = entire_file + file_baseline_offset;

    msgx=game+fix_address(fix_word(table));

    msg0=fix_address(get_word((uint8_t *)msgx));

    msgx+=table_offset*2;
    msg1=fix_address(get_word((uint8_t *)msgx));
    msg2=fix_address(get_word((uint8_t *)msgx+2));

    msgy=game+msg2;
    msgx=game+msg1;

    while(msgx<msgy)
    {
        msgx=get_TI994A_word(msgx, &nextword, &length);
        if (length > 100) {
            free(nextword);
            return NULL;
        }
        memcpy(buffer + total_length, nextword, length);
        free(nextword);
        total_length += length;
        if (total_length > 1000)
            break;
        if(msgx<msgy)
            buffer[total_length++] = ' ';
    }
    total_length++;
    result = MemAlloc(total_length);
    memcpy(result, buffer, total_length);
    result[total_length - 1] = '\0';
    return result;
}

void load_TI994A_dict(int vorn, uint16_t table, int num_words, const char **dict)
{
    uint16_t *wtable;
    int     i;
    int     word_len;
    char    *w1;
    char    *w2;

    /* table is either verb or noun table */
    wtable = (uint16_t*)(entire_file + fix_address(fix_word(table)));

    i = 0;

    while(i <= num_words)
    {
        do {
            w1 = (char*)entire_file + fix_address(get_word((uint8_t *) wtable+(i*2)));
            w2 = (char*)entire_file + fix_address(get_word((uint8_t *) wtable+((1+i)*2)));
        } while(w1 == w2);

        word_len = w2 - w1;

        dict[i] = MemAlloc(word_len + 1);

        strncpy((char *)dict[i], w1, word_len);

        i += 1;
    }
}

struct Keyword
{
    char     *name;
    int        opcode;
    int        count;
    int tsr80equiv;
    int swapargs;
};

// From Scott2Zip source
/*%action_xlat = (
 0,  '00 0',        # No operation
 52, '0xdb 1',      # Get item (check that player can carry it)
 53, '0xdc 1',      # Drop item
 54, '0xdd 1',      # Move player to the given room
 55, '0xde 1',      # Move item to room 0
 56, '0xdf 0',      # Set darkness flag (flag 15)
 57, '0xe0 0',      # Clear darkness flag (flag 15)
 58, '0xe1 1',      # Set given bit flag
 59, '0xde 1',      # Same as 55???
 60, '0xe2 1',      # Clear given bit flag
 61, '0xe5 0',      # Kill player
 62, '0xe6 2 1',    # Put item in given room
 63, '0xe7 0',      # Game over
 64, '0xf0 0',      # Describe room
 65, '0xe8 0',      # Score
 66, '0xe9 0',      # Inventory
 67, '0xe3 0',      # Set bit flag 0
 68, '0xe4 0',      # Clear bit flag 0
 69, '0xea 0',      # Refill lamp
 70, '0 0',         # Ignore
 71, '0xeb 0',      # Save game
 72, '0xec 2 1',    # Swap the locations of the given items
 73, '0xda 0',      # Continue -- specially handled
 74, '0xed 1',      # Give item to player (no check that it can
 # carried)
 75, '0xee 2 0',    # Put item1 with item2
 76, '0xf1 0',      # Look
 77, '0xf3 0',      # Decrement counter
 78, '0xf4 0',      # Print counter
 79, '0xd5 1',      # Set counter -- translated to home-made
 # opcode which supports 16 bits
 80, '0xf8 0',      # Swap location with saved location 0
 81, '0xfa 1',      # Swap counter with given saved counter
 82, '0xf6 1',      # Add argument to counter
 83, '0xf7 1',      # Subtract argument from counter
 84, '0xfc 0',      # Echo last noun entered without new line
 85, '0xfb 0',      # Echo last noun entered with new line
 86, '0xfd 0',      # New line
 87, '0xf9 1',      # Swap location with given saved location
 88, '0xfe 0',      # Wait two seconds
 89, '0 1',         # Draw picture -- ignore
 );*/

/*%condition_xlat = (
 1,  0xb7,    # Item carried
 2,  0xb8,    # Item in current room
 3,  0xb9,    # Item carried or in current room
 4,  0xbf,    # Player in given room
 5,  0xba,    # Item not in current room
 6,  0xbb,    # Item not carried
 7,  0xc0,    # Player not in given room
 8,  0xc1,    # Given bit flag is set
 9,  0xc2,    # Given bit flag is cleared
 10, 0xc3,    # Something carried (no argument)
 11, 0xc4,    # Nothing carried (no argument)
 12, 0xbc,    # Item not carried nor in current room
 13, 0xbd,    # Item not in room 0
 14, 0xbe,    # Item in room 0
 15, 0xc5,    # Counter <= argument
 16, 0xc6,    # Counter > argument
 17, 0xc8,    # Item in initial room
 18, 0xc9,    # Item not in initial room
 19, 0xc7,    # Counter == argument
 );*/


struct Keyword actions[] =
{
    {"get",         0xDB, 1, 52, 0 },
    {"drop",        0xDC, 1, 53, 0 },
    {"goto",        0xDD, 1, 54, 0 },
    {"zap",         0xDE, 1, 55, 0 },
    {"on",          0xDF, 0, 56, 0 },   /* on dark */
    {"off",         0xE0, 0, 57, 0 },   /* off dark */
    {"on",          0xE1, 1, 58, 0 },   /* set flag */
    {"off",         0xE2, 1, 60, 0 },   /* clear flag */
    {"on",          0xE3, 0, 67, 0 },   /* set flag 0 */
    {"off",         0xE4, 0, 68, 0 },   /* clear flag 0 */
    {"die",         0xE5, 0, 61, 0 },
    {"move",        0xE6, 2, 62, 1 },
    {"quit",        0xE7, 0, 63, 0 },
    {".score",      0xE8, 0, 65, 0 },
    {".inv",        0xE9, 0, 66, 0 },
    {"refill",      0xEA, 0, 69, 0 },
    {"save",        0xEB, 0, 71, 0 },
    {"swap",        0xEC, 2, 72, 0 },    /* swap items */
    {"steal",       0xED, 1, 74, 0 },
    {"same",        0xEE, 2, 75, 0 },
    {"nop",         0xEF, 0, 0 },

    {".room",       0xF0, 0, 76, 0 },


    {"has",         0xB7, 1, 1, 0 },
    {"here",        0xB8, 1, 2, 0 },
    {"avail",       0xB9, 1, 3, 0 },
    {"!here",       0xBA, 1, 5, 0 },
    {"!has",        0xBB, 1, 6, 0 },
    {"!avail",      0xBC, 1, 12, 0 },
    {"exists",      0xBD, 1, 13, 0 },
    {"!exists",     0xBE, 1, 14, 0 },
    {"in",          0xBF, 1, 4, 0 },
    {"!in",         0xC0, 1, 7, 0 },
    {"set",         0xC1, 1, 8, 0 },
    {"!set",        0xC2, 1, 9, 0 },
    {"something",   0xC3, 0, 10, 0 },
    {"nothing",     0xC4, 0, 11, 0 },
    {"le",          0xC5, 1, 15, 0 },
    {"gt",          0xC6, 1, 16, 0 },
    {"eq",          0xC7, 1, 19, 0 },
    {"!moved",      0xC8, 1, 17, 0 },
    {"moved",       0xC9, 1, 18, 0 },

    {"--0xCA--",    0xCA, 0 },
    {"--0xCB--",    0xCB, 0 },
    {"--0xCC--",    0xCC, 0 },
    {"--0xCD--",    0xCD, 0 },
    {"--0xCE--",    0xCE, 0 },
    {"--0xCF--",    0xCF, 0 },
    {"--0xD0--",    0xD0, 0 },
    {"--0xD1--",    0xD1, 0 },
    {"--0xD2--",    0xD2, 0 },
    {"--0xD3--",    0xD3, 0 },

    {"cls",            0xD4, 0, 70 },
    {"pic",            0xD5, 0, 89 },
    {"inv",            0xD6, 0, 66 },
    {"!inv",           0xD7, 0, 0 },
    {"ignore",         0xD8, 0, 0 },
    {"success",        0xD9, 0, 0 },
    {"try",            0xDA, 1, 73 },

    {"--0xF1--",       0xF1, 0 },
    {"add",            0xF2, 1, 82 }, // add 1
    {"sub",            0xF3, 0, 77 },


    {".timer",         0xF4, 0, 78 },
    {"timer",          0xF5, 1, 79 },

    {"add",            0xF6, 1, 82 },
    {"sub",            0xF7, 1, 83 },

    /* TODO : implement Select RV (0xF8) and Swap RV (0xF9) */
    {"select_rv",      0xF8, 0, 80 },
    {"swap_rv",        0xF9, 1, 87 },

    {"swap",           0xFA, 1, 81 },    /* swap flag */

    {".noun",          0xFB, 0, 84 },
    {".noun_nl",       0xFC, 0, 85 },
    {".nl",            0xFD, 0, 86 },
    {"delay",          0xFE, 0, 88 },

    {"",               0xFF, 0, 0}
};

int getEntryIndex(int opcode) {
    for (int i = 0; i < 73; i++) {
        if (actions[i].opcode == opcode)
            return i;
    }
    return -1;
}

void CreateTRS80Action(int verb, int noun, uint16_t *conditions, int numconditions, uint16_t *commands, int numcommands, uint16_t *parameters, int numparameters) {


    GameHeader.NumActions++;

    if (numconditions > 5)
        fprintf(stderr, "Error: %d conditions!\n", numconditions);
    if (numparameters > 5)
        fprintf(stderr, "Error: %d parameters!\n", numparameters);
    if (numcommands > 4)
        fprintf(stderr, "Error: %d commands!\n", numcommands);

    int na = GameHeader.NumActions;

    fprintf(stderr, "\nTRS80 Action %d  with verb: %d noun %d \n", GameHeader.NumActions, verb, noun);

    fprintf(stderr, "Number of conditions: %d\n", numconditions);
    fprintf(stderr, "Number of parameters: %d\n", numparameters);


//    if (Actions) {
//        Actions = realloc(Actions, sizeof(Action) * (GameHeader.NumActions + 1));
//    } else {
//        Actions = MemAlloc(sizeof(Action));
//    }

    Action *action = Actions + na;
    fprintf(stderr, "Creating action %d\n", na);

    action->Vocab = 150 * verb + noun;

    fprintf(stderr, "Vocab 150 * verb (%d) + noun (%d) = %d \n", verb, noun, action->Vocab);

//    5 repeats of condition+20*value

    for (int i = 0; i < numconditions; i++) {
        action->Condition[i] = conditions[i] + 20 * parameters[i];
        fprintf(stderr, "Condition[%d] = %d + 20 * parameter %d = %d\n", i, conditions[i], parameters[i], action->Condition[i]);
    }
    if (numparameters > numconditions) {
        for (int i = numconditions; i < numparameters; i++) {
            action->Condition[i] = 20 * parameters[i];
            fprintf(stderr, "Condition[%d] = 0 + 20 * parameter %d = %d\n", i, parameters[i], action->Condition[i]);
        }
    }

    action->Action[0] = 150 * commands[0] + commands[1];
    fprintf(stderr, "Action[0] = 150 * commands[0] (%d) + commands[1] (%d) = %d\n", commands[0], commands[1], action->Action[0]);

    action->Action[1] = 150 * commands[2] + commands[3];
    fprintf(stderr, "Action[1] = 150 * commands[2] (%d) + commands[3] (%d) = %d\n", commands[2], commands[3], action->Action[1]);
}


void ReadTI99Action(int verb, int noun, uint8_t *ptr, size_t size) {
    fprintf(stderr, "\nAction with verb: %d (%s)\n", verb, Verbs[verb]);
    fprintf(stderr, "%s: %d", verb == 0 ? "chance" : "noun", noun);
    if (GameHeader.NumWords >= noun && verb != 0)
        fprintf(stderr, "(%s)", Nouns[noun]);
    fprintf(stderr, "\n");
    uint16_t conditions[5] = { 0,0,0,0,0 };
    uint16_t commands[4] = { 0,0,0,0 };
    uint16_t parameters[5] = { 0,0,0,0,0 };

    int numconditions = 0;
    int numcommands = 0;
    int numparameters = 0;
    int try_at = 0;


    for (int i = 0; i < size; i++) {
        uint8_t value = ptr[i];
        if (value == 0xff)
            break;
        int index = getEntryIndex(value);
        if (index == -1) {
            if (value < 0xDB) {
                fprintf(stderr, "(%x) Print message %d: \"%s\"\n", value, value, Messages[value]);
                if (value < 51) {
                    commands[numcommands++] = value;
                } else {
                    commands[numcommands++] = 50 + value;
                }
                fprintf(stderr, "Set commands[%d] = %d\n", numcommands - 1, commands[numcommands - 1]);
            } else
                fprintf(stderr, "Error! Invalid opcode at %d! (%x)\n", i, value);
            continue;
        }
        struct Keyword entry = actions[index];
        fprintf(stderr, "%s at %X: %s (%x).", value > 0xc9 ? "action" : "condition",  i, entry.name, value);
        if (numparameters + entry.count > 5) {
            try_at = i;
            commands[numcommands++] = 73;
            fprintf(stderr, "Creating a FAKE continue action!\n");
            break;
        }
        if (value > 0xc9) {
            if (numcommands == 3 && ptr[i + entry.count] != 0xff && i + entry.count < size + 1) {
                try_at = i;
                commands[numcommands++] = 73;
                fprintf(stderr, "Creating a FAKE continue action!\n");
                break;
            }
            commands[numcommands++] = entry.tsr80equiv;
            fprintf(stderr, "Set commands[%d] = %d\n", numcommands - 1, commands[numcommands - 1]);
        } else {
            if (numconditions == 5) {
                try_at = i;
                commands[numcommands++] = 73;
                fprintf(stderr, "Creating a FAKE continue action!\n");
                break;
            }
            conditions[numconditions++] = entry.tsr80equiv;
            fprintf(stderr, "Set conditions[%d] = %d\n", numconditions - 1, conditions[numconditions - 1]);
        }
        if (entry.count) {
            fprintf(stderr, " %d parameter%s: ", entry.count, entry.count > 1 ? "s" : "");
        } else if (value <= 0xc9)
             parameters[numparameters++] = 0;

        for (int j = 0; j < entry.count; j++) {
            if (value == 0xf2) {
                fprintf(stderr, "0x01 (1)");
                parameters[numparameters++] = 1;
            } else {
                fprintf(stderr, "0x%x (%d)", ptr[1+i+j],  ptr[1+i+j]);
                parameters[numparameters++] = ptr[1+i+j];
            }
        }
        if (entry.swapargs) {
            int temp = parameters[numparameters - 1];
            parameters[numparameters - 1] = parameters[numparameters - 2];
            parameters[numparameters - 2] = temp;
        }
        fprintf(stderr, "\n");

//        uint8_t value2 = ptr[i + 1];

        i += entry.count;
        if (value == 0xf2)
            i--;
        if (value == 0xda) { // try
            try_at = i + 1;
            break;
        }
    }

    CreateTRS80Action(verb, noun, conditions, numconditions, commands, numcommands, parameters, numparameters);

    if (try_at) {
        ReadTI99Action(0,0, ptr + try_at, size - try_at);
    }
    fprintf(stderr, "\n\n");
}

void print_action(int index);

void read_implicit(struct DATAHEADER dh)
{
    uint8_t *ptr;
    int loop_flag;

    /* run all act auto codes */

    ptr = entire_file + fix_address(fix_word(dh.p_implicit));
    loop_flag = 0;

    /* fall out, if no auto acts in the game. */
    if(*ptr == 0x0)
        loop_flag = 1;

    while(loop_flag == 0)
    {
        int noun = ptr[0];
        int size = ptr[1];
        fprintf(stderr, "size: %d\n", size);

        ReadTI99Action(0, noun, ptr + 2, size - 2);
        fprintf(stderr, "Created action %d\n", GameHeader.NumActions);
//        print_action(GameHeader.NumActions);

        if(ptr[1] == 0)
            loop_flag = 1;

        /* skip code chunk */
        ptr += 1 + ptr[1];
    }
}

void read_explicit(struct DATAHEADER dh)
{
    uint8_t *p;
    uint16_t addy;
    int flag;
    int i;

    for(i=0; i<=dh.num_verbs; i+=1)
    {
        p = entire_file + fix_address(fix_word(dh.p_explicit));
        addy = get_word(p + ( (i ) *2) );

        if(addy != 0)
        {
            p=entire_file+fix_address(addy);
            flag = 0;

            while(flag != 4)
            {
                if(p[1] == 0)
                    flag = 4;    /* now run get/drop stuff */

                int noun = p[0];
                int size = p[1];
                ReadTI99Action(i, noun, p + 2, size - 2);

                /* go to next block. */
                p += 1 + p[1];
            }
        }
    }
}

void print_action(int ct) {
    Action *ap = &Actions[ct];
    fprintf(stderr, "Action %d Vocab: %d (%d/%d)\n",ct, ap->Vocab, ap->Vocab % 150, ap->Vocab / 150);
    fprintf(stderr, "Action %d Condition[0]: %d (%d/%d)\n",ct, ap->Condition[0], ap->Condition[0] % 20, ap->Condition[0] / 20);
    fprintf(stderr, "Action %d Condition[1]: %d (%d/%d)\n",ct, ap->Condition[1], ap->Condition[1] % 20, ap->Condition[1] / 20);
    fprintf(stderr, "Action %d Condition[2]: %d (%d/%d)\n",ct, ap->Condition[2], ap->Condition[2] % 20, ap->Condition[2] / 20);
    fprintf(stderr, "Action %d Condition[0]: %d (%d/%d)\n",ct, ap->Condition[3], ap->Condition[3] % 20, ap->Condition[3] / 20);
    fprintf(stderr, "Action %d Condition[0]: %d (%d/%d)\n",ct, ap->Condition[4], ap->Condition[4] % 20, ap->Condition[4] / 20);
    fprintf(stderr, "Action %d Action[0]: %d\n",ct, ap->Action[0]);
    fprintf(stderr, "Action %d Action[1]: %d\n\n",ct, ap->Action[1]);
}

void print_actions(void) {
    for (int ct = 0; ct <= GameHeader.NumActions; ct++) {
        print_action(ct);
    }
}

int try_loading_ti994a(struct DATAHEADER dh, int loud) {
    int ni,na,nw,nr,mc,pr,tr,wl,lt,mn,trm;
    int ct;

    Room *rp;
    Item *ip;
    /* Load the header */

    ni = dh.num_objects;
    nw = MAX(dh.num_verbs, dh.num_nouns);
    nr = dh.red_room;
    mc = dh.max_items_carried;
    pr = dh.begin_locn;
    tr = 0;
    trm = dh.treasure_locn;
    wl = dh.cmd_length;
    lt = fix_word(dh.light_turns);
    mn = max_messages;

    uint8_t *ptr = entire_file;

    GameHeader.NumItems=ni;
    Items=(Item *)MemAlloc(sizeof(Item)*(ni+1));
    na = 300;
    GameHeader.NumActions= - 1;
    Actions=(Action *)MemAlloc(sizeof(Action)*(na+1));

    GameHeader.NumWords=nw;
    GameHeader.WordLength=wl;
    Verbs=MemAlloc(sizeof(char *)*(nw+2));
    Nouns=MemAlloc(sizeof(char *)*(nw+2));
    GameHeader.NumRooms=nr;
    Rooms=(Room *)MemAlloc(sizeof(Room)*(nr+1));
    GameHeader.MaxCarry=mc;
    GameHeader.PlayerRoom=pr;
    GameHeader.LightTime=lt;
    LightRefill=lt;
    GameHeader.NumMessages=mn;
    Messages=MemAlloc(sizeof(char *)*(mn+1));
    GameHeader.TreasureRoom=trm;

    int offset;

#pragma mark rooms

    if (seek_if_needed(fix_address(fix_word(dh.p_room_descr)), &offset, &ptr) == 0)
        return 0;

    ct=0;
    rp=Rooms;

    do {
        rp->Text = get_TI994A_string(dh.p_room_descr, ct);
            fprintf(stderr, "Room %d: %s\n", ct, rp->Text);
            rp->Image = 255;
            ct++;
            rp++;
    } while (ct<nr+1);

#pragma mark messages

    ct=0;
    while(ct<mn+1)
    {
        Messages[ct] = get_TI994A_string(dh.p_message, ct);
        fprintf(stderr, "Message %d: %s\n", ct, Messages[ct]);
        ct++;
    }

#pragma mark items
    ct=0;
    ip=Items;
    do {
        ip->Text = get_TI994A_string(dh.p_obj_descr, ct);
        if (ip->Text && ip->Text[0] == '*')
            tr++;
        fprintf(stderr, "Item %d: %s\n", ct, ip->Text);
        ct++;
        ip++;
    } while(ct<ni+1);

    GameHeader.Treasures = tr;
    fprintf(stderr, "Number of treasures %d\n", GameHeader.Treasures);

#pragma mark room connections

    if (seek_if_needed(fix_address(fix_word(dh.p_room_exit)), &offset, &ptr) == 0)
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

    if (seek_if_needed(fix_address(fix_word(dh.p_orig_items)), &offset, &ptr) == 0)
        return 0;

    ct=0;
    ip=Items;
    while(ct<ni+1) {
        ip->Location = *(ptr++);
        ip->InitialLoc=ip->Location;
        ip++;
        ct++;
    }

#pragma mark dictionary

    load_TI994A_dict(0, dh.p_verb_table, dh.num_verbs + 1, Verbs);
    load_TI994A_dict(1, dh.p_noun_table, dh.num_nouns + 1, Nouns);

    for (int i = 0; i <= dh.num_nouns - dh.num_verbs; i++)
        Verbs[dh.num_verbs + i] = ".\0";

    for (int i = 0; i <= dh.num_verbs - dh.num_nouns; i++)
        Nouns[dh.num_nouns + i] = ".\0";

    for (int i = 0; i <= GameHeader.NumWords; i++)
        fprintf(stderr, "Verb %d: %s\n", i, Verbs[i]);
    for (int i = 0; i <= GameHeader.NumWords; i++)
        fprintf(stderr, "Noun %d: %s\n", i, Nouns[i]);

#pragma mark autoget

    if (seek_if_needed(fix_address(fix_word(dh.p_obj_link)), &offset, &ptr) == 0)
        return 0;

    ct=0;
    ip=Items;

    int objectlinks[ni + 1];

    do {
        objectlinks[ct] = *(ptr++);
        if (objectlinks[ct]) {
            ip->AutoGet = (char *)Nouns[objectlinks[ct]];
        } else {
            ip->AutoGet = NULL;
        }
        ct++;
        ip++;
    } while (ct<ni+1);

    read_implicit(dh);
    read_explicit(dh);

//    print_actions();

    return TI994A;
}

