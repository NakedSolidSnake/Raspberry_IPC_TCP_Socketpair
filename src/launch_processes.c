#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define BUTTON_SOCKET   0
#define LED_SOCKET      1
#define BUFFER_SIZE     10

int main(int argc, char const *argv[])
{
    int pair[2];
    int pid_button, pid_led;
    int button_status, led_status;
    char buffer[BUFFER_SIZE];

    if(socketpair(AF_LOCAL, SOCK_STREAM, 0, pair) == -1)
        return EXIT_FAILURE;

    pid_button = fork();

    if(pid_button == 0)
    {
        //start button process
        snprintf(buffer, BUFFER_SIZE, "%d", pair[BUTTON_SOCKET]);
        char *args[] = {"./button_process", buffer, NULL};
        button_status = execvp(args[0], args);
        printf("Error to start button process, status = %d\n", button_status);
        abort();
    }   

    pid_led = fork();

    if(pid_led == 0)
    {
        //Start led process
        snprintf(buffer, BUFFER_SIZE, "%d", pair[LED_SOCKET]);
        char *args[] = {"./led_process", buffer, NULL};
        led_status = execvp(args[0], args);
        printf("Error to start led process, status = %d\n", led_status);
        abort();
    }

    close(pair[BUTTON_SOCKET]);
    close(pair[LED_SOCKET]);

    return EXIT_SUCCESS;
}