//
//  main.c
//  h2o_binder
//
//  Created by Tomas Scavnicky on 25.4.2015.
//  Copyright (c) 2015 Tomas. All rights reserved.
//
// TODO: pozit free_resources() po kazdom procese,
//       nastavit potrebny chybove hlasky a kody podla zadania, pouzit waitpid aby sa
//	 neukoncovali rodicovske procesi pred potomkami
 
 
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/mman.h>
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <h2o.h> 
 
//shm id
int shm_id_vars,
    shm_id_sems;
 
 
//semafory
typedef struct
{
    sem_t mutex,
          oxyQueue,
          hydroQueue,
          barrier,
          count_inc,
          barrier_counter,
	  turnstile,
	  turnstile2,
	  bar_ct_mtx,
	  bar_ct_mtx2;       
}TSems;
TSems *sems;
 
 
//pocet vytvorenych hydro a oxy
typedef struct
{
    int global_count, //count of executed operations (I)
        N,
        GH,
        GO,
        B,
        oxygen,
        hydrogen,
        barrier_count,
	count;
    FILE *h2o;
} TVars;
TVars *vars;
 
 
int load_resources()
{
    sems = mmap(NULL, sizeof(TSems), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, 0, 0);
    vars = mmap(NULL, sizeof(TVars), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, 0, 0);
   
    if ((shm_id_vars = shmget(IPC_PRIVATE, sizeof(TVars), IPC_CREAT | IPC_EXCL | 0666)) == -1) return -1;
    if ((shm_id_sems = shmget(IPC_PRIVATE, sizeof(TVars), IPC_CREAT | IPC_EXCL | 0666)) == -1) return -1;
 
    if ((vars = (TVars *)shmat(shm_id_vars, NULL, 0)) == (void *)-1) return -1;
    if ((sems = (TSems *)shmat(shm_id_sems, NULL, 0)) == (void *)-1) return -1;
 
    if (sem_init(&sems->mutex, 1, 1) == -1) return -1;
    if (sem_init(&sems->barrier, 1, 0) == -1) return -1;
    if (sem_init(&sems->oxyQueue, 1, 0) == -1) return -1;
    if (sem_init(&sems->hydroQueue, 1, 0) == -1) return -1;
    if (sem_init(&sems->count_inc, 1, 1) == -1) return -1;
    if (sem_init(&sems->barrier_counter, 1, 1) == -1) return -1;
    if (sem_init(&sems->turnstile, 1, 0) == -1) return -1;
    if (sem_init(&sems->turnstile2, 1, 1) == -1) return -1;
    if (sem_init(&sems->bar_ct_mtx, 1, 1) == -1) return -1;
    if (sem_init(&sems->bar_ct_mtx2, 1, 1) == -1) return -1;

    return 0;
}

int free_resources()
{
    int err = 0;
    if (sem_destroy(&sems->mutex) == -1) err = -1; //skus dat za if hned return -1; malo by to skratit kod a
    if (sem_destroy(&sems->barrier) == -1) err = -1; //nemusis pouzivat int err
    if (sem_destroy(&sems->oxyQueue) == -1) err = -1;
    if (sem_destroy(&sems->hydroQueue) == -1) err = -1;
    if (sem_destroy(&sems->count_inc) == -1) err = -1;
    if (sem_destroy(&sems->barrier_counter) == -1) err = -1;
    if (sem_destroy(&sems->turnstile) == -1) err = -1;
    if (sem_destroy(&sems->turnstile2) == -1) err = -1;
    if (sem_destroy(&sems->bar_ct_mtx) == -1) err = -1;
    if (sem_destroy(&sems->bar_ct_mtx2) == -1) err = -1;
   
    if (shmctl(shm_id_vars, IPC_RMID, NULL) == -1) err = -1;
    if (shmctl(shm_id_sems, IPC_RMID, NULL) == -1) err = -1;
    if (shmdt(vars) == -1) err = -1;
    if (shmdt(sems) == -1) err = -1;
 
    return err;
}

void bond(char* NAME, int I)
{
	sem_wait(&sems->count_inc);
	fflush(vars->h2o);
	fprintf(vars->h2o, "%d: %s %d: begin bonding\n", vars->global_count, NAME, I);
	fflush(vars->h2o);
	vars->global_count++;
	sem_post(&sems->count_inc);

	usleep((rand() % vars->B*1000 + 1));

	sem_wait(&sems->count_inc);
	fflush(vars->h2o);
	fprintf(vars->h2o, "%d: %s %d: bonded\n", vars->global_count, NAME, I);
	fflush(vars->h2o);
	vars->global_count++;
	sem_post(&sems->count_inc);
}

 

