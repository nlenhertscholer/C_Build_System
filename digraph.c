#include "digraph.h"
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <assert.h>

// Initial size, will allocate more if necessary
#define INITSIZE 10
// Size for calloc
#define CSIZE 1

struct vararray;
typedef struct vararray vararray;

// Node structure
struct digraph_node_t{
	vararray * children;
	unsigned int num_incoming_nodes;
	void * nodedata;
};


// Variable array that will be used to store array of void * as needed
struct vararray{
	digraph_node_t ** list;
	unsigned int cursize;
	unsigned int maxsize;
};


// Entire graph structure. Will store each node in the array and have nodes
// point to each other as needed.
struct digraph_t{
	vararray * nodes;
	digraph_destroy_cb_t cb;
};


// Function to create a vararray
static vararray * new_vararray(){
	vararray * v = calloc(CSIZE, sizeof(vararray));
	v->maxsize = INITSIZE;
	v->cursize = 0;
	v->list = calloc(v->maxsize, sizeof(digraph_node_t *));
	return v;
}

// Function to delete a vararray
static void free_vararray(vararray * v){
	assert(v);
	if(v->list){
		free(v->list);
	}
	free(v);
}

// Function to resize the vararray
static void resize_array(vararray * v, bool increase){
	assert(v);
	v->maxsize = increase ? v->maxsize * 2 : v->maxsize / 2;
	v->list = realloc(v->list, sizeof(digraph_node_t *) * v->maxsize);
}

// Function to remove NULL in the middle of an array
static void shift_array(vararray * v){
	digraph_node_t ** walker = v->list;
	unsigned int iterations = 0;
	while(*walker != NULL && iterations != v->cursize){
		walker++;
		iterations++;
	}
	if(iterations != v->cursize - 1){
		// At a null so can shit down
		while(iterations != v->cursize - 1){
			*walker = *(walker + 1);
			walker++;
			iterations++;
		}
	}
	v->cursize--;
	if(v->cursize <= v->maxsize / 2){
		resize_array(v, false);
	}

	return;
}

// Internal function to find the specific node
//static digraph_node_t * find_node(digraph_t * d, digraph_node_t * n){
//	assert(d);
//	assert(n);
//
//	unsigned int count = d->nodes->cursize;
//	for(int i = 0; i < count; i++){
//		if(n == d->nodes->list[i]){
//			return d->nodes->list[i];
//		}
//	}
//	return (digraph_node_t *) 0
//}

// Create digraph
digraph_t * digraph_create(digraph_destroy_cb_t cb){
	digraph_t * new_digraph = calloc(CSIZE, sizeof(digraph_t));
	new_digraph->nodes = new_vararray();
	new_digraph->cb = cb;
	return new_digraph;
}

// Free graph memory
void digraph_destroy(digraph_t * graph){
	assert(graph);
	for(int i = 0; i < graph->nodes->cursize; i++){
		if(graph->cb){
			graph->cb(graph->nodes->list[i]->nodedata);
		}
		free_vararray(graph->nodes->list[i]->children);
		free(graph->nodes->list[i]);
	}
	free_vararray(graph->nodes);
	free(graph);
}

// Create a digraph node
digraph_node_t * digraph_node_create(digraph_t * d, void * userdata){
	// Create the node
	digraph_node_t * n = calloc(CSIZE, sizeof(digraph_node_t));
	n->children = new_vararray();
	n->num_incoming_nodes = 0;
	n->nodedata = userdata;

	// Add it to the digraph
	if(d->nodes->cursize == d->nodes->maxsize){
		// resize the array
		resize_array(d->nodes, true);
	}
	d->nodes->list[d->nodes->cursize] = n;
	d->nodes->cursize += 1;
	return n;
}

