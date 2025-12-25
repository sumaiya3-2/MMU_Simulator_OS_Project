#include <stdio.h>
#include <stdlib.h>

#define MAX_PAGES 50
#define MAX_FRAMES 20
#define MAX_TLB 10
#define INVALID -1

/* Global MMU Parameters */
int pageSize;
int numPages;
int numFrames;
int tlbSize;
int memoryAccessTime;
int tlbAccessTime;

/* MMU Structures */
int pageTable[MAX_PAGES];
int frames[MAX_FRAMES];
int tlbPage[MAX_TLB];
int tlbFrame[MAX_TLB];

/* Statistics */
int tlbHits = 0;
int tlbMisses = 0;

/* Function Prototypes */
void configureMMU();
void initializeMemory();
void logicalToPhysical();
void pageReplacementMenu();
void fifo(int pages[], int n);
void lru(int pages[], int n);
void optimal(int pages[], int n);
void tlbAndEMAT();
float calculateEMAT(int pageFaults, int totalReferences);
void displayState();


/* ---------------- MAIN ---------------- */
int main()
{
    int choice;

    printf("=====================================\n");
    printf("      MMU SIMULATOR (Paging Based)\n");
    printf("=====================================\n");

    configureMMU();
    initializeMemory();

    while (1)
    {
        printf("\n=========== MAIN MENU ===========\n");
        printf("1. Logical to Physical Address Translation\n");
        printf("2. Page Replacement Algorithms\n");
        printf("3. TLB Simulation & Effective Memory Access Time\n");
        printf("4. Display Page Table, Frames & TLB\n");
        printf("0. Exit\n");
        printf("Enter your choice: ");
        scanf("%d", &choice);

        switch (choice)
        {
        case 1:
            logicalToPhysical();
            break;
        case 2:
            pageReplacementMenu();
            break;
        case 3:
            tlbAndEMAT();
            break;
        case 4:
            displayState();
            break;
        case 0:
            printf("Exiting MMU Simulator...\n");
            exit(0);
        default:
            printf("Invalid choice!\n");
        }
    }
    return 0;
}

/* ---------------- FUNCTIONS ---------------- */

/*Takes MMU parameters from the user */

void configureMMU()
{
    printf("\n--- MMU Configuration ---\n");

    printf("Enter Page Size (bytes): ");
    scanf("%d", &pageSize);

    printf("Enter Number of Logical Pages: ");
    scanf("%d", &numPages);

    printf("Enter Number of Physical Frames: ");
    scanf("%d", &numFrames);

    printf("Enter Number of TLB Entries: ");
    scanf("%d", &tlbSize);

    printf("Enter Memory Access Time (ns): ");
    scanf("%d", &memoryAccessTime);

    printf("Enter TLB Access Time (ns): ");
    scanf("%d", &tlbAccessTime);
}

/*Simulates an empty memory from the start*/

void initializeMemory()
{
    for (int i = 0; i < numPages; i++)
        pageTable[i] = INVALID;

    for (int i = 0; i < numFrames; i++)
        frames[i] = INVALID;

    for (int i = 0; i < tlbSize; i++)
    {
        tlbPage[i] = INVALID;
        tlbFrame[i] = INVALID;
    }
}

/*Simulates MMU Physical address from logical address using Paging, TLB, Page table */

void logicalToPhysical()
{
    int logicalAddress;
    printf("\nEnter Logical Address (decimal): ");
    scanf("%d", &logicalAddress);

    int pageNumber = logicalAddress / pageSize;
    int offset = logicalAddress % pageSize;

    printf("Page Number: %d\n", pageNumber);
    printf("Offset: %d\n", offset);

    /* TLB Lookup */
    for (int i = 0; i < tlbSize; i++)
    {
        if (tlbPage[i] == pageNumber)
        {
            tlbHits++;
            int physicalAddress = tlbFrame[i] * pageSize + offset;
            printf("TLB HIT\n");
            printf("Physical Address: %d\n", physicalAddress);
            return;
        }
    }

    /* TLB MISS*/

    tlbMisses++;
    printf("TLB MISS\n");

    /* Page Table look up */

    if (pageTable[pageNumber] == INVALID)
    {
        printf("Page Fault Occurred!\n");

        // Load page into first free frame

        for (int i = 0; i < numFrames; i++)
        {
            if (frames[i] == INVALID)
            {
                frames[i] = pageNumber;
                pageTable[pageNumber] = i;
                break;
            }
        }
    }

    int frame = pageTable[pageNumber];
    // Update TLB
    tlbPage[0] = pageNumber;
    tlbFrame[0] = frame;

    int physicalAddress = frame * pageSize + offset;
    printf("Physical Address: %d\n", physicalAddress);
}

/* User Chooses among the page replacement algorithms: FIFO, Optimal, LRU */

void pageReplacementMenu()
{
    int n, choice;
    int pages[50];

    printf("\nEnter number of page references: ");
    scanf("%d", &n);

    printf("Enter page reference string: ");
    for (int i = 0; i < n; i++)
        scanf("%d", &pages[i]);

    printf("\n1. FIFO\n2. LRU\n3. Optimal\nChoice: ");
    scanf("%d", &choice);

    if (choice == 1)
        fifo(pages, n);
    else if (choice == 2)
        lru(pages, n);
    else if (choice == 3)
        optimal(pages, n);
}

/* First-in-First-Out Page replacement: Oldest loaded page is replaced first */

