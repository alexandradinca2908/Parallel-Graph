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

/* TODO: Define graph synchronization mechanisms. */
pthread_mutex_t sum_mutex;
pthread_mutex_t neighbour_mutex;

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

	//  There are still unvisited nodes
	//  We look for them in the crt node's neighbours
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
}

static void process_node(unsigned int idx)
{
	/* TODO: Implement thread-pool based processing of graph. */

	os_node_t *nodeZero = graph->nodes[idx];

	os_task_t *task = create_task(task_argument, nodeZero, NULL);

	enqueue_task(tp, task);
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

	/* TODO: Initialize graph synchronization mechanisms. */
	pthread_mutex_init(&sum_mutex, NULL);
	pthread_mutex_init(&neighbour_mutex, NULL);

	tp = create_threadpool(NUM_THREADS);
	process_node(0);
	wait_for_completion(tp);
	destroy_threadpool(tp);

	pthread_mutex_destroy(&sum_mutex);
	pthread_mutex_destroy(&neighbour_mutex);

	printf("%d", sum);

	return 0;
}
