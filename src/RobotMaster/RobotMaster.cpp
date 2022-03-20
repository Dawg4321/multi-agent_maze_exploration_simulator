#include "RobotMaster.h"

RobotMaster::RobotMaster(RequestHandler* r, int num_of_robots, unsigned int xsize, unsigned int ysize): max_num_of_robots(num_of_robots){
    id_tracker = 0; // initializing id counter to zero
    Message_Handler = r; // gathering request handler to use for receiving robot -> master communications
    
    maze_xsize = xsize;
    maze_ysize = ysize;

    // TODO: Implement master_status to cause any new incoming requests to be left incomplete
    //master_status = 0; // setting status

    GlobalMap = new GridGraph(maze_xsize, maze_ysize); // allocating GlobalMap to maze size

    GlobalMapInfo.resize(maze_ysize,std::vector<CellInfo>(maze_xsize)); // resizing GlobalMap info to maze size
}

RobotMaster::~RobotMaster(){
    delete GlobalMap; // deallocating GlobalMap
}

void RobotMaster::runRobotMaster(){ // function to continously run RobotMaster until the maze has been mapped
    
    bool maze_mapped = false;

    // RobotMaster continues to receive requests until the maze is fully mapped
    while(!maze_mapped){
        maze_mapped = receiveRequests(); 
    } 

    return;
}

bool RobotMaster::receiveRequests(){ // function to handle incoming requests from robots
    
    Message* request = Message_Handler->getMessage(); // gathering request from msg_queue
                                                      // pointer is gathered so response can be gathered by robot threads
                                                      // returned pointer will be NULL is no messages to get

    if(request != NULL){ // if there is a a request to handle, process it

        // tracking this request
        request_id_tracker++; // get next request id for request tracking purposes

        // gathering request type for switch statement
        m_genericRequest* r = (m_genericRequest*) request->msg_data; // use generic message pointer to gather request type

        switch (r->request_type){ // determining type of request before processing
                case shutDownRequestID: // shutDown confirmation request ( telling master robot has finished exploring)
                {
                    shutDownRequest(request);

                    if(tracked_robots.size() == 0){ // if all robots have successfully shut down
                        return true; // maze exploration done
                    }
                    
                    break;
                }
                case addRobotRequestID: // addRobot request
                {
                    addRobotRequest(request);

                    // if all robots have been added
                    // send signal to all robots to begin exploration
                    if(tracked_robots.size() == max_num_of_robots)
                        updateAllRobotState(1); // updating all robot states to 1
                                                // this causes them to all begin exploring by first scanning their cell
                    break;
                }
                case updateGlobalMapRequestID: // updateGlobalMap request
                {   
                    updateGlobalMapRequest(request);

                    if(number_of_unexplored < 1){ // if no more cells to explore
                        updateAllRobotState(-1); // tell all robots to shut down
                        //master_status = -1; // maze completely mapped, can ignore any incoming requests that do not contain shut down complete messages
                    }

                    break;
                }
                case move2CellRequestID: // move2cell request
                {
                    move2CellRequest(request); // unimplemented in parent class as collision management is used in child class 

                    break;
                }
                case reserveCellRequestID: // reserveCell request (robot wants to start exploring from a cell without other robots using it)
                {
                    reserveCellRequest(request);

                    break;
                }
                case updateRobotLocationRequestID: // update Robot Location  (tells master that robot has completed move operation)
                {
                    updateRobotLocationRequest(request);

                    break;
                }
                default:
                {
                    break;
                }
            }

    }
    else{ // if no message to handle, do nothing

    }
    
    return false; // return false as maze is not completely mapped
}

void RobotMaster::shutDownRequest(Message* request){ // disconnects robot from system
    // shutDown request
    // [0] = type (unsigned int*), content: id of robot shutting down
    
    // no return data

    // gathering data from request
    m_shutDownRequest* request_data = (m_shutDownRequest*)request->msg_data; // first pointer of msg_data points to robot id
    
    removeRobot(request_data->robot_id); // remove robot which is shutting down from system
    
    delete request_data; // deleting dynamically allocated message data

    sem_post(request->res_sem);  // tell robot that shutdown can occur

    delete request; // delete dynamically allocated request

    return;
}

