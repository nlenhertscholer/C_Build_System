#include "mymake.h"
#include "digraph.h"
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "util.h"

#define CSIZE 1

// Structure which will be hold in digraph_node_t as nodedata
typedef struct target{
	char * name;
	char ** recipies;
	unsigned int rcount;
} target;


static target * new_target(const char * name, const char ** recipies, const unsigned int rcount){
	assert(name);

	target * t = calloc(CSIZE, sizeof(target));

	// Add the target name
	int length = strlen(name) + 1;
	t->name = calloc(length, sizeof(char));
	strncpy(t->name, name, length);
	//for(int i = 0; i < namelength; i++){
	//	t->name[i] = name[i];
	//}

	// Add the recipies
	if(recipies){
		t->recipies = calloc(rcount, sizeof(char *));
		const char * currecipe = NULL;
		for(int i = 0; i < rcount; i++){
			currecipe = recipies[i];

			length = strlen(currecipe) + 1;
			t->recipies[i] = calloc(length, sizeof(char));
			strncpy(t->recipies[i], currecipe, length);

		}
	} else {
		t->recipies = NULL;
	}

	t->rcount = rcount;
	return t;
}

static void free_target(void * t){
	assert(t);
	target * myt = (target *)t;
	free(myt->name);
	for(int i = 0; i < myt->rcount; i++){
		free(myt->recipies[i]);
	}
	free(myt->recipies);
	free(myt);
}

// Function used by cb in digraph_find function to see if a node is in the graph
bool findnode(digraph_t * d, digraph_node_t * node, void * userdata){
	char * thisname = (char *) userdata;
	target * t = (target *) digraph_node_get_data(d, node);
	if(strncmp(t->name, thisname, strlen(thisname)) == 0){
		return true;
	}
	return false;
}

struct mymake_t{
	FILE * output;
	FILE * error;
	digraph_t * graph;
	digraph_node_t * firstnode;
};

mymake_t * mymake_create(FILE * output, FILE * error){
	mymake_t * make = calloc(CSIZE, sizeof(mymake_t));
	make->output = output;
	make->error = error;
	make->graph = digraph_create(free_target);
	make->firstnode = NULL;

	return make;
}

bool mymake_supports_variables(){
	return false;
}

bool mymake_add_variable(mymake_t * m, const char * varname, const char * val){
	fprintf(m->error, "Error: This make does not support variables\n");
	return false;
}

bool mymake_add_target(mymake_t * m, const char * name, const char ** deps,
					   unsigned int depcount, const char ** recipe, unsigned int recipecount){
	assert(m);
	assert(name);

	// Check to see if target is in the graph already
	digraph_node_t * search_node = digraph_find(m->graph, findnode, (void *)name);
	if(search_node){
		// Check to see if there's a recipe
		target * oldt = (target *)digraph_node_get_data(m->graph, search_node);
		if(oldt->rcount != 0 || oldt->recipies != NULL){
			// Can't have two recipies
			fprintf(m->error, "Error: Multiple recipies for %s detected.\n", name);
			return false;
		}
	}

	// Target node
	target * t = new_target(name, recipe, recipecount);
	digraph_node_t * target_node = NULL;
	target * junk_node = NULL;


	if(!search_node){
		// It's not in the graph so add it
		target_node = digraph_node_create(m->graph, (void *)t);
		if(!(m->firstnode)){
			// This is the first node added
			m->firstnode = target_node;
		}
	} else {
		// In the graph and just need to change the data it points too
		junk_node = digraph_node_set_data(m->graph, search_node, t);
		free_target(junk_node);
		target_node = digraph_find(m->graph, findnode, (void *)name);
	}

	// Add its dependencies
	target * d = NULL;
	for(int i = 0; i < depcount; i++){
		// If it's not in the graph add it
		search_node = digraph_find(m->graph, findnode, (void *)deps[i]);
		if(!search_node){
			d = new_target(deps[i], NULL, 0);
			search_node = digraph_node_create(m->graph, (void *)d);
		}

		// Add the link
		digraph_add_link(m->graph, target_node, search_node);
	}
	return true;
}

typedef struct node_array{
	digraph_node_t ** nodes;
	unsigned int cursize;
	unsigned int maxsize;
} node_array;

static node_array * new_node_array(unsigned int size){
	node_array * node = calloc(CSIZE, sizeof(node_array));
	node->nodes = calloc(size, sizeof(digraph_node_t *));
	node->cursize = 0;
	node->maxsize = size;
	return node;
}

