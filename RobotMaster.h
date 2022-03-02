#ifndef ROBOTMASTER_H
#define ROBOTMASTER_H

#include <vector>
#include <pthread.h>
#include <semaphore.h>

#include "GridGraph.h"
#include "Coordinates.h"
#include "RequestHandler.h"

struct RobotInfo{ // structure to track information of various robots in the swarm
    unsigned int robot_id; // tracks the id of a robot in order for robot differentiation

    int robot_status; // tracks the status of a robot
                       // 0 = stand by
                       // 1 = exploring

    RequestHandler* Robot_Message_Reciever; // pointer to request handler for messages from RobotMaster to Robot

    Coordinates robot_position; // tracks the current position of a robot in the graph maze in cartesian form
};

struct CellInfo{ // structure to track information of each cell within the maze
                 // used in conjuction with GridGraph struct

    unsigned int occupying_robot; // tracks the id of a robot which is occupying a cell
                                  // will be 0 if unoccupied

    unsigned int reserved;  // tracks whether the cell has been reserved for exploration purposes
};

class RobotMaster{
    public:
        RobotMaster(RequestHandler* r, int num_of_robots, unsigned int xsize, unsigned int ysize);
        ~RobotMaster();

        bool receiveRequests();  // recieves and decodes request information from imcoming request
                                 // returns false until all cells have been explored

        bool checkIfOccupied(unsigned int x, unsigned int y, unsigned int* ret_variable); // checks if a cell is occupied by a robot

        unsigned int addRobot(unsigned int x, unsigned int y, RequestHandler* r); // adds robots to tracked_robots This is important to allow for the robot to be synchronized by the control system
        void updateGlobalMap(unsigned int* id, std::vector<bool>* connections, Coordinates* C); // updates global map with information from robot scan
        bool robotMoveCheck(); // function to aid in preventing robot collisions

        int changeRobotState(); // changes robot state using return variable

        bool printGlobalMap(); // prints global map with robot locations
        
        void updateRobotLocation(unsigned int* id, Coordinates* C); // updates the location of a robot to the location specified
        void updateAllRobotState(int status); // updates the state of all robots to the specified value

    private:
        GridGraph* GlobalMap; // Global Map of maze

        std::vector<std::vector<CellInfo>> GlobalMapInfo; // vector used to track status of various cells

        std::vector<RobotInfo> tracked_robots; // vector to track information on various robots within maze

        RequestHandler* Message_Handler; // pointer to request handler shared by all Robots and a RobotMaster objects

        unsigned int id_tracker; // tracks next id to give to robot

        int number_of_unexplored; // number of unexplored cells encountered by Robots

        unsigned int maze_xsize; // size of maze
        unsigned int maze_ysize; // this is only used for printing purposes

        const int max_num_of_robots; // variable which specifies number of robots needed for exploration
                                     // exploration won't begin until enough robots have been added
};

#endif