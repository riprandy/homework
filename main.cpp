    #include <iostream>
    #include <vector>
    #include <unistd.h>
    #include <sys/socket.h>
#include <sys/wait.h>
#include <cstring>

#define MAX_PLAYERS 100


    struct Player{ 
        char player_character ;
        int player_total_argument_count;
        std::vector<std::string> args;
    };





    void setup_pipes(int player_count, int (&pipes)[MAX_PLAYERS][2]) {
        for (int i = 0; i < player_count; i++) {
            if (socketpair(AF_UNIX, SOCK_STREAM, PF_UNIX, pipes[i]) == -1) {
                perror("socketpair failed");
                exit(1);
            }
        }

    }


    

    int main(){
    int pipes[MAX_PLAYERS][2];
    int grid_width;
    int grid_height; 
    int streak_size;
    int player_count;

    std::cout<< "hello" << std::endl;
    std::cin >>grid_width >> grid_height >> streak_size >> player_count ; 


    Player player_array[player_count];


    for (int i = 0 ; i < player_count ;  i++ ){
        Player& current_player=player_array[i];
        
        std:: cin>> current_player.player_character >>current_player.player_total_argument_count; 

        for (int j = 0 ; j < current_player.player_total_argument_count ; j ++ ){
            std::string arg; 
            std::cin >> arg; 
            current_player.args.push_back(arg);
        }


    }

   
    return 0 ;

    }
