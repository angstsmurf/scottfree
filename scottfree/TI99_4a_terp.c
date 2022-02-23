#include <string.h>

#include "glk.h"
#include "load_TI99_4a.h"
#include "scott.h"
#include "TI99_4a_terp.h"

enum {
    RC_NULL = 0,
    RC_OK,
    RC_RAN_ALL_BLOCKS,
    RC_RAN_ALL_BLOCKS_FAILED
};

struct Keyword {
    const char *name;
    int opcode;
    int count;
};

struct Keyword actions[] = {
    { "has", 0xB7, 1 },
    { "here", 0xB8, 1 },
    { "avail", 0xB9, 1 },
    { "!here", 0xBA, 1 },
    { "!has", 0xBB, 1 },
    { "!avail", 0xBC, 1 },
    { "exists", 0xBD, 1 },
    { "!exists", 0xBE, 1 },
    { "in", 0xBF, 1 },
    { "!in", 0xC0, 1 },
    { "set", 0xC1, 1 },
    { "!set", 0xC2, 1 },
    { "something", 0xC3, 0 },
    { "nothing", 0xC4, 0 },
    { "le", 0xC5, 1 },
    { "gt", 0xC6, 1 },
    { "eq", 0xC7, 1 },
    { "!moved", 0xC8, 1 },
    { "moved", 0xC9, 1 },

    { "--0xCA--", 0xCA, 0 },
    { "--0xCB--", 0xCB, 0 },
    { "--0xCC--", 0xCC, 0 },
    { "--0xCD--", 0xCD, 0 },
    { "--0xCE--", 0xCE, 0 },
    { "--0xCF--", 0xCF, 0 },
    { "--0xD0--", 0xD0, 0 },
    { "--0xD1--", 0xD1, 0 },
    { "--0xD2--", 0xD2, 0 },
    { "--0xD3--", 0xD3, 0 },

    { "cls", 0xD4, 0 },
    { "pic", 0xD5, 0 },
    { "inv", 0xD6, 0 },
    { "!inv", 0xD7, 0 },
    { "ignore", 0xD8, 0 },
    { "success", 0xD9, 0 },
    { "try", 0xDA, 1 },
    { "get", 0xDB, 1 },
    { "drop", 0xDC, 1 },
    { "goto", 0xDD, 1 },
    { "zap", 0xDE, 1 },
    { "on", 0xDF, 0 }, /* on dark */
    { "off", 0xE0, 0 }, /* off dark */
    { "on", 0xE1, 1 }, /* set flag */
    { "off", 0xE2, 1 }, /* clear flag */
    { "on", 0xE3, 0 },
    { "off", 0xE4, 0 },
    { "die", 0xE5, 0 },
    { "move", 0xE6, 2 },
    { "quit", 0xE7, 0 },
    { ".score", 0xE8, 0 },
    { ".inv", 0xE9, 0 },
    { "refill", 0xEA, 0 },
    { "save", 0xEB, 0 },
    { "swap", 0xEC, 2 }, /* swap items */
    { "steal", 0xED, 1 },
    { "same", 0xEE, 2 },
    { "nop", 0xEF, 0 },

    { ".room", 0xF0, 0 },

    { "--0xF1--", 0xF1, 0 },
    { "add", 0xF2, 0 },
    { "sub", 0xF3, 0 },

    { ".timer", 0xF4, 0 },
    { "timer", 0xF5, 1 },

    { "add", 0xF6, 1 },
    { "sub", 0xF7, 1 },

    /* TODO : implement Select RV (0xF8) and Swap RV (0xF9) */
    { "select_rv", 0xF8, 0 },
    { "swap_rv", 0xF9, 1 },

    { "swap", 0xFA, 1 }, /* swap flag */

    { ".noun", 0xFB, 0 },
    { ".noun_nl", 0xFC, 0 },
    { ".nl", 0xFD, 0 },
    { "delay", 0xFE, 0 },

    { "", 0xFF, 0 }
};

