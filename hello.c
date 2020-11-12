#include <mpi.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
    int numtasks, rank;

    MPI_Init(&argc, &rgv);

    MPI_Comm_size(MPI_COMM_WORLD, &ntask);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    printf("Bonjour, je suis la tâche n°%d sur %d tâches\n", ntask, rank);

    MPI_Finalize();

    return MPI_SUCCESS;
}
