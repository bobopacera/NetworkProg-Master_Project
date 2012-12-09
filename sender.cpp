/* 
 * Author: Erin Jamroz
 * Dater: November 2012
 * class: CSCI 325 University of Puget Sound
 */

//files included
#include <iostream> //For cout & endl
#include "sender.h"
#include <unistd.h> //For sleep()
#include <math.h> //for pwr

//Wait IFS (SIFS + 2*slotTime)
#define waitIFS usleep((aSIFSTime + 2*aSlotTime)*1000);

//Items used
using std::queue;
using std::rand;

void 
Sender::MasterSend() {
    //Overhead
    cmd3 = 0;     //Initialize to 0 so we send a beacond right away
    long long sendBeaconTime = theRF->clock() + (cmd0 * 1000);  //Current time + 
                                                                                                         //cmd 3 val (sec)
    int packetSent = 0; //Flag to mark if data was sent
    int beaconSent = 0; //Flag to mark if a beacon was sent

    //Run forever doing all the things that sender does
    while (true) {
        //Check for updated cmd values
        cmd0 = cmdVals[0];
        cmd1 = cmdVals[1];
        cmd2 = cmdVals[2];
        cmd3 = cmdVals[3];

        //Write to ostream
        if (cmd0) {
            pthread_mutex_lock(mutexSenderOstream);   //lock the ostream 
            *outputBuff << "Diagnostic levels: \n" << 
                                    "1- Full, display all state information \n" <<
                                    "2- Display beacon state information only \n" <<
                                    "3- Display transmittion and sender state information together \n" <<
                                    "4- Display transmittion state information only \n" <<
                                    "5- Display reciever state information only \n \n" <<
                                    "Current Diagnostic level: " << cmd1 << "\n" <<
                                    "Beacon window: " << cmd3 * 1000 << "seconds \n" << endl;
                        if (cmd2) {
                            *outputBuff << 
                                "Collision window: set to Max(" << waitTime << ")" << "\n" << endl;
                        } else {
                            *outputBuff << 
                                "Collision window: set to Random(" << waitTime << 
                                ")" << "\n" << endl;
                        }
            pthread_mutex_unlock(mutexSenderOstream);   //Unlock the ostream
            cmd0 = 0;   //So that this does not keep printing
        }

        //Send Beacon if it is time:
        if (theRF->clock() + *fudgeFactor >= sendBeaconTime) {     //Current time is at or past the time to 
                                                                            //send a beacon
            //update time fudge factor
            pthread_mutex_lock(mutexFudgeFactor);   //lock the mutex
            long long fudFact = *fudgeFactor;   //update fudge factor
            pthread_mutex_unlock(mutexFudgeFactor); //unlock mutex

            //Write to ostream
            if (cmd1 == 1 || cmd1 == 2) {
                pthread_mutex_lock(mutexSenderOstream);   //lock the ostream 
                *outputBuff << "Sending Beacon with timestamp: " << 
                theRF->clock() + fudFact + TRANSTIME << endl;
                pthread_mutex_unlock(mutexSenderOstream);   //Unlock the ostream
            }

            buildBeacon(theRF->clock() + *fudgeFactor, TRANSTIME);  //Fill pachyderm with a beacon
            char beaconFrame[pachyderm.frame_size]; //Gets the byte array to send
            pachyderm.buildByteArray(&beaconFrame[0]);  //Fill the frame

            int beaconSent = send(&beaconFrame[0], pachyderm.frame_size, false, aCWmin);
            sendBeaconTime = theRF->clock() + *fudgeFactor + (cmd3 * 1000); //Set a new beacon
                                                                                                                        //time 
            //Write to ostream
            if (cmd1 == 1 || cmd1  == 2) { 
                pthread_mutex_lock(mutexSenderOstream);   //lock the ostream 
                if (beaconSent) {
                    *outputBuff << "Beacon transmitted" << endl;
                } else {
                    *outputBuff << "Beacon failed to transmit" << endl;
                }
                pthread_mutex_unlock(mutexSenderOstream);   //Unlock the ostream
            }
        }   //End send beacon


        //Lock mutex, block until you can
        pthread_mutex_lock(mutexSender);

        //Check for data to send
        if (infoToSend->empty()) {
            pthread_mutex_unlock(mutexSender); //Unlock because the queue is not ready
            usleep(1000);
        }
        else {
            //Get incomplete packet to send
            pachyderm = infoToSend->front();
            infoToSend->pop(); 
            pthread_mutex_unlock(mutexSender); //Unlock b/c we are done with queue 
            
            //Construct the frame to transmit
            seqTable.increment(pachyderm.destination); //Increment the seq nume
            buildFrame(0, false, seqTable.getSeqNum(pachyderm.destination), 0xff); 

            //Build the byte[] to be send
            char theFrame[pachyderm.frame_size];
            pachyderm.buildByteArray(&theFrame[0]); //Fill theFrame
            
            //Prepare to transmit
            *ackReceived = false;   //Set acknowlegement to false because message has not
                                                //yet been sent, so it cant have been acknowleged 
            *expSeqNum = seqTable.getSeqNum(pachyderm.destination); //Alert reciever of 
                                                                                                             //which seqNum to look for
            //Write to ostream
            if (cmd1 == 1 || cmd1 == 3) {
                pthread_mutex_lock(mutexSenderOstream);   //lock the ostream 
                *outputBuff << "Attempting to transmit packet with seqNum: " << 
                    seqTable.getSeqNum(pachyderm.destination) << endl;
                pthread_mutex_unlock(mutexSenderOstream);   //Unlock the ostream
            }

            //Transmit
            int packetSent = send(&theFrame[0], pachyderm.frame_size, false, aCWmin);

            //Write to ostream
            pthread_mutex_unlock(mutexSenderOstream);
            *outputBuff << "Data transmitted" << endl;
            pthread_mutex_unlock(mutexSenderOstream);

            //Handle retransmit
            while (pachyderm.resTransAttempts < dot11RetryLimit && !*ackReceived) {    //Less than max resend
                                                                                                                                     // and no ack recieved 
                usleep(WAITTIME);   //Sleep for predetermined amount of time
                if (!(*ackReceived))   {   //No ack recieved

                    //Write to ostream
                    if (cmd1 == 1 || cmd1 == 3) {
                        pthread_mutex_lock(mutexSenderOstream);   //lock the ostream 
                        *outputBuff << "Sender timeout: Restransmitting("<< pachyderm.resTransAttempts << ")" << endl;
                        pthread_mutex_unlock(mutexSenderOstream);   //Unlock the ostream
                    }

                    //Update status code
                    pthread_mutex_lock(mutexStatus);    //Lock the status mutex
                    *statCode = 5;      //TX_FAILED
                    pthread_mutex_unlock(mutexStatus);  //Unlock the mutex

                    resend();   //retransmit 
                }
            }

            //Check for status update
            if (pachyderm.resTransAttempts == dot11RetryLimit) {
                //Write to ostream
                if (cmd1 == 1 || cmd1 == 3) {
                    pthread_mutex_lock(mutexSenderOstream);   //lock the ostream 
                    *outputBuff << "Sender timeout: Resend limit reached -> transmition terminated: Attempt # " << pachyderm.resTransAttempts <<  endl;
                    pthread_mutex_unlock(mutexSenderOstream);   //Unlock the ostream
                }
                
            }
        }
    }
}