void RobotMaster::addRobotRequest(Message* request){ // adds robot to controller system with any required information
    // addRobot request msg_data layout:
    // [0] = type: (unsigned int*), content: x coordinate of robot
    // [1] = type: (unsigned int*), content: y coordinate of robot
    // [2] = type: (RequestHandler*), content: message handler for master -> robot messages
    
    // return data
    // [0] = type: (unsigned int*), content: id to be assigned to robot

    // gathering incoming request
    m_addRobotRequest* request_data = (m_addRobotRequest*)request->msg_data;
    
    // processing request data
    unsigned int x = request_data->x;
    unsigned int y = request_data->y;
    RequestHandler* robot_request_handler = request_data->robot_request_handler;

    unsigned int robot_id = addRobot(x, y, robot_request_handler); // add robot using coordinates
                                                // return value is assigned id of robot

    delete request_data; // deleting dynamically allocated message data

    // gathering response data
    m_addRobotResponse* response_data = new m_addRobotResponse;
    response_data->robot_id = robot_id;

    // assigning response to message
    request->return_data = (void*) response_data;
    
    sem_post(request->res_sem); // signalling Robot that response message is ready
    sem_wait(request->ack_sem); //  waiting for Robot to be finished with response so message can be deleted
    
    // sent message was dynamically allocated thus must be deleted
    // this part of the code should be thread safe as the robot and Controller have finished using these variables
    delete request;

    return;
}

void RobotMaster::updateGlobalMapRequest(Message* request){
    // updateGlobalMap request msg_data layout:
    // [0] = type: (unsigned int*), content: id of robot sending request
    // [1] = type: (vector<bool>*), content: vector containing information on walls surrounding robot
            // [0] = north, [1] = south, [2] = east, [3] = west
            // 0 = connection, 1 = wall
    // [2] = type: (Coordinates*), content: current coordinates of where the read occured
    
    // return data = none

    // gathering incoming request data
    m_updateGlobalMapRequest* request_data = (m_updateGlobalMapRequest*)request->msg_data;

    unsigned int robot_id = request_data->robot_id;
    std::vector<bool> wall_info = request_data->wall_info;
    Coordinates cords = request_data->cords; // gathering position of scanned reading

    updateGlobalMap(&robot_id, &wall_info, &cords); // updating global map with information

    //request->return_data.push_back((void*)&robot_id); // telling Robot that data was successfully added to GlobalMap and it can continue

    delete request_data; // deleting dynamically allocated message data

    sem_post(request->res_sem); // signalling Robot that response message is ready
    sem_wait(request->ack_sem); // waiting for Robot to be finished with response so message can be deleted

    // sent message was dynamically allocated thus must be deleted
    // this part of the code should be thread safe as the robot and Controller have finished using these variabless
    delete request;

    return;
}

void RobotMaster::move2CellRequest(Message* request){

    // does nothing in parent class as this type of request is only handled in child class
    
    return;
}

void RobotMaster::reserveCellRequest(Message* request){
    // reserveCell request msg_data layout:
    // [0] = type: (unsigned int*), content: id of robot sending request
    // [1] = type: (Coordinates*), content: target unexplored cell to reseve
    // [2] = type: (Coordinates*), content: neighbouring cell used to enter target cell 
                                            // used to determine what aspect of tree must be sent back in event node has already been explored 
    // return data
    // [0] = type (bool*), content: whether cell has been reserved
    // [1] = type (vector<Coordinates>*), content: coordinates correspoonding to full map down a node path if path has been explored already
    // [2] = type (vector<vector<bool>>*), content: wall information corresponding to cell locations in [1] 
    // [3] = type (vector<char>*), content: information on node status

    // gathering incoming request data
    m_reserveCellRequest* request_data = (m_reserveCellRequest*)request->msg_data;

    unsigned int robot_id = request_data->robot_id;
    Coordinates target_cell = request_data->target_cell;                    
    Coordinates neighbouring_cell = request_data->neighbouring_cell; 

    // return data allocation
    m_reserveCellResponse* response_data = new m_reserveCellResponse; // response message

    bool cell_reserved; // variable to whether cell reservation is possible

    // processing if cell can be reserved
    // in this case, vectors allocated in response_data will be modified
    if(GlobalMap->nodes[target_cell.y][target_cell.x] == 1){ // if the target cell has already been explored
        // gathering portion of map outwards from unexplored node to return to robot
        gatherPortionofMap(target_cell, neighbouring_cell, response_data->map_coordinates, response_data->map_connections, response_data->map_status);
        cell_reserved = false; // cell has not been successfully reserved for requesting robot
    }
    else if (GlobalMapInfo[target_cell.y][target_cell.x].reserved > 0){ // if the target cell has already been reserved
        cell_reserved = false; // return false as cell has not been reserved
    }
    else{ // if unreserved and unexplored
        GlobalMapInfo[target_cell.y][target_cell.x].reserved = robot_id; // reserving cell for exploration
        cell_reserved = true; // cell has been reserved
    }

    *response_data->cell_reserved = cell_reserved; // adding information is cell was reserved to response

    // attaching response data
    request->return_data = (void*)response_data;
    
    delete request_data; // deleting dynamically allocated message data

    sem_post(request->res_sem); // signalling Robot that return message is ready
    sem_wait(request->ack_sem); // waiting for Robot to be finished with response so message can be deleted


    // sent message was dynamically allocated thus must be deleted
    // this part of the code should be thread safe as the robot and Controller have finished using these variabless
    delete request;

    return;
}