static void free_node_array(node_array * node){
	free(node->nodes);
	free(node);
}

static void resize_node(node_array * node){
	node->nodes = realloc(node->nodes, sizeof(digraph_node_t *) * (node->maxsize * 2));
	node->maxsize = node->maxsize * 2;
}

static bool visited(node_array * node, digraph_node_t * checknode){
	unsigned int size = node->cursize;
	for(int i = 0; i < size; i++){
		if(node->nodes[i] == checknode){
			return true;
		}
	}
	return false;
}

static void add_node(node_array * node, digraph_node_t * newnode){
	if(node->cursize == node->maxsize){
		resize_node(node);
	}
	node->nodes[node->cursize] = newnode;
	node->cursize++;
}

// Does the check to see if timestamps need to be built
static bool check_timestamp(const char * depname, const char * curname){
	assert(depname);
	assert(curname);
	if(last_modification(depname) == 0 ||
	   last_modification(depname) > last_modification(curname)){
		return true;
	}
	return false;
}

static bool build(mymake_t * m, digraph_node_t * node, bool verbose,
				  bool dryrun, bool isfirst, node_array * visited_nodes){

	// Check if we've already been here
	target * data = (target *)digraph_node_get_data(m->graph, node);
	if(visited(visited_nodes, node)){
		if(verbose) fprintf(m->output, "Cycle detected on %s. Skipping\n", data->name);
		return true;
	} else {
		// Add the node
		add_node(visited_nodes, node);
	}

	// Check if it has a target or not
	if(data->rcount == 0 && !isfirst){
		if(last_modification(data->name) == 0){
			fprintf(m->output, "No rule to build %s...\n", data->name);
			return false;
		} else {
			// .h or .c file
			return true;
		}
	}

	// Traverse dependencies if there are any and build those
	unsigned int num_deps = digraph_node_outgoing_link_count(m->graph, node);
	digraph_node_t * nextnode = NULL;
	target * dependency_data = NULL;
	bool built = false;

	if(num_deps == 0){
		// No dependency, just check if we need to build this file
		if(last_modification(data->name) == 0){
			return execute_recipe((const char **)data->recipies, data->rcount,
								  m->output, m->error, dryrun);

		} else {
			return false;
		}
	} else {
		for(int i = 0; i < num_deps; i++){
			if(!digraph_node_get_link(m->graph, node, i, &nextnode)){
				fprintf(m->error, "Error getting dependencies for %s.\n", data->name);
				return false;
			}

			// Get the last_modification time for the dependency
			dependency_data = (target *)digraph_node_get_data(m->graph, nextnode);
			if(check_timestamp(dependency_data->name, data->name)){
				// build the dependency
				if(verbose) fprintf(m->output, "Building: Dependency %s is newer than its target %s.\n", dependency_data->name, data->name);
				if(build(m, nextnode, verbose, dryrun, false, visited_nodes)){
					built = true;
				}
			} else {
				if(verbose) fprintf(m->output, "Not Building: Dependency %s is not newer than its target %s.\n", dependency_data->name, data->name);
			}
		}
	}


	if(built){
		// build this target
		if(verbose) fprintf(m->output, "Building Target %s.\n", data->name);
		if(isfirst && data->rcount == 0){
			return true;
		}
		return execute_recipe((const char **)data->recipies, data->rcount,
							  m->output, m->error, dryrun);
	} else {
		if(verbose) fprintf(m->output, "No criteria met for building target %s.\n", data->name);
		if(isfirst) fprintf(m->output, "No need to build %s...\n", data->name);
		return false;
	}

}

bool mymake_build(mymake_t * m, const char * target, bool verbose, bool dryrun){
	digraph_node_t * target_node = NULL;
	if(target){
		target_node = digraph_find(m->graph, findnode, (void *)target);
		if(!target_node){
			fprintf(m->error, "Error: Unable to find target %s.\n", target);
			return false;
		}
	} else {
		target_node = m->firstnode;
		if(!target_node){
			return true;
		}
	}

	// Build the node
	unsigned int size = digraph_node_outgoing_link_count(m->graph, target_node);
	node_array * checked_nodes = new_node_array(size+1);
	bool built = build(m, target_node, verbose, dryrun, true, checked_nodes);
	free_node_array(checked_nodes);
	return built;


}

void mymake_destroy(mymake_t * m){
	digraph_destroy(m->graph);
	free(m);
}
