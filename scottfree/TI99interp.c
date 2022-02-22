#include <string.h>

#include "glk.h"
#include "scott.h"
#include "load_TI99_4a.h"
#include "TI99interp.h"

enum
{
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

struct Keyword actions[] =
{
    {"has",            0xB7, 1 },
    {"here",        0xB8, 1 },
    {"avail",        0xB9, 1 },
    {"!here",        0xBA, 1 },
    {"!has",        0xBB, 1 },
    {"!avail",        0xBC, 1 },
    {"exists",        0xBD, 1 },
    {"!exists",        0xBE, 1 },
    {"in",            0xBF, 1 },
    {"!in",            0xC0, 1 },
    {"set",            0xC1, 1 },
    {"!set",        0xC2, 1 },
    {"something",    0xC3, 0 },
    {"nothing",        0xC4, 0 },
    {"le",            0xC5, 1 },
    {"gt",            0xC6, 1 },
    {"eq",            0xC7, 1 },
    {"!moved",        0xC8, 1 },
    {"moved",        0xC9, 1 },

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

    {"cls",            0xD4, 0 },
    {"pic",            0xD5, 0 },
    {"inv",            0xD6, 0 },
    {"!inv",        0xD7, 0 },
    {"ignore",        0xD8, 0 },
    {"success",        0xD9, 0 },
    {"try",            0xDA, 1 },
    {"get",            0xDB, 1 },
    {"drop",        0xDC, 1 },
    {"goto",        0xDD, 1 },
    {"zap",            0xDE, 1 },
    {"on",            0xDF, 0 },    /* on dark */
    {"off",             0xE0, 0 },    /* off dark */
    {"on",            0xE1, 1 },    /* set flag */
    {"off",            0xE2, 1 },    /* clear flag */
    {"on",            0xE3, 0 },
    {"off",            0xE4, 0 },
    {"die",            0xE5, 0 },
    {"move",        0xE6, 2 },
    {"quit",        0xE7, 0 },
    {".score",        0xE8, 0 },
    {".inv",        0xE9, 0 },
    {"refill",        0xEA, 0 },
    {"save",        0xEB, 0 },
    {"swap",        0xEC, 2 },    /* swap items */
    {"steal",        0xED, 1 },
    {"same",        0xEE, 2 },
    {"nop",            0xEF, 0 },

    {".room",        0xF0, 0 },

    {"--0xF1--",    0xF1, 0 },
    {"add",            0xF2, 0 },
    {"sub",            0xF3, 0 },


    {".timer",        0xF4, 0 },
    {"timer",        0xF5, 1 },

    {"add",            0xF6, 1 },
    {"sub",            0xF7, 1 },

    /* TODO : implement Select RV (0xF8) and Swap RV (0xF9) */
    {"select_rv",    0xF8, 0 },
    {"swap_rv",        0xF9, 1 },

    {"swap",        0xFA, 1 },    /* swap flag */

    {".noun",        0xFB, 0 },
    {".noun_nl",     0xFC, 0 },
    {".nl",            0xFD, 0 },
    {"delay",        0xFE, 0 },

