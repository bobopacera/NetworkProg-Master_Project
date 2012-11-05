#include <iostream>
#include <arpa/inet.h>
#include <pthread.h> 
#include <time.h>
#include "packet.h"
using namespace std;


// DO NOT CHANGE THESE TYPES. IT WILL MAKE EVERYTHING BREAK.
short frametype; // can either be 1-4 based on the type of frame.
bool resend; // if true, this packet is a resend packet.
unsigned short sequence_number; // the sequence number for the packet.
unsigned short destination; // the destination mac address for everything
unsigned short sender; // the sender mac address
unsigned char* data; // a pointer to an unsigned char array. It cointains the data that the user wants to send
unsigned int CRC; // currently, you can pass in the CRC, this will eventually be taken out but until CRC is implemented it will remain in. 
int bytes_to_send; // this is the size of the data char array.
int frame_size; // this is the size of the frame. It is always 10 more that bytes_to_send. 

// Basic constructor. CS is the value for CRC this will eventually be taken out
void Packet::initPacket(short frm, bool resen, unsigned short sn, unsigned short dest, unsigned short sendr, unsigned char* dta, int CS, int size){
	frametype = frm;
	resend = resen;
	sequence_number = sn;
	destination = dest;
	sender = sendr;
	data = dta;
	CRC = CS;
	bytes_to_send = size;
	frame_size = size + 10;

}

// This is really confusing, carefully read each line and each comment.
int Packet::buildByteArray(unsigned char* buffer){
	// we are passed a buffer that is equal to frame size.
	// this is because this unsigned char array will be the final
	// packet that will be sent across the network.

	//These next few lines of code breaks the CRC into
	//four bytes. and then puts those bytes in the
	// last four slots of the packet.
	int i = 0; // create a counter variable
	unsigned int temp_int = 0; // make a temporary integer.
	while(i < 4){
		temp_int = CRC << 24 - 8*i; // shift the ith byte of the integer all the way over to the left.
		temp_int = temp_int >> 24; // shift that byte back to its original position.
		buffer[frame_size-4+i] = temp_int; // add that byte to the appropriate section of the buffer.
		i++;
	}

	//Now we need to put the destination and sender addresses in their
	//proper positions
	unsigned short temp_short = 0; // create a temporary short variable.
	temp_short = sender << 8; // shift the first byte of sender the the leftmost position of the short
	temp_short = temp_short >> 8;// shift the previously mentioned byte back to its original position.
								 // this removes all the unnesessary zeros.
	buffer[4] = temp_short; // store this byte in the fifth position in the buffer
	temp_short = sender >> 8; // shift the second byte of the sender variable over to be the first byte.
	buffer[5] = temp_short; // store the second byte in the sixth position.
	// do the same process as above.
	temp_short = destination << 8;
	temp_short = temp_short >> 8;
	buffer[2] = temp_short;
	temp_short = destination >> 8;
	buffer[3] = temp_short;

	// this is kind of complicated.
	// the first two bytes of information need to store three variables.
	// they need to store the frame type, the retry bit and the sequence number.
	// The frame type takes up three bits, the retry takes up one, and the sequence number takes up 12.
	// EXAMPLE
	// 1   0   0   1   0   1   1   0      1   0   1   0   0   0   1   0
	// ---frame- retry -----------------sequence number----------------
	// when I stored destination address and sender address, I broke a short (2 bytes) up into two 1 byte
	// segments that could easily be put into the buffer. I take a similar approach here. I merge the three
	// variables into a short and then split it into two bytes to be put into the buffer.
	unsigned short temp_short_two = 0; // I needed a second temporary variable.
	temp_short = 0; // i zeroed out the first temporary variable, just to be safe.
	temp_short = frametype << 13; // the frame type is stored in a short, so I shift it over 13 bits so that only the
								  // last three bits will be in the leftmost position of the number
	if (resend == true) // if resend is true, then we need to flip the 13th bit. Otherwise we leave it at 0.
	{
		temp_short = temp_short + 4096; // 4096 is the number where only the 13th bit is flipped. 
	}
	//sequence number is stored in a short. We only want the first 12 bits. So we remove the last 4
	//with bitshift operations.
	temp_short_two = sequence_number << 4;
	temp_short_two = temp_short_two >> 4;
	temp_short = temp_short + temp_short_two; // temp_short now holds the short that we need to break up.
	// Now we break up the short into two packets. 
	temp_short_two = temp_short << 8;
	temp_short_two = temp_short_two >> 8;
	buffer[1] = temp_short_two;
	temp_short_two = temp_short >> 8;
	buffer[0] = temp_short_two;
	//at this point, our frame is completely build.
	// now we must fill our packet with data.
	i = 0; // reset our counter variable
	// the data we want is stored in the char array called 'data'
	// we want to put it in the buffer, but we want to put the data
	// after the header. we go through the data in 'data' and 
	// put it in the buffer.
	while(i < bytes_to_send){
		buffer[i+6] = data[i];
		i++;
	}
	//time for a nap...
}
//make the resend variable true.
int Packet::make_resend(){
	resend = true;
	return 1;
}
int Packet::get_crc(){
	return CRC;
}//TEMPORARY
