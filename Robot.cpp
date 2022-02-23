#include "Robot.h"

Robot::Robot(int x, int y, RequestHandler* r){
    x_position = x; // initialising robots current position with passed in values
    y_position = y;

    Message_Handler = r; // assigning message handler for robot -> master communications

    maze_xsize = 4; // hard coding 4x4 maze as used by sample maze
    maze_ysize = 4; // TODO: change constructor to assign maze size

    number_of_unexplored = 1; // set to 1 as current occupied cell is unknown to robot

    printf("ROBOT: calling robot master\n");
    // addRobot request to RobotMaster
    Message* temp_message = new Message(); // buffer to load data into before sending message

    temp_message->request_type = 0; // request_type = 0 as addRobot request required

    temp_message->msg_data.push_back((void*) &x); // adding x position of robot to [0]
    temp_message->msg_data.push_back((void*) &y); // adding y position of robot to [1]

    pthread_cond_t cond_var; // gathering condition variable to block robot until it the robotmaster responds
    pthread_cond_init(&cond_var, NULL); // initializing condition variable

    pthread_mutex_t mutex; // generating mutex for use with condition variable
    pthread_mutex_init(&mutex, NULL); // intializing mutex

    temp_message->condition_var = &cond_var; // assigning condition variable to message before sending
    
    Message_Handler->sendMessage(temp_message); // sending message to robot controller

    printf("ROBOT: waiting for id\n");
    while(temp_message->return_data.empty()){
        pthread_cond_wait(temp_message->condition_var, &mutex); //  blocking robot until RobotMaster unblocks it with response message
    }

    id = *(unsigned int*)temp_message->return_data[0]; // gather assigned id from RobotMaster's response

    printf("ROBOT: id = %d %x\n",id, temp_message);

    pthread_mutex_destroy(&mutex); // destorying mutex and condition variable for messaging as no longer needed
    pthread_cond_destroy(&cond_var);

    delete temp_message; // deleting temp message as its no longer needed
}

void Robot::scanCell(GridGraph* maze){ // scans current cell for walls on all sides
                                       // function assumes current cell has not been scanned yet
    
    number_of_unexplored--;
    
    // gathering x edges within maze at robot's current position
    LocalMap.x_edges[y_position][x_position] = maze->x_edges[y_position][x_position]; // east
    LocalMap.x_edges[y_position][x_position+1] = maze->x_edges[y_position][x_position+1]; // west
    
    // gathering y edges within maze at robot's current position
    LocalMap.y_edges[y_position][x_position] = maze->y_edges[y_position][x_position]; // north
    LocalMap.y_edges[y_position+1][x_position] = maze->y_edges[y_position+1][x_position]; // south

    // updating state of current node 
    LocalMap.nodes[y_position][x_position] = 1; // setting currently scanned node to 1 to signifiy its been scanned 

    // updating state of neighbouring nodes to unexplored if possible
    
    // checking north
    if(!LocalMap.y_edges[y_position][x_position] && LocalMap.nodes[y_position - 1][x_position] != 1){ // checking if node to north hasn't been explored
        LocalMap.nodes[y_position - 1][x_position] = 2; // if unexplored and no wall between robot and cell, set northern node to unexplored
        number_of_unexplored++;
    }
    // checking south
    if(!LocalMap.y_edges[y_position + 1][x_position] && LocalMap.nodes[y_position + 1][x_position] != 1){ // checking if node to north hasn't been explored
        LocalMap.nodes[y_position + 1][x_position] = 2; // if unexplored and no wall between robot and cell, set southern node to unexplored
        number_of_unexplored++;
    }
    // checking east
    if(!LocalMap.x_edges[y_position][x_position] && LocalMap.nodes[y_position][x_position - 1] != 1){ // checking if node to north hasn't been explored
        LocalMap.nodes[y_position][x_position - 1] = 2; // if unexplored and no wall between robot and cell, set eastern node to unexplored
        number_of_unexplored++;
    }
    // checking wast
    if(!LocalMap.x_edges[y_position][x_position + 1] && LocalMap.nodes[y_position][x_position + 1] != 1){ // checking if node to north hasn't been explored
        LocalMap.nodes[y_position][x_position + 1] = 2; // if unexplored and no wall between robot and cell, set western node to unexplored
        number_of_unexplored++;
    }
    return;
}

