#include <stdio.h>
#include <stdbool.h>

#define MAX_PROCESSES 20
#define MAX_RESOURCES 20

void calculateNeed(
    int processes,
    int resources,
    int need[MAX_PROCESSES][MAX_RESOURCES],
    int max[MAX_PROCESSES][MAX_RESOURCES],
    int allocation[MAX_PROCESSES][MAX_RESOURCES]
) {
    for (int i = 0; i < processes; i++) {
        for (int j = 0; j < resources; j++) {
            need[i][j] = max[i][j] - allocation[i][j];
        }
    }
}

void printNeedMatrix(
    int processes,
    int resources,
    int need[MAX_PROCESSES][MAX_RESOURCES]
) {
    printf("\nNeed Matrix:\n");

    for (int i = 0; i < processes; i++) {
        printf("P%d: ", i);

        for (int j = 0; j < resources; j++) {
            printf("%d ", need[i][j]);
        }

        printf("\n");
    }
}

bool isSafe(
    int processes,
    int resources,
    int available[MAX_RESOURCES],
    int allocation[MAX_PROCESSES][MAX_RESOURCES],
    int need[MAX_PROCESSES][MAX_RESOURCES],
    int safeSequence[MAX_PROCESSES]
) {
    int work[MAX_RESOURCES];
    bool finish[MAX_PROCESSES] = {false};

    for (int j = 0; j < resources; j++) {
        work[j] = available[j];
    }

    int completed = 0;

    while (completed < processes) {
        bool processFound = false;

        for (int i = 0; i < processes; i++) {
            if (!finish[i]) {
                bool canExecute = true;

                /*
                 * Check whether Need[i] <= Work
                 */
                for (int j = 0; j < resources; j++) {
                    if (need[i][j] > work[j]) {
                        canExecute = false;
                        break;
                    }
                }

                if (canExecute) {
                    /*
                     * Process completes and releases
                     * its allocated resources.
                     */
                    for (int j = 0; j < resources; j++) {
                        work[j] += allocation[i][j];
                    }

                    safeSequence[completed] = i;
                    completed++;

                    finish[i] = true;
                    processFound = true;
                }
            }
        }

        /*
         * No unfinished process can execute.
         * Therefore, the state is unsafe.
         */
        if (!processFound) {
            return false;
        }
    }

    return true;
}

bool requestResources(
    int processNumber,
    int request[MAX_RESOURCES],
    int processes,
    int resources,
    int available[MAX_RESOURCES],
    int allocation[MAX_PROCESSES][MAX_RESOURCES],
    int need[MAX_PROCESSES][MAX_RESOURCES]
) {
    /*
     * Check Request <= Need
     */
    for (int j = 0; j < resources; j++) {
        if (request[j] > need[processNumber][j]) {
            printf("\nError: Process requested more than its maximum need.\n");
            return false;
        }
    }

    /*
     * Check Request <= Available
     */
    for (int j = 0; j < resources; j++) {
        if (request[j] > available[j]) {
            printf("\nResources are not currently available.");
            printf("\nProcess P%d must wait.\n", processNumber);
            return false;
        }
    }

    /*
     * Temporarily allocate the requested resources.
     */
    for (int j = 0; j < resources; j++) {
        available[j] -= request[j];
        allocation[processNumber][j] += request[j];
        need[processNumber][j] -= request[j];
    }

    int safeSequence[MAX_PROCESSES];

    if (isSafe(
            processes,
            resources,
            available,
            allocation,
            need,
            safeSequence
        )) {

        printf("\nRequest can be granted safely.\n");

        printf("New Safe Sequence: ");

        for (int i = 0; i < processes; i++) {
            printf("P%d", safeSequence[i]);

            if (i != processes - 1) {
                printf(" -> ");
            }
        }

        printf("\n");
        return true;
    }

    /*
     * Unsafe state: rollback the temporary allocation.
     */
    for (int j = 0; j < resources; j++) {
        available[j] += request[j];
        allocation[processNumber][j] -= request[j];
        need[processNumber][j] += request[j];
    }

    printf("\nRequest cannot be granted.");
    printf("\nIt would make the system unsafe.\n");

    return false;
}

int main(void) {
    int processes;
    int resources;

    int allocation[MAX_PROCESSES][MAX_RESOURCES];
    int max[MAX_PROCESSES][MAX_RESOURCES];
    int need[MAX_PROCESSES][MAX_RESOURCES];
    int available[MAX_RESOURCES];
    int safeSequence[MAX_PROCESSES];

    printf("Enter number of processes: ");
    scanf("%d", &processes);

    printf("Enter number of resource types: ");
    scanf("%d", &resources);

    if (processes <= 0 || processes > MAX_PROCESSES ||
        resources <= 0 || resources > MAX_RESOURCES) {

        printf("Invalid number of processes or resources.\n");
        return 1;
    }

    printf("\nEnter Allocation Matrix:\n");

    for (int i = 0; i < processes; i++) {
        printf("Allocation for P%d: ", i);

        for (int j = 0; j < resources; j++) {
            scanf("%d", &allocation[i][j]);
        }
    }

    printf("\nEnter Max Matrix:\n");

    for (int i = 0; i < processes; i++) {
        printf("Maximum requirement for P%d: ", i);

        for (int j = 0; j < resources; j++) {
            scanf("%d", &max[i][j]);

            if (max[i][j] < allocation[i][j]) {
                printf(
                    "Error: Max cannot be smaller than Allocation.\n"
                );
                return 1;
            }
        }
    }

    printf("\nEnter Available Resources: ");

    for (int j = 0; j < resources; j++) {
        scanf("%d", &available[j]);
    }

    calculateNeed(
        processes,
        resources,
        need,
        max,
        allocation
    );

    printNeedMatrix(processes, resources, need);

    if (isSafe(
            processes,
            resources,
            available,
            allocation,
            need,
            safeSequence
        )) {

        printf("\nSystem is in a SAFE state.\n");
        printf("Safe Sequence: ");

        for (int i = 0; i < processes; i++) {
            printf("P%d", safeSequence[i]);

            if (i != processes - 1) {
                printf(" -> ");
            }
        }

        printf("\n");
    } else {
        printf("\nSystem is in an UNSAFE state.\n");
        printf("No safe sequence exists.\n");
        return 0;
    }

    int choice;

    printf("\nDo you want to test a new resource request?");
    printf("\nEnter 1 for Yes, 0 for No: ");
    scanf("%d", &choice);

    if (choice == 1) {
        int processNumber;
        int request[MAX_RESOURCES];

        printf("Enter process number: ");
        scanf("%d", &processNumber);

        if (processNumber < 0 || processNumber >= processes) {
            printf("Invalid process number.\n");
            return 1;
        }

        printf(
            "Enter request for P%d: ",
            processNumber
        );

        for (int j = 0; j < resources; j++) {
            scanf("%d", &request[j]);
        }

        requestResources(
            processNumber,
            request,
            processes,
            resources,
            available,
            allocation,
            need
        );
    }

    return 0;
}
