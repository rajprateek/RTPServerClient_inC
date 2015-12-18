#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "rtp.h"

/* GIVEN Function:
 * Handles creating the client's socket and determining the correct
 * information to communicate to the remote server
 */
CONN_INFO* setup_socket(char* ip, char* port){
	struct addrinfo *connections, *conn = NULL;
	struct addrinfo info;
	memset(&info, 0, sizeof(struct addrinfo));
	int sock = 0;

	info.ai_family = AF_INET;
	info.ai_socktype = SOCK_DGRAM;
	info.ai_protocol = IPPROTO_UDP;
	getaddrinfo(ip, port, &info, &connections);

	/*for loop to determine corr addr info*/
	for(conn = connections; conn != NULL; conn = conn->ai_next){
		sock = socket(conn->ai_family, conn->ai_socktype, conn->ai_protocol);
		if(sock <0){
			if(DEBUG)
				perror("Failed to create socket\n");
			continue;
		}
		if(DEBUG)
			printf("Created a socket to use.\n");
		break;
	}
	if(conn == NULL){
		perror("Failed to find and bind a socket\n");
		return NULL;
	}
	CONN_INFO* conn_info = malloc(sizeof(CONN_INFO));
	conn_info->socket = sock;
	conn_info->remote_addr = conn->ai_addr;
	conn_info->addrlen = conn->ai_addrlen;
	return conn_info;
}

void shutdown_socket(CONN_INFO *connection){
	if(connection)
		close(connection->socket);
}

/* 
 * ===========================================================================
 *
 *			STUDENT CODE STARTS HERE. PLEASE COMPLETE ALL FIXMES
 *
 * ===========================================================================
 */


/*
 *  Returns a number computed based on the data in the buffer.
 */
static int checksum(char *buffer, int length){

	/*  ----  FIXME  ----
	 *
	 *  The goal is to return a number that is determined by the contents
	 *  of the buffer passed in as a parameter.  There a multitude of ways
	 *  to implement this function.  For simplicity, simply sum the ascii
	 *  values of all the characters in the buffer, and return the total.
	 */ 
	 int ckSum = 0;
	 for (int i = 0; i< length ; i++) {
	     ckSum += buffer[i];
	 }
	 
	 return ckSum;
}

/*
 *  Converts the given buffer into an array of PACKETs and returns
 *  the array.  The value of (*count) should be updated so that it 
 *  contains the length of the array created.
 */
static PACKET* packetize(char *buffer, int length, int *count){

	/*  ----  FIXME  ----
	 *  The goal is to turn the buffer into an array of packets.
	 *  You should allocate the space for an array of packets and
	 *  return a pointer to the first element in that array.  Each 
	 *  packet's type should be set to DATA except the last, as it 
	 *  should be LAST_DATA type. The integer pointed to by 'count' 
	 *  should be updated to indicate the number of packets in the 
	 *  array.
	 */
	 *count=0;
	 int flag = 0;
	 if (length% MAX_PAYLOAD_LENGTH !=0){
	    (*count) ++;
	    flag = 1;
	 }
	 (*count)+= (length/MAX_PAYLOAD_LENGTH);
	 
	 PACKET *array = malloc((*count) * sizeof(PACKET));
	 int i=0;
	 for(i=0; i< (*count)-1 ; i++ ) {
	    for (int j = 0 ; j< MAX_PAYLOAD_LENGTH ; j++) {
	      array[i].payload[j] = buffer[(i *MAX_PAYLOAD_LENGTH)+j];
	    }
	    array[i].type  = DATA;
	    array[i].payload_length = MAX_PAYLOAD_LENGTH;
	    array[i].checksum  = checksum(array[i].payload, MAX_PAYLOAD_LENGTH);
	 }
	 
	 if (flag ==1){
	     for (int j=0 ; j< length% MAX_PAYLOAD_LENGTH ; j++) {
	        array[i].payload[j] = buffer[(i *MAX_PAYLOAD_LENGTH)+j];	        
	     }
	     array[i].payload_length = (length% MAX_PAYLOAD_LENGTH);
	     array[i].checksum  = checksum(array[i].payload, length% MAX_PAYLOAD_LENGTH);
	 }
	 
	 else {
	    for (int j=0 ; j< MAX_PAYLOAD_LENGTH ; j++) {
	        array[i].payload[j] = buffer[(i *MAX_PAYLOAD_LENGTH)+j];
	    }
	    array[i].payload_length = MAX_PAYLOAD_LENGTH;
	 	  array[i].checksum  = checksum(array[i].payload, MAX_PAYLOAD_LENGTH);
	 }
	
   array[i].type  = LAST_DATA;  
    
	 return array;
}

