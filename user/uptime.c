#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"


void main() {
    int up = uptime();
    int seconds = up / 20;
    int minutes = seconds / 60;
    int hours = minutes / 60;
    int days = hours / 24;
    int weeks = days / 7;
    printf("up %d weeks, %d days, %d hours, %d minutes, %d seconds\n", 
        weeks,
        days,
        hours,
        minutes,
        seconds);

    exit(0);    
}
