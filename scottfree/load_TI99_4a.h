//
//  load_TI99_4a.h
//  scott
//
//  Created by Administrator on 2022-02-12.
//

#ifndef load_TI99_4a_h
#define load_TI99_4a_h

#include "definitions.h"
#include <stdint.h>
#include <stdio.h>

GameIDType DetectTI994A(void);

extern uint8_t *ti99_implicit_actions;
extern uint8_t *ti99_explicit_actions;
extern size_t ti99_implicit_extent;
extern uint8_t **VerbActionOffsets;

#endif /* load_TI99_4a_h */
