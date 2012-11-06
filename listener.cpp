#include "listener.h"
//#include "DemiBrad.h"

short MACACK_listener; //global varribles
bool ack_Received;
unsigned short MACaddr;
static const int MAXPACKETSIZE = 2048; //size guarenteed to hold all properly formated packets
char buf[MAXPACKETSIZE];// buffer for the incoming packets
CircularFifo<Packet* ,10> daLoopLine;
RF* daRF;

Listener::Listener(RF* RFLayer, CircularFifo<Packet* ,10>* incomingQueue, unsigned short* sendFlag, bool* receivedFlag, unsigned short myMAC)
{
    daRF = RFLayer;//our reference to the RF layer
    MACACK_listener = sendFlag;//where the address that requires an ACK goes
    MACACK_listener = 0;//special case no need to send an ACK
    ack_Received = receivedFlag;//flag for telling the sending an ACK has come in
    ack_Received = false;// indicates no ACKs recived
    unsigned short temp = myMAC;//our mac address for knowing if packets have come to the right place
    MACaddr = &temp;
    daLoopLine = incomingQueue;//where incoming data will be sent via pointers to tuples
}

int
Listener::read_Packet ()
{
    int status;//will be returned with different status code to help ultra listen react
    short packetDest = buf[2];//bitwise terribleness
    packetDest << 8;
    packetDest = packetDest + buf[3];
    if (packetDest != *MACaddr)//compare the destination of this packet to our MAC address
    {
        status = 0;//this packet isn't for us
        wcerr << "Packet not addressed to current MAC address." << endl;
        return status;
    }
    char frameType = buf[0];
    frameType >> 5;
    switch (frameType)//compare the frame type of the packet given to known types to figure out what kind of packet it is
    {
        case 0:
            wcerr << "Data packet received." << endl;
            status = 1;
            break;

        case 1:
            wcerr << "ACK Received." << endl;
            status = 2;
            break;

        case 2:
            wcerr << "Beacon Received." << endl;
            status = 3;
            break;

        default:
            wcerr << "Unknown packet type received." << endl;
            status = 3;
            break;
    }
    return status;
}

int
Listener::UltraListen()
{
    while(true){
        int PRR;//Packet Read Result
        // wait for data
        bytesReceived = daRF->receive(buf, MAXPACKETSIZE);//block until data comes our way
            //print the bytes received and checks for errors
        if (bytesReceived != MAXPACKETSIZE){
            wcerr << "Received  partial Packet with " << bytesReceived << " bytes of data!" << endl;
        }
        else{
            wcerr << bytesReceived << " !Full Packet Received! woo! ";
        }
        PRR = read_Packet();
        if (PRR == 1)//if the packet is relevent to us and is data queue it up
        {
            unsigned short dataSource = buf[4];//extract the source address
            dataSource << 8;
            dataSource = dataSource + buf[5] + 0;
            unsigned short temp = dataSource;//let the sender know to send an ACK for this data
            MACACK_listener = &temp;
            queue_data();//put data in the cirfifo
        }
        if (PRR == 2)//if the packet is relevent and is an ACK adjust ack recived flag
        {
            bool temp = true;
            ack_Received = &temp;
        }
    }
}

int
Listener::queue_data()
{
    Packet toDemiBrad;
    toDemiBrad.initPacket(&buf[0], bytesReceived);
    Packet * rawr = &toDemiBrad;
    daLoopLine->push(rawr);
    
    /*  
     * this is the code that has moved over to packet now 
    int size = bytesReceived-10;//total size of incoming data minus 10 bytes of header and CRC
    char dataIn[size];//a new char array for just the incoming data
    short dataSource = buf[4];//extract the source address
    dataSource << 8;
    dataSource = dataSource + buf[5];
    for (int i = 6; i < bytesReceived-4; ++i)//strip the headers and put just the data in our tuple
    {
        dataIn[i-6] = buf[i];//the offset of six is the front header being skipped in our buf and the four less is the CRC
    }
    */ 
    //need packet shit  daLoopLine->push(*packetInfo);
}




