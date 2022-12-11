#include <mpi.h>
#include <omp.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INF INT32_MAX

#define MAX_DISTANCE 100

#define flat2d(i, j, n) ((i) * (n) + (j))

typedef struct minPath {
    int32_t *path;
    int32_t distance;
} minPath;

minPath *initMinPath(int32_t n) {
    minPath *minimumPath = malloc(sizeof(minPath));
    minimumPath->path = malloc(sizeof(int32_t) * (n));
    minimumPath->distance = INF;
    return minimumPath;
}

int32_t randomInt(int32_t min, int32_t max) {
    return rand() % (max - min + 1) + min;
}

int32_t randomDistance() {
    int32_t distance = randomInt(0, MAX_DISTANCE);
    if (distance == 0) {
        return INF;
    }
    return distance;
}

int32_t *generateGraph(int32_t n) {
    int32_t *graph = malloc(sizeof(int32_t) * n * n);
    for (int32_t i = 0; i < n; i++) {
        for (int32_t j = 0; j < n; j++) {
            if (i == j) {
                graph[flat2d(i, j, n)] = 0;
            } else {
                graph[flat2d(i, j, n)] = randomDistance();
            }
        }
    }
    return graph;
}

int32_t *readGraph(int32_t n) {
    int32_t *graph = malloc(sizeof(int32_t) * n * n);
    for (int32_t i = 0; i < n; i++) {
        for (int32_t j = 0; j < n; j++) {
            scanf("%d", &graph[flat2d(i, j, n)]);
        }
    }
    return graph;
}

void swap(int32_t *a, int32_t *b) {
    int32_t temp = *a;
    *a = *b;
    *b = temp;
}

int32_t pathDistance(int32_t *path, int32_t pathSize, int32_t *graph,
                     int32_t n) {
    int32_t distance = 0;
    for (int32_t i = 0; i < pathSize - 1; i++) {
        if (graph[flat2d(path[i], path[i + 1], n)] != INF) {
            distance += graph[flat2d(path[i], path[i + 1], n)];
        } else {
            return INF;
        }
    }
    return distance;
}

void searchAllPaths(int32_t *graph, int32_t n, int32_t start, int32_t end,
                    int32_t *currentPath, minPath *minimumPath) {
    if (start == end) {
        // Calculate the distance of the current permutation
        int32_t currDistance = pathDistance(currentPath, n + 1, graph, n);

        // Update the minimum path if the current path is shorter
        if (currDistance < minimumPath->distance) {
            minimumPath->distance = currDistance;
            memcpy(minimumPath->path, currentPath, sizeof(int32_t) * (n + 1));
        }
        return;
    }

    // Generate all permutations
    for (int32_t i = start; i <= end; i++) {
        swap(currentPath + start, currentPath + i);
        searchAllPaths(graph, n, start + 1, end, currentPath, minimumPath);
        swap(currentPath + start, currentPath + i);
        // Undo the swap to backtrack
    }
}

minPath *localTravelingSalesman(int32_t *graph, int32_t n, int32_t init,
                                int32_t end) {

    // Create the initial path
    int32_t *path = malloc(sizeof(int32_t) * (n + 1));
    for (int32_t i = 0; i < n; i++) {
        path[i] = i;
    }
    path[n] = 0;

    minPath *minimumPath = initMinPath(n + 1);

    for (int32_t i = init; i <= end; i++) {
        swap(path + 1, path + i);
        if (n > 2) {
            searchAllPaths(graph, n, 2, n - 1, path, minimumPath);
        } else {
            minimumPath->distance = pathDistance(path, n + 1, graph, n);
            memcpy(minimumPath->path, path, sizeof(int32_t) * (n + 1));
        }
        swap(path + 1, path + i);
    }

    free(path);
    return minimumPath;
}

void printGraph(int32_t *graph, int32_t n) {
    for (int32_t i = 0; i < n; i++) {
        for (int32_t j = 0; j < n; j++) {
            if (graph[flat2d(i, j, n)] == INF) {
                printf("INF ");
            } else {
                printf("%d ", graph[flat2d(i, j, n)]);
            }
        }
        printf("\n");
    }
}

void printPath(int32_t *path, int32_t pathSize) {

    for (int32_t i = 0; i < pathSize; i++) {
        printf("%d ", path[i]);
    }
    printf("\n");
}

