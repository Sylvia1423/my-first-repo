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

#define N 360

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
/*  init matrix									                             */
/* ************************************************************************* */
static void
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

/* ************************************************************************* */
/*  init matrix reading file					         	                             */
/* ************************************************************************* */
static
void
read_matrix (double** matrix)
{

    int fd, cl;
    int i;

	(void)matrix;

	fd = open("matrix.out", O_RDWR);

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
calculate (double** matrix, int iterations, int threads)
{
	int i, j, k, l;
	int tid;
	int lines, from, to;
	int fd;
	int cl;

	//opens file matrix out,
	//if it does not exist, it will be created with read write  rights for user
	fd = open("matrix.out", O_RDWR | O_CREAT, 00777);

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
					    //nur für testzwecke
					    //matrix[i][j] = tid;

					    //eigentliche berechnung
						matrix[i][j] = cos(matrix[i][j]) * sin(matrix[i][j]) * sqrt(matrix[i][j]) / tan(matrix[i][j]) / log(matrix[i][j]) * k * l;
					}
				}

			}

            //pwrite(int fd, const void *buf, size_t count, off_t offset)
            //fd filedesc, buf data to write, count size of data, offset offset in file

            #pragma omp single nowait
            {
                gettimeofday(&start_time_iops, NULL);
            }


            for (i = from; i < to;++i)
            {
                pwrite(fd, (void *) matrix[i], N * sizeof(double), N * i * sizeof(double));
            }

			#pragma omp barrier
            #pragma omp single
            {
                gettimeofday(&end_time_iops, NULL);
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
	double time_iops = (end_time_iops.tv_sec - start_time_iops.tv_sec) + (end_time_iops.tv_usec - start_time_iops.tv_usec) * 1e-6;
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
	double** matrix;
	int file_exists; //1 if exists, 0 otherwise

	if (argc < 3)
	{
		printf("Usage: %s threads iterations\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	else
	{
		sscanf(argv[1], "%d", &threads);
		sscanf(argv[2], "%d", &iterations);
	}

	matrix = alloc_matrix();

	//check if file exists

    file_exists = cfileexists("matrix.out");

	if (file_exists)
	{
		read_matrix(matrix);
	}
	else
	{
		init_matrix(matrix);
	}

	//show_matrix(matrix);

	gettimeofday(&start_time, NULL);
	calculate(matrix, iterations, threads);
	gettimeofday(&comp_time, NULL);

	//printf("After calculation\n");
	//show_matrix(matrix);

	displayStatistics();

	free_matrix(matrix);

	return 0;
}
