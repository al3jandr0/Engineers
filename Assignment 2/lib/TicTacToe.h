#ifndef __DAGAME_TicTacToe_H__
#define __DAGAME_TicTacToe_H__

#include <stdio.h>
#include <string.h> 

void append(char* s, char c);

int addPlayer(int id);

char * game();

void resetGame();

int removePlayer(int id);
 
int logic(int id, int go);

#endif
