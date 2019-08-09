/** @file part1.c */

/*
 * Machine Problem #1
 * CS 241
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "mp1-functions.h"

/**
 * (Edit this function to print out the ten "Illinois" lines in mp1-functions.c in order.)
 */
int main()
{
	first_step(81);

	int value = 132;
	second_step(&value);

	int **v3 = (int**) malloc(sizeof(int *));
	v3[0] = (int *) malloc(sizeof(int));
	*v3[0] = 8942;
	double_step(v3);
	free(v3[0]);
	free(v3);

	int *v4;
	v4 = 0;
	strange_step(v4);


	char v5[4];
	v5[3] = 0;
	empty_step( (void *)v5);

	char s6[5];
	s6[3] = 'u';
	two_step( (void *)s6, s6);

	three_step(s6, s6+2, s6+4);

	char f1[2],f2[3],f3[4];
	f1[1] = 0;
	f2[2] = 8;
	f3[3] = 16;
	step_step_step (f1,f2,f3);

	char a[1];
	int b = 4;
	a[0] = (char) b;
	it_may_be_odd(a,b);

	char orange[5];
	orange[0] = 1;
	orange[1] = 2;
	the_end(orange, orange);
	
	return 0;
}