void oxygen(int I)
{
    sem_wait(&sems->count_inc);
    fflush(vars->h2o);
    fprintf(vars->h2o, "%d: O %d: started\n", vars->global_count, I);               //STARTED
    fflush(vars->h2o);
    vars->global_count++;
    sem_post(&sems->count_inc);


    sem_wait(&sems->mutex);

    vars->oxygen += 1;   

    if (vars->hydrogen >= 2)
    {
        sem_wait(&sems->count_inc);
        fflush(vars->h2o);
	fprintf(vars->h2o, "%d: O %d: ready\n", vars->global_count, I);             //READY
        fflush(vars->h2o);
	vars->global_count++;
        sem_post(&sems->count_inc);
 
        sem_post(&sems->hydroQueue);
        sem_post(&sems->hydroQueue);
        vars->hydrogen -= 2;
 
        sem_post(&sems->oxyQueue);
        vars->oxygen -= 1;
    }
    else
    {
	sem_wait(&sems->count_inc);
	fflush(vars->h2o);
	fprintf(vars->h2o, "%d: O %d: waiting\n", vars->global_count, I);               //WAITING
	fflush(vars->h2o);
	vars->global_count++;
	sem_post(&sems->count_inc);

        sem_post(&sems->mutex);
    }

    sem_wait(&sems->oxyQueue);
   


    bond("O", I);


    //reusable barrier for three atoms
    sem_wait(&sems->bar_ct_mtx);
    vars->count++;
    if (vars->count == 3)
    {
	sem_wait(&sems->turnstile2);
	sem_post(&sems->turnstile);
    }
    sem_post(&sems->bar_ct_mtx);

    sem_wait(&sems->turnstile);
    sem_post(&sems->turnstile);




    sem_wait(&sems->bar_ct_mtx2);
    vars->count--;
    if (vars->count == 0)
    {
	sem_wait(&sems->turnstile);
	sem_post(&sems->turnstile2);
    }
    sem_post(&sems->bar_ct_mtx2);

    sem_wait(&sems->turnstile2);
    sem_post(&sems->turnstile2);
    sem_post(&sems->mutex);
    //reusable barrier for three atoms

    sem_wait(&sems->barrier_counter);
    vars->barrier_count++;
    //printf("barrier_count: %d\n", vars->barrier_count);
    sem_post(&sems->barrier_counter);
 
    if (vars->barrier_count == vars->N * 3) sem_post(&sems->barrier);
 
    sem_wait(&sems->barrier);
    sem_post(&sems->barrier);
    


 
    sem_wait(&sems->count_inc);
    fflush(vars->h2o);
    fprintf(vars->h2o, "%d: O %d: finished\n", vars->global_count, I);              //FINISHED
    fflush(vars->h2o);
    vars->global_count++;
    sem_post(&sems->count_inc);

    free_resources();
    _Exit(0);
 
} //oxygen
 
   
//hydrogen
void hydrogen(int I)
{
    sem_wait(&sems->count_inc);
    fflush(vars->h2o);
    fprintf(vars->h2o, "%d: H %d: started\n", vars->global_count, I);               //STARTED
    fflush(vars->h2o);
    vars->global_count++;
    sem_post(&sems->count_inc);
 


    sem_wait(&sems->mutex);
   
    vars->hydrogen += 1; 

    if (vars->hydrogen >= 2 && vars->oxygen >= 1)
    {
	sem_wait(&sems->count_inc);
        fflush(vars->h2o);
	fprintf(vars->h2o, "%d: H %d: ready\n", vars->global_count, I);             //READY
        fflush(vars->h2o);
	vars->global_count++;
        sem_post(&sems->count_inc);

        sem_post(&sems->hydroQueue);
        sem_post(&sems->hydroQueue);
        vars->hydrogen -= 2;
 
        sem_post(&sems->oxyQueue);
        vars->oxygen -= 1;
    }
    else
    {
	sem_wait(&sems->count_inc);
	fflush(vars->h2o);
	fprintf(vars->h2o, "%d: H %d: waiting\n", vars->global_count, I);               //WAITING
	fflush(vars->h2o);
	vars->global_count++;
	sem_post(&sems->count_inc);

        sem_post(&sems->mutex);
    }

 
    
    sem_wait(&sems->hydroQueue);


    bond("H", I);
 

    //reusable barrier for three atoms, when three atoms are boding into h2o molecule, 
    //no other action except 'started' may happen
    sem_wait(&sems->bar_ct_mtx);
    vars->count++;
    if (vars->count == 3)
    {
	sem_wait(&sems->turnstile2);
	sem_post(&sems->turnstile);
    }
    sem_post(&sems->bar_ct_mtx);

    sem_wait(&sems->turnstile);
    sem_post(&sems->turnstile);


    //kriticka zona bariery


    sem_wait(&sems->bar_ct_mtx2);
    vars->count--;
    if (vars->count == 0)
    {
	sem_wait(&sems->turnstile);
	sem_post(&sems->turnstile2);
    }
    sem_post(&sems->bar_ct_mtx2);

    sem_wait(&sems->turnstile2);
    sem_post(&sems->turnstile2);
    //reusable barrier for three atoms



    sem_wait(&sems->barrier_counter);
    vars->barrier_count++;
    //printf("barrier_count: %d\n", vars->barrier_count);
    sem_post(&sems->barrier_counter);
 
    if (vars->barrier_count == vars->N * 3) sem_post(&sems->barrier);
    
    sem_wait(&sems->barrier);
    sem_post(&sems->barrier);


    //barrier for all atoms, befor finish
    sem_wait(&sems->count_inc);
    fflush(vars->h2o);
    fprintf(vars->h2o, "%d: H %d: finished\n", vars->global_count, I);              //FINISHED
    fflush(vars->h2o);
    vars->global_count++;
    sem_post(&sems->count_inc);
    //barrier for all atoms

    free_resources();
    _Exit(0);
 
} //hydrogen
 
 
void hydrogen_maker()
{
    int hydro_pid;
 
    //creates 2N hydrogen atom processes
    for (int i = 0; i < (vars->N)*2; i++)
    {
        hydro_pid = fork();
        if (hydro_pid == 0)
        {
            usleep((rand() % vars->GH*1000 + 1));
            hydrogen(i+1);
        }
    }
    free_resources();
    _Exit(0);
}

