#include <stdio.h>
#include <limits.h>

#define MAX_PROCESSES 20

typedef struct {
    int pid;
    int arrival;
    int burst;
    int remaining;
    int completion;
    int waiting;
    int turnaround;
    int started;
} Process;

void printGanttLog(int log[], int totalTime) {
    printf("\nGantt Chart (per time unit):\n");
    int t = 0;
    while (t < totalTime) {
        int cur = log[t], start = t;
        while (t < totalTime && log[t] == cur) t++;
        if (cur == -1)
            printf("| IDLE(%d-%d) ", start, t);
        else
            printf("| P%d(%d-%d) ", cur, start, t);
    }
    printf("|\n");
}

int main() {
    Process p[MAX_PROCESSES];
    int n;

    printf("===== SRTN CPU Scheduling =====\n");
    printf("Enter number of processes: ");
    scanf("%d", &n);

    for (int i = 0; i < n; i++) {
        p[i].pid     = i + 1;
        p[i].started = 0;
        printf("Enter arrival time and burst time for P%d: ", i + 1);
        scanf("%d %d", &p[i].arrival, &p[i].burst);
        p[i].remaining = p[i].burst;
    }

    int totalBurst = 0;
    for (int i = 0; i < n; i++) totalBurst += p[i].burst;

    int ganttLog[10000];
    int completed = 0, t = 0;

    while (completed < n) {
        int idx = -1, minRem = INT_MAX;
        for (int i = 0; i < n; i++) {
            if (p[i].arrival <= t && p[i].remaining > 0) {
                if (p[i].remaining < minRem) {
                    minRem = p[i].remaining;
                    idx    = i;
                }
            }
        }

        if (idx == -1) {
            ganttLog[t++] = -1;   /* CPU idle thakbe ekhane */
            continue;
        }

        ganttLog[t] = p[idx].pid;
        p[idx].remaining--;
        t++;

        if (p[idx].remaining == 0) {
            p[idx].completion  = t;
            p[idx].turnaround  = p[idx].completion - p[idx].arrival;
            p[idx].waiting     = p[idx].turnaround - p[idx].burst;
            completed++;
        }
    }

    printGanttLog(ganttLog, t);

    printf("\n%-10s %-12s %-12s %-15s %-15s %-15s\n",
           "Process", "Arrival(ms)", "Burst(ms)",
           "Completion(ms)", "Waiting(ms)", "Turnaround(ms)");
    printf("------------------------------------------------------------------------\n");

    float totalWT = 0, totalTAT = 0;
    for (int i = 0; i < n; i++) {
        printf("P%-9d %-12d %-12d %-15d %-15d %-15d\n",
               p[i].pid, p[i].arrival, p[i].burst,
               p[i].completion, p[i].waiting, p[i].turnaround);
        totalWT  += p[i].waiting;
        totalTAT += p[i].turnaround;
    }

    printf("------------------------------------------------------------------------\n");
    printf("Average Waiting Time    : %.2f ms\n", totalWT  / n);
    printf("Average Turnaround Time : %.2f ms\n", totalTAT / n);

    return 0;
}