void fifo(int pages[], int n)
{
    int frame[MAX_FRAMES];       //Represents Frames
    int faults = 0, index = 0;   //Initialized page faults. Index points to next page to be replaced

    for (int i = 0; i < numFrames; i++)
        frame[i] = INVALID;      //Initialized with empty frames

    for (int i = 0; i < n; i++)
    {
        int hit = 0;            //Flag for page hit or miss

        //To check if page is loaded or not
        for (int j = 0; j < numFrames; j++)
            if (frame[j] == pages[i])
                hit = 1;            //page hit. No replacement is needed

        //If page fault occurs

        if (!hit)
        {
            frame[index] = pages[i];               //We replace oldest page
            index = (index + 1) % numFrames;       //We keep the index pointing next page to be replaced circular.
            faults++;                              //We increment page fault counter
        }

        //To display frame contents

        printf("Page %d -> ", pages[i]);
        for (int j = 0; j < numFrames; j++)
            printf("%d ", frame[j]);
        printf("\n");
    }
    printf("Total Page Faults: %d\n", faults);  //Total page fault
    float emat = calculateEMAT(faults, n);
    printf("Effective Memory Access Time (FIFO): %.2f ns\n", emat);   //EMAT

}

/* Least Recently Used Page replacement: Replaces page that was used farthest in the past.*/

void lru(int pages[], int n)
{
    int frame[MAX_FRAMES], time[MAX_FRAMES];   //The frame array represents frame and time array stores pages last used time
    int faults = 0, counter = 0;

    for (int i = 0; i < numFrames; i++)
        frame[i] = time[i] = INVALID;

    for (int i = 0; i < n; i++)
    {
        int hit = 0;                         // Flag for page hit
        for (int j = 0; j < numFrames; j++)
        {
            if (frame[j] == pages[i])
            {
                hit = 1;
                time[j] = counter++;        //We Update last used time for this page
            }
        }

        //if Page fault occurs

        if (!hit)
        {
            int lru = 0;                        // Index of least recently used page
            for (int j = 1; j < numFrames; j++)
                if (time[j] < time[lru])            // Find frame with smallest time
                    lru = j;

            frame[lru] = pages[i];                  // Replace LRU page
            time[lru] = counter++;                  // Update its time
            faults++;
        }

        //Display frame contents
        printf("Page %d -> ", pages[i]);
        for (int j = 0; j < numFrames; j++)
            printf("%d ", frame[j]);
        printf("\n");
    }
    printf("Total Page Faults: %d\n", faults);
    float emat = calculateEMAT(faults, n);
    printf("Effective Memory Access Time (LRU): %.2f ns\n", emat);

}

/* Optimal Page Replacement: Replaces page that will not be used for the longest Time*/

void optimal(int pages[], int n)
{
    int frame[MAX_FRAMES], faults = 0;

    for (int i = 0; i < numFrames; i++)
        frame[i] = INVALID;

    for (int i = 0; i < n; i++)
    {
        int hit = 0;
        for (int j = 0; j < numFrames; j++)
            if (frame[j] == pages[i])
                hit = 1;
        //If page fault occurs
        if (!hit)
        {
            int index = -1, farthest = i;          //Farthest array represents pages that will be used in farthest future
            for (int j = 0; j < numFrames; j++)
            {
                int k;
                for (k = i + 1; k < n; k++)
                    if (frame[j] == pages[k])
                        break;

                if (k > farthest)         // If used farthest in future
                {
                    farthest = k;
                    index = j;
                }
            }

            if (index == -1)
                index = 0;

            frame[index] = pages[i];     //Replace Optimal page
            faults++;
        }

        //Display Frame contents

        printf("Page %d -> ", pages[i]);
        for (int j = 0; j < numFrames; j++)
            printf("%d ", frame[j]);
        printf("\n");
    }
    printf("Total Page Faults: %d\n", faults);
    float emat = calculateEMAT(faults, n);
    printf("Effective Memory Access Time (Optimal): %.2f ns\n", emat);

}

/* Calculating EMAT*/

void tlbAndEMAT()
{
    int total = tlbHits + tlbMisses;
    if (total == 0)
    {
        printf("No memory access yet.\n");
        return;
    }

    float hitRatio = (float)tlbHits / total;
    float missRatio = 1 - hitRatio;

    float emat = hitRatio * (tlbAccessTime + memoryAccessTime)
               + missRatio * (tlbAccessTime + 2 * memoryAccessTime);

    printf("\nTLB Hits: %d\n", tlbHits);
    printf("TLB Misses: %d\n", tlbMisses);
    printf("Hit Ratio: %.2f\n", hitRatio);
    printf("Effective Memory Access Time: %.2f ns\n", emat);
}

/* Function for Calculating EMAT of page replacement algorithm.

The formula: EMAT=(1−p)× Memory access time + p × Page fault service time.

Since, We are not simulating any disk in this MMU Simulator,
we can assume Page fault service time = 2* Memory access time*/

float calculateEMAT(int pageFaults, int totalReferences)
{
    float pageFaultRate = (float)pageFaults / totalReferences;

    float emat = (1 - pageFaultRate) * memoryAccessTime
               + pageFaultRate * (2 * memoryAccessTime);

    return emat;
}


/* Displaying the current state of the Page Table, Frames, and TLB.*/

void displayState()
{
    printf("\nPage Table:\n");
    for (int i = 0; i < numPages; i++)
        printf("Page %d -> Frame %d\n", i, pageTable[i]);

    printf("\nFrames:\n");
    for (int i = 0; i < numFrames; i++)
        printf("%d ", frames[i]);

    printf("\n\nTLB:\n");
    for (int i = 0; i < tlbSize; i++)
        printf("Page %d -> Frame %d\n", tlbPage[i], tlbFrame[i]);
}
