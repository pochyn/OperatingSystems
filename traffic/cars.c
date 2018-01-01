#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "traffic.h"

extern struct intersection isection;

/**
 * Populate the car lists by parsing a file where each line has
 * the following structure:
 *
 * <id> <in_direction> <out_direction>
 *
 * Each car is added to the list that corresponds with 
 * its in_direction
 * 
 * Note: this also updates 'inc' on each of the lanes
 */
void parse_schedule(char *file_name) {
    int id;
    struct car *cur_car;
    struct lane *cur_lane;
    enum direction in_dir, out_dir;
    FILE *f = fopen(file_name, "r");

    /* parse file */
    while (fscanf(f, "%d %d %d", &id, (int*)&in_dir, (int*)&out_dir) == 3) {

        /* construct car */
        cur_car = malloc(sizeof(struct car));
        cur_car->id = id;
        cur_car->in_dir = in_dir;
        cur_car->out_dir = out_dir;

        /* append new car to head of corresponding list */
        cur_lane = &isection.lanes[in_dir];
        cur_car->next = cur_lane->in_cars;
        cur_lane->in_cars = cur_car;
        cur_lane->inc++;
    }

    fclose(f);
}

/**
 * TODO: Fill in this function
 *
 * Do all of the work required to prepare the intersection
 * before any cars start coming
 * 
 */
void init_intersection() {
	int i = 0;
	while (i < 4){
		//lanes locks initialization
		isection.lanes[i].lock = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
		isection.lanes[i].producer_cv = (pthread_cond_t)PTHREAD_COND_INITIALIZER;
		isection.lanes[i].consumer_cv = (pthread_cond_t)PTHREAD_COND_INITIALIZER;

		isection.lanes[i].in_cars = NULL;
		isection.lanes[i].out_cars = NULL;

		isection.lanes[i].inc = 0;
		isection.lanes[i].passed = 0;

		isection.lanes[i].buffer = malloc(sizeof(struct car *) * LANE_LENGTH);

		isection.lanes[i].head = 0;
		isection.lanes[i].tail = 0;
		isection.lanes[i].capacity = LANE_LENGTH;
		isection.lanes[i].in_buf = 0;
        
		//quad lock inizializatoin
		isection.quad[i] = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
		i++;
	}

}

/**
 * TODO: Fill in this function
 *
 * Populates the corresponding lane with cars as room becomes
 * available. Ensure to notify the cross thread as new cars are
 * added to the lane.
 * 
 */
void *car_arrive(void *arg) {
    struct lane *l = arg;
    while (l->inc > 0) {
	    pthread_mutex_lock(&l->lock);
	    //wait for empty space in buffer
	    while (l->in_buf == l->capacity){
	    	pthread_cond_wait(&l->consumer_cv, &l->lock);
	    }
	    //adding car from in_cars to buffer of correspomding lane
	    struct car *c = l->in_cars;
	    l->buffer[l->tail] = c;
	    if (l->tail < l->capacity - 1){
	        l->tail++;
	    } else {
			l->tail = 0;
	    }
	    l->in_cars = c->next;
		l->inc--;
	    l->in_buf++;
	    pthread_cond_signal(&l->producer_cv);
	    pthread_mutex_unlock(&l->lock);
	}


    return NULL;
}

/**
 * TODO: Fill in this function
 *
 * Moves cars from a single lane across the intersection. Cars
 * crossing the intersection must abide the rules of the road
 * and cross along the correct path. Ensure to notify the
 * arrival thread as room becomes available in the lane.
 *
 * Note: After crossing the intersection the car should be added
 * to the out_cars list of the lane that corresponds to the car's
 * out_dir. Do not free the cars!
 *
 * 
 * Note: For testing purposes, each car which gets to cross the 
 * intersection should print the following three numbers on a 
 * new line, separated by spaces:
 *  - the car's 'in' direction, 'out' direction, and id.
 * 
 * You may add other print statements, but in the end, please 
 * make sure to clear any prints other than the one specified above, 
 * before submitting your final code. 
 */