/*
 * Send a message via RTP using the connection information
 * given on UDP socket functions sendto() and recvfrom()
 */
int rtp_send_message(CONN_INFO *connection, MESSAGE* msg){
	/* ---- FIXME ----
	 * The goal of this function is to turn the message buffer
	 * into packets and then, using stop-n-wait RTP protocol,
	 * send the packets and re-send if the response is a NACK.
	 * If the response is an ACK, then you may send the next one
	 */
	 int* packetCount = malloc(sizeof(int));
	 PACKET* packets = packetize(msg->buffer, msg->length, packetCount);
	 int i =0;
	 do{
	    sendto(connection->socket, (void*)(packets+i), sizeof(PACKET), 0 , connection->remote_addr, connection->addrlen);
	    
	    PACKET* response = malloc(sizeof(PACKET));
	    recvfrom(connection->socket, (void*)response, sizeof(PACKET), 0, connection->remote_addr , &(connection->addrlen));
	    
	    if(response->type==ACK){
	      if(packets[i].type==LAST_DATA){
	         break;
	      }
	      i++;
	    }
	    free(response);
	 }while(i< *packetCount);
    
   free(packets);
	 return 0;   
	 
}

/*
 * Receive a message via RTP using the connection information
 * given on UDP socket functions sendto() and recvfrom()
 */
MESSAGE* rtp_receive_message(CONN_INFO *connection){
	/* ---- FIXME ----
	 * The goal of this function is to handle 
	 * receiving a message from the remote server using
	 * recvfrom and the connection info given. You must 
	 * dynamically resize a buffer as you receive a packet
	 * and only add it to the message if the data is considered
	 * valid. The function should return the full message, so it
	 * must continue receiving packets and sending response 
	 * ACK/NACK packets until a LAST_DATA packet is successfully 
	 * received.
	 */

	 MESSAGE* message = malloc(sizeof(MESSAGE));
	 int bufferSize = 0;
	 do{
	    PACKET* currPacket = malloc(sizeof(PACKET));
	    recvfrom(connection->socket, (void*)currPacket, sizeof(PACKET), 0, connection->remote_addr , &(connection->addrlen));
	    PACKET* response = malloc(sizeof(PACKET));
	    
	    if(currPacket->checksum != checksum(currPacket->payload, currPacket->payload_length)) {
	       response->type = NACK;
	       sendto(connection->socket, (void*)response, sizeof(PACKET), 0 , connection->remote_addr, connection->addrlen);
	    }
	    else {
	       response->type = ACK;
	       int prevSize = bufferSize;
	       bufferSize += currPacket->payload_length;
	       message->buffer = (char*)realloc(message->buffer, bufferSize);
	       for (int i=0 ; i< currPacket->payload_length ; i++,prevSize++) {
	          message->buffer[prevSize] = currPacket->payload[i];
	       }
	       
	       sendto(connection->socket, (void*)response, sizeof(PACKET), 0 , connection->remote_addr, connection->addrlen);
	       
	       if(currPacket->type== LAST_DATA) {
	          free(currPacket);
	          free(response);
	          break;
	       }
	    }	
	    
	    free(currPacket);
	    free(response); 
	 }while(1==1);
	 
	 return message;
	 
	 
}
