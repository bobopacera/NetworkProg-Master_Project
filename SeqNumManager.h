/**
 *This class manages the sequence numbers for all seen MAC addresses. It 
 *has the ability to increment seqNum's and add new ones for yet unseen 
 *MAC addresses
 *
 * Author: Erin Jamroz, Weston Thornburg 
 * Date: November 2012
 * CSCI 325 University of Puget Sound
 */

using namespace std;
#include <tr1/unordered_map>


class SeqNumManager {
    public:
        /**
         * Constructor 
         */
        SeqNumManager(unsigned short maxSeqNum); 

        /**
         * Increments the sequence number for a given MAC address or 
         * initializes value for unseen MAC
         * param mac the MAC address to increment
         */
        void increment(short mac);

        /**
         * returns the sequence number for a given MAC address
         * param mac the MAC address to look up
         * return the seqNum or -1 is the address is not in list
         */
        short getSeqNum(short mac);

    private:
        unordered_map<short, short> theMap; //Underlying data structure       
        unsigned short max; //Gets max sequence number
};

SeqNumManager::SeqNumManager(unsigned short maxSeqNum) {
    max = maxSeqNum;
}

void
SeqNumManager::increment(short mac) {
    short temp; //gets seqNum
    //check existance 
    if (theMap.count(mac)) {
        if (theMap.at(mac) < max) {
            temp = theMap[mac] + 1; //Get previous seqNum and increment
            theMap.erase(mac);  //Get rid of previous value
            theMap[mac] = temp; //Set new incremented value
        }
        else {
            theMap.erase(mac);  //Get rid of previous value
            theMap[mac] = 0; //SeqNum has rolled over
        }
    }
    else {
        theMap[mac] = 0;    //Create entry for new MAC addr
    }
}

short
SeqNumManager::getSeqNum(short mac) {
    if (theMap.count(mac)) {
        return theMap.at(mac);  //Return the sequNum
    }
    else {
        return -1;  //Address did not exist
    }
}














