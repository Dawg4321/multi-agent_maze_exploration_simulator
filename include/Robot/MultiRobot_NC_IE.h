#ifndef MULTIROBOT_NC_IE_H
#define MULTIROBOT_NC_IE_H

#include "MultiRobot.h"

class MultiRobot_NC_IE: public MultiRobot{
    public:

        MultiRobot_NC_IE(unsigned int x, unsigned int y, RequestHandler* r, unsigned int xsize, unsigned int ysize); // constructor for multi-robot exploration purposes

        // ** Robot loop function **
        void robotLoop(GridGraph* maze); // loop used by robot for operation

        // ** Robot -> Master Communication Functions **
        bool requestReserveCell(); // attempts to reserve a cell to explore from the RobotMaster
                                   // if cell to reserve fails, LocalMap is updated with GlobalMap information
}; 

#endif