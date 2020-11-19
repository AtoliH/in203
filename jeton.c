#include <mpi.h>
#include <stdio.h>

int main(int argc, char *argv[])
{
    MPI_Status status;
    int n = 0, rank, ntask;

    MPI_Init(&argc, &argv);

    MPI_Comm_size(MPI_COMM_WORLD, &ntask);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

//    if (rank == 0)
        MPI_Send(&n, 1, MPI_INT, (rank+1)%ntask, 0, MPI_COMM_WORLD);

    MPI_Recv(&n, 1, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status);
    
    printf("%d\n", ++n);

//    if (rank != 0)
	MPI_Send(&n, 1, MPI_INT, (rank+1)%ntask, 0, MPI_COMM_WORLD);

    MPI_Finalize();

    return MPI_SUCCESS;
}
