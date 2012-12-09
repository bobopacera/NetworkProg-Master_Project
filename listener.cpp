/**
 * Weston Thornburg
 * University of Puget Sound CS 325
 * October 2012
 */
#include "listener.h"

int
Listener::read_Packet ()
{
    int status;//will be returned with different status code to help ultra listen react
    short packetDest = buf[2];//bitwise terribleness
    unsigned short temp_dest = packetDest;
    temp_dest = temp_dest << 8;
    unsigned short temp = buf[3];
    temp = temp << 8;
    temp = temp >> 8;
    packetDest = temp_dest + temp;
    unsigned char frameType = buf[0];
    frameType = frameType >> 5;
    switch (frameType)//compare the frame type of the packet given to known types to figure out what kind of packet it is
    {
        case 0:
            if (prints) wcerr << "Data packet received." << endl;
            if ( commands[1] == 1 || commands[1] == 5 ) *streamy << "Data packet received." << endl;
            if (packetDest != MACaddrList && packetDest != -1)//compare the destination of this packet to our MAC address and to the broadcast address
            {
                status = 0;//this packet isn't for us
                if (prints) wcerr << "Packet not addressed to current MAC address. Our Mac address: " << MACaddrList << ".  Destination of incoming packet: " << packetDest << "MAC ADDRESS: " << endl;
                if (commands[1] == 1 || commands[1] == 5) *streamy << "Packet not addressed to current MAC address. Our Mac address: " << MACaddrList << ".  Destination of incoming packet: " << packetDest << endl;
                return status;
            }
            status = 1;
            break;

        case 1:
            if (prints) wcerr << "ACK Received." << endl;
            if (commands[1] == 1 || commands[1] == 5) *streamy << "ACK Received." << endl;
            if (packetDest != MACaddrList)//compare the destination of this packet to our MAC address and to the broadcast address
            {
                status = 0;//this packet isn't for us
                if (prints) wcerr << "Packet not addressed to current MAC address. Our Mac address: " << MACaddrList << ".  Destination of incoming packet: " << packetDest << endl;
                if (commands[1] == 1 || commands[1] == 5) *streamy << "Packet not addressed to current MAC address. Our Mac address: " << MACaddrList << ".  Destination of incoming packet: " << packetDest << endl;
                return status;
            }
            status = 2;
            break;

        case 2:
            if (prints) wcerr << "Beacon Received." << endl;
            if ( commands[1] == 1 || commands[1] == 5 ) *streamy << "Beacon Received." << endl;
            status = 3;
            break;

        default:
            if (prints) wcerr << "Unknown packet type received." << endl;
            if (commands[1] == 1 || commands[1] == 5) *streamy << "Unknown packet type received." << endl;
            status = 4;
            break;
    }
    return status;
}

