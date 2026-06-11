#include <stdio.h>

#define MAX_PROCESSES 20

typedef struct {
    int pid;
    int burst;
    int remaining;
    int completion;
    int waiting;
    int turnaround;
} Process;

typedef struct {
    int data[MAX_PROCESSES * 200];
    int front, rear, size;
} Queue;

void initQueue(Queue *q) { q->front = q->rear = q->size = 0; }

void enqueue(Queue *q, int val) {
    q->data[q->rear++] = val;
    q->size++;
}

int dequeue(Queue *q) {
    int val = q->data[q->front++];
    q->size--;
    return val;
}

int main() {
    Process p[MAX_PROCESSES];
    int n, quantum;

    printf("===== Round Robin CPU Scheduling =====\n");
    printf("Enter number of processes: ");
    scanf("%d", &n);

    for (int i = 0; i < n; i++) {
        p[i].pid = i + 1;
        printf("Enter burst time for P%d: ", i + 1);
        scanf("%d", &p[i].burst);
        p[i].remaining = p[i].burst;
    }
    printf("Enter time quantum: ");
    scanf("%d", &quantum);

    Queue q;
    initQueue(&q);

    /* Enqueue all processes initially */
    for (int i = 0; i < n; i++)
        enqueue(&q, i);

    printf("\nExecution Order:\n");

    int t = 0, completed = 0;
    int inQueue[MAX_PROCESSES] = {0};
    for (int i = 0; i < n; i++) inQueue[i] = 1;

    while (completed < n) {
        int idx = dequeue(&q);
        inQueue[idx] = 0;

        int runTime = (p[idx].remaining < quantum) ? p[idx].remaining : quantum;
        printf("P%d runs from %d to %d\n", p[idx].pid, t, t + runTime);
        t                 += runTime;
        p[idx].remaining  -= runTime;

        if (p[idx].remaining == 0) {
            p[idx].completion  = t;
            p[idx].turnaround  = p[idx].completion;
            p[idx].waiting     = p[idx].turnaround - p[idx].burst;
            completed++;
        } else {
            enqueue(&q, idx);
            inQueue[idx] = 1;
        }
    }

    printf("\n%-10s %-15s %-15s %-15s\n",
           "Process", "Burst Time(ms)", "Waiting Time(ms)", "Turnaround Time(ms)");
    printf("----------------------------------------------------------\n");

    float totalWT = 0, totalTAT = 0;
    for (int i = 0; i < n; i++) {
        printf("P%-9d %-15d %-15d %-15d\n",
               p[i].pid, p[i].burst, p[i].waiting, p[i].turnaround);
        totalWT  += p[i].waiting;
        totalTAT += p[i].turnaround;
    }

    printf("----------------------------------------------------------\n");
    printf("Time Quantum            : %d ms\n",  quantum);
    printf("Average Waiting Time    : %.2f ms\n", totalWT  / n);
    printf("Average Turnaround Time : %.2f ms\n", totalTAT / n);

    return 0;
}