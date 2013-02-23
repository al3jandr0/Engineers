#include <stdio.h>
#include <string.h> 

  int i = 0;                                   /* Loop counter                         */
  int r = 0;                                   /* Loop counter                         */
  int col = 0;                                 /* Loop counter                         */
  char ch;									   /* Temp char							   */
  int temp;									   /* For the - loop					   */
 // char * str = malloc(256 * sizeof(char));						   /* Str for single board 				   */
  int player = 0;                              /* Player number - 1 or 2               */
  int turn = 0;								   /* Whose turn is it?					   */
  int playerX = 0;                             /* Select player X                      */
  int playerY = 0;                             /* Select player O                      */
  int go = 0;                                  /* Square selection number for turn     */
  int row = 0;                                 /* Row index for a square               */  
  int column = 0;                              /* Column index for a square            */
  int line = 0;                                /* Row or column index in checking loop */
  int winner = 0;                              /* The winning player                   */
  char board[3][3] = {                         /* The board                            */
                       {'1','2','3'},          /* Initial values are reference numbers */
                       {'4','5','6'},          /* used to select a vacant square for   */
                       {'7','8','9'}           /* a turn.                              */
                     };
					 
void append(char* s, char c)
{
        int len = strlen(s);
        s[len] = c;
        s[len+1] = '\0';
}

int addPlayer(int id)
{

// LOCK
printf("%d",turn);
if (playerX == 0)
{
      playerX = id;
	  turn = playerX;
	  
printf("%d",turn);
return 1;
}
else if (playerY == 0)
{
	  playerY = id;
if (turn == 0)
	turn = playerY;
	  
printf("%d",turn);
return 2;
}
else 
	  return 3;
	  // NO more players 
	  
// UNLOCK

}


char * game(char * str)
{
		if (winner  == 1)
		{
				ch = 'X';
				append(str, ch);
		}
		else if (winner == 2)
		{
				ch = 'O';
				append(str, ch);
		}
		else if (winner == 3)
		{
				ch = 'D';
				append(str, ch);
		}
		else if (winner == 4)
		{
				ch = 'T';
				append(str, ch);
		}
		else if (winner == 5)
		{
				ch = 'U';
				append(str, ch);
		}
		else
		{
				ch = 'N';
				append(str, ch);
		}
		
		ch = '-';
		for(temp = 0; temp < 7; temp++)
			append(str, ch);

		ch = '\n';
        append(str, ch);

		for(r = 0; r < 3; r++)
		{
			for(col = 0; col < 3; col++)
			{
				ch = '|';
				append(str, ch);

				ch = board[r][col];
				append(str, ch);

				if(col == 2)
				{
					ch = '|';
					append(str, ch);
					ch = '\n';
					append(str, ch);
					ch = '-';
					
					for(temp = 0; temp < 7; temp++)
						append(str, ch);

					ch = '\n';
					append(str, ch);
				}
			}
		}
winner = 0;
		return str;
}

void resetGame()
{
playerX = 0;
playerY = 0;

board[0][0] = '1';
board[0][1] = '2';
board[0][2] = '3';

board[1][0] = '4';
board[1][1] = '5';
board[1][2] = '6';

board[2][0] = '7';
board[2][1] = '8';
board[2][2] = '9';

winner = 0;

turn = 0;
}

int removePlayer(int id)
{
	  if (playerX == id)
	  { 
		winner = 4;
		return 1;
	  }
	  else if (playerY == id)
	  { 
		winner = 5;
		return 1;
	  }
	  else 
		return 2;
}

// Adding moves to the board - go is the number 1-9 for place on the board. 

 int logic(int id, int go)

 {

	  // Check whose turn it is

	  if (id != turn)
		return 0;

	  // Check for valid move

	  if(go>0 && go<=9)
	  {
	  row = --go/3;                                 //Get row index of square   

         column = go%3;                                //Get column index of square  

	  }

	  else 
	  {
		return 1;									  //Not a valid move. 
	  }
	
	  if(board[row][column]>'9')

	  {

		return 1; 

	  }



	  if (playerX == id)

	  {

		board[row][column] = 'X';					/* Insert player symbol   */

		turn = playerY;

	  }

	  

	  if (playerY == id)

	  {

		board[row][column] = 'O';					/* Insert player symbol   */

		turn = playerX;

	  }

	  

      // board[row][column] = (player == 1) ? 'X' : 'O';        /* Insert player symbol   */



      /* Check for a winning line - diagonals first */     

      if((board[0][0] == board[1][1] && board[0][0] == board[2][2]) ||

         (board[0][2] == board[1][1] && board[0][2] == board[2][0])){

        if (board[1][1] == 'X')
{
			winner = 1;
}
		else 
{
			winner = 2;
}
		return 2;

		}

      else

      /* Check rows and columns for a winning line */

        for(line = 0; line <= 2; line ++){

          if((board[line][0] == board[line][1] && board[line][0] == board[line][2])||

             (board[0][line] == board[1][line] && board[0][line] == board[2][line])){

                    if ((board[line][0] ==  'X' && board[line][1] == 'X' && board[line][2] == 'X') || (board[0][line] == 'X' && board[1][line] == 'X' && board[2][line] == 'X' ))
{
						winner = 1;
}
					else 
{
						winner = 2;
}
			return 2;

			}

		}

		

	if((board[0][0] > '9' && board[0][1] > '9' && board[0][2] > '9' && board[1][0] > '9' && board[1][1] > '9' && board[1][2] > '9' && board[2][0] > '9' && board[2][1] > '9' && board[2][2] > '9')){

        winner = 3;
		return 2;

		}

		

		return 3;

}
 
