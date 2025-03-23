    #include <iostream>
    #include <vector>
    #include <unistd.h>
    #include <sys/socket.h>
#include <sys/wait.h>
#include <cstring>
    struct Player{ 
        char player_character ;
        int player_total_argument_count;
        std::vector<std::string> args;
    };



    int main(){

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

    std::cout<< player_array[3].player_character<< std::endl;
    

    return 0 ;

    }
