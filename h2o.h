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

int load_resources();

int free_resources();

void bond(char* NAME, int I);

void oxygen(int I);

void hydrogen(int I);

void hydrogen_maker();

void oxygen_maker();

int parse(int argc, const char *argv[]);

