#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <led.h>
#include <led_interface.h>

bool Init(void *object);
bool Set(void *object, uint8_t state);

int main(int argc, char *argv[])
{   
    if(argc != 2) 
        return EXIT_FAILURE;

    int fd = atoi(argv[1]);
    if(fd <= 0)
        return EXIT_FAILURE;

    LED_t led =
    {
        .gpio.pin = 0,
        .gpio.eMode = eModeOutput
    };

    LED_Interface led_interface = 
    {
        .Init = Init,
        .Set = Set
    };

    LED_Run(&led, fd, &led_interface);
    
    return 0;
}

bool Init(void *object)
{
    LED_t *led = (LED_t *)object; 
    return LED_init(led) == EXIT_SUCCESS ? true : false;   
}

bool Set(void *object, uint8_t state)
{
    LED_t *led = (LED_t *)object;
    return LED_set(led, (eState_t)state) == EXIT_SUCCESS ? true : false;
}
