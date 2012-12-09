/*
 * This class runs in a glorious infinite loop in undying devotion to the high and mighty demiBrad
 * working ceaslessly, once started, as his reciver of messages. With its ear to the ground hearing even
 * the most lightfooted packets and determining their usefulness to us acting accordingly on information recived.
 * This means either alerting the sender the status of ACKs(ours or others), or saving data for our lordly demiBrad.
 * Weston Thornburg
 * University of Puget Sound CS 325
 * october 2012
 */



#include <iostream>
#include "packet.h"
#include "RF.h"
#include <queue>
#include <pthread.h>
#include "SeqNumManager.h"
using namespace std;

class Listener
{
public:

    /*
     * constructor for the listener class that sets up all our sexy varribles and
     * starts the thread listening for imcoming messages
     */
    Listener(RF* RFLayer, 
        queue<Packet>* incomingQueue, 
        volatile bool* receivedFlag, 
        short myMAC, 
        pthread_mutex_t * mutexListenr, 
        volatile short* exSN, 
        pthread_mutex_t *statusCodeMutex, 
        pthread_mutex_t *mutexListenerOstreamInput, 
        pthread_mutex_t *mutexDemibradFudgeFactorInput, 
        volatile long long *fudgeFactorDemibrad, 
        volatile int *cmds, 
        volatile int *statusCode,
        ostream * streamypaul) 
    : seqNumMang(MAXSEQNUM), 
    MACaddrList(myMAC), 
    ackReceivedL(receivedFlag), 
    daLoopLine(incomingQueue), 
    daRF(RFLayer), 
    expectedSN(exSN), 
    mutexListener(mutexListenr), 
    prints(false), 
    fugFacMutex(mutexDemibradFudgeFactorInput), 
    ostreamMutex(mutexListenerOstreamInput), 
    fudgeFactor(fudgeFactorDemibrad), 
    commands(cmds), 
    status(statusCode), 
    statusMutex(statusCodeMutex),
    streamy(streamypaul)
     {}
    
    /*
     * the heart of the listener watches activity on the RF layer and blocks until a packet is recived
     */
     int UltraListen();

private:

    ostream *streamy; //the given output stream for data to the layer above
    SeqNumManager seqNumMang;//a hashmap with added funtionality for dealing with sequence numbers
    queue<Packet> * daLoopLine;//a queue for the outgoing data
    RF* daRF;//the reference to the RF layer
    int bytesReceived;// bytes from the last packet
    int *statusCode;// the most recent error to happen in any of the threads
    short MACaddrList; //a pointer to our MAC address
    char buf[aMPDUMaximumLength];// buffer for the incoming packets
    bool prints;//bool for turning prints on or off
    static const int MAXPACKETSIZE = 2048; //size guarenteed to hold all properly formated packets
    static const short MAXSEQNUM = 4095;//the largest possible sequence number that can be used00
    volatile bool* ackReceivedL;// a pointer to a boolean that indicates whether or not a ACK has been recived
    volatile long long *fudgeFactor;// the fudge factor for the time stamp we get from the RF layer
    volatile int *commands;// an array of command code values where the index repersents the command to be altered and the returned value is the command level
    volatile int *status;//an int that hold the most recent error code returned
    volatile short* expectedSN;//the sequence number to check against incoming acks
    pthread_mutex_t *fugFacMutex;//a mutex for the fudge factor on our time stamp
    pthread_mutex_t *statusMutex;//mutex for changing the status code
    pthread_mutex_t *ostreamMutex;//a mutex for the output stream
    pthread_mutex_t* mutexListener;//a mutex for locking the queue


    /*
     * looks at a packet to check for three things from every packet that comes across the the RF layer
     * first it makes sure the packet conforms to the 802.11~ specs if not it disregards it
     * second it looks to see if the packet is addressed to our MAC address if not it disregards it
     * finally if the packet is for us it determines if it is an ACK or incoming data responding accordingly
     */
    int read_Packet();

    /*
     * if an incoming packet is for us and has data for us then this method packs the relevent information,
     * the sender's MAC, an array of characters of the data, and the size of said array into a tupal that
     * then is put into our incoming_Queue\
     */
    int queue_data();

    /*
     * a method for hiding away those nasty bitwise operaters for getting a squence number outta sight outta my way 
     */
    short extractSequenceNumber();

    /*
     * a method for hinging the bitwise terrible of getting a source address out of a packet
     */
    short extractSourceAddress();

    /*
     * a another method for hiding the terrible bitshifting madness of these lifes and times when extracting a beacon's time stamp
     */
    long long extractTimeStamp();

};
//#endif

