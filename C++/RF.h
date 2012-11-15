/*
 * The RF class models a broadcast-based wireless system via UDP services.
 * The layer simulates collisions properly (packets involved in a collision
 * are never delivered to the MAC layer), and provides the carrier-sense
 * facility required to implement a CSMA-like system.  The RF layer knows 
 * nothing about addresing, packet structures, etc.  It simply takes a 
 * collection of bytes from the caller and broadcasts them.  Similarly, 
 * incoming transmissions from any and all sources are collected and 
 * delivered, as long as they were not involved in a collision.
 * 
 * Note that most switches and routers are configured <i>not</i> to pass UDP
 * broadcast traffic.  This limits the use of the simulated RF layer to machines
 * within the same physical subnet.
 * 
 * By default, the IP broadcast address 255.255.255.255 is used.  This can 
 * be changed by editing the bcastAddr array, defined at the top of RF.cpp.
 * 
 * Brad Richards, 10/2008
 */

#ifndef __RF_H_INCLUDED__
#define __RF_H_INCLUDED__


// The SIFS inter-frame spacing time, in milliseconds 
#define aSIFSTime    100

// The slot time, in milliseconds 
#define aSlotTime    200

// The minimum size of the collision window 
#define aCWmin       3

// The maximum size of the collision window 
#define aCWmax       31

// The maximum nubmer of retransmission attempts 
#define dot11RetryLimit    5

// The maximum number of bytes allowed in an RF packet 
#define aMPDUMaximumLength    2048


#include <jni.h>


class RF
{
 public:
    
    /*
     * The constructor initializes the simulated RF transceiver and prepares it 
     * for use.  An RF instance initializes its internal clock when created, and
     * reports all times as milliseconds since creation, so times are not 
     * directly comparable across RF transceivers.  (It prints the exact time 
     * of creation though, and this information could be used to convert local 
     * times to absolute wall-clock time if one really wanted to.)  It is not
     * possible to create more than one RF layer on a single machine, as they 
     * must bind to a fixed port and will conflict.
     *     The C++ constructor creates a JVM instance, loads the Java RF
     * class, and links the C++ member functions to their Java counterparts
     * The RF.class file must be in the same directory as RF.cpp for this to 
     * work.
     */     
    RF();
    
    /*
     * Each thread must call this method before attempting to use any of the
     * other member functions.  No harm is done if an already-attached thread 
     * calls this method again.
     */
    void attachThread();
    
    /*
     * This non-blocking method returns <code>true</code> if the RF layer has 
     * one or more packets to deliver to the caller.  Since the RF layer is 
     * broadcast based, and has no notion of addresses, it will deliver any
     * intact packet that it observes on the channel.  In other words, this 
     * method might return true even if the packet wasn't intended for the 
     * MAC layer that called dataWaiting().
     */
    bool dataWaiting();
    
    /*
     * The RF layer's local clock.  Returns the number of milliseconds elapsed
     * since the RF layer was created.  (Uses System.nanoTime() internally in an 
     * effort to improve accuracy on Windows machines.)
     */
    long long clock();
    
    /*
     * Returns the number of milliseconds elapsed since the last transmission
     * on the shared channel ended.  If the channel is currently in use, this 
     * method returns zero.  Note that the elapsed time reported is the time
     * since the last transmission by any host, and is therefore not
     * necessarily the time since this host last transmitted.
     */
    long long getIdleTime();
    
    /*
     * Inspects the shared channel to see if it's currently being used.  This
     * method does not block, and returns true if at least one host is
     * currently broadcasting, false otherwise.
     */
    bool inUse();
    
    /*
     * The receive method blocks until an RF packet arrives, then copies
     * its contents into the buffer provided by the caller.  As with the
     * sockets routines, receive will respect the size of the buffer as
     * specified by the second argument.  The actual number of bytes 
     * received is returned.  If blocking is unacceptable, only call
     * receive() once dataWaiting() reveals that a packet has arrived.
     */
    int receive(char* buf, int bufSize);
    
    /*
     * This routine broadcasts the specified number of bytes from the
     * user-provided buffer.  It returns the number of bytes sent. 
     */
    int transmit(char* buf, int len);
    
    
  private:
  
    JavaVM *jvm;
    jclass rfClass;
    jobject theRF;
    jmethodID constructorID;
    jmethodID inUseID;
    jmethodID getIdleTimeID;
    jmethodID dataWaitingID;
    jmethodID receiveID;
    jmethodID transmitID;
    jmethodID clockID;
    
    JNIEnv *getEnvPtr();
};
#endif
