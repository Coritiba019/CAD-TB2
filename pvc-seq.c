#include <omp.h>
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

minPath *travelingSalesman(int32_t *graph, int32_t n) {
    // Create the initial path
    int32_t *path = malloc(sizeof(int32_t) * (n + 1));
    for (int32_t i = 0; i < n; i++) {
        path[i] = i;
    }
    path[n] = 0;

    minPath *minimumPath = initMinPath(n + 1);

    searchAllPaths(graph, n, 1, n - 1, path, minimumPath);

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

    if (argc < 2) {
        printf("Usage: %s <n>\n", argv[0]);
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
    minPath *minimumPath = travelingSalesman(graph, n);
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

    free(graph);
    free(minimumPath->path);
    free(minimumPath);

    return 0;
}
