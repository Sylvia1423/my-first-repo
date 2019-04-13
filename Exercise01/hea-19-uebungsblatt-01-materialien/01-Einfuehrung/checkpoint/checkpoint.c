/* ************************************************************************ */
/* Include standard header file.                                            */
/* ************************************************************************ */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <malloc.h>
#include <errno.h>
#include <sys/time.h>
#include <time.h>
#include <omp.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>

#define DEBUG 0

#if DEBUG
#define N 10
#else
#define N 360
#endif

/* time measurement variables */
struct timeval start_time;       /* time when program started               */
struct timeval comp_time;        /* time when calculation complet           */
struct timeval start_time_iops;
struct timeval end_time_iops;


/* ************************************************************************ */
/*  allocate matrix of size N x N                                           */
/* ************************************************************************ */
static
double**
alloc_matrix (void)
{
	double** matrix;
	int i;

	// allocate pointer array to the beginning of each row
	// rows
	matrix = malloc(N * sizeof(double*));

	if (matrix == NULL)
	{
		printf("Allocating matrix lines failed!\n");
		exit(EXIT_FAILURE);
	}

	// allocate memory for each row and assig it to the frist dimensions pointer
	// columns
	for (i = 0; i < N; i++)
	{
		matrix[i] = malloc(N * sizeof(double));

		if (matrix[i] == NULL)
		{
			printf("Allocating matrix elements failed!\n");
			exit(EXIT_FAILURE);
		}
	}

	return matrix;
}

/* ************************************************************************* */
/*  free matrix								                                 */
/* ************************************************************************* */
static void free_matrix (double **matrix)
{
	int i;

	// free memory for allocated for each row
	for (i = 0; i < N; i++)
	{
		free(matrix[i]);
	}
	// free pointer array to the beginning of each row
	free(matrix);
}

/* ************************************************************************* */
/*  init matrix									                             */
/* ************************************************************************* */
static
void
init_matrix (double** matrix)
{
	int i, j;

	srand(time(NULL));

	for (i = 0; i < N; i++)
	{
		for (j = 0; j < N; j++)
		{
			matrix[i][j] = 10 * j;
		}
	}
}


/* ************************************************************************* */
/*  show matrix									                             */
/* ************************************************************************* */
#if DEBUG
static
void
show_matrix(double** matrix)
{
   //for debugging purpose only

   int i,j;

   for(i = 0; i < N;++i)
   {
       for(j = 0;j < N;++j)
       {
           printf("%f ",matrix[i][j]);
       }
       printf("\n");
   }
}
#endif
/* ************************************************************************* */
/*  init matrix reading file					         	                             */
/* ************************************************************************* */
static
void
read_matrix (double** matrix,char* filepath)
{

    int fd, cl;
    int i;
    //header
    int t_1,i_1,i_c;

	(void)matrix;

	fd = open(filepath, O_RDWR);

	//read header
    read(fd,&t_1,sizeof(int));
    read(fd,&i_1,sizeof(int));
    read(fd,&i_c,sizeof(int));

    //handle cases listed in exercise

    // t1 != t2, does this matter?

    // i_c < i_1 : first run was interrupted somehow
    //TODO, if atomic writing: do nothing, start from iteration i_c until i_2
    // if no atomic writing: data may be corrupted

    // i_c == i_1 & i_c > i_2: first run completed, and..? does this matter?


    // i_c == i_1 & i_1 < i_2: first run completed, second run longer than first one? does this matter?



    printf("%d %d %d\n",t_1,i_1,i_c);


	//read matrix values, here sequential, use pread and a more non sequential approach

	for(i = 0; i < N;++i)
    {
            read(fd, matrix[i], N*sizeof(double));
    }

	cl = close(fd);

    if (cl == -1)
    {
        printf("%d error",cl);
    }
    
}