int main(int argc, char *argv[]) {

    int32_t n;
    int32_t *graph;
    double timeStart, timeEnd;
    int32_t mpiSize, mpiRank, mpiProvidedThreadSupport;
    MPI_Status mpiStatus;

    MPI_Init_thread(&argc, &argv, MPI_THREAD_SINGLE, &mpiProvidedThreadSupport);

    MPI_Comm_size(MPI_COMM_WORLD, &mpiSize);
    MPI_Comm_rank(MPI_COMM_WORLD, &mpiRank);

    if (mpiRank == 0) {
        if (argc < 2) {
            printf("Usage: mpirun %s <n>\n", argv[0]);
            MPI_Abort(MPI_COMM_WORLD, 1);
            return 1;
        }

        n = atoi(argv[1]);

        srand(42);

#ifdef DEBUG
        graph = readGraph(n);
#else
        graph = generateGraph(n);
#endif

#ifdef DEBUG
        printGraph(graph, n);
#endif

        timeStart = omp_get_wtime();

        MPI_Bcast(&n, 1, MPI_INT, 0, MPI_COMM_WORLD);
        MPI_Bcast(graph, n * n, MPI_INT, 0, MPI_COMM_WORLD);
    }

    else {
        MPI_Bcast(&n, 1, MPI_INT, 0, MPI_COMM_WORLD);
        graph = malloc(sizeof(int32_t) * n * n);
        MPI_Bcast(graph, n * n, MPI_INT, 0, MPI_COMM_WORLD);
    }

    // Number of vertices to be assigned to each rank
    int32_t workLoad = (n - 1) < mpiSize ? 1 : (n - 1) / mpiSize;
    minPath *localMinPath = NULL;
    minPath *minimumPath = NULL;

    // If the number of vertices is not divisible by the number of ranks, the
    // last rank will have to process the paths starting from the remaining
    // vertices
    if (mpiRank == mpiSize - 1 && (n - 1) >= mpiSize) {

        int32_t remainingVertices = ((n - 1) % mpiSize) + workLoad;

        localMinPath =
            localTravelingSalesman(graph, n, workLoad * mpiRank + 1,
                                   workLoad * mpiRank + remainingVertices);
    }
    // Each rank will process paths starting from a portion of the vertices
    else if (mpiRank < n - 1) {
        localMinPath = localTravelingSalesman(graph, n, workLoad * mpiRank + 1,
                                              workLoad * (mpiRank + 1));
    }

    // Each rank sends its minimum path to rank 0 and rank 0 selects the global
    // minimum path
    if (mpiRank != 0) {
        if (localMinPath != NULL) {
            MPI_Send(&localMinPath->distance, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
            MPI_Send(localMinPath->path, n + 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
        }
    } else {

        int32_t receives = (n - 1) < mpiSize ? n - 1 : mpiSize;
        minimumPath = initMinPath(n + 1);
        minimumPath->distance = localMinPath->distance;
        memcpy(minimumPath->path, localMinPath->path,
               sizeof(int32_t) * (n + 1));
        for (int32_t i = 1; i < receives; i++) {
            int32_t receivedDistance;
            int32_t *receivedPath = malloc(sizeof(int32_t) * (n + 1));
            MPI_Recv(&receivedDistance, 1, MPI_INT, i, 0, MPI_COMM_WORLD,
                     &mpiStatus);
            MPI_Recv(receivedPath, n + 1, MPI_INT, i, 0, MPI_COMM_WORLD,
                     &mpiStatus);
            if (receivedDistance < minimumPath->distance) {
                minimumPath->distance = receivedDistance;
                memcpy(minimumPath->path, receivedPath,
                       sizeof(int32_t) * (n + 1));
            }
            free(receivedPath);
        }
    }

    if (mpiRank == 0) {
        timeEnd = omp_get_wtime();

        if (minimumPath->distance == INF) {
            printf("Não existe caminho possível\n");
        } else {
            printf("Menor distancia: %d\n", minimumPath->distance);

            printf("Caminho: ");
            printPath(minimumPath->path, n + 1);
        }

        printf("Tempo de resposta sem considerar E/S, em segundos: %lfs\n",
               timeEnd - timeStart);
    }

    free(graph);

    if (localMinPath != NULL) {
        free(localMinPath->path);
        free(localMinPath);
    }

    if (minimumPath != NULL) {
        free(minimumPath->path);
        free(minimumPath);
    }

    MPI_Finalize();

    return 0;
}