void RobotMaster::updateRobotLocationRequest(Message* request){
    // updateRobotLocation request msg_data layout:
    // [0] = type: (unsigned int*), content: id of robot sending request
    // [1] = type: (Coordinates*), content: cell which robot now occupies

    // no return data

    // gathering incoming request data
    m_updateRobotLocationRequest* request_data = (m_updateRobotLocationRequest*)request->msg_data; 
    unsigned int robot_id = request_data->robot_id;
    Coordinates new_robot_location = request_data->new_robot_location;                    

    for(int i = 0; i < tracked_robots.size(); i++){
        if(tracked_robots[i].robot_id == robot_id){
            tracked_robots[i].robot_position = new_robot_location;
            break;
        }
    }

    delete request_data; // deleting dynamically allocated message data

    sem_post(request->res_sem); // signalling Robot that message is ready
    sem_wait(request->ack_sem); // waiting for Robot to be finished with response so message can be deleted


    // sent message was dynamically allocated thus must be deleted
    // this part of the code should be thread safe as the robot and Controller have finished using these variabless
    delete request;

    return;
}

unsigned int RobotMaster::addRobot(unsigned int x, unsigned int y, RequestHandler* r){ // adding robot to control system
                                                                                       // this must be completed by all robots before beginning exploration
    
    number_of_unexplored++; // incrementing number of unexplored by 1 as current robot cells has presumably not been explored

    id_tracker++; // incrementing inorder to determine next id to give a robot

    RobotInfo temp; // buffer to store robot info before pushing it to the tracked_robots vecto

    temp.robot_id = id_tracker; // assigning id to new robot entry 
    temp.robot_position.x = x;  // assigning position to new robot entry 
    temp.robot_position.y = y;
    temp.robot_status = 0;      // updating current robot status to 0 to leave it on stand by
    temp.Robot_Message_Reciever = r; // assigning Request handler for Master -> robot communications

    tracked_robots.push_back(temp); // adding robot info to tracked_robots

    return temp.robot_id; // returning id to be assigned to the robot which triggered this function
}

void RobotMaster::removeRobot(unsigned int id){
    for(int i = 0; i < tracked_robots.size(); i++){ // search for robot in tracked_robots
        if(tracked_robots[i].robot_id == id){ // if robot has been identified
            tracked_robots.erase(tracked_robots.begin()+i); // delete it from tracked_robots
        }
    }
    
    return;
}

