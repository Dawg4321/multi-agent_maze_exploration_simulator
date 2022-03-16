#ifndef REQUEST_TEMPLATES_H
#define REQUEST_TEMPLATES_H

#include <vector>

#include "RequestHandler.h"
#include "Coordinates.h"

// this header file defines the various message types used for robot <-> master communication

// ~~~~~~~~~~~~~~~~~~~~~~~~~
// Defining IDs to various requests
// ~~~~~~~~~~~~~~~~~~~~~~~~~

// IDs used for Robot -> Master Requests
#define shutDownRequestID -1
#define addRobotRequestID 0
#define updateGlobalMapRequestID 1
#define move2CellRequestID 2
#define reserveCellRequestID 3
#define updateRobotLocationRequestID 4

// IDs used for Robot -> Master Requests
#define updateRobotStateRequestID 0


// ~~~~~~~~~~~~~~~~~~~~~~~~~
// Request Template
// ~~~~~~~~~~~~~~~~~~~~~~~~~

struct m_genericRequest{ // template structure used by all requests
    const int request_type; // defines type of request sent

    m_genericRequest(int x): request_type(x){ // assigning request type to const int 

    }
};

// ~~~~~~~~~~~~~~~~~~~~~~~~~
// Robot -> Master Messages 
// ~~~~~~~~~~~~~~~~~~~~~~~~~

// Standard Message Layout

// ** shutDownRequest Messages **
struct m_shutDownRequest:m_genericRequest{ // request to notify robot master of robot shutting down
    unsigned int robot_id; // id of robot shutting down

    // Constructor
    m_shutDownRequest():m_genericRequest(shutDownRequestID){ // assigning request id to request message    
    
    }    
};

struct m_shutDownResponse{ // no response needed to shutdown request hence empty

};

// ** addRobotRequest Messages **
struct m_addRobotRequest:m_genericRequest{ // request to notify controller of robot's existance and location

    unsigned int x; // cordinates of robot
    unsigned int y; // this will be updated in robot information of robot master

    RequestHandler* robot_request_handler; // pointer to request handler for master -> robot communications

    // Constructor
    m_addRobotRequest():m_genericRequest(addRobotRequestID){ // assigning request id to request message
       
    }  

};
struct m_addRobotResponse{ // response containing robot's assigned id
                           // this is critical in allowing the master to differentiate between each robot's incoming requests
    unsigned int robot_id; // id assigned to the robot
};

// ** updateGlobalMapRequest **
struct m_updateGlobalMapRequest:m_genericRequest{
    unsigned int robot_id; // id of robot sending request
    std::vector<bool> wall_info; // content: vector containing information on walls surrounding robot
                                 // [0] = north, [1] = south, [2] = east, [3] = west
                                 // 0 = connection, 1 = wall

    Coordinates cords; // current coordinates of where the read occured

    // Constructor
    m_updateGlobalMapRequest():m_genericRequest(updateGlobalMapRequestID){ // assigning request id to request message

    }  
};
struct m_updateGlobalMapResponse{ // no response needed hence empty

};

// ** reserveCellRequest **
struct m_reserveCellRequest:m_genericRequest{
    unsigned int robot_id; // id of robot sending request
    Coordinates target_cell; // coordinates correspoonding to full map down a node path if path has been explored already
    Coordinates neighbouring_cell; // neighbouring cell used to enter target cell 
                                   // used to determine what aspect of tree must be sent back in event node has already been explored 
    // Constructor
    m_reserveCellRequest():m_genericRequest(reserveCellRequestID){ // assigning request id to request message
        
    }  
};
struct m_reserveCellResponse{
    bool* cell_reserved; // bool determining whether cell was successfully reserved
    
    // returned portion of global map
    // this is only used if the cell was not reserved
    std::vector<Coordinates>* map_coordinates; // coordinates correspoonding to full map down the requested node path
    std::vector<std::vector<bool>>* map_connections; // wall information corresponding to nodes in map_coordinates
    std::vector<char>* map_status; // node status information of nodes in map_coordinates

    m_reserveCellResponse(){ // allocating various vectors
        cell_reserved = new bool;
        map_coordinates = new std::vector<Coordinates>;
        map_connections = new std::vector<std::vector<bool>>;
        map_status = new std::vector<char>;
    }
    
    ~m_reserveCellResponse(){ // deallocating variou vectors
        delete map_connections;
        delete map_coordinates;
        delete map_status;
    }
};

// ** updateRobotLocationRequest **
struct m_updateRobotLocationRequest:m_genericRequest{
    unsigned int robot_id; // id of robot sending request
    Coordinates new_robot_location; // cell which robot now occupies

    // Constructor
    m_updateRobotLocationRequest():m_genericRequest(updateRobotLocationRequestID){ // assigning request id to request message
        
    }  
};
struct m_updateRobotLocationResponse{ // no response needed hence empty

};

struct m_move2CellRequest:m_genericRequest{
    unsigned int robot_id; // id of robot sending request
    Coordinates target_cell; // target cell of robot move request

    // Constructor
    m_move2CellRequest():m_genericRequest(move2CellRequestID){ // assigning request id to request message
        
    }  
};
struct m_move2CellResponse{
    bool can_movement_occur;
};

// ~~~~~~~~~~~~~~~~~~~~~~~~~
// Master -> Robot Messages 
// ~~~~~~~~~~~~~~~~~~~~~~~~~

struct m_updateRobotStateRequest:m_genericRequest{
    int target_state; // target state to set robot to

    m_updateRobotStateRequest():m_genericRequest(updateRobotStateRequestID){ // assigning request id to request message
        
    }
};

#endif