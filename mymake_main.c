#define _XOPEN_SOURCE
#include "mymake.h"
#include "makefile_parser.h"
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>

//#define DEBUG

bool build_graph(void * userdata,
				 const char ** target, unsigned int tcount,
				 const char ** dependencies, unsigned int dcount,
				 const char ** recipe, unsigned int rcount){

	assert(userdata);
	mymake_t * this = (mymake_t *) userdata;
	for(int i = 0; i < tcount; i++){
		#ifdef DEBUG
		printf("%s\n", target[i]);
		#endif
		if(!mymake_add_target(this, target[i], dependencies, dcount,
							  recipe, rcount)){
			// Error happened
			return false;
		}
	}
	return true;

}

int main(int argc, char * argv[]){
	int c;
	bool verbose = false;
	bool dryrun = false;
	char * filename = "Makefile.mymake";    // Default value
	int exit_stat = EXIT_SUCCESS;

	while((c = getopt(argc, argv, ":hvnf:")) != -1){
		switch(c){
		case 'h':
			printf("\
Usage: mymake [-f filename] [-v] [-n] targets...\n\n\
\t-h\t\t print help\n\
\t-v\t\t enable verbose mode\n\
\t-n\t\t enable dryrun mode\n\
\t-f filename\t one argument which is the makefile to read\n\n");
			return EXIT_SUCCESS;
		case 'v':
			verbose = true;
			break;
		case 'n':
			dryrun = true;
			break;
		case 'f':
			filename = optarg;
			break;
		case ':':
			break;
		case '?':
			fprintf(stderr, "Unknown option -%c.\n", optopt);
			return EXIT_FAILURE;
			break;
		}
	}

	FILE * f = fopen(filename, "r");
	if(!f){
		fprintf(stderr, "Error opening file.\n");
		return EXIT_FAILURE;
	}

	mymake_t * m = mymake_create(stdout, stderr);
	mfp_cb_t parser;
	parser.rule_cb = build_graph;
	parser.error = stderr;
	if(!(mfp_parse(f, &parser, m))){
		exit_stat = EXIT_FAILURE;
		goto end;
	}

	int target_count = argc - optind;
	char * curtarget = NULL;
	if(target_count == 0){
		mymake_build(m, curtarget, verbose, dryrun);
	} else {
		for(int i = optind; i < argc; i++){
			curtarget = argv[i];
			mymake_build(m, curtarget, verbose, dryrun);
		}
	}

end:
	if(m)mymake_destroy(m);
	if(f)fclose(f);
	return exit_stat;
}