void RobotMaster::updateGlobalMap(unsigned int* id, std::vector<bool>* connections, Coordinates* C){

    updateRobotLocation(id, C);

    GlobalMapInfo[C->y][C->x].reserved = 0; // unreserving cell as it has been scanned

    if (GlobalMap->nodes[C->y][C->x] != 1){ // checking if there is a need to update map (has the current node been explored?)
        
        number_of_unexplored--; // subtracting number of unexplored cells as new cell has been explored

        // updating vertical edges in GlobalMap using robot reading
        GlobalMap->y_edges[C->y][C->x] = (*connections)[0]; // north
        GlobalMap->y_edges[C->y + 1][C->x] = (*connections)[1]; // south

        GlobalMap->x_edges[C->y][C->x] = (*connections)[2]; // east
        GlobalMap->x_edges[C->y][C->x + 1] = (*connections)[3]; // west

        GlobalMap->nodes[C->y][C->x] = 1; // updating state of node to be 1 as it has been explored

        // now we will update the neighbouring cells to see if they have previously been explored
        // if not, they will be marked with a '2' on the GlobalMap Nodes Array

        if(!GlobalMap->y_edges[C->y][C->x] && GlobalMap->nodes[C->y - 1][C->x] == 0){ // checking if node to north hasn't been explored by a Robot
            GlobalMap->nodes[C->y - 1][C->x] = 2; // if unexplored and no wall between robot and cell, set northern node to unexplored
            number_of_unexplored++; // incrementing number of unexplored nodes by 1 as this neighbouring node has not been explored
        }
        // checking south
        if(!GlobalMap->y_edges[C->y + 1][C->x] && GlobalMap->nodes[C->y + 1][C->x] == 0){ // checking if node to north hasn't been explored by a Robot
            GlobalMap->nodes[C->y + 1][C->x] = 2; // if unexplored and no wall between robot and cell, set southern node to unexplored
            number_of_unexplored++; // incrementing number of unexplored nodes by 1 as this neighbouring node has not been explored
        }
        // checking east
        if(!GlobalMap->x_edges[C->y][C->x] && GlobalMap->nodes[C->y][C->x - 1] == 0){ // checking if node to north hasn't been explored by a Robot
            GlobalMap->nodes[C->y][C->x - 1] = 2; // if unexplored and no wall between robot and cell, set eastern node to unexplored
            number_of_unexplored++; // incrementing number of unexplored nodes by 1 as this neighbouring node has not been explored
        }
        // checking wast
        if(!GlobalMap->x_edges[C->y][C->x + 1] && GlobalMap->nodes[C->y][C->x + 1] == 0){ // checking if node to north hasn't been explored by a Robot
            GlobalMap->nodes[C->y][C->x + 1] = 2; // if unexplored and no wall between robot and cell, set western node to unexplored
            number_of_unexplored++; // incrementing number of unexplored nodes by 1 as this neighbouring node has not been explored
        }
    }
    else{ // if there is no need to update map

    }

    printGlobalMap(); // print updated GlobalMap

    return;
}

std::vector<bool> RobotMaster::getNodeEdgeInfo(Coordinates* C){
    std::vector<bool> edge_info; // vector to return with information on edges surrounding node C

    edge_info.push_back(GlobalMap->y_edges[C->y][C->x]);    // [0] = north edge
    edge_info.push_back(GlobalMap->y_edges[C->y + 1][C->x]);// [1] = south edge
    edge_info.push_back(GlobalMap->y_edges[C->y][C->x]);    // [2] = east edge
    edge_info.push_back(GlobalMap->y_edges[C->y][C->x + 1]);// [3] = west edge

    return edge_info;
}

std::vector<Coordinates> RobotMaster::getSeenNeighbours(unsigned int x, unsigned  int y){ // function to gather seen neighbouring cells of a selected cell based on global map

    std::vector<Coordinates> ret_value; // vector of Coordinates to return
                                        // this will contain the coordinates of valid neighbouring nodes
    
    Coordinates buffer; // buffer structor to gather positions of neighbouring nodes before pushing to vector

    // check if neighbour to the north is valid and connected via an edge (no wall)
    if(y != 0){ // protection to ensure invalid part of nodes is not accessed if y = 0
        if(!GlobalMap->y_edges[y][x] && GlobalMap->nodes[y-1][x] > 0){
            buffer.x = x; 
            buffer.y = y - 1;
            ret_value.push_back(buffer);
        }
    }
    // check if neighbour to the south is valid and connected via an edge (no wall)
    if(y != maze_ysize - 1){
        if(!GlobalMap->y_edges[y+1][x] && GlobalMap->nodes[y+1][x] > 0){
            buffer.x = x; 
            buffer.y = y + 1;
            ret_value.push_back(buffer);
        }
    }
    // check if neighbour to the east is valid and connected via an edge (no wall)
    if(x != 0){ // protection to ensure invalid part of nodes is not accessed if x = 0
        if(!GlobalMap->x_edges[y][x] && GlobalMap->nodes[y][x-1] > 0){
            buffer.x = x - 1; 
            buffer.y = y;
            ret_value.push_back(buffer);
        }
    }
    // check if neighbour to the west is valid and connected via an edge (no wall)
    if(x != maze_xsize - 1){
        if(!GlobalMap->x_edges[y][x+1] && GlobalMap->nodes[y][x+1] > 0){
            buffer.x = x + 1; 
            buffer.y = y;
            ret_value.push_back(buffer);
        }
    }
    return ret_value; // returning vector
}

