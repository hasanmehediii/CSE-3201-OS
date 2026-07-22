#include <stdio.h>

#define NUMBER_OF_PROCESSES 4
#define MAX_RESOURCES 20

int main(void)
{
    int m;

    int existing[MAX_RESOURCES];
    int available[MAX_RESOURCES];
    int work[MAX_RESOURCES];
    int columnSum[MAX_RESOURCES];

    int allocation[NUMBER_OF_PROCESSES][MAX_RESOURCES];
    int maximum[NUMBER_OF_PROCESSES][MAX_RESOURCES];
    int need[NUMBER_OF_PROCESSES][MAX_RESOURCES];

    int finish[NUMBER_OF_PROCESSES] = {0};
    int safeSequence[NUMBER_OF_PROCESSES];

    int completed = 0;
    int processFound;
    int canRun;

    printf("Banker's Algorithm - Safe State Detection\n");
    printf("-----------------------------------------\n");

    printf("Number of processes is fixed at %d.\n",
           NUMBER_OF_PROCESSES);

    printf("Enter number of resource types: ");
    scanf("%d", &m);

    if (m <= 0 || m > MAX_RESOURCES)
    {
        printf("Invalid number of resource types.\n");
        return 1;
    }

    /*
     * Input Existing Resource vector.
     */
    printf("\nEnter Existing Resources (E):\n");

    for (int j = 0; j < m; j++)
    {
        printf("E[%d]: ", j);
        scanf("%d", &existing[j]);

        if (existing[j] < 0)
        {
            printf("Resource values cannot be negative.\n");
            return 1;
        }
    }

    /*
     * Input Allocation / Possessed matrix.
     */
    printf("\nEnter Allocation Matrix (P):\n");

    for (int i = 0; i < NUMBER_OF_PROCESSES; i++)
    {
        printf("Allocation for P%d:\n", i);

        for (int j = 0; j < m; j++)
        {
            printf("P[%d][%d]: ", i, j);
            scanf("%d", &allocation[i][j]);

            if (allocation[i][j] < 0)
            {
                printf("Allocation cannot be negative.\n");
                return 1;
            }
        }
    }

    /*
     * Input Maximum Demand matrix.
     */
    printf("\nEnter Maximum Demand Matrix (Max):\n");

    for (int i = 0; i < NUMBER_OF_PROCESSES; i++)
    {
        printf("Maximum demand for P%d:\n", i);

        for (int j = 0; j < m; j++)
        {
            printf("Max[%d][%d]: ", i, j);
            scanf("%d", &maximum[i][j]);

            if (maximum[i][j] < allocation[i][j])
            {
                printf("\nInvalid input.\n");
                printf("Max[%d][%d] cannot be smaller than ",
                       i, j);
                printf("Allocation[%d][%d].\n", i, j);

                return 1;
            }
        }
    }

    /*
     * Calculate Need matrix:
     *
     * Need[i][j] = Max[i][j] - Allocation[i][j]
     */
    for (int i = 0; i < NUMBER_OF_PROCESSES; i++)
    {
        for (int j = 0; j < m; j++)
        {
            need[i][j] =
                maximum[i][j] - allocation[i][j];
        }
    }

    /*
     * Calculate the sum of each Allocation column.
     */
    for (int j = 0; j < m; j++)
    {
        columnSum[j] = 0;

        for (int i = 0; i < NUMBER_OF_PROCESSES; i++)
        {
            columnSum[j] += allocation[i][j];
        }
    }

    /*
     * Calculate Available vector:
     *
     * Available[j] = Existing[j] - ColumnSum[j]
     */
    for (int j = 0; j < m; j++)
    {
        available[j] = existing[j] - columnSum[j];

        if (available[j] < 0)
        {
            printf("\nInvalid system state.\n");
            printf("Allocated amount of resource R%d ", j);
            printf("is greater than the existing amount.\n");

            return 1;
        }
    }

    /*
     * Print the computed Need matrix.
     */
    printf("\nComputed Need Matrix:\n");

    printf("Process\t");

    for (int j = 0; j < m; j++)
    {
        printf("R%d\t", j);
    }

    printf("\n");

    for (int i = 0; i < NUMBER_OF_PROCESSES; i++)
    {
        printf("P%d\t", i);

        for (int j = 0; j < m; j++)
        {
            printf("%d\t", need[i][j]);
        }

        printf("\n");
    }

    /*
     * Print the computed Available vector.
     */
    printf("\nComputed Available Vector:\n");

    for (int j = 0; j < m; j++)
    {
        printf("R%d = %d", j, available[j]);

        if (j < m - 1)
        {
            printf(", ");
        }
    }

    printf("\n");

    /*
     * Initially:
     *
     * Work = Available
     */
    for (int j = 0; j < m; j++)
    {
        work[j] = available[j];
    }

    /*
     * Safety Algorithm
     */
    while (completed < NUMBER_OF_PROCESSES)
    {
        processFound = 0;

        for (int i = 0; i < NUMBER_OF_PROCESSES; i++)
        {
            /*
             * Check only unfinished processes.
             */
            if (finish[i] == 0)
            {
                canRun = 1;

                /*
                 * Check whether:
                 *
                 * Need[i] <= Work
                 */
                for (int j = 0; j < m; j++)
                {
                    if (need[i][j] > work[j])
                    {
                        canRun = 0;
                        break;
                    }
                }

                /*
                 * If the process can run, assume that
                 * it finishes and releases its resources.
                 */
                if (canRun == 1)
                {
                    for (int j = 0; j < m; j++)
                    {
                        work[j] += allocation[i][j];
                    }

                    safeSequence[completed] = i;
                    completed++;

                    finish[i] = 1;
                    processFound = 1;
                }
            }
        }

        /*
         * If no unfinished process can run,
         * no safe sequence exists.
         */
        if (processFound == 0)
        {
            break;
        }
    }

    /*
     * Print the final result.
     */
    if (completed == NUMBER_OF_PROCESSES)
    {
        printf("\nThe system is in a SAFE state.\n");

        printf("Safe Sequence: < ");

        for (int i = 0; i < NUMBER_OF_PROCESSES; i++)
        {
            printf("P%d", safeSequence[i]);

            if (i < NUMBER_OF_PROCESSES - 1)
            {
                printf(", ");
            }
        }

        printf(" >\n");

        printf("Final Work Vector: (");

        for (int j = 0; j < m; j++)
        {
            printf("%d", work[j]);

            if (j < m - 1)
            {
                printf(", ");
            }
        }

        printf(")\n");

        /*
         * Validate that the final Work vector
         * equals the Existing vector.
         */
        int finalWorkIsValid = 1;

        for (int j = 0; j < m; j++)
        {
            if (work[j] != existing[j])
            {
                finalWorkIsValid = 0;
                break;
            }
        }

        if (finalWorkIsValid == 1)
        {
            printf("Final Work equals Existing Resources.\n");
            printf("All processes can finish successfully.\n");
        }
    }
    else
    {
        printf("\nThe system is in an UNSAFE state.\n");
        printf("No safe sequence exists.\n");
    }

    return 0;
}