bool Robot::move2Cell(int direction){ // function to move robot depending on location of walls within local map
                                      // ensure current cell is scanned with scanCell before calling this function
                                      // direction = 1 ^ north
                                      // direction = 2 v south
                                      // direction = 3 < east
                                      // direction = 4 > west

    bool ret_value = false; // return variable
                            // used to track whether move operation was a success

    switch (direction){ // switch statement to move robot in specific direction based on know information from local map
    case 1:
        if (!LocalMap.y_edges[y_position][x_position]){ // if there is an edge between current node and node above
            y_position--; // move robot to node above
            ret_value = true; // ret_value = true as movement was a success
        }
        break;

    case 2:
        if (!LocalMap.y_edges[y_position+1][x_position]){ // if there is an edge between current node and node below
            y_position++; // move robot to node below
            ret_value = true; // ret_value = true as movement was a success
        }
        break;

    case 3:
        if (!LocalMap.x_edges[y_position][x_position]){ // if there is an edge between current node and node to the left
            x_position--; // move robot to node to the left
            ret_value = true; // ret_value = true as movement was a success
        }
        break;

    case 4:
        if (!LocalMap.x_edges[y_position][x_position+1]){ // if there is an edge between current node and node to the right 
            x_position++; // move robot to node to the right
            ret_value = true; // ret_value = true as movement was a success
        }
        break;

    default: // if invalid direction is passed into function
        break;
    }
    if (ret_value) // if robot position movement successfull
        std::cout << "Robot Sucessfully moved to " << x_position  << ", "<< y_position << "\n";
    return ret_value; // ret_value = true if robot moved sucessfully
                      // ret_value = false if robot failed to move
}

bool Robot::move2Cell(Coordinates* destination){ // overloaded version of move2Cell using destination
                                                 // converts target destination to a direction then uses original move2Cell function

    // first must determine direction based on two values
    int dx = x_position - destination->x; // gather change in x and y directions
    int dy = y_position - destination->y;

    int direction = 0;  // variable to store determined direction 
                        // direction = 1 ^ north
                        // direction = 2 v south
                        // direction = 3 < east
                        // direction = 4 > west

    if(dy == 1 && dx == 0){ // if moving north
        direction = 1;
    }
    else if(dy == -1 && dx == 0){ // if moving south
        direction = 2;
    }
    else if(dx == 1 && dy == 0){ // if moving east
        direction = 3;
    }
    else if (dx == -1 && dy == 0){ // if moving west
        direction = 4;
    }
    else{
        printf("Error: invalid movement passed into move2Cell\n");
        return false; // return false as movement failed dur to invalid direction
    }

    //

    return move2Cell(direction); 
    
}

std::vector<Coordinates> Robot::getValidNeighbours(unsigned int x, unsigned  int y){ // function to gather valid neighbouring cells of a selected cell based on robot's local map

    std::vector<Coordinates> ret_value; // vector of Coordinates to return
                                        // this will contain the coordinates of valid neighbouring nodes
    
    Coordinates buffer; // buffer structor to gather positions of neighbouring nodes before pushing to vector

    // check if neighbour to the north is valid and connected via an edge (no wall)
    if(!LocalMap.y_edges[y][x] && LocalMap.nodes[y-1][x] > 0){
        buffer.x = x; 
        buffer.y = y - 1;
        ret_value.push_back(buffer);
    }
    // check if neighbour to the south is valid and connected via an edge (no wall)
    if(!LocalMap.y_edges[y+1][x] && LocalMap.nodes[y+1][x] > 0){
        buffer.x = x; 
        buffer.y = y + 1;
        ret_value.push_back(buffer);
    }
    // check if neighbour to the east is valid and connected via an edge (no wall)
    if(!LocalMap.x_edges[y][x] && LocalMap.nodes[y][x-1] > 0){
        buffer.x = x - 1; 
        buffer.y = y;
        ret_value.push_back(buffer);
    }
    // check if neighbour to the west is valid and connected via an edge (no wall)
    if(!LocalMap.x_edges[y][x+1] && LocalMap.nodes[y][x+1] > 0){
        buffer.x = x + 1; 
        buffer.y = y;
        ret_value.push_back(buffer);
    }

    // printing all valid neighbouring nodes of selected node
    for(int i = 0; i < ret_value.size(); i ++){ // TODO: after debugging
        printf("%d,%d\n",ret_value[i].x,ret_value[i].y);
    }

    return ret_value; // returning vector
    // TODO: return by pointer may be desirable
}

