/* 
 * Author: Erin Jamroz
 * Dater: November 2012
 * class: CSCI 325 University of Puget Sound
 */

//files included
#include <iostream> //For cout & endl
#include "sender.h"
#include <unistd.h> //For sleep()
//#include <stdlib.h> //For rand()
#include <math.h> //for pwr

//Wait IFS (SIFS + 2*slotTime)
#define waitIFS usleep((aSIFSTime + 2*aSlotTime)*1000);

//Items used
using std::queue;
using std::rand;


/*
//FROM PAUL: impolementing the intPow function
// it simply does powers for ints. The native c++ 
// pow function does not like ints, it uses doubles and floats.
int Sender::intPow(int base, int power){
    int acc = 0;
    while(power > 0){
        acc = acc * base;
        power--;
    }
    return acc;
}
*/

void 
Sender::MasterSend() {
    int cmd0, cmd1, cmd2, cmd3;     //Get cmd values

    //Run forever doing all the things that sneder does
    while (true) {
        //Check for updated cmd values
        cmd0 = cmdVals[0];
        cmd1 = cmdVals[1];
        cmd2 = cmdVals[2];
        cmd3 = cmdVals[3];

        //Lock mutex, block until you can
        pthread_mutex_lock(mutexSender);

        //Check for data to send
        if (infoToSend->empty()) {
            //wcerr << "QUEUE IS EMPTY" << endl;
            pthread_mutex_unlock(mutexSender); //Unlock because the queue is not ready
            usleep(1000);
        }
        else {
            //wcerr << " pop should happen" << endl;
           
            //Get incomplete packet to send
            pachyderm = infoToSend->front();
            infoToSend->pop(); 
            pthread_mutex_unlock(mutexSender); //Unlock bc we are done with queue 
            //wcerr << "Got stuff off the queue" << endl;
            seqTable.increment(pachyderm.destination);  //Increment the sequence number 
            
            //Construct the frame to transmit
            seqTable.increment(pachyderm.destination); //Increment the seq nume
            buildFrame(0, false, seqTable.getSeqNum(pachyderm.destination), 1111); 

            //Build the byte[] to be send
            char theFrame[pachyderm.frame_size];
            pachyderm.buildByteArray(&theFrame[0]); //Fill theFrame
            
            //Transmit
            //wcerr << "sending data" << endl;
            *ackReceived = false;   //Set acknowlegement to false because message has not
                                                //yet been sent, so it cant have been acknowleged 
            *expSeqNum = seqTable.getSeqNum(pachyderm.destination); //Alert reciever of 
                                                                                                              //which seqNum to look for
            int doesItSend = send(&theFrame[0], pachyderm.frame_size, false, aCWmin);
            //wcerr << "data sent!" << endl;

            //Handle retransmit
            while (pachyderm.resTransAttempts < dot11RetryLimit || !*ackReceived) {    //Less than max resend
                                                                                                                                  // and no ack recieved 
                usleep(WAITTIME);   //Sleep for predetermined amount of time
                if (!*ackReceived)   {   //No ack recieved
                    resend();   //retransmit 
                }
            }

            //Check for stsus update
            if (pachyderm.resTransAttempts = dot11RetryLimit) {
                //TODO WRITE STATUS CODE 5 TO OSTREAM
            }
        }
    }
             //Free memory because this is c++
}

int
Sender::buildFrame(short frm, bool resend,  short seqNum, int CS) {
    pachyderm.frametype = frm;
    pachyderm.resend = resend;
    pachyderm.sequence_number = seqNum;
    pachyderm.sender = macAddr_Sender;
    pachyderm.CRC = CS;
}