// Destroy digraph node
void digraph_node_destroy(digraph_t * d, digraph_node_t * n){
	// First remove any connections to it
	int nodeidx = -1;
	unsigned int graphcount = d->nodes->cursize;
	digraph_node_t * curnode = NULL;

	if(n->num_incoming_nodes > 0){
		unsigned int nodecount;
		for(int i = 0; i < graphcount; i++){
			curnode = d->nodes->list[i];
			if(curnode == n){
				// At node we are looking for, can skip it
				nodeidx = i;   // but first grab it's position so we don't loop again
				continue;
			}
			nodecount = curnode->children->cursize;

			for(int j = 0; j < nodecount; j++){
				if(curnode->children->list[j] == n){
					curnode->children->list[j] = NULL;
					if(j != nodecount-1){
						shift_array(curnode->children);  // Altering of array happens here
					}
				}
			}
		}
	}

	if(nodeidx == -1){
		// find array
		for(int i = 0; i < graphcount; i++){
			curnode = d->nodes->list[i];
			if(curnode == n){
				nodeidx = i;
				break;
			}
		}
	}
	// Just to make sure
	curnode = d->nodes->list[nodeidx];

	if(d->cb){
		d->cb(curnode->nodedata);
	}
	free_vararray(curnode->children);
	free(curnode);
	d->nodes->list[nodeidx] = NULL;
	shift_array(d->nodes);
}

// Visits all the nodes as long as cb returns true
bool digraph_visit(digraph_t * g, digraph_visit_cb_t cb, void * data){
	unsigned int count = g->nodes->cursize;
	if(!count) return false;
	for(int i = 0; i < count; i++){
		if(!cb(g, g->nodes->list[i], data)){
			return false;
		}
	}

	return true;
}

// Function to return the first node for which cb returns true
digraph_node_t * digraph_find(digraph_t * g, digraph_visit_cb_t cb, void * userdata){
	unsigned int count = g->nodes->cursize;
	for(int i = 0; i < count; i++){
		if(cb(g, g->nodes->list[i], userdata)){
			return g->nodes->list[i];
		}
	}
	return NULL;
}

// Function to add a link between to nodes
void digraph_add_link(digraph_t * d, digraph_node_t * from, digraph_node_t * to){
	assert(d);
	assert(from);
	assert(to);
	assert(from != to);   // don't connect to yourself

	if(from->children->cursize == from->children->maxsize){
		resize_array(from->children, true);
	}
	from->children->list[from->children->cursize] = to;
	to->num_incoming_nodes++;
	from->children->cursize += 1;

}

// Visit each outgoing node
bool digraph_node_visit(digraph_t * d, digraph_node_t * n,
						digraph_visit_cb_t visit, void * userdata){
	if(!n){
		return false;
	}

	unsigned int count = digraph_node_outgoing_link_count(d, n);
	digraph_node_t * curnode = NULL;
	for(int i = 0; i < count; i++){
		if(!digraph_node_get_link(d, n, i, &curnode)){
			// Something bad has happened
			return false;
		}
		if(curnode){
			visit(d, curnode, userdata);
		}
	}
	return true;
}

// Retrieve the target node of the specified outgoing link
bool digraph_node_get_link(digraph_t * d, digraph_node_t * n,
						   unsigned int idx, digraph_node_t ** ret){
	if(idx < 0 || idx >= digraph_node_outgoing_link_count(d, n)){
		return false;
	}

	*ret = n->children->list[idx];
	return true;
}

// Return how many outgoing links a node has
unsigned int digraph_node_outgoing_link_count(const digraph_t * d, const digraph_node_t * n){
	return n->children->cursize;
}

// Get how many incoming nodes a node has
unsigned int digraph_node_incoming_link_count(const digraph_t * d, const digraph_node_t * n){
	return n->num_incoming_nodes;
}

// Set the data of a node to a new value and return it's old data
void * digraph_node_set_data(digraph_t * d, digraph_node_t * n, void * userdata){
	void * old_data = n->nodedata;
	n->nodedata = userdata;
	return old_data;
}

// Return the data of a node
void * digraph_node_get_data(const digraph_t * d, const digraph_node_t * n){
	return n->nodedata;
}
