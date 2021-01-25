#define  _GNU_SOURCE      // Needed for getline
#include "makefile_parser.h"
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <ctype.h>

// Initial values, will allocate more if needed
//#define MAXARRAY 10    
#define MAXWORD 50
#define CSIZE 1    // Size for calloc function

// Structure which will hold variable length words
struct varstring{
	char * word;
	unsigned int wordlength;
	unsigned int arraylength;
};

typedef struct varstring varstring;


// Structure to hold array of varstrings
struct vararray{
	varstring ** words;
	unsigned int cursize;
};

typedef struct vararray vararray;

static void expand_varstring(varstring * mystr, unsigned int size){
	unsigned int newsize = size * 2;
	char * new_array = realloc(mystr->word, sizeof(char)*newsize);
	mystr->word = new_array;
	mystr->arraylength = newsize;
}



// Function to strip comments from lines
static ssize_t strip_comments(char ** array, ssize_t length){
	bool hashtag = false;
	for(int i = 0; i < length; i++){
		if(!hashtag){
			// Check currect character
			if((*array)[i] == '#'){
				// See if the previous character is an escape character
				if(i == 0 || (*array)[i-1] != '\\'){
					hashtag = true;
				}
			}
		}
		if(hashtag){
			(*array)[i] = '\n';
			(*array)[i+1] = '\0';
			break;
		}
	}
	ssize_t newlen = strlen(*array);
	return newlen;
}


// Function to strip beginning whitespace
static ssize_t strip_whitespace_beg(char ** array, ssize_t length){
	if((*array)[0] != ' '){
		return length;
	}
	
	char * walker = *array;
	while(*walker == ' '){
		walker++;
	}

	unsigned int i = 0;
	while(*walker){
		(*array)[i] = *walker;
		walker++;
		i++;
	}

	(*array)[i] = '\0';
	return strlen(*array);
}


// Function to get a new vararray
static vararray * new_vararray(){
	vararray * array = malloc(sizeof(vararray));
//	array->maxsize = MAXARRAY;
	array->words = NULL;
	array->cursize = 0;
	return array;
}


// Function to free vararray
static void free_vararray(vararray * array){
	assert(array);
	if(array->words){
		// TODO: Free each varstring in array
		free(array->words);
	}
	free(array);
}

// Function to free the varstring
static void free_varstring(varstring * mystr){
	assert(mystr);
	if(mystr->word){
		free(mystr->word);
	}
	free(mystr);
}

// Function to free the list of words pointed to by array->words
static void free_list(vararray * array){
	assert(array);
	for(int i = 0; i < array->cursize; i++){
		free_varstring(array->words[i]);
	}
	free(array->words);
	array->words = NULL;
	array->cursize = 0;
}

// Function for a new varstring
static varstring * new_varstring(){
	varstring * new_string = malloc(sizeof(varstring));
	new_string->wordlength = 0;
	new_string->arraylength = MAXWORD;
	new_string->word = calloc(CSIZE, sizeof(char) * new_string->arraylength);
	return new_string;
}

// Function to check for valid characters
static bool is_valid_char(char c){
	if(!isalnum(c) && c != '_' && c != '.' && c != '-' && c != '/'){
			// Not a valid char
			return false;
		}
	return true;
}

// Checks if a target line is valid
static bool is_valid_target_line(char * line, ssize_t length){
	bool hit_colon = false;
	for(int i = 0; i < length - 1; i++){
		if(line[i] == ':'){
			if(!hit_colon){
				hit_colon = true;
				continue;
			}
			else{
				return false;
			}
		}
		if(!is_valid_char(line[i]) && line[i] != ' ' && line[i] != '\t'){
			// Not a valid char
			return false;
		}
	}	
	return true;
}

// Function to append character to varstring
static void append_char(varstring * mystr, char c){
	assert(mystr);
	if(mystr->wordlength + 1 >= mystr->arraylength){
		// Expand the array
		expand_varstring(mystr, mystr->arraylength);
	}
	mystr->word[mystr->wordlength] = c;
	mystr->wordlength++;
	mystr->word[mystr->wordlength] = '\0';
}


// Function to get the targets in a line
static unsigned int get_words(vararray * targets, const char * line, ssize_t length, char startchar, char endchar){
	assert(targets);
	if(line[0] == ':') return false;  // Should have a target. Something bad has happened
	
	const char * walker = line;
	while(*walker != startchar){
		walker++;
	}
	if(startchar == ':') walker++;  // get past colon if adding dependencies

	unsigned int num_targets = 0;
	// Count how many targets there are
	while(*walker != endchar){
		if(is_valid_char(*walker) &&
		   (*(walker + 1) == ' ' || *(walker + 1) == endchar || *(walker + 1) == '\t')){
			num_targets++;
		}
		walker++;
	}

	if(startchar == ':' && num_targets == 0){
		// There are no dependencies
		return true;
	}

	walker = line;    // Reset the walker back to beginning
	while(*walker != startchar){
		walker++;
	}
	if(startchar == ':') walker++;
	
	// Create memory for the number of targets and add word
	targets->words = calloc(num_targets, sizeof(varstring *));
	for(int i = 0; i < num_targets; i++){
		targets->words[i] = new_varstring();

		// Skip empty spaces
		while(!is_valid_char(*walker)){
			walker++;
		}

		// At a valid character, start adding
		while(*walker != ' ' && *walker != endchar && *walker != '\t'){
			append_char(targets->words[i], *walker);
			walker++;
		}
	  
	}

	targets->cursize = num_targets;

	return true;
}

