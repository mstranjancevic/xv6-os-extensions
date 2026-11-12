#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(void)
{
    char *p = sbrk(4096);
    printf("count snap holes: %d\n", snap_count_holes((int)p));
    exit();
}
