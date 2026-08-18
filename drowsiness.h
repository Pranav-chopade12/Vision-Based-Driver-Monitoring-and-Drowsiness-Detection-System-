#ifndef DROWSINESS_H
#define DROWSINESS_H

#include "stm32f4xx.h"

void Drowsiness_Init(void);
void Drowsiness_Task(void);

char GetDriverState(void);


#endif