int run_code_chunk(uint8_t *code_chunk)
{
    if (code_chunk == NULL)
        return 1;

    uint8_t *ptr = code_chunk;
    int run_code = 0;
    int index = 0;
    int result = 0;
    int opcode;

    int try_index;
    int try[32];

    int bytes_from_end = 100;

    /* 0: Success, 1: Failure  */
    result = 1;

    try_index = 0;
    int temp;

    while (run_code == 0) {
        int dv = 0, param2 = 0;
        if (bytes_from_end > 0)
            dv = code_chunk[index + 1];
        if (bytes_from_end > 1)
            param2 = code_chunk[index + 2];

        opcode = *(ptr++);

        switch (opcode) {
        case 183: /* ITEM is in inventory */
#ifdef DEBUG_ACTIONS
            fprintf(stderr, "Does the player carry %s?\n", Items[*ptr].Text);
#endif
            if (Items[*(ptr++)].Location != CARRIED) {
                run_code = 1;
                result = 1;
            }
            break;

        case 184: /* ITEM is in room */
#ifdef DEBUG_ACTIONS
            fprintf(stderr, "Is %s in location?\n", Items[*ptr].Text);
#endif
            if (Items[*(ptr++)].Location != MyLoc) {
                run_code = 1;
                result = 1;
            }

            break;

        case 185: /* ITEM is available */
#ifdef DEBUG_ACTIONS
            fprintf(stderr, "Is %s held or in location?\n", Items[*ptr].Text);
#endif
            if (Items[*ptr].Location != CARRIED && Items[*(ptr++)].Location != MyLoc) {
                run_code = 1;
                result = 1;
            }
            break;

        case 186: /* ITEM is NOT in room */
#ifdef DEBUG_ACTIONS
            fprintf(stderr, "Is %s NOT in location?\n", Items[dv].Text);
#endif
            if (Items[dv].Location == MyLoc) {
                run_code = 1;
                result = 1;
            }
            break;

        case 187: /* ITEM is NOT in inventory */
#ifdef DEBUG_ACTIONS
            fprintf(stderr, "Does the player NOT carry %s?\n", Items[dv].Text);
#endif
            if (Items[dv].Location == CARRIED) {
                run_code = 1;
                result = 1;
            }
            break;

        case 188: /* object NOT available */
#ifdef DEBUG_ACTIONS
            fprintf(stderr, "Is %s neither carried nor in room?\n", Items[dv].Text);
#endif
            if (Items[dv].Location == CARRIED || Items[dv].Location == MyLoc) {
                run_code = 1;
                result = 1;
            }
            break;

        case 189: /* object exists */
#ifdef DEBUG_ACTIONS
            fprintf(stderr, "Is %s (%d) in play?\n", Items[dv].Text, dv);
#endif
            if (Items[dv].Location == 0) {
                run_code = 1;
                result = 1;
            }
            break;

        case 190: /* object does not exist */
#ifdef DEBUG_ACTIONS
            fprintf(stderr, "Is %s NOT in play?\n", Items[dv].Text);
#endif
            if (Items[dv].Location != 0) {
                run_code = 1;
                result = 1;
            }
            break;

        case 191: /* Player is in room X */
#ifdef DEBUG_ACTIONS
            fprintf(stderr, "Is location %s?\n", Rooms[dv].Text);
#endif
            if (MyLoc != dv) {
                run_code = 1;
                result = 1;
            }
            break;

        case 192: /* Player not in room X */
#ifdef DEBUG_ACTIONS
            fprintf(stderr, "Is location NOT %s?\n", Rooms[dv].Text);
#endif
            if (MyLoc == dv) {
                run_code = 1;
                result = 1;
            }
            break;

        case 193: /* Is bitflag dv clear? */
#ifdef DEBUG_ACTIONS
            fprintf(stderr, "Is bitflag %d set?\n", dv);
#endif
            if ((BitFlags & (1 << dv)) == 0) {
                run_code = 1;
                result = 1;
            }
            break;

        case 194: /* Is bitflag dv set? */
#ifdef DEBUG_ACTIONS
            fprintf(stderr, "Is bitflag %d NOT set?\n", dv);
#endif
            if (BitFlags & (1 << dv)) {
                run_code = 1;
                result = 1;
            }
            break;

        case 195: /* Does the player carry anything? */
#ifdef DEBUG_ACTIONS
            fprintf(stderr, "Does the player carry anything?\n");
#endif
            if (CountCarried() == 0) {
                run_code = 1;
                result = 1;
            }
            break;

        case 196: /* Does the player carry nothing? */
#ifdef DEBUG_ACTIONS
            fprintf(stderr, "Does the player carry nothing?\n");
#endif
            if (CountCarried()) {
                run_code = 1;
                result = 1;
            }
            break;

        case 197: /* Is CurrentCounter <= dv */
#ifdef DEBUG_ACTIONS
            fprintf(stderr, "Is CurrentCounter <= %d?\n", dv);
#endif
            if (CurrentCounter > dv) {
                run_code = 1;
                result = 1;
            }
            break;

        case 198: /* Is CurrentCounter > dv */
#ifdef DEBUG_ACTIONS
            fprintf(stderr, "Is CurrentCounter > %d?\n", dv);
#endif
            if (CurrentCounter <= dv) {
                run_code = 1;
                result = 1;
            }
            break;

        case 199: /* Is current counter ==  dv */
#ifdef DEBUG_ACTIONS
            fprintf(stderr, "Is current counter == %d?\n", dv);
            if (CurrentCounter != dv)
                fprintf(stderr, "Nope, current counter is %d\n", CurrentCounter);
#endif
            if (CurrentCounter != dv) {
                run_code = 1;
                result = 1;
            }
            break;

        case 200: /* Is item dv still in initial room? */
#ifdef DEBUG_ACTIONS
            fprintf(stderr, "Is %s still in initial room?\n", Items[dv].Text);
#endif
            if (Items[dv].Location != Items[dv].InitialLoc) {
                run_code = 1;
                result = 1;
            }
            break;

        case 201: /* Has item dv been moved? */
#ifdef DEBUG_ACTIONS
            fprintf(stderr, "Has %s been moved?\n", Items[dv].Text);
#endif
            if (Items[dv].Location == Items[dv].InitialLoc) {
                run_code = 1;
                result = 1;
            }
            break;

        case 212: /* clear screen */
            glk_window_clear(Bottom);
            break;

        case 214: /* inv */
            AutoInventory = 1;
            break;

        case 215: /* !inv */
            AutoInventory = 0;
            break;
        case 216:
        case 217:
            break;
        case 218:
            if (try_index >= 32) {
                Fatal("ERROR Hit upper limit on try method.\n");
            }

            try[try_index++] = index + dv + 1;
            break;

        case 219: /* get item */
            if (CountCarried() == GameHeader.MaxCarry) {
                Output(sys[YOURE_CARRYING_TOO_MUCH]);
                run_code = 1;
                result = 1;
                break;
            } else {
                Items[dv].Location = CARRIED;
            }
            break;

        case 220: /* drop item */
#ifdef DEBUG_ACTIONS
            fprintf(stderr, "item %d (\"%s\") is now in location.\n", dv,
                Items[dv].Text);
#endif
            Items[dv].Location = MyLoc;
            break;

        case 221: /* goto room */
#ifdef DEBUG_ACTIONS
            fprintf(stderr, "player location is now room %d (%s).\n", dv,
                Rooms[dv].Text);
#endif
            MyLoc = dv;
            Look();
            break;

        case 222: /* move item B to room 0 */
#ifdef DEBUG_ACTIONS
            fprintf(stderr,
                "Item %d (%s) is removed from the game (put in room 0).\n",
                dv, Items[dv].Text);
#endif
            Items[dv].Location = 0;
            break;

        case 223: /* darkness */
            BitFlags |= 1 << DARKBIT;
            break;

        case 224: /* light */
            BitFlags &= ~(1 << DARKBIT);
            break;

        case 225: /* set flag dv */
#ifdef DEBUG_ACTIONS
            fprintf(stderr, "Bitflag %d is set\n", dv);
#endif
            BitFlags |= (1 << dv);
            break;

        case 226: /* clear flag dv */
#ifdef DEBUG_ACTIONS
            fprintf(stderr, "Bitflag %d is cleared\n", dv);
#endif
            BitFlags &= ~(1 << dv);
            break;

        case 227: /* set flag 0 */
#ifdef DEBUG_ACTIONS
            fprintf(stderr, "Bitflag 0 is set\n");
#endif
            BitFlags |= (1 << 0);
            break;

        case 228: /* clear flag 0 */
#ifdef DEBUG_ACTIONS
            fprintf(stderr, "Bitflag 0 is cleared\n");
#endif
            BitFlags &= ~(1 << 0);
            break;

        case 229: /* die */
#ifdef DEBUG_ACTIONS
            fprintf(stderr, "Player is dead\n");
#endif
            Output(sys[IM_DEAD]);
            dead = 1;
            LookWithPause();
            BitFlags &= ~(1 << DARKBIT);
            MyLoc = GameHeader.NumRooms; /* It seems to be what the code says! */
            stop_time = 1;
            break;

        case 230: /* move item B to room A */
#ifdef DEBUG_ACTIONS
            fprintf(stderr, "Item %d (%s) is put in room %d (%s).\n",
                param2, Items[param2].Text, dv, Rooms[dv].Text);
#endif
            Items[param2].Location = dv;
            break;

        case 231: /* quit */
            DoneIt();
            break;

        case 232: /* print score */
            PrintScore();
            stop_time = 1;
            break;

        case 233: /* list contents of inventory */
            ListInventory();
            stop_time = 1;
            break;

        case 234: /* refill */
            GameHeader.LightTime = LightRefill;
            Items[LIGHT_SOURCE].Location = CARRIED;
            BitFlags &= ~(1 << LIGHTOUTBIT);
            break;

        case 235: /* save */
            SaveGame();
            stop_time = 1;
            break;

        case 236: /* swap items 1 and 2 around */
            temp = Items[dv].Location;
            Items[dv].Location = Items[param2].Location;
            Items[param2].Location = temp;
            break;

        case 237: /* move an item to the inventory */
#ifdef DEBUG_ACTIONS
            fprintf(stderr,
                "Player now carries item %d (%s).\n",
                dv, Items[dv].Text);
#endif
            Items[dv].Location = CARRIED;
            break;

        case 238: /* make item1 same room as item2 */
            Items[dv].Location = Items[param2].Location;
            break;

        case 239: /* nop */
            break;

        case 240: /* look at room */
            Look();
            break;

        case 242: /* add 1 to timer */
            CurrentCounter++;
            break;

        case 243: /* sub 1 from timer */
            if (CurrentCounter >= 1)
                CurrentCounter--;
            break;

        case 244: /* print current timer */
            OutputNumber(CurrentCounter);
            Output(" ");
            break;

        case 245: /* set current counter */
#ifdef DEBUG_ACTIONS
            fprintf(stderr,
                "CurrentCounter is set to %d.\n",
                dv);
#endif
            CurrentCounter = dv;
            break;

        case 246: /*  add to current counter */
#ifdef DEBUG_ACTIONS
            fprintf(stderr,
                "%d is added to currentCounter. Result: %d\n",
                dv, CurrentCounter + dv);
#endif
            CurrentCounter += dv;
            break;

        case 247: /* sub from current counter */
            CurrentCounter -= dv;
            if (CurrentCounter < -1)
                CurrentCounter = -1;
            break;

        case 248: /* select room counter */
#ifdef DEBUG_ACTIONS
            fprintf(stderr, "switch location to stored location (%d) (%s).\n",
                SavedRoom, Rooms[SavedRoom].Text);
#endif
            temp = MyLoc;
            MyLoc = SavedRoom;
            SavedRoom = temp;
            break;

        case 249: /* swap room counter */
#ifdef DEBUG_ACTIONS
            fprintf(stderr, "swap location<->roomflag[%d]. New location: %s\n", dv, Rooms[RoomSaved[dv]].Text);
#endif
            temp = MyLoc;
            MyLoc = RoomSaved[dv];
            RoomSaved[dv] = temp;
            Look();
            break;

        case 250: /* swap timer */
#ifdef DEBUG_ACTIONS
            fprintf(stderr,
                "Select a counter. Current counter is swapped with backup "
                "counter %d\n",
                dv);
#endif
            {
                int c1 = CurrentCounter;
                if (dv > 15) {
                    fprintf(stderr, "ERROR! parameter out of range. Max 15, got %d\n", dv);
                    dv = 15;
                }
                CurrentCounter = Counters[dv];
                Counters[dv] = c1;
#ifdef DEBUG_ACTIONS
                fprintf(stderr, "Value of new selected counter is %d\n",
                    CurrentCounter);
#endif
                break;
            }

        case 251: /* print noun */
            PrintNoun();
            break;

        case 252: /* print noun + newline */
            PrintNoun();
            Output("\n");
            break;

        case 253: /* print newline */
            Output("\n");
            break;

        case 254: /* delay */
            Delay(1);
            break;

        case 255: /* end of code block. */
            result = 0;
            run_code = 1;
            try_index = 0; /* drop out of all try blocks! */
            break;

        default:
            if (opcode <= 182 && opcode <= GameHeader.NumMessages + 1) {
                const char *message = Messages[opcode];
                if (message != NULL && message[0] != 0) {
                    Output(message);
                    const char lastchar = message[strlen(message) - 1];
                    if (lastchar != 13 && lastchar != 10)
                        Output(sys[MESSAGE_DELIMITER]);
                }
            } else {
                fprintf(stderr, "~ERR!: %04i) %02X %02X %02X~", index, code_chunk[0 + index], code_chunk[1 + index], code_chunk[2 + index]);
                glk_exit();
            }
            break;
        }

        if (dead) {
            DoneIt();
            return 0;
        }

        /* we are on the 0xFF opcode, or have fallen through */
        if (run_code == 1 && try_index > 0) {
            if (opcode == 0xFF) {
                run_code = 1;
            } else {
                /* dropped out of TRY block */
                /* or at end of TRY block */
                index = try[try_index - 1];

                try_index -= 1;
                try[try_index] = 0;
                run_code = 0;
            }
        } else {
            /* continue */
            if (opcode >= 183)
                index += 1 + actions[(code_chunk[0 + index]) - 183].count;
            else
                index += 1;
        }
    }

    return result;
}

