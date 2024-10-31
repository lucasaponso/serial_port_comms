#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <signal.h>


typedef struct
{
    int device_file;
    char *filename;
} thread_param_t;


static thread_param_t params;
bool running = false;

int open_com_port(char * dev_name);
FILE* open_output_file(const char *filename);
void signal_handler( int sig );
void init_per_thread_signal_ignorer( void );
static void init_signal_handler( void );
void * read_write_serial(void * arg);

int open_com_port(char * dev_name)
{
    int dev_file_fd = open(dev_name, O_RDWR);
    if (dev_file_fd < 0)
    {
        printf("Failed to open the device file\n");
        close(dev_file_fd);
        return -1;
    }

    return dev_file_fd;
}

FILE* open_output_file(const char *filename)
{
    FILE *output_file_fd = fopen(filename, "a");
    if (output_file_fd == NULL)
    {
        perror("Failed to open the output file");
        return NULL;
    }

    return output_file_fd;
}


void signal_handler( int sig )
{
    switch ( sig )
    {
        case SIGINT:
            printf("Received SIGINT signal.\n");
            running = false;
            usleep(1000000);
            close(params.device_file);

            exit( EXIT_SUCCESS );
            break;

        default:
            printf("Unhandled signal %s\n", strsignal( sig ));
            break;
    }
}

void init_per_thread_signal_ignorer( void )
{
    sigset_t newSigSet;

    /* Set signal mask - signals we want to block */
    sigemptyset( &newSigSet );
    sigaddset( &newSigSet, SIGCHLD ); /* ignore child - i.e. we don't need to wait for it */
    sigaddset( &newSigSet, SIGTSTP ); /* ignore Tty stop signals */
    sigaddset( &newSigSet, SIGTTOU ); /* ignore Tty background writes */
    sigaddset( &newSigSet, SIGTTIN ); /* ignore Tty background reads */
    sigaddset( &newSigSet, SIGPIPE ); /* ignore pipe fails */
    /* The use of sigprocmask() is unspecified in a multithreaded process. Using pthread_sigmask instead */
    pthread_sigmask( SIG_BLOCK, &newSigSet, NULL );
}

static void init_signal_handler( void )
{
    struct sigaction newSigAction;

    init_per_thread_signal_ignorer();

    /* Set up a signal handler */
    newSigAction.sa_handler = signal_handler;
    sigemptyset( &newSigAction.sa_mask );
    newSigAction.sa_flags = 0;

    /* Signals to handle */
    sigaction( SIGHUP, &newSigAction, NULL );   /* catch hangup signal */
    sigaction( SIGTERM, &newSigAction, NULL );  /* catch term signal */
    sigaction( SIGINT, &newSigAction, NULL );   /* catch interrupt signal */
}


int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        printf("Invalid Arguments: ./main /dev/ttyUSB0 /mnt/output.txt\n");
        return 1;
    }

    // Set up how we want to handle signals
    init_signal_handler();

    printf("Device File: %s\n", argv[1]);
    printf("Output File: %s\n", argv[2]);

    int dev_file_fd = open_com_port(argv[1]);
    if (dev_file_fd == -1)
    {
        printf("Error\n");
        return 1;
    }

    params.device_file = dev_file_fd;
    params.filename = argv[2];

    pthread_t thread;

    if (pthread_create(&thread, NULL, read_write_serial, (void*)&params) != 0)
    {
        printf("Failed to create thread");
        return 1;
    }

    if (pthread_join(thread, NULL) != 0)
    {
        printf("Failed to join thread");
        return 1;
    }

    printf("Thread has finished execution.\n");
    return 0;
}


void * read_write_serial(void * arg)
{
    thread_param_t* params = (thread_param_t*)arg; //cast args as defined struct (fd.h)
    char read_buffer[32];
    ssize_t bytes_read;
    int index;
    FILE * output_file_fd;
    running = true;

    while (running)
    {
        bytes_read = read(params->device_file, read_buffer, sizeof(read_buffer)); //read byte from /dev/ttyUSB0 and store in din

        if (bytes_read < 0)
        {
            printf("Failed to read from the device file\n");
            close(params->device_file);
            return NULL;
        }

        output_file_fd = fopen(params->filename, "a");

        if (output_file_fd != NULL)
        {
            for (index = 0; index < bytes_read; index++)
            {
                fprintf(output_file_fd, "%c", read_buffer[index]);
            }
            fflush(output_file_fd);
            fclose(output_file_fd);
        }
    }

    return NULL;
}