/* ************************************************************************ */
/*  calculate                                                               */
/* ************************************************************************ */
static
void
calculate (double** matrix, int iterations, int threads,char *filepath)
{
	int i, j, k, l;
	int tid;
	int lines, from, to;
	int fd;
	int cl;
	int last_iteration_written;

	//opens file matrix out,
	//if it does not exist, it will be created with read write  rights for user
	last_iteration_written = 0;
	fd = open(filepath, O_RDWR | O_CREAT, 00777);

	//write header, size 3 * int, order: threads, iteration, last iteration written
    pwrite(fd,(void*)&threads,sizeof(int),0);
    pwrite(fd,(void*)&iterations,sizeof(int),sizeof(int));
    pwrite(fd,(void*)&last_iteration_written,sizeof(int),2 * sizeof(int));

	tid = 0;
	lines = from = to = -1;

	// Explicitly disable dynamic teams
	omp_set_dynamic(0);
	omp_set_num_threads(threads);

	#pragma omp parallel firstprivate(tid, lines, from, to) private(k, l, i, j)
	{
		tid = omp_get_thread_num();

		lines = (tid < (N % threads)) ? ((N / threads) + 1) : (N / threads);
		from =  (tid < (N % threads)) ? (lines * tid) : ((lines * tid) + (N % threads));
		to = from + lines;

		for (k = 1; k <= iterations; k++)
		{
			for (i = from; i < to; i++)
			{
				for (j = 0; j < N; j++)
				{
					for (l = 1; l <= 4; l++)
					{
					    //only for debugging
					    matrix[i][j] = tid;

					    //actual calculaton
						//matrix[i][j] = cos(matrix[i][j]) * sin(matrix[i][j]) * sqrt(matrix[i][j]) / tan(matrix[i][j]) / log(matrix[i][j]) * k * l;
					}
				}

			}

            //pwrite(int fd, const void *buf, size_t count, off_t offset)
            //fd filedesc, buf data to write, count size of data, offset offset in file


            //first thread to reach this will execute, only this thread: single
            //other will not wait: nowait
            #pragma omp single nowait
            {
                gettimeofday(&start_time_iops, NULL);
            }


            for (i = from; i < to;++i)
            {
                //+ 3 sizeof * int because of header
                pwrite(fd, (void *) matrix[i], N * sizeof(double), N * i * sizeof(double) + 3 * sizeof(int));
            }

            //synchronize threads after writing
			#pragma omp barrier

			//measure time after all have finished and write last iteration to header
			//omp single introduces implicit barrier, doesnt matter?
            #pragma omp single
            {
                gettimeofday(&end_time_iops, NULL);
                //write last iteration
                pwrite(fd,(void*)&k,sizeof(int),2*sizeof(int));
            }
		}
	}



	cl = close(fd);

    if (cl == -1)
    {
       printf("%d error",cl);
    }
}

/* ************************************************************************ */
/*  displayStatistics: displays some statistics                             */
/* ************************************************************************ */
static
void
displayStatistics (void)
{
	double time = (comp_time.tv_sec - start_time.tv_sec) + (comp_time.tv_usec - start_time.tv_usec) * 1e-6;
	double time_iops =  (end_time_iops.tv_sec - start_time_iops.tv_sec)
	                    + (end_time_iops.tv_usec - start_time_iops.tv_usec)
	                    * 1e-6;
	double iops_per_sec = N*N / time_iops;
	double mb_per_sec = N*N*sizeof(double) * 1e-6 / time_iops;
	printf("Berechnungszeit: %fs\n", time);

	printf("Durchsatz:       %f MB/s\n", mb_per_sec);

	printf("IOPS:            %f Op/s\n", iops_per_sec);
}


/* ************************************************************************ */
/*  cfileexists: check if file already exists                               */
/*  source: http://www.zentut.com/c-tutorial/c-file-exists/                 */
/*  2019-04-12, 8:42 CET                                                    */
/* ************************************************************************ */
static
int
cfileexists(const char* filename){
    struct stat buffer;
    int exist = stat(filename,&buffer);
    if(exist == 0)
        return 1;
    else // -1
        return 0;
}



/* ************************************************************************ */
/*  main                                                                    */
/* ************************************************************************ */
int
main (int argc, char** argv)
{
	int threads, iterations;
	char filepath[256] = "";
	char standardpath[256] = "matrix.out";
	double** matrix;
	int file_exists; //1 if exists, 0 otherwise

#if DEBUG
    printf("DEBUG RUN\n");
    printf("Matrix will only be %dx%d\n",N,N);
#endif

	if (argc < 3)
	{
		printf("Usage: %s threads iterations OPTIONAL file\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	else if(argc == 3)
	{
		sscanf(argv[1], "%d", &threads);
		sscanf(argv[2], "%d", &iterations);
		//use standardpath matrix.out in same directory if no argument was given
        strncpy(filepath,standardpath,256);
	}
	else if(argc == 4)
    {
        sscanf(argv[1], "%d", &threads);
        sscanf(argv[2], "%d", &iterations);
        sscanf(argv[3], "%s", filepath);
    }

	matrix = alloc_matrix();

	//check if file exists

    file_exists = cfileexists(filepath);

	if (file_exists)
	{
	    printf("file does exist: %s\n",filepath);
		read_matrix(matrix,filepath);
	}
	else
	{
        printf("file does not exist: %s\n",filepath);
		init_matrix(matrix);
	}

#if DEBUG
	printf("After reading\n");
	show_matrix(matrix);
#endif

	gettimeofday(&start_time, NULL);
	calculate(matrix, iterations, threads,filepath);
	gettimeofday(&comp_time, NULL);

#if DEBUG
	printf("After calculation\n");
	show_matrix(matrix);
#endif

	displayStatistics();

	free_matrix(matrix);

	return 0;
}