void oxygen_maker()
{
    int oxy_pid;

    //creates N oxygen atom processes
    for (int i = 0; i < vars->N; i++)
    {
        oxy_pid = fork();
        if (oxy_pid == 0)
        {
            usleep((rand() % vars->GO*1000 + 1));
            oxygen(i+1);
        }
    }
    free_resources();
    _Exit(0);
}
 
int parse(int argc, const char *argv[])
{
    if (argc != 5) return -1;
    char *ptr;
 
    vars->N = strtol(argv[1], &ptr, 10);        // !!! overit ci strtol vyslo !!!
    vars->GH = strtol(argv[2], &ptr, 10);
    vars->GO = strtol(argv[3], &ptr, 10);
    vars->B = strtol(argv[4], &ptr, 10);
 
    if (0 > vars->N || 5002 < vars->N) return -1;
    if (0 > vars->GH || 5002 < vars->GH) return -1;
    if (0 > vars->GO || 5002 < vars->GO) return -1;
    if (0 > vars->B || 5002 < vars->B) return -1;
    return 0;
}
 
 
int main(int argc, const char * argv[])
{
    int exclude = 0;

    if (load_resources() == -1) {fprintf(stderr, "Chyba systemovych volani.\n"); free_resources(); exit(2);}
 
    if (parse(argc, argv) == -1) {fprintf(stderr,"Nespravne zadane parametre.\n"); free_resources(); exit(1);}

    srand(time(NULL));
    vars->oxygen = 0;
    vars->hydrogen = 0;
    vars->global_count = 1;
    vars->barrier_count = 0;
    vars->count = 0;

    pid_t childPID;

    vars->h2o = fopen("h2o.out", "w+");

    //hydrogen_maker
    childPID = fork();
    if (childPID == 0)
    {
        exclude = 1;
        hydrogen_maker();
    }
    else if (childPID == -1)
    {
	fprintf(stderr, "Chyba systemovych volani.\n");
	free_resources();
	exit(2);
    }
 
    //oxygen_maker
    childPID = fork();
    if (childPID == 0 && exclude != 1)
    {
        oxygen_maker();
    }
    else if (childPID == -1)
    {
	fprintf(stderr, "Chyba systemovych volani.\n");
	free_resources();
	exit(2);
    }
 
    fclose(vars->h2o);
    free_resources();

    exit(EXIT_SUCCESS);
}