    {"",            0xFF, 0}
};

int run_code_chunk(uint8_t *code_chunk)
{
    if (code_chunk == NULL)
        return 1;
    int run_code = 0;
    int index = 0;
    int result = 0;

    int try_index;
    int try[32];

    int bytes_from_end = 100;

    /* set result to fail (0=ok, 1=fail  */
    result = 0;

    try_index = 0;
    int temp;

    while(run_code == 0)
    {
        int dv = 0, param2 = 0;
        if (bytes_from_end > 0)
            dv = code_chunk[index + 1];
        if (bytes_from_end > 1)
            param2 = code_chunk[index + 2];

        switch(code_chunk[index])
        {
            case 0xB7:		/* ITEM is in inventory */
#ifdef DEBUG_ACTIONS
                fprintf(stderr, "Does the player carry %s?\n", Items[dv].Text);
#endif
                if (Items[dv].Location != CARRIED)
                {
                    run_code = 1;
                    result = 1;
                }
                break;

            case 0xB8:		/* ITEM is in room */
#ifdef DEBUG_ACTIONS
                fprintf(stderr, "Is %s in location?\n", Items[dv].Text);
#endif
                if (Items[dv].Location != MyLoc)
                {
                    run_code = 1;
                    result = 1;
                }

                break;

            case 0xB9:		/* ITEM is available */
#ifdef DEBUG_ACTIONS
                fprintf(stderr, "Is %s held or in location?\n", Items[dv].Text);
#endif
                if (Items[dv].Location != CARRIED && Items[dv].Location != MyLoc)
                {
                    run_code = 1;
                    result = 1;
                }
                break;

            case 0xBA:		/* ITEM is NOT in room */
#ifdef DEBUG_ACTIONS
                fprintf(stderr, "Is %s NOT in location?\n", Items[dv].Text);
#endif
                if (Items[dv].Location == MyLoc)
                {
                    run_code = 1;
                    result = 1;
                }
                break;

            case 0xBB:		/* ITEM is NOT in inventory */
#ifdef DEBUG_ACTIONS
                fprintf(stderr, "Does the player NOT carry %s?\n", Items[dv].Text);
#endif
                if (Items[dv].Location == CARRIED)
                {
                    run_code = 1;
                    result = 1;
                }
                break;

            case 0xBC:		/* object NOT available */
#ifdef DEBUG_ACTIONS
                fprintf(stderr, "Is %s neither carried nor in room?\n", Items[dv].Text);
#endif
                if (Items[dv].Location == CARRIED || Items[dv].Location == MyLoc)
                {
                    run_code = 1;
                    result = 1;
                }
                break;

            case 0xBD:		/* object exists */
#ifdef DEBUG_ACTIONS
                fprintf(stderr, "Is %s (%d) in play?\n", Items[dv].Text, dv);
#endif
                if (Items[dv].Location == 0)
                {
                    run_code = 1;
                    result = 1;
                }
                break;

            case 0xBE:		/* object does not exist */
#ifdef DEBUG_ACTIONS
                fprintf(stderr, "Is %s NOT in play?\n", Items[dv].Text);
#endif
                if (Items[dv].Location != 0)
                {
                    run_code = 1;
                    result = 1;
                }
                break;

            case 0xBF:		/* Player is in room X */
#ifdef DEBUG_ACTIONS
                fprintf(stderr, "Is location %s?\n", Rooms[dv].Text);
#endif
                if (MyLoc != dv)
                {
                    run_code = 1;
                    result = 1;
                }
                break;

            case 0xC0:		/* Player not in room X */
#ifdef DEBUG_ACTIONS
                fprintf(stderr, "Is location NOT %s?\n", Rooms[dv].Text);
#endif
                if (MyLoc == dv)
                {
                    run_code = 1;
                    result = 1;
                }
                break;

            case 0xC1: /* Is bitflag dv clear? */
#ifdef DEBUG_ACTIONS
                fprintf(stderr, "Is bitflag %d set?\n", dv);
#endif
                if ((BitFlags & (1 << dv)) == 0)
                {
                    run_code = 1;
                    result = 1;
                }
                break;

            case 0xC2:		/* Is bitflag dv set? */
#ifdef DEBUG_ACTIONS
                fprintf(stderr, "Is bitflag %d NOT set?\n", dv);
#endif
                if (BitFlags & (1 << dv))
                {
                    run_code = 1;
                    result = 1;
                }
                break;

            case 0xC3:		/* Does the player carry anything? */
#ifdef DEBUG_ACTIONS
                fprintf(stderr, "Does the player carry anything?\n");
#endif
                if (CountCarried() == 0)
                {
                    run_code = 1;
                    result = 1;
                }
                break;

            case 0xC4:		/* Does the player carry nothing? */
#ifdef DEBUG_ACTIONS
                fprintf(stderr, "Does the player carry nothing?\n");
#endif
                if (CountCarried())
                {
                    run_code = 1;
                    result = 1;
                }
                break;

            case 0xC5:		/* Is CurrentCounter <= dv */
#ifdef DEBUG_ACTIONS
                fprintf(stderr, "Is CurrentCounter <= %d?\n", dv);
#endif
                if (CurrentCounter > dv)
                {
                    run_code = 1;
                    result = 1;
                }
                break;

            case 0xC6:		/* Is CurrentCounter > dv */
#ifdef DEBUG_ACTIONS
                fprintf(stderr, "Is CurrentCounter > %d?\n", dv);
#endif
                if (CurrentCounter <= dv)
                {
                    run_code = 1;
                    result = 1;
                }
                break;

            case 0xC7:		/* Is current counter ==  dv */
#ifdef DEBUG_ACTIONS
                fprintf(stderr, "Is current counter == %d?\n", dv);
                if (CurrentCounter != dv)
                    fprintf(stderr, "Nope, current counter is %d\n", CurrentCounter);
#endif
                if (CurrentCounter != dv)
                {
                    run_code = 1;
                    result = 1;
                }
                break;

            case 0xC8:		/* Is item dv still in initial room? */
#ifdef DEBUG_ACTIONS
                fprintf(stderr, "Is %s still in initial room?\n", Items[dv].Text);
#endif
                if (Items[dv].Location != Items[dv].InitialLoc)
                {
                    run_code = 1;
                    result = 1;
                }
                break;

            case 0xC9:		/* Has item dv been moved? */
#ifdef DEBUG_ACTIONS
                fprintf(stderr, "Has %s been moved?\n", Items[dv].Text);
#endif
                if (Items[dv].Location == Items[dv].InitialLoc)
                {
                    run_code = 1;
                    result = 1;
                }
                break;

            case 0xD4:		/* clear screen */
                glk_window_clear(Bottom);
                break;

            case 0xD6:		/* inv */
                AutoInventory = 1;
                break;

            case 0xD7:		/* !inv */
                AutoInventory = 0;
                break;
            case 0xD8:		/* ignore */
            case 0xD9:		/* success */
                break;

            case 0xDA:
                if(try_index>=32)
                {
                    Fatal("ERROR Hit upper limit on try method.\n");
                }

                try[try_index++] = index + dv + 1;
                break;

            case 0xDB:		/* get item */
                if (CountCarried() == GameHeader.MaxCarry) {
                    Output(sys[YOURE_CARRYING_TOO_MUCH]);
                    run_code = 1;
                    result = 1;
                    break;
                } else {
                    Items[dv].Location = CARRIED;
                }
                break;

            case 0xDC:		/* drop item */
#ifdef DEBUG_ACTIONS
                fprintf(stderr, "item %d (\"%s\") is now in location.\n", dv,
                        Items[dv].Text);
#endif
                Items[dv].Location = MyLoc;
                break;

            case 0xDD:		/* goto room */
#ifdef DEBUG_ACTIONS
                fprintf(stderr, "player location is now room %d (%s).\n", dv,
                        Rooms[dv].Text);
#endif
                MyLoc = dv;
                Look();
                break;

            case 0xDE:		/* move item B to room 0 */
#ifdef DEBUG_ACTIONS
                fprintf(stderr,
                        "Item %d (%s) is removed from the game (put in room 0).\n",
                        dv, Items[dv].Text);
#endif
                Items[dv].Location = 0;
                break;

            case 0xDF:		/* on darkness */
                BitFlags |= 1 << DARKBIT;
                break;

            case 0xE0:		/* off darkness */
                BitFlags &= ~(1 << DARKBIT);
                break;

            case 0xE1:		/* set flag dv */
#ifdef DEBUG_ACTIONS
                fprintf(stderr, "Bitflag %d is set\n", dv);
#endif
                BitFlags |= (1 << dv);
                break;

            case 0xE2:		/* clear flag dv */
#ifdef DEBUG_ACTIONS
                fprintf(stderr, "Bitflag %d is cleared\n", dv);
#endif
                BitFlags &= ~(1 << dv);
                break;

            case 0xE3:		/* set flag 0 */
#ifdef DEBUG_ACTIONS
                fprintf(stderr, "Bitflag 0 is set\n");
#endif
                BitFlags |= (1 << 0);
                break;

            case 0xE4:		/* clear flag 0 */
#ifdef DEBUG_ACTIONS
                fprintf(stderr, "Bitflag 0 is cleared\n");
#endif
                BitFlags &= ~(1 << 0);
                break;

            case 0xE5:		/* die */
#ifdef DEBUG_ACTIONS
                fprintf(stderr, "Player is dead\n");
#endif
                Output(sys[IM_DEAD]);
                dead = 1;
                LookWithPause();
                BitFlags &= ~(1 << DARKBIT);
                MyLoc = GameHeader.NumRooms; /* It seems to be what the code says! */
                //				count_light = 1;
                break;

            case 0xE6:		/* move item B to room A */
#ifdef DEBUG_ACTIONS
                fprintf(stderr, "Item %d (%s) is put in room %d (%s).\n",
                        param2, Items[param2].Text, dv, Rooms[dv].Text);
#endif
                Items[param2].Location = dv;
                break;

            case 0xE7:		/* quit */
                DoneIt();
                break;

            case 0xE8:		/* print score */
            {
                int i = 0;
                int n = 0;
                while (i <= GameHeader.NumItems) {
                    if (Items[i].Location == GameHeader.TreasureRoom &&
                        *Items[i].Text == '*')
                        n++;
                    i++;
                }
                Display(Bottom, "%s %d %s%s %d.\n", sys[IVE_STORED], n, sys[TREASURES],
                        sys[ON_A_SCALE_THAT_RATES], (n * 100) / GameHeader.Treasures);
                if (n == GameHeader.Treasures) {
                    Output(sys[YOUVE_SOLVED_IT]);
                    DoneIt();
                }
                break;
            }
            case 0xE9:		/* list contents of inventory */
                ListInventory();
                break;
                //				count_light = 1;
                break;

            case 0xEA:		/* refill */
                GameHeader.LightTime = LightRefill;
                Items[LIGHT_SOURCE].Location = CARRIED;
                BitFlags &= ~(1 << LIGHTOUTBIT);
                break;

            case 0xEB:		/* save */
                SaveGame();
                //				count_light = 1;
                break;

            case 0xEC:		/* swap items 1 and 2 around */
                temp = Items[dv].Location;
                Items[dv].Location = Items[param2].Location;
                Items[param2].Location = temp;
                break;
            case 0xED:		/* move an item to the inventory */
#ifdef DEBUG_ACTIONS
                fprintf(stderr,
                        "Player now carries item %d (%s).\n",
                        dv, Items[dv].Text);
#endif
                Items[dv].Location = CARRIED;
                break;

            case 0xEE:		/* make item1 same room as item2 */
                Items[dv].Location = Items[param2].Location;
                break;

            case 0xEF:		/* nop */
                break;

            case 0xF0:		/* look at room */
                Look();
                break;

            case 0xF2:		/* add 1 to timer */
                CurrentCounter++;
                break;

            case 0xF3:		/* sub 1 from timer */
                if (CurrentCounter >= 1)
                    CurrentCounter--;
                break;

            case 0xF4:		/* print current timer */
                OutputNumber(CurrentCounter);
                Output(" ");
                break;

            case 0xF5:		/* set current counter */
#ifdef DEBUG_ACTIONS
                fprintf(stderr,
                        "CurrentCounter is set to %d.\n",
                        dv);
#endif
                CurrentCounter = dv;
                break;

            case 0xF6:		/*  add to current counter */
#ifdef DEBUG_ACTIONS
                fprintf(stderr,
                        "%d is added to currentCounter. Result: %d\n",
                        dv, CurrentCounter + dv);
#endif
                CurrentCounter += dv;
                break;

            case 0xF7:		/* sub from current counter */
                CurrentCounter -= dv;
                if (CurrentCounter < -1)
                    CurrentCounter = -1;
                break;

            case 0xF8:		/* select room counter */
#ifdef DEBUG_ACTIONS
                fprintf(stderr, "switch location to stored location (%d) (%s).\n",
                        SavedRoom, Rooms[SavedRoom].Text);
#endif
                temp = MyLoc;
                MyLoc = SavedRoom;
                SavedRoom = temp;
                break;

            case 0xF9:		/* swap room counter */
#ifdef DEBUG_ACTIONS
                fprintf(stderr, "swap location<->roomflag[%d]. New location: %s\n", dv, Rooms[RoomSaved[dv]].Text);
#endif
                temp = MyLoc;
                MyLoc = RoomSaved[dv];
                RoomSaved[dv] = temp;
                Look();
                break;

            case 0xFA:		/* swap timer */
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

            case 0xFB:		/* print noun */
                PrintNoun();
                break;

            case 0xFC:		/* print noun + newline */
                PrintNoun();
                Output("\n");
                break;

            case 0xFD:		/* print newline */
                Output("\n");
                break;

            case 0xFE:		/* delay */
                Delay(1);
                break;

            case 0xFF:		/* end of code block. */
                result = 0;
                run_code = 1;
                try_index = 0;	/* drop out of all try blocks! */
                break;

            default:
                if(code_chunk[index] <= 0xB6)
                {
                    const char *message = Messages[code_chunk[index]];
                    if (message != NULL && message[0] != 0) {
                        fprintf(stderr, "(%s)\n", message);
                        Output(message);
                        const char lastchar = message[strlen(message) - 1];
                        if (lastchar != 13 && lastchar != 10)
                            Output(sys[MESSAGE_DELIMITER]);
                    }
                }
                else
                {
                    fprintf(stderr, "~ERR!: %04i) %02X %02X %02X~", index, code_chunk[0 + index], code_chunk[1 + index], code_chunk[2 + index]);
                    glk_exit();
                }
                break;
        }

        if (dead)
            return 0;

        /* we are on the 0xFF opcode, or have fallen through */
        if(run_code == 1 && try_index > 0)
        {
            if(code_chunk[index] == 0xFF)
            {
                run_code = 1;
            }
            else
            {
                /* not matched at end of TRY block */
                /* or AT end of try block */
                index = try[try_index-1];

                try_index -= 1;
                try[try_index] = 0;
                run_code = 0;
            }
        }
        else
        {
            /* continue */
            if(code_chunk[index] >= 0xB7)
                index += 1 + actions[(code_chunk[0 + index])- 0xB7].count;
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

    /* fall out, if no auto acts in the game. */
    if(*ptr == 0x0)
        loop_flag = 1;

    while(loop_flag == 0)
    {
        /*
         p+0 = percentage of happening
         p+1 = size of code chunk
         p+2 = start of code
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

#define BAD_WORD 255

/* parses verb noun actions */
int run_explicit(int verb_num, int noun_num)
{
    uint8_t *p;
    int flag = 1;
    int runcode;

    if(verb_num != BAD_WORD)
    {
        if(noun_num == BAD_WORD)
        {
            return -1;
        }

        runcode = 0;

        /* continue_code: */
        /* run code.... */
        p = ti99_explicit_actions;

        flag = RC_NULL;

        p = VerbActionOffsets[verb_num];

        /* process all code blocks for this verb
         until we come to the end of code blocks
         or until we successfully end a block.
         */

        flag = RC_NULL;
        runcode = 0;
        while(flag == RC_NULL)
        {
            /* we match CLIMB NOUN or CLIMB ANY */
            if(p[0] == noun_num || p[0] == 0)
            {
                /* we have verb/noun match. run code! */

                runcode = run_code_chunk(p + 2);

                if(runcode == 0)		/* success */
                {
                    flag = RC_OK;
                    return 0;
                }
                else			/* failure */
                {
                    if(p[1] == 0)
                        flag = RC_RAN_ALL_BLOCKS_FAILED;
                    else
                        p += 1 + p[1];
                }
            }
            else
            {
                if(p[1] == 0)
                    flag = RC_RAN_ALL_BLOCKS;
                else
                    p += 1 + p[1];
            }
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