void run_implicit(void)
{
    int probability;
    uint8_t *ptr;
    int loop_flag;

    ptr = ti99_implicit_actions;
    loop_flag = 0;

    /* fall out if no auto acts in the game. */
    if (*ptr == 0x0)
        loop_flag = 1;

    while (loop_flag == 0) {
        /*
         p + 0 = chance of happening
         p + 1 = size of code chunk
         p + 2 = start of code
         */

        probability = ptr[0];

        if (RandomPercent(probability))
            run_code_chunk(ptr + 2);

        if (ptr[1] == 0 || ptr - ti99_implicit_actions >= ti99_implicit_extent)
            loop_flag = 1;

        /* skip code chunk */
        ptr += 1 + ptr[1];
    }
}

/* parses verb noun actions */
int run_explicit(int verb_num, int noun_num)
{
    uint8_t *p;
    int flag = 1;
    int runcode;

    p = VerbActionOffsets[verb_num];

    /* process all code blocks for this verb
     until success or end. */

    flag = RC_NULL;
    while (flag == RC_NULL) {
        /* we match VERB NOUN or VERB ANY */
        if (p[0] == noun_num || p[0] == 0) {
            /* we have verb/noun match. run code! */

            runcode = run_code_chunk(p + 2);

            if (runcode == 0) { /* success */
                return 0;
            } else { /* failure */
                if (p[1] == 0)
                    flag = RC_RAN_ALL_BLOCKS_FAILED;
                else
                    p += 1 + p[1];
            }
        } else {
            if (p[1] == 0)
                flag = RC_RAN_ALL_BLOCKS;
            else
                p += 1 + p[1];
        }
    }

    if (flag == RC_RAN_ALL_BLOCKS) {
        return -2;
    }

    if (flag == RC_OK)
        return 0;
    else
        return -1;
}
