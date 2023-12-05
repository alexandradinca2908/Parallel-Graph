// SPDX-License-Identifier: BSD-3-Clause

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <time.h>

#include "os_graph.h"
#include "os_threadpool.h"
#include "log/log.h"
#include "utils.h"

#define NUM_THREADS		4

static int sum;
static os_graph_t *graph;
static os_threadpool_t *tp;
int nrNodes;

/* TODO: Define graph synchronization mechanisms. */
pthread_mutex_t sum_mutex;
pthread_mutex_t neighbour_mutex;
pthread_mutex_t nrNodes_mutex;

/* TODO: Define graph task argument. */
void task_argument(void *node)
{
	os_node_t *crtNode = (os_node_t *)node;

	//  Add the current info to the overall sum
	pthread_mutex_lock(&sum_mutex);
	sum += crtNode->info;
	pthread_mutex_unlock(&sum_mutex);

	//  Mark as done
	graph->visited[crtNode->id] = DONE;

	for (unsigned int i = 0; i < crtNode->num_neighbours; i++) {
		pthread_mutex_lock(&neighbour_mutex);

		os_node_t *crtNeighbour = graph->nodes[crtNode->neighbours[i]];

		if (graph->visited[crtNeighbour->id] == NOT_VISITED) {
			graph->visited[crtNeighbour->id] = PROCESSING;

			pthread_mutex_unlock(&neighbour_mutex);

			os_task_t *task = create_task(task_argument, (void *)crtNeighbour, NULL);
			enqueue_task(tp, task);

		} else {
			pthread_mutex_unlock(&neighbour_mutex);
		}
	}

	pthread_mutex_lock(&nrNodes_mutex);
	nrNodes--;
	pthread_mutex_unlock(&nrNodes_mutex);

	if (nrNodes == 0) {
		tp->noTaskLeft = 1;
	}
}

static void process_node(unsigned int idx)
{
	/* TODO: Implement thread-pool based processing of graph. */

	os_node_t *nodeZero = graph->nodes[idx];
	
	os_task_t *task = create_task(task_argument, nodeZero, NULL);

	enqueue_task(tp, task);
}

void count_nodes(os_graph_t *graph, int idx, int *visited) {
	visited[idx] = 1;

	int neighbour;

	for (unsigned int i = 0; i < graph->nodes[idx]->num_neighbours; i++) {
		neighbour = graph->nodes[idx]->neighbours[i];

		if (visited[neighbour] == 0) {
			nrNodes++;
			count_nodes(graph, neighbour, visited);
		}		
	}
}

int main(int argc, char *argv[])
{
	FILE *input_file;

	if (argc != 2) {
		fprintf(stderr, "Usage: %s input_file\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	input_file = fopen(argv[1], "r");
	DIE(input_file == NULL, "fopen");

	graph = create_graph_from_file(input_file);

	//  Count how many nodes are connected to node 0
	int visited[graph->num_nodes];
	for (unsigned int i = 0; i < graph->num_nodes; i++) {
		visited[i] = 0;
	}

	nrNodes++;
	count_nodes(graph, 0, visited);

	/* TODO: Initialize graph synchronization mechanisms. */
	tp = create_threadpool(NUM_THREADS);
	process_node(0);
	wait_for_completion(tp);
	destroy_threadpool(tp);

	printf("%d", sum);

	return 0;
}