// Function to get recipe
static bool get_recipe(vararray * recipe_array, const char * recipe, ssize_t length){
	if(recipe[0] != '\t'){
		// Something bad has happened
		return false;
	}

	// Make size for recipe
	recipe_array->words = realloc(recipe_array->words, sizeof(varstring *) * \
								  (recipe_array->cursize + 1));
	recipe_array->words[recipe_array->cursize] = new_varstring();

	const char * walker = &(recipe[1]);    // Skip the \t character
	while(*walker != '\n'){
		append_char(recipe_array->words[recipe_array->cursize], *walker);
		walker++;
	}
	
	recipe_array->cursize += 1;

	return true;
	
}

static const char ** vararray_to_list(vararray * array){
	const char ** list = calloc(array->cursize, sizeof(char *));
	for(int i = 0; i < array->cursize; i++){
		list[i] = array->words[i]->word;
	}

	return list;
}

static bool process_rule(vararray * t, vararray * d, vararray * r,
						 const mfp_cb_t * cb, void * extradata){

	const char ** t_list;
	const char ** d_list;
	const char ** r_list;

	t_list = vararray_to_list(t);
	d_list = vararray_to_list(d);
	r_list = vararray_to_list(r);

				
	if(cb){
		if(!cb->rule_cb(extradata, t_list, t->cursize,			\
						d_list, d->cursize,					\
						r_list, r->cursize)){
			return false;
		}
	}
														 
	free_list(t);
	free_list(d);
	free_list(r);
	
	free(t_list);
	free(d_list);
	free(r_list);

	return true;
}


bool mfp_parse(FILE * f, const mfp_cb_t * cb, void * extradata){
	char * line = NULL;     // Will point to each line in the makefile
	size_t len = 0;         // Specifying to getline() to go to end of the line
	ssize_t read;           // Getting how many bytes were read
	bool target = false; // See if we are actively in a target or not
	bool first = true;      // See if we are at the first target or not

	bool exit_status = true;

	vararray * targets = NULL;
	vararray * dependencies = NULL;
	vararray * recipies = NULL;
	
	while((read = getline(&line, &len, f)) != -1){
		read = strip_whitespace_beg(&line, read);
		read = strip_comments(&line, read);

        // If the first character is a newline just continue
		if(line[0] == '\n'){
			continue;
		}

		// Check if there's a comment
		if(line[0] == '#'){
			continue;
		}

		// Check to see if it's a target line or rule line
		if(line[0] != '\t'){
			for(int i = 0; i < read; i++){
				if(line[i] == ':'){
					if(!is_valid_target_line(line, read)){
						// Invlaid character
						printf("Error: Invalid char in target or dependency.\n");
						exit_status = false;
						goto end;
					}
					target = true;
				}
			}
			if(!target){
				// something wrong happened
				printf("Error: Line not target/dependency or rule\n");
				exit_status = false;
				goto end;
			}
		} else if (line[0] == '\t' && line[1] == '\n'){
			// No rule and can skip
			continue;\
		}

		if(target){		 
			if(first){
				// first target encountered -> no need to send previous stuff to function
				targets = new_vararray();
				dependencies = new_vararray();
				recipies = new_vararray();
				
				first = false;

			} else {
				if(!process_rule(targets, dependencies, recipies, cb, extradata)){
					printf("Error: Unable to process rule\n");
					exit_status = false;
					goto end;
				}

			}

			// Read the targets and dependencies
			if(!get_words(targets, line, read, line[0], ':') ||
			   !get_words(dependencies, line, read, ':', line[read-1])){
				// Error occured with invalid character in targets
				printf("Error: Unable to get targets/dependencies\n");
				exit_status = false;
				goto end;
			}

		} else {
			// In the rule
			if(!get_recipe(recipies, line, read)){
				// Something bad happened
				printf("Error: Unable to get recipe\n");
				exit_status = false;
				goto end;
			}
			
		}
		if(target) target = false;

	}

	if(targets->cursize > 0){
		if(!process_rule(targets, dependencies, recipies, cb, extradata)){
			printf("Error: Unable to process last rule\n");
			exit_status = false;
		}
	}
	
end:
	if(targets)free_list(targets);
	if(dependencies)free_list(dependencies);
	if(recipies)free_list(recipies);
	if(line) free(line);
	if(targets) free_vararray(targets);
	if(dependencies) free_vararray(dependencies);
	if(recipies) free_vararray(recipies);
	return exit_status;
}