void RobotMaster::gatherPortionofMap(Coordinates curr_node, Coordinates neighbour_node, std::vector<Coordinates>* map_nodes, std::vector<std::vector<bool>>* map_connections, std::vector<char>* node_status){ // generates a portion of the map for transfer to robot using breadth first search

    std::queue<Coordinates> node_queue; // creating node queue to store nodes to be "explored" by algorithm

    // initializing value of queue and 
    node_queue.push(curr_node); // adding first node to explore to node queue
    
    // gathering starting cell info for return
    map_nodes->push_back(curr_node); // node coordinates
    map_connections->push_back(getNodeEdgeInfo(&curr_node)); // node connections
    node_status->push_back(GlobalMap->nodes[curr_node.y][curr_node.x]); // node status

    while(node_queue.size() != 0){// while nodes to explore are in node_queue
        
        curr_node = node_queue.front(); // gathering node from front of queue

        //printf("curr node: %d,%d\n", curr_node.x, curr_node.y);

        node_queue.pop(); // removing node from front of the queue as new nodes must be added to queue

        std::vector<Coordinates> valid_neighbours = getSeenNeighbours(curr_node.x, curr_node.y); // gathering neighbours of current node

        for(int i = 0; i < valid_neighbours.size(); i++){ // iterate through all of the current node's neighbours to see if they have been explored
            
            bool already_visited = false;
            for(int j = 0; j < map_nodes->size(); j++){
                if(valid_neighbours[i] == (*map_nodes)[j]){
                    already_visited = true;
                    break;
                }
            }

            if(!already_visited){
                map_nodes->push_back(valid_neighbours[i]); // node coordinates
                map_connections->push_back(getNodeEdgeInfo(&valid_neighbours[i])); // node connections
                node_status->push_back(GlobalMap->nodes[valid_neighbours[i].y][valid_neighbours[i].x]); // node status
            }
        }
    }

    return; // can return with map information
}

bool RobotMaster::checkIfOccupied(unsigned int x, unsigned int y, unsigned int* ret_variable){ // checks if a robot is within the cell passed into the function
                                                                                               // returns true is a robot is detected
                                                                                               // if a robot is found, ret_variable is modified to contain the id of the found robot
    unsigned int temp = 0;
    for(int i = 0; i < tracked_robots.size(); i++){ // looping through robots position
        if(tracked_robots[i].robot_position.x == x && tracked_robots[i].robot_position.y == y){ // if robot is occupying the location passed in
            temp = tracked_robots[i].robot_id; // returning found robot id
            *ret_variable = temp;

            return true; // return true as robot is occupying the cell
        }
    }
    return false; // returning false as robot is not found within the occupied cell
}

void RobotMaster::updateRobotLocation(unsigned int* id, Coordinates* C){ // updates the location of a robot to the location specified

    for(int i = 0; i < tracked_robots.size(); i++){ // finding robot to update
        if (tracked_robots[i].robot_id == *id){ // if robot found using id
            tracked_robots[i].robot_position = *C; // update position in RobotInfo
            
            // TODO: update occupying robot information in CellInfo matrix
            
            break; 
        }
    }
    return;
}
    
void RobotMaster::updateAllRobotState(int status){

    for(int i = 0; i < max_num_of_robots; i++){ // creating messages to update state of all robots

        Message* messages = new Message; // creating new messages for each robot

        // assigning data to message
        m_updateRobotStateRequest* message_data = new m_updateRobotStateRequest;
        message_data->target_state = status;

        messages->msg_data = (void*) message_data; // specifying state to update all robots to
        
        tracked_robots[i].Robot_Message_Reciever->sendMessage(messages); // sending message
        tracked_robots[i].robot_status = status; // updating local robot information to current status
    }

    return;    
}

