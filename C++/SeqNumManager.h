/**
 *This class manages the sequence numbers for all seen MAC addresses. It 
 *has the ability to increment seqNum's and add new ones for yet unseen 
 *MAC addresses
 *
 * Author: Erin Jamroz, Weston Thornburg 
 * Date: November 2012
 * CSCI 325 University of Puget Sound
 */

#pragma once

#include <iostream>
#include <tr1/unordered_map>

using std::tr1::unordered_map;


class SeqNumManager {
    public:
        /**
         * Constructor 
         */
        SeqNumManager(short maxSeqNum) 
            : max(maxSeqNum) {}; 

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
        short max; //Gets max sequence number
};

inline
void
SeqNumManager::increment(short mac) {
    short temp; //gets seqNum
    //check existance 
    if (theMap.count(mac)) {
        if (theMap[mac] < max) {
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

inline
short
SeqNumManager::getSeqNum(short mac) {
    if (theMap.count(mac)) {
        return theMap[mac];  //Return the seqNum
    }
    else {
        return -1;  //Address did not exist
    }
}