bool Robot::pf_BFS(int x_dest, int y_dest){ // function to plan a path for robot to follow from current position to a specified destination
    
    planned_path.clear(); // clearing planned_path as new path is to be planned using breadth first search

    // initial checks to ensure path needs to be planned
    // no point using computation if robot is at destination or destination does not exist on robot's local map
    if(LocalMap.nodes[y_dest][x_dest] == 0) // if the destination node has not been explored
        return false;                       // can't create a path thus return false;

    else if(x_position == x_dest && y_position == y_dest) // if robot is at the destination already
        return true;                                      // no need to move thus return true

    bool ret_value = false; // return value

    std::queue<Coordinates> node_queue; // creating node queue to store nodes to be "explored" by algorithm

    std::map<Coordinates, Coordinates> visited_nodes; // map to track explored nodes to their parent node

    // initializing first node to explore from with robot's current coordinates
    Coordinates curr_node(x_position, y_position); 

    node_queue.push(curr_node); // adding first node to explore to node queue
    visited_nodes.insert({curr_node, curr_node}); // adding first node to visited node maps using itself as parent

    while(node_queue.size() != 0){// while nodes to explore are in node_queue
        
        curr_node = node_queue.front(); // gathering node from front of queue

        printf("curr node: %d,%d\n", curr_node.x, curr_node.y);

        node_queue.pop(); // removing node from front of the queue

        if (curr_node.x == x_dest && curr_node.y == y_dest){ // if the target node has been located
            ret_value = true; // return true as path found
            break; // break from while loop
        }
        
        std::vector<Coordinates> valid_neighbours = getValidNeighbours(curr_node.x, curr_node.y); // gathering neighbours of current node

        for(int i = 0; i < valid_neighbours.size(); i ++){ // iterate through all of the current node's neighbours to see if they have been explored

            if(visited_nodes.contains(valid_neighbours[i]) == 0){ // if current neighbour has not been visited
                node_queue.push(valid_neighbours[i]);       // TODO: fix map.contains, still causes same error where not detecting key         
                visited_nodes.insert({valid_neighbours[i], curr_node});   
            }
        }
    }
    
    // TODO: implement handling if path to target location is not found 

    // as a valid path has been found from current position to target using robot's local map
    // must travel from destination back through parent nodes to reconstruct path
    
    curr_node.x = x_dest; // ensuring the current node is pointing to destination
    curr_node.y = y_dest; // this allows for the "walk back" through nodes to start to occur
    
    while(!(curr_node.x == x_position && curr_node.y == y_position)){ // while the starting node has not been found from parents

        planned_path.push_back(curr_node); // add current node to planned path

        for(auto [key, val]: visited_nodes){ // TODO: use better search for key function
            if (key == curr_node){
                curr_node = val;
                break;
            }
        }
    }

    printf("Planned Path\n"); // printing planned path
    for(int i = 0; i <  planned_path.size(); i++){ // TODO: Remove after further debugging
        printf("%d,%d\n",planned_path[i].x,planned_path[i].y);
    }

    return ret_value; 
}

bool Robot::BFS_pf2NearestUnknownCell(std::vector<Coordinates>* ret_vector){

    bool ret_value = false; // return value

    std::queue<Coordinates> node_queue; // creating node queue to store nodes to be "explored" by algorithm

    std::map<Coordinates, Coordinates> visited_nodes; // map to track explored nodes to their parent node

    // initializing first node to explore from with robot's current coordinates
    Coordinates curr_node(x_position, y_position); 

    node_queue.push(curr_node); // adding first node to explore to node queue
    visited_nodes.insert({curr_node, curr_node}); // adding first node to visited node maps using itself as parent

    while(node_queue.size() != 0){// while nodes to explore are in node_queue
        
        curr_node = node_queue.front(); // gathering node from front of queue

        printf("curr node: %d,%d\n", curr_node.x, curr_node.y);

        if (LocalMap.nodes[curr_node.y][curr_node.x] == 2){ // if an unexplored node has been found
            ret_value = true; // return true as path to unexplored node found
            break; // break from while loop
        }
        else if(node_queue.size() < 1){ // if node_queue is empty, no unexplored nodes found 
            printf("bruh\n");           // true can be returned as no errors occured in pathfinding
            return true;                // ret_vector will be empty as no unexplored nodes found therefore no need to move
        }

        node_queue.pop(); // removing node from front of the queue as new nodes must be added to queue

        std::vector<Coordinates> valid_neighbours = getValidNeighbours(curr_node.x, curr_node.y); // gathering neighbours of current node

        for(int i = 0; i < valid_neighbours.size(); i ++){ // iterate through all of the current node's neighbours to see if they have been explored
            if(visited_nodes.contains(valid_neighbours[i]) == 0){ // TODO: fix map.contains, still causes same error where not detecting key
                node_queue.push(valid_neighbours[i]);              
                visited_nodes.insert({valid_neighbours[i], curr_node});   
            }
        }
    }
    
    // TODO: implement handling if path unexplored cell not found 

    if (ret_value == false){ // if ret_value == fsle, 

    }

    // as a valid path has been found from current position to target using robot's local map
    // must travel from destination back through parent nodes to reconstruct path

    ret_vector->clear(); // clearing planned_path as new path is to be planned using breadth first search

    while(!(curr_node.x == x_position && curr_node.y == y_position)){ // while the starting node has not been found from parents

        ret_vector->push_back(curr_node); // add current node to planned path

        for(auto [key, val]: visited_nodes){ // TODO: use better search for key function
            if (key == curr_node){
                curr_node = val;
                break;
            }
        }
    }

    printf("Planned Path\n"); // printing planned path
    for(int i = 0; i <  ret_vector->size(); i++){ // TODO: Remove after further debugging
        printf("%d,%d\n",(*ret_vector)[i].x,(*ret_vector)[i].y);
    }

    return ret_value; 
}