int
Sender::buildFrame(short frm, bool resend,  short seqNum, int CS) {
    pachyderm.frametype = frm;
    pachyderm.resend = resend;
    pachyderm.sequence_number = seqNum;
    pachyderm.sender = macAddr_Sender;
    pachyderm.CRC = CS;
    if (!resend) {
        pachyderm.resTransAttempts = 0;
    }
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
    pachyderm.resTransAttempts = 0; //Because we dont retrans these

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
        if (!theRF->inUse()) {
            waitIFS
            if (!theRF->inUse()) {  //Perfect transmittion
                //Align the send with a 50 milsec interval
                while ((theRF->clock() + fudFact) % 50) {
                        usleep(1000);  //Sleep 1 milsec  
                }
               if (theRF->transmit(frame, size) != size) {  //Makes the transmission
                    //Update status code
                    pthread_mutex_lock(mutexStatus);    //Lock the status mutex
                    *statCode = 2;      //UNSPECIFIED_ERROR
                    pthread_mutex_unlock(mutexStatus);  //Unlock the mutex

                    return 0; //Did not send all of frame or something failed internally
                } else {
                    //Write to ostream
                    if (cmd1 == 1 || cmd1 == 3 || cmd1 == 4) {
                        pthread_mutex_lock(mutexSenderOstream);   //lock the ostream 
                        *outputBuff << "Channel status: idle" << endl;
                        pthread_mutex_unlock(mutexSenderOstream);   //Unlock the ostream
                    }
                    return 1; //Frame transmitted properly 
                }
            }
        } 
        //Write to ostream
        if (cmd1 == 1 || cmd1 == 3 || cmd1 == 4) {
            pthread_mutex_lock(mutexSenderOstream);   //lock the ostream 
            *outputBuff << "Channel status: busy -> waiting for transmittion to end" << endl;
            pthread_mutex_unlock(mutexSenderOstream);   //Unlock the ostream
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
    if (cmd2) {
        waitTime = (int)pow((float)2, (float)cWindow); //Pick max size for collision window
    } else {
        waitTime = rand() % (int)pow((float)2, (float)cWindow);  //Random number in range 
                                                                                              //[0,2^aCWmin) 
    }  

    //Write to ostream
    if (cmd1 == 1 || cmd1 == 3 || cmd1 == 4) {
        pthread_mutex_lock(mutexSenderOstream);   //lock the ostream 
        *outputBuff << "Channel status: busy -> picking wait time in range [0, " << 
                (int)pow((float)2, (float)cWindow) << "]" << endl;
        pthread_mutex_unlock(mutexSenderOstream);   //Unlock the ostream
    }

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
                return 0; //Did not send all of frame or something failed internally
            } else {
                //Write to ostream
                if (cmd1 == 1 || cmd1 == 3 || cmd1 == 4) {
                    pthread_mutex_lock(mutexSenderOstream);   //lock the ostream 
                    *outputBuff << "Channel status: idle" << endl;
                    pthread_mutex_unlock(mutexSenderOstream);   //Unlock the ostream
                }

                return 1; //Frame transmitted properly 
            }
        }
    }   //End Exponential Backoff
}

int 
Sender::resend() {
    buildFrame(0, true, seqTable.getSeqNum(pachyderm.destination), 1111);
    char theFrame[pachyderm.frame_size];
    pachyderm.buildByteArray(&theFrame[0]); //Fill theFrame
    pachyderm.resTransAttempts++;     //Increment the times we have tried to resend 

    return send(&theFrame[0], pachyderm.frame_size, true, 
                        aCWmin + pachyderm.resTransAttempts); 
}














