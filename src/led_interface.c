#include <stdio.h> 
#include <stdlib.h>
#include <string.h> 
#include <fcntl.h> 
#include <sys/stat.h> 
#include <sys/types.h> 
#include <sys/socket.h>
#include <unistd.h>
#include <led_interface.h>

#define _1ms        1000
#define BUFFER_LEN	10

bool LED_Run(void *object, int handle, LED_Interface *led)
{
	char buffer[BUFFER_LEN];
	int state;

	if(led->Init(object) == false)
		return false;
	
	while(true)
	{
		recv(handle, buffer, BUFFER_LEN, 0);
		sscanf(buffer, "%d", &state);
		led->Set(object, state);
	}

	close(handle);
	
	return false;	
}
