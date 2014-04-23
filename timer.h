/*************************************************************************
 *
 *	File:	timer.h
 *	Author:  Jing Huang & Liang Wu
 *
 *
 ************************************************************************/

#ifndef __TIMER_H_
#define __TIMER_H_

#include <stdio.h>
#include <stdlib.h>
#include <time.h>


void StartTimer(void);
void StopTimer(void);
double CurrentTime(void);
double ElapsedTime(void);
char *today(void); 


#endif 
