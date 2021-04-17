#include <stdio.h>
#include <stdlib.h>
#include <string.h> 
#include <fcntl.h> 
#include <sys/stat.h> 
#include <sys/types.h> 
#include <sys/socket.h>
#include <unistd.h>
#include <button_interface.h>

#define _1ms        1000
#define BUFFER_LEN	10

static void wait_press(void *object, Button_Interface *button)
{
    while (true)
    {
        if (!button->Read(object))
        {
            usleep(_1ms * 100);
            break;
        }
        else
        {
            usleep(_1ms);
        }
    }
}

bool Button_Run(void *object, int handle, Button_Interface *button)
{
    char buffer[BUFFER_LEN];
    int state = 0;

    if(button->Init(object) == false)
        return false;        
   
    while(true)
    {
        wait_press(object, button);

        state ^= 0x01;
        memset(buffer, 0, BUFFER_LEN);
        snprintf(buffer, BUFFER_LEN, "%d", state);
        send(handle, buffer, strlen(buffer), 0);        
    }

    close(handle);
    
    return false;
}
