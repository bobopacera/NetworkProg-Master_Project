/**
 * This class specifies the methods for the Sender class
 * 
 * Author: Erin Jamroz
 * Date: October 2012
 * Class: CSCI 325 University of Puget Sound
 */

using namespace std;
#include <iostream>;
#include <Packet.h>; 

class Sender {
    public:
        /**
         * Constructor for objects of the Sender class
         */
        sender();

    private:
    //Fields
    //TODO Clean up * locations on fields
    //TODO Make field types align with flag types for ack methods
        short *MacAddr; //Our MAC address
        *ostream dataStream; //ostream provided to us
        *queue<short,char,int> outgoing_Queue:  //pointer to outgoing message queue
        short *MACAck; //Pointer to the MAC address of the most recently
                       // received ack 
        *bool ackReceived; //Pointer to flag for received acks
        packet pachyderm; //The packet to send

    //Methods
        /**
         *Checks the sender queue for data to be sent and checks the Ack
         *field for flags indicated acknowledgments needing to be sent
         *@return 0 nothing to send
         *@return 1 data to send
         *@return 2 ack to send
         */
        //TODO Clean this method so that it is a one step process
        int  check_QueueAndAck();

        /**
         *Checks for an acknowledgment received in demiBrad
         @return 0 No ack received
         @return Mac address of received ack  
         */
        //TODO How will you tell which message an ack is for using this system?
        unsigned short check_ReceivedAck();

        /**
         *Checks to see if an ack needs to be sent
         *@return 0 No ack needs to be sent
         *@return The MAC address to send and ack to 
         */
        unsigned short check_SendAck();

        /**
         *Builds a packet object for sending
         *@param frm
         *@param resend Indicates if this packet is part of  a retransmission
         *@param seqNum This packets sequence number
         *@param destAddr The destination's MAC address
         *@param sendAddr The sender's MAC address
         *@param data The data to transmit
         *@param CS
         //TODO What are CS and frm?
         *@return 1 if packet was successfully built
         */
        int buildPacket(enum frm, bool resend, unsigned short seqNum, 
                unsigned short destAddr, unsigned short senderAddr,
                *char data, int CS);
       
        /**
         *Resends the current packet
         *@return 1 if packet was successfully sent
         */
        int resend()

















}
