#include <poll.h>
#include <stdio.h>

int main()
{
    int events = POLLIN;
    printf("POLLIN is %d\n", POLLIN);
    printf("~POLLIN is %d\n", ~POLLIN);
    printf("events is %d\n", events);
    events |= ~POLLIN;
    printf("events is %d\n", events);
    events |= POLLOUT;
    printf("events is %d\n", events);
    events |= ~POLLOUT;
    printf("events is %d\n", events);
    events |= POLLIN;
    printf("events is %d\n", events);

    return 0;
}
