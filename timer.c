/*************************************************************************
 *
 *	File:	timer.c
 *	Author:  Jing Huang & Liang Wu
 *
 *
 ************************************************************************/

#include "timer.h"


static clock_t start;
static clock_t finish;


void StartTimer(void)
{
	start = clock(); 
}


void StopTimer(void)
{
	finish = clock(); 
}


double CurrentTime(void)
{
	return ((double)(clock() - start) / CLOCKS_PER_SEC);
}


double ElapsedTime(void)
{
	return ((double)(finish - start) / CLOCKS_PER_SEC);
}


char *today(void) 
{
	time_t now = time(NULL);
	struct tm * curtime = localtime(&now);
	
	return (asctime(curtime));		
}