void Robot::startRobot(GridGraph* maze){ // function to initialize robot before it starts operating
    
    scanCell(maze); // scans cell that the robot currently belongs to initialze local map before exploring

    return;
}

void Robot::soloExplore(GridGraph* maze){
    while(1){

        scanCell(maze); // scan cell 
        
        if(number_of_unexplored == 0){ // no more cells to explore therefore break from scan loop
            printf("Done exploring!\n");
            break;
        }

        printRobotMaze(); // print maze contents for analysis

        BFS_pf2NearestUnknownCell(&planned_path); // move to nearest unseen cell

        for (int i = 0; i < planned_path.size(); i++){ // while there are movements left to be done by robot
            move2Cell(&(planned_path[planned_path.size()-i-1]));
        }
        planned_path.clear(); // clear planned_path as movement has been completed
    }

    return;
}

bool Robot::printRobotMaze(){ // function to print robot's local map of maze
                              // maze design based off what can be seen here: https://www.chegg.com/homework-help/questions-and-answers/using-c-1-write-maze-solving-program-following-functionality-note-implementation-details-a-q31826669

    std::string logos[8] = { "   ", "---", "|", " ", " R ", " . ", " X ", " * "}; // array with logos to use when printing maze
    
    if(maze_xsize == 0 || maze_ysize == 0){ // if maze has not been allocated
        printf("Error: Maze size has not been specified\n");
        return false; // return false as printing failed
    }

    printf("*Robot Local Map**\n"); // printing title and maze information

    int string_pointer = 0; // integer used to determine which logo needs to be printed from logo vector

    int count = 0; // counter to determine if both the column and row edges have been printed
    int i = 0; // counter to track if the whole maze has been printed

    while(!(i >= maze_ysize && count == 1)){ // while loop to determing which row to print (i = node row number)
    
        if(count == 0){ // printing the horizontal walls of maze

            for(int j = 0; j < maze_xsize; j++){

                if(LocalMap.y_edges[i][j]){ // if there is no edge between two nodes
                    string_pointer = 1; // print horizontal line
                }
                else{ // if there is an edge between two nodes
                    string_pointer = 0; // print horizontal line
                }

                printf("+%s", logos[string_pointer].c_str());
            }
            printf("+");
        }
        else{ // printing vertical walls of the maze

            for(int j = 0; j < maze_xsize + 1; j++){

                // checking the walls between two nodes (e.g. wall?, no wall?)
                if(LocalMap.x_edges[i][j]){ // if there is no edge between two nodes
                    string_pointer = 2; // print horizontal line
                }
                else{ // if there is an edge between two nodes
                    string_pointer = 3; // print horizontal line
                }
                printf("%s", logos[string_pointer].c_str());

                // checking contents of a node (e.g. does it have a robot?)
                if (j >= maze_xsize){ // if iterating outside of valid node
                    string_pointer = 3; // print empty space as node is outside of maze
                }
                else if(j == x_position && i == y_position){ // if current node is the robot's location
                    string_pointer = 4; // print R for robot
                }
                else if(LocalMap.nodes[i][j] == 0){ // if current node is invalid (unseen and unexplored)
                    string_pointer = 6; // print I for invalid cell
                }
                else if(LocalMap.nodes[i][j] == 2){ // if current node has been seen but not explored
                    string_pointer = 7; // print * for seen node
                }
                else{ // if current cell has been seen and explored (valid)
                    string_pointer = 0; // print empty space
                }


                printf("%s",logos[string_pointer].c_str());
            }
        }
        
        if (count == 1){ // if both the row and column corresponding to i value have been printed
            count = 0; // reset count value
            i++; // increment i to access next row
        }
        else // if only the column edges corresponding to i have been printed
            count++;

        printf("\n");

    }

    return true; // return true as printing succeeded
}

void Robot::printRobotNodes(){ // function to print Robot's map of explored nodes
    printNodes(&LocalMap);
}

void Robot::printRobotXMap(){ // function to print Robot's X edge map 
    printXEdges(&LocalMap);
}
void Robot::printRobotYMap(){ // function to print Robot's Y edge map 
    printYEdges(&LocalMap);
}