void *car_cross(void *arg) {
    struct lane *l = arg;
    //taking car out of corresponding buffer (to cross)
    while (l->in_buf > 0 || l->in_cars > 0) {
	    pthread_mutex_lock(&l->lock);
	    while (l->in_buf == 0) {
	    	pthread_cond_wait(&l->producer_cv, &l->lock);
	    }
	    
	    struct car *c = l->buffer[l->head];
	   	if (l->head < l->capacity - 1){
	   	    l->head++;
	   	} else {
	   	    l->head = 0;
	   	}
	    l->in_buf--;
	    pthread_cond_signal(&l->consumer_cv);
	    pthread_mutex_unlock(&l->lock);

	    //compute path to lock needed quadrants
	    printf("%d %d %d\n", c->in_dir, c->out_dir, c->id);
	    int *path = compute_path(c->in_dir, c->out_dir);

	    //lock quadrants and add car to out_cars
	    int i = 0;
	    while (i < 4){
	    	if (path[i] != -1){
	    		pthread_mutex_lock(&isection.quad[path[i]]);
	    	}
	        i++;
	    }
	    struct lane *out = &isection.lanes[c->out_dir];
	    pthread_mutex_lock(&out->lock);
	    c->next = out->out_cars;
	    out->out_cars = c;
	    out->passed++;
	    pthread_mutex_unlock(&out->lock);
	
	    //car crossed - unlock quadrants and free path
		i = 0;
	    while (i < 4){
	    	if (path[i] != -1){
	    		pthread_mutex_unlock(&isection.quad[path[i]]);
	    	}
	        i++;
	    }
	    free(path);
	}

    return NULL;
}

/**
 * TODO: Fill in this function
 *
 * Given a car's in_dir and out_dir return a sorted 
 * list of the quadrants the car will pass through.
 * 
 */
int *compute_path(enum direction in_dir, enum direction out_dir) {
    int *path = malloc(4 * sizeof(int));
	if (in_dir == NORTH) {
		if (out_dir == NORTH) {
			path[0] = 1;
			path[1] = 2;
			path[2] = 3;
			path[3] = 0;
		}
		if (out_dir == WEST) {
			path[0] = 1;
			path[1] = -1;
			path[2] = -1;
			path[3] = -1;
		}
		if (out_dir == SOUTH) {
			path[0] = 1;
			path[1] = 2;
			path[2] = -1;
			path[3] = -1;
		}
		if (out_dir == EAST) {
			path[0] = 1;
			path[1] = 2;
			path[2] = 3;
			path[3] = -1;
		}
	}

	if (in_dir == WEST) {
		if (out_dir == NORTH) {
			path[0] = 2;
			path[1] = 3;
			path[2] = 0;
			path[3] = -1;
		}
		if (out_dir == WEST) {
			path[0] = 2;
			path[1] = 3;
			path[2] = 0;
			path[3] = 1;
		}
		if (out_dir == SOUTH) {
			path[0] = 2;
			path[1] = -1;
			path[2] = -1;
			path[3] = -1;
		}
		if (out_dir == EAST) {
			path[0] = 2;
			path[1] = 3;
			path[2] = -1;
			path[3] = -1;
		}
	}

	if (in_dir == SOUTH) {
		if (out_dir == NORTH) {
			path[0] = 3;
			path[1] = 0;
			path[2] = -1;
			path[3] = -1;
		}
		if (out_dir == WEST) {
			path[0] = 3;
			path[1] = 0;
			path[2] = 1;
			path[3] = -1;
		}
		if (out_dir == SOUTH) {
			path[0] = 3;
			path[1] = 0;
			path[2] = 1;
			path[3] = 2;
		}
		if (out_dir == EAST) {
			path[0] = 3;
			path[1] = -1;
			path[2] = -1;
			path[3] = -1;
		}
	}

	if (in_dir == EAST) {
		if (out_dir == NORTH) {
			path[0] = 0;
			path[1] = -1;
			path[2] = -1;
			path[3] = -1;
		}
		if (out_dir == WEST) {
			path[0] = 0;
			path[1] = 1;
			path[2] = -1;
			path[3] = -1;
		}
		if (out_dir == SOUTH) {
			path[0] = 0;
			path[1] = 1;
			path[2] = 2;
			path[3] = -1;
		}
		if (out_dir == EAST) {
			path[0] = 0;
			path[1] = 1;
			path[2] = 2;
			path[3] = 3;
		}
	}

    return path;
}
