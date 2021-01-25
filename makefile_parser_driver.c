#include "makefile_parser.h"
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

/**
 * This program should use makefile_parser.h functions and output the input mymake file
 * in a standardized format.
 *
 * There should not be a blank line at the top of the file.
 *
 * Each rule (and associated recipe) should be output without extra
 * whitespace, in the order found in the input file.
 * After each rule (and optional recipe) there should be a single blank line.
 *
 * If supporting variables, each variable definition found should be output
 * in the form:
 * 
 * VARIABLE=VALUE
 *
 * (no added whitespace around =).
 * There should be no blank line after variables.
 *
 * If multitarget support was implemented, targets should be separated by
 * a single space. There should not be a space before ':'.
 *
 * If the rule has dependencies, each dependency should be prefix with a
 * single space.
 *
 * (Which means that if a rule has no dependencies, there will not be a space
 * following ':')
 *
 * You can test your output by comparing against the provided example files.
 * (You can compare using the `diff -u <file1> <file2>` command).
 *
 * Your program should take a single command line argument (and complain if more or less are provided)
 * which is the name of the input file.
 *
 * The output should be written to the screen (stdout).
 * Any parser errors should be written to stderr.
 */

bool print(void * data, const char ** target, unsigned int tcount,
		   const char ** dependencies, unsigned int dcount,
		   const char ** recipe, unsigned int rcount){

//	printf("tcount : %u\n", tcount);
//	printf("dcount : %u\n", dcount);	
//	printf("rcount : %u\n", rcount);
	for(int i = 0; i < tcount; i++){
		printf("%s", target[i]);
		if(i == tcount - 1){
			printf(":");
		} else {
			printf(" ");
		}
	}
	
	for(int i = 0; i < dcount; i++){
		printf(" %s", dependencies[i]);
	}
	printf("\n");
	
	for(int i = 0; i < rcount; i++){
		printf("\t%s\n", recipe[i]);
	}
	printf("\n");

	return true;
}


int main(int argc, char ** args){
	mfp_cb_t cb;
	cb.error = stderr;
	cb.rule_cb = print;
	if(argc != 2){
		printf("Error: Must have a command line argument.\n");
		return EXIT_FAILURE;
	}
		
	FILE * makefile = fopen(args[1], "r");
	if(!makefile){
		printf("Error opening file.\n");
		return EXIT_FAILURE;
	}
	
	if(!mfp_parse(makefile, &cb, NULL)){
		// Error happened
		printf("Error parsing makefile.\n");
		if(makefile){
			fclose(makefile);
		}
		return EXIT_FAILURE;
	}
	
	fclose(makefile);
	return EXIT_SUCCESS;
}

