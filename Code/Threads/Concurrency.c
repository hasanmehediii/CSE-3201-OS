// #include <pthread.h>
// #include <stdio.h>
// #include <stdlib.h>

// #define NUM_THREADS 10

// void *print_hello(void *tid)
// {
//     printf("Hello World from thread %d\n", (int)tid);
//     pthread_exit(NULL);
// }

// int main(int argc, char *argv[])
// {
//     pthread_t threads[NUM_THREADS];
//     int status, i;

//     for (i = 0; i < NUM_THREADS; i++)
//     {
//         printf("Main Thread: Creating thread =%d\n", i);
//         status = pthread_create(&threads[i], NULL, print_hello, (void *)i);

//         if (status != 0)
//         {
//             printf("pthread_create error: %d\n", status);
//             exit(-1);
//         }
//     }
//     pthread_exit(NULL);
// }

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#define NUM_THREADS 10

void *print_hello(void *arg)
{
    int tid = *(int *)arg;
    printf("Hello World from thread %d\n", tid);
    pthread_exit(NULL);
}

int main()
{
    pthread_t threads[NUM_THREADS];
    int thread_ids[NUM_THREADS];
    int status, i;

    for (i = 0; i < NUM_THREADS; i++)
    {
        thread_ids[i] = i;
        printf("Main Thread: Creating thread = %d\n", i);

        status = pthread_create(&threads[i], NULL, print_hello, &thread_ids[i]);

        if (status != 0)
        {
            printf("pthread_create error: %d\n", status);
            exit(-1);
        }
    }

    pthread_exit(NULL);
}