void
Sender::buildBeacon (long long timeParam, const long long beaconTime) {
    int jj = 0; //index into the data array

    //Fill parts of packet we know
    pachyderm.frametype = 2;    //Of type beacon
    pachyderm.resend = 0;   //Not a resend
    pachyderm.sequence_number = 0;  //No sequence number meaning
    pachyderm.destination = -1; //Sent to broadcast channel
    pachyderm.CRC = 0xff;   //CRC of -1
    pachyderm.bytes_to_send = 8;    //8 bytes of data
    pachyderm.frame_size = 18;  //18 total bytes to transmit

    //Get time
    long long timeTemp = timeParam + beaconTime;    //Our clock time plus how long 
                                                                                    //it takes to build and send a beacon

    //Fill the data array 
    for (int ii=sizeof(timeParam) - 1; ii>=0; ii--) {
        pachyderm.physical_data_array[jj] = pullByte(timeTemp, ii);
        jj++;
    }
}

unsigned char
Sender::pullByte(long long number, int index) {
    unsigned long long temp = (unsigned long long) number;  
    unsigned long long ii = 0xff << 8 * index;    //create a bitmask for byte at pos index
    unsigned char kk = (temp & ii) >> 8 * index;    //use the bitmask and shift back

    return kk;
}

int 
Sender::send(char* frame, int size, bool reSend, int cWparam) {
    //Adjust fudge factor
    pthread_mutex_lock(mutexFudgeFactor);   //lock the mutex
    long long fudFact = *fudgeFactor;   //update fudge factor
    pthread_mutex_unlock(mutexFudgeFactor); //unlock mutex

    if (!reSend) {  //This is not a retransmittion 
        //Wait for current channel to be idle
        //wcerr << "This is not a retransmission" << endl;
        if (!theRF->inUse()) {
            //wcerr << "The RF layer is not in use." << endl;
            waitIFS
            //wcerr << "Waiting IFS" << endl;
            if (!theRF->inUse()) {  //Perfect transmittion
                //wcerr << "The RF layer is still not in use yay!" << endl;
                //Align the send with a 50 milsec interval
                while ((theRF->clock() + fudFact) % 50) {
                        usleep(1000);  //Sleep 1 milsec  
                }
               if (theRF->transmit(frame, size) != size) {  //Makes the transmission
                //wcerr << "Did not send correctly" << endl;
                    return 2; //Did not send all of frame or something failed internally
                    //TODO Add status output
                } else {
                    //wcerr << "sent correctly" << endl;
                    return 1; //Frame transmitted properly 
                    //TODO Add status output
                }
            }
        } 
       //Channel was busy
        bool idleFlag = false;
        //Wait for channel to open
        while (!idleFlag) { 
            while ((theRF->clock() + fudFact) % 50 || theRF->inUse() ) {    //They are sending
                                                                                                                //And aligns with 50 
                                                                                                                //milsec mark
                usleep(1000); //Sleep 1 milSec
            }
            waitIFS
            //Check is channel is idle
            if (!theRF->inUse()) {
                idleFlag = true;    //Break
            }
        }
    } 

    //Exponential backoff:
    //Possibility for this to be a resend
    int cWindow = cWparam;
    int waitTime = rand() % (int)pow((float)2, (float)cWindow);  //Random number in range 
                                                                                                //[0,2^aCWmin) 
    while (true) {
        while (waitTime>0 && !theRF->inUse()) {     //We havent waited waitTime and 
                                                                            //No one else is transmitting
            usleep(1000);   //Sleep 1 milSec
            waitTime--;
        }   //StopClock:
        if (waitTime>0) {   //We havent finished waiting, but someone had transmitted
                while (theRF->inUse()) {    //They are transmitting
                    usleep(1000);   //Sleep 1 milSec
                }
                waitIFS
        } else {
            if (theRF->transmit(frame, size) != size) {  //Makes the transmission 
                return 2; //Did not send all of frame or something failed internally
            } else {
                return 1; //Frame transmitted properly 
            }
        }
    }   //End Exponential Backoff
}

int 
Sender::resend() {
    buildFrame(0, true, seqTable.getSeqNum(pachyderm.destination), 1111);
    char theFrame[pachyderm.frame_size];
    //char* pointerToTheFrame = &theFrame[0];
    pachyderm.buildByteArray(&theFrame[0]); //Fill theFrame
    pachyderm.resTransAttempts++;     //Increment the times we have tried to resend 

    return send(&theFrame[0], pachyderm.frame_size, true, 
                        aCWmin + pachyderm.resTransAttempts); 
}














