#include <stdio.h>

#define MAX_PROCESSES 20

typedef struct {
    int pid;
    int burst;
    int waiting;
    int turnaround;
} Process;

void printGantt(Process p[], int n) {
    printf("\nGantt Chart:\n|");
    for (int i = 0; i < n; i++)
        printf(" P%d |", p[i].pid);
    printf("\n0");
    int t = 0;
    for (int i = 0; i < n; i++) {
        t += p[i].burst;
        printf("   %d", t);
    }
    printf("\n");
}

int main() {
    Process p[MAX_PROCESSES];
    int n;

    printf("===== FCFS CPU Scheduling =====\n");
    printf("Enter number of processes: ");
    scanf("%d", &n);

    for (int i = 0; i < n; i++) {
        p[i].pid = i + 1;
        printf("Enter burst time for P%d: ", i + 1);
        scanf("%d", &p[i].burst);
    }

    p[0].waiting    = 0;
    p[0].turnaround = p[0].burst;
    for (int i = 1; i < n; i++) {
        p[i].waiting    = p[i-1].waiting + p[i-1].burst;
        p[i].turnaround = p[i].waiting   + p[i].burst;
    }

    printGantt(p, n);

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
    printf("Average Waiting Time    : %.2f ms\n", totalWT  / n);
    printf("Average Turnaround Time : %.2f ms\n", totalTAT / n);

    return 0;
}