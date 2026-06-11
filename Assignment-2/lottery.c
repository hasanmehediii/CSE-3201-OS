#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define MAX_PROCESSES 20

typedef struct {
    int pid;
    int burst;
    int tickets;
    int remaining;
    int completion;
    int waiting;
    int turnaround;
} Process;

int main() {
    Process p[MAX_PROCESSES];
    int n, quantum;

    srand((unsigned int)(time(NULL) ^ ((unsigned int)clock() << 16)));

    printf("===== Lottery CPU Scheduling =====\n");
    printf("Enter number of processes: ");
    scanf("%d", &n);

    for (int i = 0; i < n; i++) {
        p[i].pid = i + 1;
        printf("Enter burst time and tickets for P%d: ", i + 1);
        scanf("%d %d", &p[i].burst, &p[i].tickets);
        p[i].remaining = p[i].burst;
    }
    printf("Enter time quantum: ");
    scanf("%d", &quantum);

    int t = 0, completed = 0;

    printf("\n--- Lottery Draw Log ---\n");

    while (completed < n) {
        int totalTickets = 0;
        for (int i = 0; i < n; i++)
            if (p[i].remaining > 0)
                totalTickets += p[i].tickets;

        if (totalTickets == 0) break;

        /* Draw a winning ticket */
        int draw = rand() % totalTickets;
        printf("Draw: %d (out of %d) -> ", draw, totalTickets);

        /* Find the winning process */
        int cumulative = 0, winner = -1;
        for (int i = 0; i < n; i++) {
            if (p[i].remaining > 0) {
                cumulative += p[i].tickets;
                if (draw < cumulative) {
                    winner = i;
                    break;
                }
            }
        }

        int runTime = (p[winner].remaining < quantum) ? p[winner].remaining : quantum;
        printf("P%d runs %d-%d\n", p[winner].pid, t, t + runTime);
        t                    += runTime;
        p[winner].remaining  -= runTime;

        if (p[winner].remaining == 0) {
            p[winner].completion  = t;
            p[winner].turnaround  = t;
            p[winner].waiting     = t - p[winner].burst;
            completed++;
        }
    }

    printf("\n%-10s %-12s %-10s %-15s %-15s %-15s\n",
           "Process", "Burst(ms)", "Tickets",
           "Completion(ms)", "Waiting(ms)", "Turnaround(ms)");
    printf("------------------------------------------------------------------------\n");

    float totalWT = 0, totalTAT = 0;
    for (int i = 0; i < n; i++) {
        printf("P%-9d %-12d %-10d %-15d %-15d %-15d\n",
               p[i].pid, p[i].burst, p[i].tickets,
               p[i].completion, p[i].waiting, p[i].turnaround);
        totalWT  += p[i].waiting;
        totalTAT += p[i].turnaround;
    }

    printf("------------------------------------------------------------------------\n");
    printf("Average Waiting Time    : %.2f ms\n", totalWT  / n);
    printf("Average Turnaround Time : %.2f ms\n", totalTAT / n);

    return 0;
}