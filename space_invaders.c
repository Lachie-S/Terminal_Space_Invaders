#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/epoll.h>
#include <time.h>
#include <stdbool.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/eventfd.h>

#define MAX_KEYS 2
#define FPS 1
#define FRAME_WIDTH 5
#define FRAME_DEPTH 10
#define MAX_KEYS_STOR 2
#define CLOCKS_PER_SEC_INV 1/CLOCKS_PER_SEC


typedef struct {
    char* keys_pressed;
} key_stor;

struct listener_input{
    key_stor* keys;
    pthread_mutex_t* mutex;
    struct epoll_event* epoll_events;
    int epoll_fd;
};

void* listener(void* input);
int frame_timer(clock_t frame_start_time);
int print_frame(char* board);
int game(char* board, key_stor* keys, pthread_mutex_t* mutex);
char get_index(int x, int y);
int create_next_frame(int player_pos, char* board, key_stor* keys, pthread_mutex_t* mutex);
char* initialise_board();
int delete_board(char* board);


bool exit_game = false;

void* listener(void* in){
    struct listener_input* input = (struct listener_input*) in;
    pthread_mutex_t* mutex = input->mutex;
    key_stor* keys = input->keys;
    struct epoll_event* events = input->epoll_events;
    int epoll_fd = input->epoll_fd;

    while(!exit_game){
        fflush(stdin);
        int num_keys = epoll_wait(epoll_fd, events, MAX_KEYS_STOR, 100);
        write(1, "input recieved", 15);

        pthread_mutex_lock(mutex);
        keys->keys_pressed[0] = '\n';
        keys->keys_pressed[1] = '\n';

        for(int i = 0; i<num_keys; i++){
            keys->keys_pressed[i] = getchar();
        }
        pthread_mutex_unlock(mutex);
    }

    return NULL;
}

int frame_timer(clock_t frame_start_time){
    int run_time = (clock() - frame_start_time)*CLOCKS_PER_SEC_INV;

    int wait_time = 0.03125 - run_time;     //0.03125 = 1/32

    sleep(wait_time);
    return 0;
}

int print_frame(char* board){
    char move[] = "\x1b[u";
    write(1, move, 4);
    write(1, board, FRAME_DEPTH*(FRAME_WIDTH+1));
}

int game(char* board, key_stor* keys, pthread_mutex_t* mutex){

    //save pos to write to:
    char save[] = "\x1b[s";
    write(1,save,4);

    int player_pos = FRAME_WIDTH*0.5 + 1;
    board[get_index(player_pos, FRAME_DEPTH)] = 'M';
    print_frame(board);

    while(!exit_game){
        clock_t frame_start_time = clock();
        player_pos = create_next_frame(player_pos, board, keys, mutex);
        print_frame(board);
        //frame_timer(frame_start_time);
        sleep(1);
    }

    return 0;
}

char get_index(int x, int y){
    if( x < 0 || FRAME_WIDTH < x){
        return -1;
    }

    if( y < 0 || FRAME_DEPTH < y){
        return -1;
    }


    return y*(FRAME_WIDTH+1) + x;
}

int create_next_frame(int player_pos, char* board, key_stor* keys, pthread_mutex_t* mutex){

    pthread_mutex_lock(mutex);
    char key1 = keys->keys_pressed[0];
    char key2 = keys->keys_pressed[1];
    pthread_mutex_unlock(mutex);

    int player_shot = -1;

    board[get_index(player_pos, FRAME_DEPTH)] = '\0';

    switch(key1){
        case 'a':
            if(player_pos != 0){
                player_pos -= 1;
            }
            break;

        case 'd':
            if(player_pos != FRAME_WIDTH-1){
                player_pos += 1;
            }
            break;

        case ' ':
            player_shot = player_pos;
            if(key2 == 'a' && player_pos != 0){
                player_pos -= 1;
            } else if(key2 == 'a' && player_pos != FRAME_WIDTH-1){
                player_pos += 1;
            }
    }

    board[get_index(player_pos, FRAME_DEPTH)] = 'M';

    return player_pos;
}

char* initialise_board(){
    int len = (FRAME_WIDTH+1)*(FRAME_DEPTH);
    char* board = (char*) malloc(len);
    memset(board, '.', len);

    for(int i = 1; i<=FRAME_DEPTH; i++){
        board[i*(FRAME_WIDTH+1)-1] = '\n';
    }

    board[len-1] = '\0';

    return board;
}

int delete_board(char* board){
    free(board);                    //error check
    return 0;
}

void exit_func(int signo, siginfo_t *info, void *context){
    if(signo == SIGINT){
        exit_game = true;
    }
    return;
}

int main(){

    //create key stuff
    char* key_array = (char*) malloc(MAX_KEYS);                     //errror check
    key_stor* keys = (key_stor*) malloc(sizeof(key_stor));          //error check
    *keys = (key_stor) {.keys_pressed = key_array};


    //create key mutex
    pthread_mutex_t* mutex = (pthread_mutex_t*) malloc(sizeof(pthread_mutex_t));        //error check
    pthread_mutex_init(mutex, NULL);                                                    //error check


    //create stdin epoll
    struct epoll_event* event = (struct epoll_event*) malloc(sizeof(struct epoll_event));
    struct epoll_event* events = (struct epoll_event*) malloc(sizeof(struct epoll_event)*MAX_KEYS_STOR);
    event->events = EPOLLIN;
    event->data.fd = 0;
    int key_listener = epoll_create1(0);                //error check
    epoll_ctl(key_listener, EPOLL_CTL_ADD, 0, event);


    //block all signals and create a signal handling thread
    pthread_t signal_thread;
    sigset_t* signal_block = (sigset_t*) malloc(sizeof(sigset_t));      //error check
    sigfillset(signal_block);
    pthread_sigmask(SIG_BLOCK, signal_block, NULL);                     //error check

    //create listener input and start thread
    struct listener_input* input = (struct listener_input*) malloc(sizeof(struct listener_input));
    *input = (struct listener_input) {.keys = keys, .mutex = mutex, .epoll_fd = key_listener, .epoll_events = events};
    pthread_t listener_thread;
    pthread_create(&listener_thread, NULL, &listener, input);         //error check


    //unblock all signals in thread
    pthread_sigmask(SIG_UNBLOCK, signal_block, NULL);


    //create signal handler
    struct sigaction act = {0};
    act.sa_sigaction = &exit_func;

    sigaction(SIGINT, &act, NULL);      //error check


    char* board = initialise_board();

    //runs infinite while loop until ctr-c is pressed
    game(board, keys, mutex);

    //clean up

    pthread_join(listener_thread, NULL);

    free(board);
    free(key_array);
    free(keys);
    free(mutex);
    free(event);
    free(events);
    free(signal_block);
    free(input);


    return 0;

}
