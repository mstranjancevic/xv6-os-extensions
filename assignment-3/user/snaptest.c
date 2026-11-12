// Testiranje snapshot mehanizma
// Pokretanje: snaptest > output.txt
//             cat output.txt

#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(void)
{
    struct snap_info si;
    char *p;
    int pid;

    printf("Test snap_empty\n");
    printf("snap_empty: %d (ocekivano 0)\n", snap_empty());

    printf("Test snap_take\n");
    p = sbrk(4096);
    p[0] = 'A';
    printf("snap_take(0): %d (ocekivano 0)\n", snap_take(0));
    printf("snap_take(0) ponovo: %d (ocekivano -3)\n", snap_take(0));
    printf("snap_take(5): %d (ocekivano -2)\n", snap_take(5));

    printf("Test snap_status\n");
    printf("snap_status(0): %d (ocekivano 0)\n", snap_status(0, &si));
    printf("  size=%d, num_pages=%d\n", si.size, si.num_pages);

    printf("Test snap_diff\n");
    p[0] = 'B';
    snap_take(1);
    printf("snap_diff(0,1): %d (ocekivano >0)\n", snap_diff(0, 1));
    printf("snap_diff(0,0): %d (ocekivano 0)\n", snap_diff(0, 0));

    printf("Test snap_release\n");
    printf("snap_release(1): %d (ocekivano 0)\n", snap_release(1));
    printf("snap_release(1): %d (ocekivano -1)\n", snap_release(1));
    printf("snap_empty: %d (ocekivano 1)\n", snap_empty());

    printf("Test snap_restore\n");
    pid = fork();
    if(pid == 0){
        p[0] = 'A';
        snap_take(2);
        p[0] = 'B';
        snap_restore(2);
        exit();
    } else {
        sleep(5);
        kill(pid);
        wait();
        printf("restore radi - dete je loopovalo kao sto se ocekuje\n");
    }

    printf("Test fork nasledjivanje\n");
    pid = fork();
    if(pid == 0){
        printf("dete snap_status(0): %d (ocekivano 0)\n", snap_status(0, &si));
        snap_release(0);
        exit();
    } else {
        wait();
    }

    snap_release(0);
    printf("Testiranje zavrseno\n");
    exit();
}