/*
void RobotMaster::printRequestInfo(Message* request){
    // gathering request information
    unsigned int request_id = request_id_tracker;
    int request_type = request->msg_data;
    
    //printing general request information
    printf("~~~~~\n");
    printf("Request %d\n", request_id);
    printf("Type = %d", request_type);
    
    switch(request_type){
        case -1: // shutDown confirmation request ( telling master robot has finished exploring)
        {
            printf(" - Shut Down Request\n");
            printf("~~~~~\n");
            printf("Robot Shutting Down: %d", request->msg_data[0]);
            
            break;
        }
        case 0: // addRobot request
        {
            printf(" - Add Robot Request\n");

            printf("~~~~~\n");
            printf("Request Data:\n");
            printf("Robot X Position: %d\n", request->msg_data[0]);
            printf("Robot Y Position: %d\n", request->msg_data[1]);

            printf("~~~~~\n");
            printf("Response Data:\n");
            printf("Robot Assigned id: %d", request->return_data[0]);
            
            break;
        }
        case 1: // updateGlobalMap request
        {   
            printf(" - Update Global Map Request\n");

            printf("~~~~~\n");
            printf("Request Data:\n");
            printf("Robot ID: %d\n", request->msg_data[0]);

            // TODO: print wall information

            printf("Scan Position: x - %d, y = %d\n");

            break;
        }
        case 2: // move2cell request
        {
            break;
        }
        case 3: // reserveCell request (robot wants to start exploring from a cell without other robots using it)
        {
            break;
        }
        case 4: // update Robot Location  (tells master that robot has completed move operation)
        {
            break;
        }
    }

    printf("~~~~~\n");
}*/

bool RobotMaster::printGlobalMap(){ // function to print global map of maze including robot location
                                     // maze design based off what can be seen here: https://www.chegg.com/homework-help/questions-and-answers/using-c-1-write-maze-solving-program-following-functionality-note-implementation-details-a-q31826669

    std::string logos[9] = { "   ", "---", "|", " ", " R ", " . ", " X ", " * "}; // array with logos to use when printing maze
    
    if(maze_xsize == 0 || maze_ysize == 0){ // if maze has not been allocated
        printf("Error: Maze size has not been specified\n");
        return false; // return false as printing failed
    }

    printf("*Global Map*\n"); // printing title and maze information

    int string_pointer = 0; // integer used to determine which logo needs to be printed from logo vector

    int count = 0; // counter to determine if both the column and row edges have been printed
    int i = 0; // counter to track if the whole maze has been printed

    while(!(i >= maze_ysize && count == 1)){ // while loop to determing which row to print (i = node row number)
    
        if(count == 0){ // printing the horizontal walls of maze

            for(int j = 0; j < maze_xsize; j++){

                if(GlobalMap->y_edges[i][j]){ // if there is no edge between two nodes
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
                if(GlobalMap->x_edges[i][j]){ // if there is no edge between two nodes
                    string_pointer = 2; // print horizontal line
                }
                else{ // if there is an edge between two nodes
                    string_pointer = 3; // print horizontal line
                }
                printf("%s", logos[string_pointer].c_str());

                unsigned int* found_id = new unsigned int; // variable to store id of robot if found using checkIfOccupied in the second else if statement

                // checking contents of a node (e.g. does it have a robot?)
                if (j >= maze_xsize){ // if iterating outside of valid node
                    string_pointer = 3; // print empty space as node is outside of maze
                }
                else if(checkIfOccupied(j, i, found_id)){ // if current node is the robot's location
                    printf("%2d ", *found_id); // printing id number of robot within the cell
                                               // TODO: print 3 width digit numbers in centre of cell without error
                    string_pointer = -1; // print nothing after this ifelse statement as the printing has been handled locally
                }
                else if(GlobalMap->nodes[i][j] == 0){ // if current node is invalid (unseen and unexplored)
                    string_pointer = 6; // print I for invalid cell
                }
                else if(GlobalMap->nodes[i][j] == 2){ // if current node has been seen but not explored
                    string_pointer = 7; // print * for seen node
                }
                else{ // if current cell has been seen and explored (valid)
                    string_pointer = 0; // print empty space
                }
                if (string_pointer != -1) // if printing is needed
                    printf("%s",logos[string_pointer].c_str());

                delete found_id; // deleting found_id as dynamically allocated
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