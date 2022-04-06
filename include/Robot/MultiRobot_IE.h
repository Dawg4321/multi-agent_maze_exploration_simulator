#ifndef MULTIROBOT_IE_H
#define MULTIROBOT_IE_H

#include "MultiRobot.h"

class MultiRobot_IE: virtual public MultiRobot{
    public:

        MultiRobot_IE(); // constructor for multi-robot collision
        virtual ~MultiRobot_IE(); // virtual destructor to ensure child destructor is called during "delete" to base class pointer

        void requestReserveCell(); // attempts to reserve a cell to explore from the RobotMaster
                                   // if cell to reserve fails, LocalMap is updated with GlobalMap information

        int handleCellReserveResponse(Message* response, int current_status); // handles response for collision messages
                                                                              // must be used with RobotMaster::handleMasterRequest

        void updateLocalMap(std::vector<Coordinates>* map_info, std::vector<std::vector<bool>>* edge_info, std::vector<char>* map_status); // updates robot's map with information from vectors

        void BFS_noPathFound(); // overriden function which handles if a path is not found
        bool BFS_exitCondition(Coordinates* node_to_test); // overriden Exit condition for finding nearest unexplored cell
                                                           // must be overriden as already reserved cells must be ignored when selecting a new cell to reserve
    private:
        bool isCellAlreadyReserved(Coordinates* C); // checks if a cell is already reserved by another robot
        
        std::vector <Coordinates> already_reserved_cells; // vector with cells that are already reserved by other robots
                                                          // ensures robot will attempt to reserve next closest unreserved cell
};

#endif