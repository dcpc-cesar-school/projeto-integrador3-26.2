/* carga.c - gera carga de CPU e memoria usando THREADS ou PROCESSOS.
 * Substitui o sysbench (threads) e o stress-ng (processos) no Lab 10.
 *
 * Compilar (o mesmo codigo vira dois programas):
 *   gcc -O2 -pthread -o carga_threads carga.c
 *   gcc -O2 -pthread -o carga_procs   carga.c
 * O modo e escolhido pelo nome do programa: se o nome contem "procs",
 * cria processos filhos (fork); senao, cria threads (pthread_create).
 *
 * Uso:  ./carga_threads [n] [segundos] [mb]
 *       ./carga_procs   [n] [segundos] [mb]
 * Ex.:  ./carga_threads 4 90 256   -> 1 processo com 4 threads de trabalho
 *       ./carga_procs   4 90 256   -> 1 processo pai + 4 processos filhos
 */
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

static int segundos = 60;

static void *trabalho(void *arg) {          /* ocupa 1 nucleo ate o tempo acabar */
    (void)arg;
    volatile double x = 0;
    time_t fim = time(NULL) + segundos;
    while (time(NULL) < fim)
        for (int i = 1; i < 1000000; i++) x += 1.0 / i;
    return NULL;
}

int main(int argc, char *argv[]) {
    int procs = strstr(argv[0], "procs") != NULL;
    int n     = argc > 1 ? atoi(argv[1]) : 4;
    segundos  = argc > 2 ? atoi(argv[2]) : 60;
    int mb    = argc > 3 ? atoi(argv[3]) : 0;
    if (n < 1 || segundos < 1 || mb < 0) {
        fprintf(stderr, "Uso: %s [n>=1] [segundos>=1] [mb>=0]\n", argv[0]);
        return 1;
    }

    if (mb > 0) {                            /* aloca e escreve -> aparece no RSS */
        size_t bytes = (size_t)mb * 1024 * 1024;
        char *buf = malloc(bytes);
        if (!buf) { perror("malloc"); return 1; }
        memset(buf, 1, bytes);
    }
    printf("%s: modo=%s n=%d tempo=%ds mem=%dMB pid=%d\n", argv[0],
           procs ? "processos" : "threads", n, segundos, mb, getpid());
    fflush(stdout);

    if (procs) {
        for (int i = 0; i < n; i++)
            if (fork() == 0) { trabalho(NULL); _exit(0); }
        while (wait(NULL) > 0) ;             /* o pai so espera os filhos */
    } else {
        pthread_t t[n];
        for (int i = 0; i < n; i++) pthread_create(&t[i], NULL, trabalho, NULL);
        for (int i = 0; i < n; i++) pthread_join(t[i], NULL);
    }
    return 0;
}