int
Listener::UltraListen()
{
    while(true){
        int PRR;//Packet Read Result
        bytesReceived = daRF->receive(buf, MAXPACKETSIZE);//block until data comes our way 
        if (prints) wcerr << "Received a Packet with " << bytesReceived << " bytes of data!" << endl;
        if (commands[1] == 1 || commands[1] == 5) *streamy << "Received a Packet with " << bytesReceived << " bytes of data!" << endl;
        short dataSource;
        dataSource = extractSourceAddress();
        short seqNum;
        seqNum = extractSequenceNumber();
        PRR = read_Packet();
        switch (PRR)// based on the frame type do different things
        {
           case 1:
                {
                if ( seqNumMang.getSeqNum(dataSource) + 1 == seqNum )
                {
                    if ( daLoopLine->size() > 4)
                    {
                        pthread_mutex_lock(statusMutex);
                        *status = 10; // report that the queue for incoming data is full
                        pthread_mutex_unlock(statusMutex);
                        if (prints) wcerr << "Queue too full to recive incoming Packets" << endl;
                        if (commands[1] == 1 || commands[1] == 5) *streamy << "Queue too full to recive incoming Packets" << endl;
                    }
                    else
                    {
                        Packet paulLovesPBR( extractSourceAddress(), extractSequenceNumber(), MACaddrList);
                        char theFrame[paulLovesPBR.frame_size];
                        paulLovesPBR.buildByteArray(&theFrame[0]);
                        if (prints) wcerr << "Paul loaths PBR :: " << paulLovesPBR.frame_size << endl;
                        usleep(aSIFSTime * 1000);
                        daRF->transmit( &theFrame[0], paulLovesPBR.frame_size );
                        seqNumMang.increment(dataSource);
                        queue_data();//put data in the queue
                    }
                }
                else
                {
                    if (prints) wcerr << "Unexpected sequence number for DATA from " << extractSourceAddress() << endl;
                    if (commands[1] == 1 || commands[1] == 5) *streamy << "Unexpected sequence number" << endl;
                }
                }
                break;

            case 2:
                {
                if (seqNum == *expectedSN)//confirm that the ack matches the data packet last sent by sender
                {
                    *ackReceivedL = true;//let sender know that an ack has come in and to send the next packet
                    pthread_mutex_lock(statusMutex);
                    *status = 4;//sucsessful tx sent
                    pthread_mutex_unlock(statusMutex);
                }
                else
                {
                    if (prints) wcerr << "Unexpected sequence number for ACK" << endl;
                    if (commands[1] == 1 || commands[1] == 5) *streamy << "Unexpected sequence number" << endl;
                }
                }
                break;

            case 3:
                {
                long long newTimeStamp = extractTimeStamp();//get their timpe stamp from them
                volatile long long ourTmSmp = daRF->clock() + *fudgeFactor;//figure out what time we think it is
                long long diff = newTimeStamp - ourTmSmp;// compute the difference 
                if (diff > 0)//if their clock is running faster than ours go to their time
                {
                    if (commands[1] == 1 || commands[1] == 5) *streamy << "Adjusting our time stamp" <<endl;
                    pthread_mutex_lock(fugFacMutex);
                    fudgeFactor = &diff;// update the fudge factor 
                    pthread_mutex_unlock(fugFacMutex);
                }
                else
                {
                    if (commands[1] == 1 || commands[1] == 5) *streamy << "Didn't adjust timeStamp" << endl; //KILL ME
                }
                }
                break;

            default:
                {
                pthread_mutex_lock(statusMutex);
                *status = 2; // general error code
                pthread_mutex_unlock(statusMutex);
                }break;
        }
    }
}

int
Listener::queue_data()
{
    if (prints) wcerr << "putting data in queue" << endl;
    if (commands[1] == 1 || commands[1] == 5) *streamy << "putting data in queue" << endl;
    Packet toDemiBrad(&buf[0], bytesReceived);//create a packet
    pthread_mutex_lock(mutexListener);//lock the queue off
    daLoopLine->push(toDemiBrad);//send the pointer to the packet up to demibrad
    pthread_mutex_unlock(mutexListener); //unlock the queue so demibrad can have shot at it  
}

short
Listener::extractSequenceNumber()
{
    unsigned short SN = buf[0];//extract the sequence number 
    SN = SN << 12;//shift off the frame type and resend info
    SN = SN >> 4;//shift the byte back into position
    unsigned short temp = buf[1];
    temp = temp << 8;
    temp = temp >> 8;
    SN = SN + temp;
    return SN; 
}

short
Listener::extractSourceAddress()
{
    unsigned short DS = buf[4];//extract the source address
    DS = DS << 8;
    unsigned short temp = buf[5];
    temp = temp << 8;
    temp = temp >> 8;
    DS = DS + temp;
    return DS;
}

long long
Listener::extractTimeStamp()
{
    int size = bytesReceived-10;//total size of incoming data minus 10 bytes of header and CRC
    char timeStamp[size];//a new char array for just the incoming data
    for (int i = 6; i < bytesReceived-4; ++i)//strip the headers and put just the data in our tuple
    {
        timeStamp[i-6] = buf[i];//the offset of six is the front header being skipped in our buf and the four less is the CRC
    }
    long long tmStmp = 0;
    for (int i = 0; i < size-1; ++i)//build the long long that will hold the time stamp
    {
        tmStmp = tmStmp + timeStamp[i];
        tmStmp = tmStmp << 8;
    }
    tmStmp = tmStmp + timeStamp[size-1];//the last byte needs to happens outside the loop to prevent over shifting  the data
    return tmStmp;
}



