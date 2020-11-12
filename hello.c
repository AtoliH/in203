#include <mpi.h>
#include <stdio.h>

int main(int argc, char *argv[])
{
    int ntask, rank;

    MPI_Init(&argc, &argv);

    MPI_Comm_size(MPI_COMM_WORLD, &ntask);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    printf("Bonjour, je suis la tâche n°%d sur %d tâches\n",rank, ntask);

    MPI_Finalize();

    return MPI_SUCCESS;
}
