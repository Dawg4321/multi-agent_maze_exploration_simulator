#include "MultiRobot_NC_IE.h"

MultiRobot_NC_IE::MultiRobot_NC_IE(unsigned int x, unsigned int y, RequestHandler* r, unsigned int xsize, unsigned int ysize): MultiRobot(x, y, r, xsize, ysize){

}

void MultiRobot_NC_IE::robotLoop(GridGraph* maze){
    
    assignIdFromMaster(); // getting id from robotmaster before begining robot exploration
    
    int status = 0; // tracks status of robot
                    // -1 = shut down
                    // 0 = stand by
                    // 1 = explore
                    // initializing to zero as robot is placed on standby until master updates status 

    bool loop_break = false; // bool to break for loop if shutdown has occured

    while(!loop_break){
        
        status = getRequestsFromMaster(status); // checking if master wants robot to update status

        switch(status){
            case -1: // shutdown mode
            {
                requestShutDown(); // notify master that robot is ready to shutdown
                loop_break = true; // setting loop break to ensure break while loop
                break;
            }
            case 0: // standby
            {   
                // do nothing
                break;
            }
            case 1: // scan cell
            {   
                std::vector<bool> connection_data = scanCell(maze); // scan cell which is occupied by the robot 

                requestGlobalMapUpdate(connection_data); // sending message to master with scanned maze information

                status = 2; // setting status to 2 so pathfinding will occur on next loop cycle

                break;
            }
            case 2: // planned path
            {   
                bool cell_reserved = false;

                // repeat loop until cell which is being planned to has been reserved
                
                BFS_pf2NearestUnknownCell(&planned_path); // create planned path to nearest unknown cell
                    
                cell_reserved = MultiRobot::requestReserveCell();
                

                if(cell_reserved) // if cell was reserved, update status to 3 so that the robot will move on the next iteration of the loop
                    status = 3; // setting status to 3 so movement will occur on next loop cycle
                else // if no cell was reserved, set status to 2 so that another cell will be located
                    status = 2;
                break;
            }
            case 3: // move robot to next cell to scan
            {   
                for (int i = 0; i < planned_path.size(); i++){ // while there are movements left to be done by robot
                    if(!MultiRobot::move2Cell((planned_path[i]))){ // if movement fails
                        break; // break from outerloop as no movements can occur now
                    }
                }

                planned_path.clear(); // clearing planned_path as movement is complete / failed

                status = 1; // setting status to 1 so scan cell will occur on next loop cycle

                break;
            }
        }        
    }

    return;
}