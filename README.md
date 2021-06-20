<p align="center">
  <img src="https://cdn0.sbnation.com/assets/3417793/moveoverlynnswann.gif"/>
</p>

# _Socketpair_

## Tópicos
* [Introdução](#introdução)
* [socketpair](#socketpair-1)
* [Systemcalls usados no socketpair](##systemcalls-usados-no-socketpair)
* [Implementação](#implementação)
* [launch_processes](#launch_processes)
* [button_interface](#button_interface)
* [led_interface](#led_interface)
* [Compilando, Executando e Matando os processos](#compilando-executando-e-matando-os-processos)
* [Compilando](#compilando)
* [Clonando o projeto](#clonando-o-projeto)
* [Selecionando o modo](#selecionando-o-modo)
* [Modo PC](#modo-pc)
* [Modo RASPBERRY](#modo-raspberry)
* [Executando](#executando)
* [Interagindo com o exemplo](#interagindo-com-o-exemplo)
* [MODO PC](#modo-pc-1)
* [MODO RASPBERRY](#modo-raspberry-1)
* [Matando os processos](#matando-os-processos)
* [Conclusão](#conclusão)
* [Referência](#referência)

## Introdução
Para que um processo se comunique com o outro de forma bidirecional é necessário ter duas instâncias de _pipe_ para que o canal de escrita do _pipe_ A se conecte ao canal de leitura do _pipe_ B e vice e versa, em termos de código para realizar essa implementação é necessário criar duas instâncias de _pipe_ e inserir como argumento ambos os handles para que a comunicação bidirecional seja possível. Existe uma alternativa para obter esse comportamento de forma simplificada.

## socketpair
O _socketpair_ é um _systemcall_ capaz de conectar dois sockets de forma simplificada sem a necessidade de toda a inicialização de uma conexão tcp ou udp, devolvendo dois sockets conectados que podem ser usados para realizar a comunicação entre si de forma bidirecional.

## _Systemcalls_ usados no _socketpair_

```c
#include <sys/types.h>
#include <sys/socket.h>

int socketpair(int domain, int type, int protocol, int sv[2]);
```

Dependendo do protocolo utilizado a forma de realizar a leitura e escrita do socket pode mudar. No caso de uso [TCP](https://www.embarcados.com.br/)socket-tcp/] usar as _systemcalls_ pertinentes a TCP, o mesmo para [UDP](https://www.embarcados.com.br/socket-udp/).


## Implementação

Para demonstrar o uso desse IPC, vai ser utilizado o modelo Produtor/Consumidor, onde o processo Produtor(_button_process_) vai escrever seu estado interno no arquivo, e o Consumidor(_led_process_) vai ler o estado interno e vai aplicar o estado para si. Aplicação é composta por três executáveis sendo eles:
* _launch_processes_ - é responsável por criar os _sockets_ e lançar os processos _button_process_ e _led_process_ através da combinação _fork_ e _exec_
* _button_interface_ - é responsável por ler o GPIO em modo de leitura da Raspberry Pi e escrever o estado interno no arquivo
* _led_interface_ - é responsável por ler do arquivo o estado interno do botão e aplicar em um GPIO configurado como saída

### *launch_processes*

No _main_ é criado algumas variáveis iniciando com _pair_ que corresponde aos _sockets_ que vão ser criados, *pid_button* e *pid_led* que recebem o pid dos processos referentes à *button_process* e *led_process*, duas variáveis para armazenar o resultado caso o _exec_ venha a falhar e um _buffer_ para montar a lista de argumentos.
```c
int pair[2];
int pid_button, pid_led;
int button_status, led_status;
char buffer[BUFFER_SIZE];
```

Aqui é criado o par de _sockets_ usando o protocolo TCP
```c
if(socketpair(AF_LOCAL, SOCK_STREAM, 0, pair) == -1)
  return EXIT_FAILURE;
```

Em seguida é criado um processo clone, se processo clone for igual a 0, os argumentos são preparados e armazenados na variável _buffer_ e é criado um _array_ de *strings* com o nome do programa que será usado pelo _exec_, em caso o _exec_ retorne, o estado do retorno é capturado e será impresso no *stdout* e aborta a aplicação. Se o _exec_ for executado com sucesso o programa *button_process* será carregado. 
```c
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
```

O mesmo procedimento é repetido novamente, porém com a intenção de carregar o *led_process*.

```c
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
```

E por fim o pair de _sockets_ é fechado para o processo pai
```c
close(pair[BUTTON_SOCKET]);
close(pair[LED_SOCKET]);
```

## *button_interface*
A implementação da interface de botão é relativamente simples, onde o botão é iniciado e em seguida fica em loop aguardando um evento de pressionamento de botão, que quando pressionado altera o estado da variável interna e envia para o _socket_ usando o handle e a mensagem formatada.
```c
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
```
## *led_interface*
A implementação da interface de LED é relativamente simples, onde o LED é iniciado e em seguida fica em loop aguardando uma mensagem, que quando recebida altera o estado do LED com o valor lido.

```c
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
```

## Compilando, Executando e Matando os processos
Para compilar e testar o projeto é necessário instalar a biblioteca de [hardware](https://github.com/NakedSolidSnake/Raspberry_lib_hardware) necessária para resolver as dependências de configuração de GPIO da Raspberry Pi.

## Compilando
Para faciliar a execução do exemplo, o exemplo proposto foi criado baseado em uma interface, onde é possível selecionar se usará o hardware da Raspberry Pi 3, ou se a interação com o exemplo vai ser através de input feito por FIFO e o output visualizado através de LOG.

### Clonando o projeto
Pra obter uma cópia do projeto execute os comandos a seguir:

```bash
$ git clone https://github.com/NakedSolidSnake/Raspberry_IPC_TCP_Socketpair
$ cd Raspberry_IPC_TCP_Socketpair
$ mkdir build && cd build
```

### Selecionando o modo
Para selecionar o modo devemos passar para o cmake uma variável de ambiente chamada de ARCH, e pode-se passar os seguintes valores, PC ou RASPBERRY, para o caso de PC o exemplo terá sua interface preenchida com os sources presentes na pasta src/platform/pc, que permite a interação com o exemplo através de FIFO e LOG, caso seja RASPBERRY usará os GPIO's descritos no [artigo](https://github.com/NakedSolidSnake/Raspberry_lib_hardware#testando-a-instala%C3%A7%C3%A3o-e-as-conex%C3%B5es-de-hardware).

#### Modo PC
```bash
$ cmake -DARCH=PC ..
$ make
```

#### Modo RASPBERRY
```bash
$ cmake -DARCH=RASPBERRY ..
$ make
```

## Executando
Para executar a aplicação execute o processo _*launch_processes*_ para lançar os processos *button_process* e *led_process* que foram determinados de acordo com o modo selecionado.

```bash
$ cd bin
$ ./launch_processes
```

Uma vez executado podemos verificar se os processos estão rodando atráves do comando 
```bash
$ ps -ef | grep _process
```

O output 
```bash
pi        2773     1  0 10:25 pts/0    00:00:00 led_process 4
pi        2774     1  1 10:25 pts/0    00:00:00 button_process 3
```
## Interagindo com o exemplo
Dependendo do modo de compilação selecionado a interação com o exemplo acontece de forma diferente

### MODO PC
Para o modo PC, precisamos abrir um terminal e monitorar os LOG's
```bash
$ sudo tail -f /var/log/syslog | grep LED
```

Dessa forma o terminal irá apresentar somente os LOG's referente ao exemplo.

Para simular o botão, o processo em modo PC cria uma FIFO para permitir enviar comandos para a aplicação, dessa forma todas as vezes que for enviado o número 0 irá logar no terminal onde foi configurado para o monitoramento, segue o exemplo

```bash
echo "0" > /tmp/socketpair_fifo
```

Output
```bash
Apr 27 11:09:54 cssouza-Latitude-5490 LED SOCKETPAIR[9883]: LED Status: On
Apr 27 11:09:55 cssouza-Latitude-5490 LED SOCKETPAIR[9883]: LED Status: Off
Apr 27 11:09:55 cssouza-Latitude-5490 LED SOCKETPAIR[9883]: LED Status: On
Apr 27 11:09:55 cssouza-Latitude-5490 LED SOCKETPAIR[9883]: LED Status: Off
Apr 27 11:09:56 cssouza-Latitude-5490 LED SOCKETPAIR[9883]: LED Status: On
Apr 27 11:09:56 cssouza-Latitude-5490 LED SOCKETPAIR[9883]: LED Status: Off
```

### MODO RASPBERRY
Para o modo RASPBERRY a cada vez que o botão for pressionado irá alternar o estado do LED.

## Matando os processos
Para matar os processos criados execute o script kill_process.sh
```bash
$ cd bin
$ ./kill_process.sh
```

## Conclusão
O _socketpair_ é uma boa alternativa para o uso de [_pipes_](https://www.embarcados.com.br/pipes/) pois facilita o modo de comunicação entre processos caso haja necessidade que os processos se comuniquem entre si, ou seja, de forma bidirecional. Devido a sua facilidade de criação do par de _sockets_, isso o torna relativamente fácil de usar. Portanto caso houver a necessidade de usar _pipes_ sempre prefira o _socketpair_ por garantir a comunicação entre ambas as direções.

## Referência
* [Link do projeto completo](https://github.com/NakedSolidSnake/Raspberry_IPC_TCP_Socketpair)
* [Mark Mitchell, Jeffrey Oldham, and Alex Samuel - Advanced Linux Programming](https://www.amazon.com.br/Advanced-Linux-Programming-CodeSourcery-LLC/dp/0735710430)
* [Linux Socket Programming by Example](https://www.amazon.com.br/Linux-Socket-Programming-Example-Warren/dp/0789722410)
* [fork, exec e daemon](https://github.com/NakedSolidSnake/Raspberry_fork_exec_daemon)
* [biblioteca hardware](https://github.com/NakedSolidSnake/Raspberry_lib_hardware)

