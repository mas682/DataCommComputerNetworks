#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// Matt Stropkey
// Project 2 CS1652 Part 2
// June 27, 2019

/* ******************************************************************
 ALTERNATING BIT AND GO-BACK-N NETWORK EMULATOR: VERSION 1.1  J.F.Kurose

   This code should be used for Project 2, unidirectional and bidirectional
   data transfer protocols.  Network properties:
   - one way network delay averages five time units (longer if there
     are other messages in the channel for GBN), but can be larger
   - packets can be corrupted (either the header or the data portion)
     or lost, according to user-defined probabilities
   - packets will be delivered in the order in which they were sent
     (although some can be lost).
**********************************************************************/

#define BIDIRECTIONAL 1
#define MAX_MSG_QUEUE 50
#define MAX_SENT_QUEUE 17
int nsim = 0;

/* a "msg" is the data unit passed from layer 5 (teachers code) to layer  */
/* 4 (students' code).  It contains the data (characters) to be delivered */
/* to layer 5 via the students transport level protocol entities.         */
struct msg {
  char data[20];
};

/* a packet is the data unit passed from layer 4 (students code) to layer */
/* 3 (teachers code).  Note the pre-defined packet structure, which all   */
/* students must follow. */
struct pkt {
   int seqnum;
   int acknum;
   int checksum;
   char payload[20];
};

// struct to hold values of previously acknowledged packet for receiver
// needed so that receiver knows if it has recived a duplicate that was
// previously acknowledged
struct prev_pkt {
    int seqnum;
    int acknum;
    int checksum;
};

// holds packets that cannot be sent yet when at max window size
struct msg_queue {
    struct msg *messages[MAX_MSG_QUEUE];
    int size;
    int front;
    int rear;
};

struct sent_packet {
    int seqnum;
    int acknum;
    int checksum;
    int payload[20];
    float start_time;
    float resent_time;
    float packet_rtt;
    char resent;
};

// holds the packets in the window that are not aknowleded yet
struct sent_pkts {
    struct sent_packet *packets[MAX_SENT_QUEUE];
    int size;
    int front;
    int rear;
};


void starttimer(int AorB, float increment);
void tolayer3(int AorB, struct pkt packet);
void stoptimer(int AorB);
void tolayer5(int AorB, char datasent[20]);
/********* STUDENTS WRITE THE NEXT SEVEN ROUTINES *********/

// for testing..remove
int packets_rcvd_a = 0;
int packets_rcvd_b = 0;

int seqnum_a;          // used to hold the current sequence number for a
int seqnum_b;          // used to hold the current sequence number for b
int acknum_a;          // used to hold value of last sent ack by a
int acknum_b;          // used to hold the value of the last sent ack by b
int expected_seq_b;    // used to hold expected sequence value for a packet coming from a
int expected_seq_a;    // used to hold expected sequence value for a packet coming from b
int packet_valid_b;       // used by b as a boolean
struct prev_pkt prev_pkt;         // used to store values of previuos packet for b
float time = 0.000;     // holds time, moved from below
const float ALPHA = 0.125;    // used for RTT calculation
const float BETA = 0.25;     // used for RTT deviation calculation

struct msg_queue msg_queue_a;       // message queue for a
struct sent_pkts sent_pkts_a;       // sent packets queue for a
int ack_ready_a;                    // boolean to indicate if there is an acknoweledgement to be sent, so ack's can be piggybacked onto data
float sample_rtt_a = 0.000;         // holds current rtt for a packet in a
float rtt_a = 10.000;      // holds the average round trip time
float rtt_dev_a = 0.000;  // holds the average deviation in round trip time
float timer_value_a = 0.000;    // holds actual timer value passed to timer
struct pkt mypktptr_a;   // pointer to the current packet being sent by a
int skip_queue_a = 0;     // boolean to help deal with taking messages out of message queue
int send_nack_a = 0;          // boolean to indicate if sending negative acknowledgment
int nacknum_a;           // used to hold latest negative acknowledgement number for a

// new stuff for B
struct msg_queue msg_queue_b;       // message queue for B
struct sent_pkts sent_pkts_b;       // sent packets queue for b
int ack_ready_b;                    // boolean to indicate if there is an acknoweledgement to be sent, so ack's can be piggybacked onto data
float sample_rtt_b = 0.000;         // holds current rtt for a packet in b
float rtt_b = 10.000;      // holds the average round trip time
float rtt_dev_b = 0.000;  // holds the average deviation in round trip time
float timer_value_b = 0.000;    // holds actual timer value passed to timer
struct pkt mypktptr_b;   // pointer to the current packet being sent by b
int skip_queue_b = 0;       // boolean to help deal with taking messages out of message queue
int send_nack_b = 0;        // boolean to indicate if sending negative acknowledgement
int nacknum_b;              // used to hold latest negative acknowledgement number for b

// function sees if the message queue is full
int is_full_msg_queue(struct msg_queue msg_queue)
{
    return msg_queue.size == MAX_MSG_QUEUE;
}

void insert_msg_queue(struct msg message, struct msg_queue *msg_queue)
{
    int i;
    if(!is_full_msg_queue(*msg_queue))
    {
        struct msg *temp = malloc(sizeof(struct msg));
        // go back to beginning if at back of array
        if(msg_queue->rear == MAX_MSG_QUEUE -1)
        {
            msg_queue->rear = -1;
        }
        msg_queue->rear += 1;
        // copy the message into the queue
        for (i=0; i<20; i++)
            temp->data[i] = message.data[i];
        msg_queue->messages[msg_queue->rear] = temp;
        // increment number of items in the array
        msg_queue->size += 1;
    }
}

void remove_msg_queue(struct msg *temp, struct msg_queue *msg_queue)
{
    int i;
    for (i=0; i<20; i++)
        temp->data[i] = msg_queue->messages[msg_queue->front]->data[i];
    free(msg_queue->messages[msg_queue->front]);
    msg_queue->front += 1;
    if(msg_queue->front == MAX_MSG_QUEUE)
    {
        msg_queue->front = 0;
    }
    msg_queue->size -= 1;
}

int is_full_sent_pkts(struct sent_pkts sent_pkts)
{
    return sent_pkts.size == MAX_MSG_QUEUE;
}

void insert_sent_pkts(struct sent_packet packet, struct sent_pkts *sent_pkts)
{
    int i;
    if(!is_full_sent_pkts(*sent_pkts))
    {
        // go back to beginning if at back of array
        if(sent_pkts->rear == MAX_SENT_QUEUE - 1)
        {
            sent_pkts->rear = 0;
        }
        sent_pkts->rear += 1;
        // copy the packet into the queue
        struct sent_packet *temp = malloc(sizeof(struct sent_packet));
        temp->seqnum = packet.seqnum;
        temp->acknum = packet.acknum;
        temp->checksum = packet.checksum;
        temp->start_time = packet.start_time;
        temp->resent_time = packet.start_time;
        temp->resent = packet.resent;
        temp->packet_rtt = packet.packet_rtt;
        for (i=0; i<20; i++)
            temp->payload[i] = packet.payload[i];
        sent_pkts->packets[sent_pkts->rear] = temp;
        // increment number of items in the array
        //printf("Old sent packets queue size %d\n", sent_pkts->size);
        sent_pkts->size += 1;
        //printf("New sent packets queue size is %d\n\n", sent_pkts->size);
    }
}

// removes the first packet from the queue
// may need to use malloc here? just simply free in method that calls it
void remove_sent_pkts(struct sent_packet *temp, struct sent_pkts *sent_pkts)
{
    int i;
    temp->seqnum = sent_pkts->packets[sent_pkts->front]->seqnum;
    temp->acknum = sent_pkts->packets[sent_pkts->front]->acknum;
    temp->checksum = sent_pkts->packets[sent_pkts->front]->checksum;
    //printf("removed packet with sequence number %d\n", temp->seqnum);
    for (i=0; i<20; i++)
        temp->payload[i] = sent_pkts->packets[sent_pkts->front]->payload[i];
    free(sent_pkts->packets[sent_pkts->front]);
    sent_pkts->front += 1;
    if(sent_pkts->front == MAX_SENT_QUEUE)
    {
        sent_pkts->front = 1;
    }
    sent_pkts->size -= 1;
}

// sends a copy of the first packet within the queue
// may need to use malloc here?
struct sent_packet peek_sent_pkts(struct sent_pkts sent_pkts)
{
    int i;
    struct sent_packet temp;
    temp.seqnum = sent_pkts.packets[sent_pkts.front]->seqnum;
    temp.acknum = sent_pkts.packets[sent_pkts.front]->acknum;
    temp.checksum = sent_pkts.packets[sent_pkts.front]->checksum;
    for (i=0; i<20; i++)
        temp.payload[i] = sent_pkts.packets[sent_pkts.front]->payload[i];
    return temp;
}

// note, may want to return unsigned integer?
int generate_checksum(struct pkt* packet)
{
    int i;   // for the for loop
    int checksum = 0;
    // add the sequence number to the checksum
    checksum += packet->seqnum;
    // add the ack number to the checksum
    checksum += packet->acknum;
    // add the message to the checksum value
    for (i=0; i<20; i++)
    {
        checksum += packet->payload[i];
    }
    // generate the ones complement of the checksum
    checksum = ~checksum;
    // return the generate checksum
    return checksum;
}

// function to calculate the rtt based off of the most recent rtt
// does not get called if a packet is retransmitted or lost
void calculate_rtt(float sample_rtt, float *rtt, float *rtt_dev, float *timer_value)
{
    // get the absolute value of the difference between the current rtt and the previous calculated average
    float absolute_dif = fabsf(sample_rtt - *rtt);
    // get the updated average rtt
    *rtt = ((1-ALPHA) * *rtt) + (ALPHA * sample_rtt);                                       // for testing
    // get the updated deviation in the average rtt
    *rtt_dev = ((1-BETA) * *rtt_dev) + (BETA * absolute_dif);
    // get the updated value to set the timer to
    *timer_value = *rtt + (4 * *rtt_dev);
}

/* called from layer 5, passed the data to be sent to other side */
void A_output(struct msg message)
{
    int i;          // for for loop
    // if the message is simply an acknowedgement, do not need to queue it
    int sending_ack = 1;
    int sending_data = 0;
    int sending_nack = 1;
    // if there is an ack ready, message may be for ACK
    if(ack_ready_a)
    {
        for(i = 0; i < 20; i++)
        {
            // if first character, check for A
            if(i == 0 && !message.data[i] == 'A')
            {
                sending_ack = 0;
                break;
            }
            // if second character, check for c
            if(i == 1 && !message.data[i] == 'C')
            {
                sending_ack = 0;
                break;
            }
            // if third characater, check for K
            if(i == 2 && !message.data[i] == 'K')
            {
                sending_ack = 0;
                break;
            }
            // if any other character, set to null terminator
            if(i != 0 && i != 1 && i != 2 && !message.data[i] == '\0')
            {
                sending_ack = 0;
                break;
            }
        }
    }
    else
    {
        sending_ack = 0;
    }
    if(send_nack_a)
    {
        for(i = 0; i < 20; i++)
        {
            // if first character, check for A
            if(i == 0 && !message.data[i] == 'N')
            {
                sending_nack = 0;
                break;
            }
            // if second character, check for c
            if(i == 1 && !message.data[i] == 'A')
            {
                sending_nack = 0;
                break;
            }
            // if third characater, check for K
            if(i == 2 && !message.data[i] == 'C')
            {
                sending_nack = 0;
                break;
            }
            // if third characater, check for K
            if(i == 3 && !message.data[i] == 'K')
            {
                sending_nack = 0;
                break;
            }
            // if any other character, set to null terminator
            if(i != 0 && i != 1 && i != 2 && i != 3 && !message.data[i] == '\0')
            {
                sending_nack = 0;
                break;
            }
        }
    }
    else
    {
        sending_nack = 0;
    }
    // mainly need this so if the message is only a acknowledgement, it does not get queued in sent messages
    // as we are not waiting for a response for the ack
    if(sending_ack)
    {
        message.data[0] = '\0';
        message.data[1] = '\0';
        message.data[2] = '\0';
    }
    else if(sending_nack)
    {
        message.data[0] = '\0';
        message.data[1] = '\0';
        message.data[2] = '\0';
        message.data[3] = '\0';
    }
    // this executes when a acknowledgement was just received and the message queue is not empty
    // will remove the message that was queued the longest ago to be sent
    else if(skip_queue_a && sent_pkts_a.size < 8 && msg_queue_a.size != 0)
    {
        skip_queue_a = 0;
        remove_msg_queue(&message, &msg_queue_a);
    }
    // for testing
    // should not ever happen
    else if(skip_queue_a)
    {
        printf("Warning: trying to skip the queue\n");
        exit(1);
    }
    // if this is skipped, then not at max messages in window and the message queue is empty
    else if(sent_pkts_a.size == 8 || msg_queue_a.size != 0)
    {
        // if window is full, quque the message
        // also queue if some other message is already waiting..
        printf("Queueing a message for A since %d packets have been sent and unaked\n", sent_pkts_a.size);
        if(!is_full_msg_queue(msg_queue_a))
        {
            //printf("Message queue not full for A. Size: %d so packet has been queued\n", msg_queue_a.size);
            insert_msg_queue(message, &msg_queue_a);
            // may want to malloc this...
            // get a message to send out of the message queue
            if(sent_pkts_a.size < 8)
            {
                remove_msg_queue(&message, &msg_queue_a);
            }
            else
            {
                return;
            }
        }
        else
        {
            printf("Warning: Message queue is full for A. Size: %d\n", msg_queue_a.size);
            printf("%d %d %d\n", packets_rcvd_a, packets_rcvd_b, nsim);
            exit(1);
        }
    }
    // handles only sending an acknowledgement
    if(sending_ack && !sending_data)
    {
        // 1. get the current sequence number for a
        mypktptr_a.seqnum = -1;                     // set to -1 when only sending an acknowledgement
        // set the acknowledgement number to 0, will always be 0 for a as a is not acknowledging anything
        mypktptr_a.acknum = acknum_a;
        // copy the message into the packet
        for (i=0; i<20; i++)
           mypktptr_a.payload[i] = message.data[i];
        // set the checksum for the packet
        mypktptr_a.checksum = generate_checksum(&mypktptr_a);
        // unset boolean so it is known that packet was sent
        ack_ready_a = 0;
        //printf("Sending acknowledgement in A for packet %d\n", acknum_a);           // for testing
        tolayer3(0, mypktptr_a);
    }
    // need to implement sending a negative acknowledgement
    else if(sending_nack && !sending_data)
    {
        // 1. get the current sequence number for a
        mypktptr_a.seqnum = 0;                     // set to -1 when only sending an acknowledgement
        // set the acknowledgement number to 0, will always be 0 for a as a is not acknowledging anything
        mypktptr_a.acknum = nacknum_a;
        // copy the message into the packet
        for (i=0; i<20; i++)
           mypktptr_a.payload[i] = message.data[i];
        // set the checksum for the packet
        mypktptr_a.checksum = generate_checksum(&mypktptr_a);
        // unset boolean so it is known that packet was sent
        send_nack_a = 0;
        tolayer3(0, mypktptr_a);
    }
    else          // sending some data and maybe a acknowledgement
    {
        float start_time = 0.000;
        // used to keep track of the sent packet if resend needed
        struct sent_packet temp;
        // 1. get the current sequence number for a
        seqnum_a += 1;
        if(seqnum_a == 17)
            seqnum_a = 1;
        // allows piggybacking to occur
        mypktptr_a.seqnum = seqnum_a;                     // set the packets sequence number
        // set the sequence number for the saved packet
        temp.seqnum = seqnum_a;
        // if sending acknowelegement with data or not
        printf("Sending packet %d with acknowedgment for packet %d in A at %f\n", seqnum_a, acknum_a, time);        // for testing
        mypktptr_a.acknum = acknum_a;
        // set the acknum for the saved packet
        temp.acknum = acknum_a;
        ack_ready_a = 0;
        // copy the message into the packet and the saved packet
        for (i=0; i<20; i++)
        {
           mypktptr_a.payload[i] = message.data[i];
           temp.payload[i] = mypktptr_a.payload[i];
        }
        // set the checksum for the packet
        mypktptr_a.checksum = generate_checksum(&mypktptr_a);
        // set the checksum for the saved packet
        temp.checksum = mypktptr_a.checksum;
        // set the start time for the packet to be saved
        start_time = time;
        temp.start_time = start_time;
        // start the timer if the first packet in the window
        temp.packet_rtt = timer_value_a;
        if(sent_pkts_a.size == 0)
            starttimer(0, timer_value_a);
        // set that the packet was not resent yet
        temp.resent = 0;
        // insert the sent packet into the sent packets array
        insert_sent_pkts(temp, &sent_pkts_a);
        // send the packet to layer 3
        tolayer3(0, mypktptr_a);
        if(sent_pkts_a.size < 8 && msg_queue_a.size > 0)
        {
            skip_queue_a = 1;
            for(i = 0; i < 20; i++)
                message.data[i] = '\0';
            // see if this ever actually executes
            A_output(message);
        }
    }
}


void B_output(struct msg message)
{
    int i;          // for for loop
    // if the message is simply an acknowedgement, do not need to queue it
    int sending_ack = 1;
    int sending_data = 0;
    int sending_nack = 1;
    // if there is an ack ready, message may be for ACK
    if(ack_ready_b)
    {
        for(i = 0; i < 20; i++)
        {
            // if first character, check for A
            if(i == 0 && !message.data[i] == 'A')
            {
                sending_ack = 0;
                break;
            }
            // if second character, check for c
            if(i == 1 && !message.data[i] == 'C')
            {
                sending_ack = 0;
                break;
            }
            // if third characater, check for K
            if(i == 2 && !message.data[i] == 'K')
            {
                sending_ack = 0;
                break;
            }
            // if any other character, set to null terminator
            if(i != 0 && i != 1 && i != 2 && !message.data[i] == '\0')
            {
                sending_ack = 0;
                break;
            }
        }
    }
    else
    {
        sending_ack = 0;
    }

    if(send_nack_b)
    {
        for(i = 0; i < 20; i++)
        {
            // if first character, check for A
            if(i == 0 && !message.data[i] == 'N')
            {
                sending_nack = 0;
                break;
            }
            // if second character, check for c
            if(i == 1 && !message.data[i] == 'A')
            {
                sending_nack = 0;
                break;
            }
            // if third characater, check for K
            if(i == 2 && !message.data[i] == 'C')
            {
                sending_nack = 0;
                break;
            }
            // if third characater, check for K
            if(i == 3 && !message.data[i] == 'K')
            {
                sending_nack = 0;
                break;
            }
            // if any other character, set to null terminator
            if(i != 0 && i != 1 && i != 2 && i != 3 && !message.data[i] == '\0')
            {
                sending_nack = 0;
                break;
            }
        }
    }
    else
    {
        sending_nack = 0;
    }
    // mainly need this so if the message is only a acknowledgement, it does not get queued in sent messages
    // as we are not waiting for a response for the ack
    if(sending_ack)
    {
        message.data[0] = '\0';
        message.data[1] = '\0';
        message.data[2] = '\0';
    }
    else if(sending_nack)
    {
        message.data[0] = '\0';
        message.data[1] = '\0';
        message.data[2] = '\0';
        message.data[3] = '\0';
    }
    else if(skip_queue_b && sent_pkts_b.size < 8 && msg_queue_b.size != 0)
    {
        skip_queue_b = 0;
        remove_msg_queue(&message, &msg_queue_b);
    }
    else if(skip_queue_b)
    {
        printf("Warning: trying to skip the queue in B\n");
        exit(1);
    }
    // if this is skipped, then not at max messages in window and the message queue is empty
    else if(sent_pkts_b.size == 8 || msg_queue_b.size != 0)
    {
        // if window is full, quque the message
        // also queue if some other message is already waiting..
//        printf("Queueing a message for B since %d packets have been sent and unaked\n", sent_pkts_b.size);
        if(!is_full_msg_queue(msg_queue_b))
        {
    //        printf("Message queue not full for B. Size: %d so packet has been queued\n", msg_queue_b.size);
            insert_msg_queue(message, &msg_queue_b);
            // may want to malloc this...
            // get a message to send out of the message queue
            if(sent_pkts_b.size < 8)
            {
                remove_msg_queue(&message, &msg_queue_b);
            }
            else
            {
                return;
            }
        }
        else
        {
            printf("Warning: Message queue is full for B. Size: %d\n", msg_queue_b.size);
            exit(1);
        }
    }
    // handles only sending an acknowledgement
    if(sending_ack && !sending_data)
    {
        // 1. get the current sequence number for a
        mypktptr_b.seqnum = -1;                     // set to -1 when only sending an acknowledgement
        // set the acknowledgement number to 0, will always be 0 for a as a is not acknowledging anything
        mypktptr_b.acknum = acknum_b;
        // copy the message into the packet
        for (i=0; i<20; i++)
           mypktptr_b.payload[i] = message.data[i];
        // set the checksum for the packet
        mypktptr_b.checksum = generate_checksum(&mypktptr_b);
        // unset boolean so it is known that packet was sent
        ack_ready_b = 0;
        tolayer3(1, mypktptr_b);
    }
    else if(sending_nack && !sending_data)
    {
        // 1. get the current sequence number for a
        mypktptr_b.seqnum = 0;                     // set to -1 when only sending an acknowledgement
        // set the acknowledgement number to 0, will always be 0 for a as a is not acknowledging anything
        mypktptr_b.acknum = nacknum_b;
//        printf("NACKNUM_B: %d\n", nacknum_b);
        // copy the message into the packet
        for (i=0; i<20; i++)
           mypktptr_b.payload[i] = message.data[i];
        // set the checksum for the packet
        mypktptr_b.checksum = generate_checksum(&mypktptr_b);
        // unset boolean so it is known that packet was sent
        send_nack_b = 0;
        tolayer3(1, mypktptr_b);
    }
    else          // sending some data and maybe a acknowledgement
    {
        float start_time = 0.000;
        // used to keep track of the sent packet if resend needed
        struct sent_packet temp;
        // 1. get the current sequence number for a
        seqnum_b += 1;
        if(seqnum_b == 17)
            seqnum_b = 1;
        mypktptr_b.seqnum = seqnum_b;                     // set the packets sequence number
        // set the sequence number for the saved packet
        temp.seqnum = seqnum_b;
        // if sending acknowelegement with data or not
        printf("Sending packet %d with acknowedgment %d in B at time %f\n", seqnum_b, acknum_b, time);        // for testing
        // set acknowledge number to 0 when no acknowledgement being sent
        // allows piggybacking..
        mypktptr_b.acknum = acknum_b;
        // set the acknum for the saved packet
        temp.acknum = acknum_b;
        ack_ready_b = 0;
        // copy the message into the packet and the saved packet
        for (i=0; i<20; i++)
        {
           mypktptr_b.payload[i] = message.data[i];
           temp.payload[i] = mypktptr_b.payload[i];
        }
        // set the checksum for the packet
        mypktptr_b.checksum = generate_checksum(&mypktptr_b);
        // set the checksum for the saved packet
        temp.checksum = mypktptr_b.checksum;
        // set the start time for the packet to be saved
        start_time = time;
        temp.start_time = start_time;
        // start the timer if the first packet in the window
        temp.packet_rtt = timer_value_b;
        if(sent_pkts_b.size == 0)
            starttimer(1, timer_value_b);
        // set that the packet was not resent yet
        temp.resent = 0;
        // insert the sent packet into the sent packets array
        insert_sent_pkts(temp, &sent_pkts_b);
        // send the packet to layer 3
        tolayer3(1, mypktptr_b);
        // if after sending a message and there is a message waiting to be sent
        // and the window is not full, send the next message
        if(sent_pkts_b.size < 8 && msg_queue_b.size > 0)
        {
            skip_queue_b = 1;
            for(i = 0; i < 20; i++)
                message.data[i] = '\0';
            B_output(message);
        }
    }
}

// returns the number of packets to remove from queue
int check_ack(int smallest_pkt, int largest_pkt, int acknum, int size)
{
    //printf("In check_ack: Smallest %d Largest %d ACKnum %d\n", smallest_pkt, largest_pkt, acknum);
    if(size == 0)
    {
        return 0;
    }
    if(smallest_pkt == 0)
    {
        return 0;
    }
    if(acknum == smallest_pkt)
    {
        return 1;
    }
    if((largest_pkt <= 16) && (largest_pkt >= 8) && (acknum <= largest_pkt) && (acknum >= smallest_pkt))
    {
        return (acknum-smallest_pkt) + 1;
    }
    if((largest_pkt < 8) && (smallest_pkt <= 16) && (smallest_pkt >= 10))
    {
        if(acknum > smallest_pkt && acknum <= 16)
        {
            return (acknum - smallest_pkt) + 1;
        }
        if(acknum > 0 && acknum <= largest_pkt)
        {
            return (16 - smallest_pkt) + acknum + 1;
        }
    }
    if((largest_pkt <= 16) && (smallest_pkt <= 16) && (acknum >= smallest_pkt) && (acknum <= largest_pkt))
    {
        return (acknum - smallest_pkt) + 1;
    }
    return 0;
}

// used to resend a packet that a negative acknowledgement was received for
void resend_packet(int AorB, int unacked_pkt, struct sent_pkts *sent_pkts, float timer_value)
{
    int x = 0;
    struct sent_packet *temp;
    struct pkt packet;
    temp = sent_pkts->packets[unacked_pkt];
    if(temp->seqnum == unacked_pkt)
    {
        if(AorB)
        {
            if((temp->resent_time + (0.7 * timer_value_b * temp->resent)) < time)
            {
                printf("Received NAK in B but not resending as packet %d resent recently\n", unacked_pkt);
                return;
            }
        }
        else
        {
            if((temp->resent_time + (0.7 * timer_value_a * temp->resent)) < time)
            {
                printf("Received NAK in A but not resending as packet %d resent recently\n", unacked_pkt);
                return;
            }
        }
        temp->resent += 1;
        packet.seqnum = temp->seqnum;
        // may want to make this also send ack's if applicable
        if(AorB)
        {
            packet.acknum = acknum_b;
        }
        else
        {
            packet.acknum = acknum_a;
        }
        for(x = 0; x < 20; x++)
        {
            packet.payload[x] = temp->payload[x];
        }
        packet.checksum = generate_checksum(&packet);
        printf("Resending packet %d with acknum %d due to a negative acknowledgement\n", unacked_pkt, packet.acknum);
        tolayer3(AorB, packet);
    }
    else         // for testing
    {
        printf("Called resend packet when it should not have been called %d %d %d\n", temp->seqnum, unacked_pkt, sent_pkts->size);
        exit(0);
    }
}

// funcion handles removing packets when
void valid_ack(int AorB, int removal_number, struct sent_pkts *sent_pkts, float timer_value)
{
    int x = 0;
    while(x < removal_number)
    {
        // stop the timer for the smallest unacknowledged packet
        if(x == 0)
        {
            stoptimer(AorB);
        }
        // holds a newly acknowledged packet
        struct sent_packet *front = sent_pkts->packets[sent_pkts->front];
        // if the packet has not been resent, calculate the rtt
        if(!front->resent)
        {
            if(!AorB)
            {
                // calculate the new sample rtt for this packet
                sample_rtt_a = time - front->start_time;
                // calculate the new rtt
                calculate_rtt(sample_rtt_a, &rtt_a, &rtt_dev_a, &timer_value_a);
                timer_value = timer_value_a;
            }
            else
            {
                // calculate the new sample rtt for this packet
                sample_rtt_b = time - front->start_time;
                // calculate the new rtt
                calculate_rtt(sample_rtt_b, &rtt_b, &rtt_dev_b, &timer_value_b);
                timer_value = timer_value_b;
            }
        }
// for testing
        if(AorB)
        {
            packets_rcvd_b += 1;
        }
        else
        {
            packets_rcvd_a += 1;
        }
        // setting to a variable just for testing
        struct sent_packet removed;
        remove_sent_pkts(&removed, sent_pkts);
        x += 1;
    }
    // get the first unacknowledged packet if there is one
    // could have with resending if packet comes in while trying to resend?
    if(sent_pkts->size != 0)
    {
        struct sent_packet *temp = sent_pkts->packets[sent_pkts->front];
        // see if new smalled unacknowledged packet is still within resend time
        // may want to calculate this as 2
        float temp_time = temp->resent_time + (timer_value * (temp->resent + 1));
        if(time >= temp_time)
        {
            // resend first packet if outside of time period
            struct pkt packet;
            int index;
            int i = 0;
            while(i < sent_pkts->size)
            {
                index = sent_pkts->front + i;
                if(index > 16)
                {
                    index = index % 16;
                }
                temp = sent_pkts->packets[index];
                packet.seqnum = temp->seqnum;
                // may want to make this also send ack's if applicable
                if(AorB)
                {
                    packet.acknum = acknum_b;
                }
                else
                {
                    packet.acknum = acknum_a;
                }
                for(x = 0; x < 20; x++)
                {
                    packet.payload[x] = temp->payload[x];
                }
                packet.checksum = generate_checksum(&packet);
                if(i == 0)
                {
                    starttimer(AorB, timer_value * (temp->resent + 1));
                }
                i += 1;
                // need to be careful as acknowledgements may come in as you do this..
                //printf("Resending a packet for A or B with sequence number %d and acknum %d due to a ACK coming in\n\n", packet.seqnum, packet.acknum);
                temp->resent_time = time;
                temp->resent += 1;
                tolayer3(AorB, packet);
            }
        }
        else
        {
            //printf("Time still left for the first packet %f after ACK received\n", temp_time);
            temp_time = temp_time - time;
            starttimer(AorB, temp_time);
        }
    }
}


/* called from layer 3, when a packet arrives for layer 4 */
void A_input(struct pkt packet)
{
    // for for loop
    int i;
    struct msg reply_msg_a;
    // first see if the checksum is correct as this indicates the packet
    // was probably not altered
    if(generate_checksum(&packet) == packet.checksum)
    {
        // in this case, the packet should probably be a acknowlegement by itself
        if(packet.seqnum == -1)
        {
            int payload_valid = 1;
            // verify that the payload contains nothing but null-terminators
            for(i = 0; i < 20; i++)
            {
                if(packet.payload[i] != '\0')
                {
                    payload_valid = 0;
                    break;
                }
            }
            // if the payload is what is expected
            if(payload_valid)
            {
                // if not expecting any acknowledgements, simply return
                if(sent_pkts_a.size == 0)
                {
                    printf("Received acknowledgement in A but not expecting any\n\n");
                    return;
                }
                int removal_number = check_ack(sent_pkts_a.front, sent_pkts_a.rear, packet.acknum, sent_pkts_a.size);
                // will be 0 if the ack number is not within a appropriate range
                if(removal_number)
                {
                    printf("Recieved ACK in A for %d packets at %f\n", removal_number, time);
                    valid_ack(0, removal_number, &sent_pkts_a, timer_value_a);
                    // if messages waiting to be sent, send them after ack comes in
                    if(sent_pkts_a.size < 8 && msg_queue_a.size != 0)
                    {
                        struct msg message;
                        skip_queue_a = 1;
                        for(i = 0; i < 20; i++)
                            message.data[i] = '\0';
                        A_output(message);
                    }
                    return;
                }
                else
                {
                    // in GBN, if duplicate ack received, do nothing...only resend on timeout
                    printf("Incorrect acknowedgment number received by A\n");
                    return;
                }
            }
            else
            {
                printf("Payload for acknowledgement invalid in A\n");
            }
            return;
        } // end of just a acknowledgement being received
        //if correct sequence number and ack valid...
        int ack_valid = check_ack(sent_pkts_a.front, sent_pkts_a.rear, packet.acknum, sent_pkts_a.size);
        if((packet.seqnum == expected_seq_a) && ack_valid)
        {
            tolayer5(0, packet.payload);
            for(i = 0; i < 20; i++)
            {
                // if first character, set to A
                if(i == 0)
                    reply_msg_a.data[i] = 'A';
                // if second character, set to C
                if(i == 1)
                    reply_msg_a.data[i] = 'C';
                // if third characater, set to K
                if(i == 2)
                    reply_msg_a.data[i] = 'K';
                // if any other character, set to null terminator
                if(i != 0 && i != 1 && i != 2)
                    reply_msg_a.data[i] = '\0';
            }
            acknum_a = packet.seqnum;
            expected_seq_a += 1;
            if(expected_seq_a == 16)
                expected_seq_a = 1;
            ack_ready_a = 1;
            // update sent packets array since acknowledgement was valid
            printf("Received ACK in A for packet %d\n", packet.acknum);
            valid_ack(0, ack_valid, &sent_pkts_a, timer_value_a);
            // if messages waiting to be sent, send them after ack comes in

            printf("Sending acknowledgement in A for packet %d\n", acknum_a);
            A_output(reply_msg_a);
            // upon returning from sending ACK, want to see if a message can be sent that
            // is queued as room was freed in the window
            if(sent_pkts_a.size < 8 && msg_queue_a.size != 0)
            {
                struct msg message;
                skip_queue_a = 1;
                for(i = 0; i < 20; i++)
                    message.data[i] = '\0';
                A_output(message);
            }
            return;
        }
        // if the sequence is 0, may be a negative acknowledgemnt if the acknum is also invalid
        // because the acknum is the negative of the next expected acknumber
        else if((packet.seqnum == 0) && !ack_valid)
        {
            int acknum_rcvd = 0;
            int unacked_pkt = 0;
            unacked_pkt = (~packet.acknum) + 1;
            if(sent_pkts_a.size == 0)
            {
                printf("May have received a negative acknowledgement in A but did not have any packets sent out that need acknowledged\n");
                // may want to send nak if other side trying to send something?
                return;
            }
            int nack_rcvd = 1;
            for(i = 0; i < 20; i++)
            {
                if(packet.payload[i] != '\0')
                {
                    nack_rcvd = 0;
                    break;
                }
            }
            // if the payload is correct, see if the reverse ack number is valid
            // it may not be valid if we already received the previous ack for this
            // number

            if(nack_rcvd)
            {
                int acknum_rcvd = (~packet.acknum);
                // corner case if the next expected is 1, then the -1 should go to 16
                if(acknum_rcvd == 0)
                {
                    acknum_rcvd = 16;
                }
                if(acknum_rcvd <= 0 || acknum_rcvd > 16)
                {
                    nack_rcvd = 0;
                }
                else
                {
                    ack_valid = check_ack(sent_pkts_a.front, sent_pkts_a.rear, acknum_rcvd, sent_pkts_a.size);
                }
            }
            // if a negative acknowledgement was received
            if(nack_rcvd && ack_valid)
            {
                // update the sent packets array to remove all that were acknowledged
                printf("Processing a negative acknowledgement in A that contains a valid ACK at time %f\n");             // for testing
                valid_ack(0, ack_valid, &sent_pkts_a, timer_value_a);
                // upon returning from sending ACK, want to see if a message can be sent that
                // is queued as room was freed in the window
                // check to make sure packet not acknowledged yet
                if(check_ack(sent_pkts_a.front, sent_pkts_a.rear, unacked_pkt, sent_pkts_a.size))
                {
                    resend_packet(0, unacked_pkt, &sent_pkts_a, timer_value_a);
                }
                if(sent_pkts_a.size < 8 && msg_queue_a.size != 0)
                {
                    struct msg message;
                    skip_queue_a = 1;
                    for(i = 0; i < 20; i++)
                        message.data[i] = '\0';
                    A_output(message);
                }
            }
            else if(unacked_pkt > 0 && unacked_pkt < 17)      // message was not a negative acknowledgement, or did not acknowledge any messages we already knew about
            {
                if(check_ack(sent_pkts_a.front, sent_pkts_a.rear, unacked_pkt, sent_pkts_a.size))
                {
                    resend_packet(0, unacked_pkt, &sent_pkts_a, timer_value_a);
                }
                else
                {
                    printf("Received a negative acknowledgement in A but nothing to do with it\n");
                }
            }
            else
            {
               printf("Negative acknowledgement received is invalid in A\n");
            }
        }
        // if sequence incorrect but ack valid
        // if sequence incorrect, may be duplicate packet..
        else if((packet.seqnum != expected_seq_a) && ack_valid)
        {
            // update the sent packets array to remove all that were acknowledged
            valid_ack(0, ack_valid, &sent_pkts_a, timer_value_a);
            // need to resend the last acknowledgement here
            send_nack_a = 1;
            nacknum_a = ~expected_seq_a + 1;
            printf("Received a packet with a incorrect sequence number but valid acknowledgement in A, sending NAK %d\n", time, nacknum_a);

            for(i = 0; i < 20; i++)
            {
                // if first character, set to A
                if(i == 0)
                    reply_msg_a.data[i] = 'N';
                // if second character, set to C
                if(i == 1)
                    reply_msg_a.data[i] = 'A';
                // if third characater, set to K
                if(i == 2)
                    reply_msg_a.data[i] = 'C';
                // if any other character, set to null terminator
                if(i == 3)
                    reply_msg_a.data[i] = 'K';
                if(i != 0 && i != 1 && i != 2 && i != 3)
                    reply_msg_a.data[i] = '\0';
            }
            A_output(reply_msg_a);
            // upon returning from sending ACK, want to see if a message can be sent that
            // is queued as room was freed in the window
            if(sent_pkts_a.size < 8 && msg_queue_a.size != 0)
            {
                struct msg message;
                skip_queue_a = 1;
                for(i = 0; i < 20; i++)
                    message.data[i] = '\0';
                A_output(message);
            }
        }
        // if packet only contains data
        else if((!ack_valid) && (packet.seqnum == expected_seq_a))
        {
            tolayer5(0, packet.payload);
            for(i = 0; i < 20; i++)
            {
                // if first character, set to A
                if(i == 0)
                    reply_msg_a.data[i] = 'A';
                // if second character, set to C
                if(i == 1)
                    reply_msg_a.data[i] = 'C';
                // if third characater, set to K
                if(i == 2)
                    reply_msg_a.data[i] = 'K';
                // if any other character, set to null terminator
                if(i != 0 && i != 1 && i != 2)
                    reply_msg_a.data[i] = '\0';
            }
            acknum_a = packet.seqnum;
            printf("Packet %d received inside of A only contains valid data, no valid ACK\n", acknum_a);
            expected_seq_a += 1;
            if(expected_seq_a == 16)
                expected_seq_a = 1;
            ack_ready_a = 1;
            A_output(reply_msg_a);
        }
        // both sequence and ack invalid
        else
        {
            nacknum_a = ~expected_seq_a + 1;
            printf("Sequence number and ACK Invalid for the packet received by A, sending NAK %d\n", nacknum_a);
            // send a negative acknoweledgement
            send_nack_a = 1;
            for(i = 0; i < 20; i++)
            {
                // if first character, set to A
                if(i == 0)
                    reply_msg_a.data[i] = 'N';
                // if second character, set to C
                if(i == 1)
                    reply_msg_a.data[i] = 'A';
                // if third characater, set to K
                if(i == 2)
                    reply_msg_a.data[i] = 'C';
                // if any other character, set to null terminator
                if(i == 3)
                    reply_msg_a.data[i] = 'K';
                if(i != 0 && i != 1 && i != 2 && i != 3)
                    reply_msg_a.data[i] = '\0';
            }
            A_output(reply_msg_a);
        }
    }// end of if that checks for valid checksum
    else
    {
        nacknum_a = ~expected_seq_a + 1;
        printf("Checksum was invalid for the packet received by A, sending NAK %d\n", nacknum_a);
        // send a negative acknowledgement
        send_nack_a = 1;
        for(i = 0; i < 20; i++)
        {
            // if first character, set to A
            if(i == 0)
                reply_msg_a.data[i] = 'N';
            // if second character, set to C
            if(i == 1)
                reply_msg_a.data[i] = 'A';
            // if third characater, set to K
            if(i == 2)
                reply_msg_a.data[i] = 'C';
            // if any other character, set to null terminator
            if(i == 3)
                reply_msg_a.data[i] = 'K';
            if(i != 0 && i != 1 && i != 2 && i != 3)
                reply_msg_a.data[i] = '\0';
        }
        A_output(reply_msg_a);
//if the first packet received, and not ack sent yet, do nothing?
    }
}

/* called when A's timer goes off */
void A_timerinterrupt()
{
    int i = 0;
    int x;
    struct sent_packet *temp;
    struct pkt packet;
    int index = 0;
    while(i < sent_pkts_a.size)
    {
        index = sent_pkts_a.front + i;
        if(index > 16)
        {
            index = index % 16;
        }
        temp = sent_pkts_a.packets[index];
        // set that packet has been resent, for rtt calculation
        temp->resent += 1;
        packet.seqnum = temp->seqnum;
        // may want to make this also send ack's if applicable
        packet.acknum = acknum_a;
        for(x = 0; x < 20; x++)
        {
            packet.payload[x] = temp->payload[x];
        }
        packet.checksum = generate_checksum(&packet);

        if(i == 0)
        {
            starttimer(0, (temp->resent + 1)*timer_value_a);
            temp->packet_rtt = (temp->resent + 1)*timer_value_a;
        }

        temp->resent_time = time;
        i += 1;
        // need to be careful as acknowledgements may come in as you do this..
        printf("Resending packet %d in A and acknum %d\n", packet.seqnum, packet.acknum);
        tolayer3(0, packet);
    }
}

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init()
{
    seqnum_a = 0;                   // set the initial packet sequence number to 0
    acknum_a = 0;                   // set initial acknowledge number to 0
    expected_seq_a = 1;             // set to 0, should always be 0
    timer_value_a = 20.000;           // initialize the tiemr to 10.0000
    // new
    // initializing message queue
    msg_queue_a.size = 0;
    msg_queue_a.front = 0;
    msg_queue_a.rear = -1;
    // initializng packet queue
    sent_pkts_a.size = 0;
    sent_pkts_a.front = 1;
    sent_pkts_a.rear = 0;
    // set that no acknowledgements ready to be sent
    ack_ready_a = 0;
}


/* Note that with simplex transfer from a-to-B, there is no B_output() */

/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(struct pkt packet)
{
    // for for loop
    int i;
    struct msg reply_msg_b;
    // first see if the checksum is correct as this indicates the packet
    // was probably not altered
    if(generate_checksum(&packet) == packet.checksum)
    {
        // in this case, the packet should probably be a acknowlegement by itself
        if(packet.seqnum == -1)
        {
            int payload_valid = 1;
            // verify that the payload contains nothing but null-terminators
            for(i = 0; i < 20; i++)
            {
                if(packet.payload[i] != '\0')
                {
                    printf("Payload invalid in B for a acknowledgement\n");
                    payload_valid = 0;
                }
            }
            // if the payload is what is expected
            if(payload_valid)
            {
                // if not expecting any acknowledgements, simply return
                if(sent_pkts_b.size == 0)
                {
                    printf("Not expecting any acknowedgments in B\n");
                    return;
                }
                int removal_number = check_ack(sent_pkts_b.front, sent_pkts_b.rear, packet.acknum, sent_pkts_b.size);
                // will be 0 if the ack number is not within a appropriate range
                if(removal_number)
                {
                    printf("Recieved ACK in B for %d packets\n", removal_number);
                    valid_ack(1, removal_number, &sent_pkts_b, timer_value_b);
                    if(sent_pkts_b.size < 8 && msg_queue_b.size != 0)
                    {
                        struct msg message;
                        skip_queue_b = 1;
                        for(i = 0; i < 20; i++)
                            message.data[i] = '\0';
                        B_output(message);
                    }
                    return;
                }
                else
                {
                    // in GBN, if duplicate ack received, do nothing...only resend on timeout
                    printf("Incorrect acknowedgment number received by B\n\n");
                    return;
                }
            }
            else
            {
                printf("The payload for the acknowledgement was invalid in B\n\n");
            }
            return;
        } // end of just a acknowledgement being received
        //if correct sequence number and ack valid...
        int ack_valid = check_ack(sent_pkts_b.front, sent_pkts_b.rear, packet.acknum, sent_pkts_b.size);
        if((packet.seqnum == expected_seq_b) && ack_valid)
        {
            tolayer5(1, packet.payload);
            for(i = 0; i < 20; i++)
            {
                // if first character, set to A
                if(i == 0)
                    reply_msg_b.data[i] = 'A';
                // if second character, set to C
                if(i == 1)
                    reply_msg_b.data[i] = 'C';
                // if third characater, set to K
                if(i == 2)
                    reply_msg_b.data[i] = 'K';
                // if any other character, set to null terminator
                if(i != 0 && i != 1 && i != 2)
                    reply_msg_b.data[i] = '\0';
            }
            ack_ready_b = 1;
            acknum_b = packet.seqnum;
            expected_seq_b += 1;
            if(expected_seq_b == 16)
                expected_seq_b = 1;
            // update sent packets array since acknowledgement was valid
            printf("Received a ACK for packet %d in B\n", packet.acknum);
            valid_ack(1, ack_valid, &sent_pkts_b, timer_value_b);
            printf("Sending acknowledgement in B for packet %d\n", acknum_b);
            B_output(reply_msg_b);
            if(sent_pkts_b.size < 8 && msg_queue_b.size != 0)
            {
                struct msg message;
                skip_queue_b = 1;
                for(i = 0; i < 20; i++)
                    message.data[i] = '\0';
                B_output(message);
            }
            return;
        }
        // if the sequence is -1, may be a negative acknowledgemnt if the acknum is also invalid
        // because the acknum is the negative of the next expected acknumber
        else if((packet.seqnum == 0) && !ack_valid)
        {
            int acknum_rcvd = 0;
            int unacked_pkt = 0;
            unacked_pkt = (~packet.acknum) + 1;
            if(sent_pkts_b.size == 0)
            {
                printf("May have received a negative acknowledgement in B but did not have any packets sent out that need acknowledged\n");
                // may want to send nak if other side trying to send something?
                return;
            }
            int nack_rcvd = 1;
            for(i = 0; i < 20; i++)
            {
                if(packet.payload[i] != '\0')
                {
//                    printf("%c\n", packet.payload[i]);
                    nack_rcvd = 0;
                    break;
                }
            }
            // if the payload is correct, see if the reverse ack number is valid
            // it may not be valid if we already received the previous ack for this
            // number

            if(nack_rcvd)
            {
                acknum_rcvd = (~packet.acknum);
                // corner case if the next expected is 1, then the -1 should go to 16
                if(acknum_rcvd == 0)
                {
                    acknum_rcvd = 16;
                }
                if(acknum_rcvd <= 0 || acknum_rcvd > 16)
                {
                    nack_rcvd = 0;
                }
                else
                {
                    ack_valid = check_ack(sent_pkts_b.front, sent_pkts_b.rear, acknum_rcvd, sent_pkts_b.size);
                }
            }
            // if a negative acknowledgement was received and there are packets to remove..
            if(nack_rcvd && ack_valid)
            {
                // update the sent packets array to remove all that were acknowledged
                printf("Processing a negative acknowledgement in B that contains a valid ack\n");             // for testing
                valid_ack(1, ack_valid, &sent_pkts_b, timer_value_b);
                // upon returning from sending ACK, want to see if a message can be sent that
                // is queued as room was freed in the window

                // check to make sure packet not acknowledged yet
                if(check_ack(sent_pkts_b.front, sent_pkts_b.rear, unacked_pkt, sent_pkts_b.size))
                {
                    resend_packet(1, unacked_pkt, &sent_pkts_b, timer_value_b);
                }
                if(sent_pkts_b.size < 8 && msg_queue_b.size != 0)
                {
                    struct msg message;
                    skip_queue_b = 1;
                    for(i = 0; i < 20; i++)
                        message.data[i] = '\0';
                    B_output(message);
                }
            }
            else if(unacked_pkt > 0 && unacked_pkt < 17)      // message was not a negative acknowledgement, or did not acknowledge any messages we already knew about
            {   // negative ack does not contain packets to remove..
                if(check_ack(sent_pkts_b.front, sent_pkts_b.rear, unacked_pkt, sent_pkts_b.size))
                {
                    resend_packet(1, unacked_pkt, &sent_pkts_b, timer_value_b);
                }
                else
                {
                    printf("Received a negative acknowledgement in B but nothing to do with it\n");
                }
            }
            else
            {
                printf("Negative acknowledgement received is invalid in B\n");
            }
        }
        // if sequence incorrect but ack valid
        else if((packet.seqnum != expected_seq_b) && ack_valid)
        {
            // update the sent packets array to remove all that were acknowledged
            valid_ack(1, ack_valid, &sent_pkts_b, timer_value_b);
            // need to resend the last acknowledgement here
            send_nack_b = 1;
            nacknum_b = ~expected_seq_b + 1;
            printf("Sequence number incorrect in B but ACK valid, sending NAK %d\n", nacknum_b);
            for(i = 0; i < 20; i++)
            {
                // if first character, set to A
                if(i == 0)
                    reply_msg_b.data[i] = 'N';
                // if second character, set to C
                if(i == 1)
                    reply_msg_b.data[i] = 'A';
                // if third characater, set to K
                if(i == 2)
                    reply_msg_b.data[i] = 'C';
                // if any other character, set to null terminator
                if(i == 3)
                    reply_msg_b.data[i] = 'K';
                if(i != 0 && i != 1 && i != 2 && i != 3)
                    reply_msg_b.data[i] = '\0';
            }
            B_output(reply_msg_b);
            // upon returning from sending ACK, want to see if a message can be sent that
            // is queued as room was freed in the window
            if(sent_pkts_b.size < 8 && msg_queue_b.size != 0)
            {
                struct msg message;
                skip_queue_b = 1;
                for(i = 0; i < 20; i++)
                    message.data[i] = '\0';
                B_output(message);
            }
        }
        // if packet only contains data
        else if((!ack_valid) && (packet.seqnum == expected_seq_b))
        {
            tolayer5(1, packet.payload);
            for(i = 0; i < 20; i++)
            {
                // if first character, set to A
                if(i == 0)
                    reply_msg_b.data[i] = 'A';
                // if second character, set to C
                if(i == 1)
                    reply_msg_b.data[i] = 'C';
                // if third characater, set to K
                if(i == 2)
                    reply_msg_b.data[i] = 'K';
                // if any other character, set to null terminator
                if(i != 0 && i != 1 && i != 2)
                    reply_msg_b.data[i] = '\0';
            }
            ack_ready_b = 1;
            acknum_b = packet.seqnum;
            printf("Packet received inside of B only contains valid data, sending ACK %d\n", acknum_b);
            expected_seq_b += 1;
            if(expected_seq_b == 16)
                expected_seq_b = 1;
            B_output(reply_msg_b);
        }
        // both sequence and ack invalid
        else
        {
            // ressend the last acknowledgement
            send_nack_b = 1;
            nacknum_b = ~expected_seq_b + 1;
            printf("The sequence number and the ack were invalid in B, sending NAK %d\n", nacknum_b);
            for(i = 0; i < 20; i++)
            {
                // if first character, set to A
                if(i == 0)
                    reply_msg_b.data[i] = 'N';
                // if second character, set to C
                if(i == 1)
                    reply_msg_b.data[i] = 'A';
                // if third characater, set to K
                if(i == 2)
                    reply_msg_b.data[i] = 'C';
                // if any other character, set to null terminator
                if(i == 3)
                    reply_msg_b.data[i] = 'K';
                if(i != 0 && i != 1 && i != 2 && i != 3)
                    reply_msg_b.data[i] = '\0';
            }
            B_output(reply_msg_b);
        }
    }// end of if that checks for valid checksum
    else
    {
        // resend the last acknowledgement
        send_nack_b = 1;
        nacknum_b = ~expected_seq_b + 1;
        printf("Checksum was invalid for the packet received by B, sending NAK %d\n", nacknum_b);
        for(i = 0; i < 20; i++)
        {
            // if first character, set to A
            if(i == 0)
                reply_msg_b.data[i] = 'N';
            // if second character, set to C
            if(i == 1)
                reply_msg_b.data[i] = 'A';
            // if third characater, set to K
            if(i == 2)
                reply_msg_b.data[i] = 'C';
            // if any other character, set to null terminator
            if(i == 3)
                reply_msg_b.data[i] = 'K';
            if(i != 0 && i != 1 && i != 2 && i != 3)
                reply_msg_b.data[i] = '\0';
        }
        B_output(reply_msg_b);
    }
}

/* called when B's timer goes off */
// do not think a timer is needed for part 1 as only a is sending data to b
// if a's timout occurs, will cause a to resend the packet that b tried to acknowledge
void B_timerinterrupt()
{
    int i = 0;
    int x;
    struct sent_packet *temp;
    struct pkt packet;
    int index;
    while(i < sent_pkts_b.size)
    {
        index = sent_pkts_b.front + i;
        if(index > 16)
        {
            index = index % 16;
        }
        temp = sent_pkts_b.packets[index];
        temp->resent += 1;
        packet.seqnum = temp->seqnum;
        // may want to make this also send ack's if applicable
        packet.acknum = acknum_b;
        for(x = 0; x < 20; x++)
        {
            packet.payload[x] = temp->payload[x];
        }
        packet.checksum = generate_checksum(&packet);
        if(i == 0)
        {
            starttimer(1, (temp->resent + 1)*timer_value_b);
            temp->packet_rtt = (temp->resent + 1)*timer_value_b;
        }

        temp->resent_time = time;
        i += 1;
        // need to be careful as acknowledgements may come in as you do this..
        printf("Resending a packet for B with sequence number %d and acknum %d\n", packet.seqnum, acknum_b);
        tolayer3(1, packet);
    }
}

/* the following rouytine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init()
{
    seqnum_b = 0;                   // set the initial packet sequence number to 0
    acknum_b = 0;                   // set initial acknowledge number to 0
    expected_seq_b = 1;             // set to 0, should always be 0
    timer_value_b = 20.000;           // initialize the tiemr to 10.0000
    // new
    // initializing message queue
    msg_queue_b.size = 0;
    msg_queue_b.front = 0;
    msg_queue_b.rear = -1;
    // initializng packet queue
    sent_pkts_b.size = 0;
    sent_pkts_b.front = 1;
    sent_pkts_b.rear = 0;
    // set that no acknowledgements ready to be sent
    ack_ready_b = 0;
}


/*****************************************************************
***************** NETWORK EMULATION CODE STARTS BELOW ***********
The code below emulates the layer 3 and below network environment:
  - emulates the tranmission and delivery (possibly with bit-level corruption
    and packet loss) of packets across the layer 3/4 interface
  - handles the starting/stopping of a timer, and generates timer
    interrupts (resulting in calling students timer handler).
  - generates message to be sent (passed from later 5 to 4)

THERE IS NOT REASON THAT ANY STUDENT SHOULD HAVE TO READ OR UNDERSTAND
THE CODE BELOW.  YOU SHOULD NOT TOUCH, OR REFERENCE (in your code) ANY
OF THE DATA STRUCTURES BELOW.  If you're interested in how I designed
the emulator, you're welcome to look at the code - but again, you should have
to, and you definitely should not have to modify
******************************************************************/

struct event {
   float evtime;           /* event time */
   int evtype;             /* event type code */
   int eventity;           /* entity where event occurs */
   struct pkt *pktptr;     /* ptr to packet (if any) assoc w/ this event */
   struct event *prev;
   struct event *next;
};
struct event *evlist = NULL;   /* the event list */

/* possible events: */
#define  TIMER_INTERRUPT 0
#define  FROM_LAYER5     1
#define  FROM_LAYER3     2

#define  OFF    0
#define  ON     1
#define  A      0
#define  B      1

int TRACE = 1;             /* for my debugging */
//int nsim = 0;              /* number of messages from 5 to 4 so far */
int nsimmax = 0;           /* number of msgs to generate, then stop */
//float time = 0.000;      // moved to above
float lossprob;            /* probability that a packet is dropped  */
float corruptprob;         /* probability that one bit is packet is flipped */
float lambda;              /* arrival rate of messages from layer 5 */
int   ntolayer3;           /* number sent into layer 3 */
int   nlost;               /* number lost in media */
int   ncorrupt;            /* number corrupted by media*/

void init();
void generate_next_arrival();
void tolayer3(int AorB, struct pkt packet);
void starttimer(int AorB, float increment);
void stoptimer(int AorB);
void insertevent(struct event *p);
void init();
float jimsrand();
void printevlist();

int main()
{
   struct event *eventptr;
   struct msg  msg2give;
   struct pkt  pkt2give;

   int i,j;
   char c;

   init();
   A_init();
   B_init();

   while (1) {
        eventptr = evlist;            /* get next event to simulate */
        if (eventptr==NULL)
           goto terminate;
        evlist = evlist->next;        /* remove this event from event list */
        if (evlist!=NULL)
           evlist->prev=NULL;
        if (TRACE>=2) {
           printf("\nEVENT time: %f,",eventptr->evtime);
           printf("  type: %d",eventptr->evtype);
           if (eventptr->evtype==0)
	       printf(", timerinterrupt  ");
             else if (eventptr->evtype==1)
               printf(", fromlayer5 ");
             else
	     printf(", fromlayer3 ");
           printf(" entity: %d\n",eventptr->eventity);
           }
        time = eventptr->evtime;        /* update time to next event time */
        if (nsim==nsimmax)
	  break;                        /* all done with simulation */
        if (eventptr->evtype == FROM_LAYER5 ) {
            generate_next_arrival();   /* set up future arrival */
            /* fill in msg to give with string of same letter */
            j = nsim % 26;
            for (i=0; i<20; i++)
               msg2give.data[i] = 97 + j;
            if (TRACE>2) {
               printf("          MAINLOOP: data given to student: ");
                 for (i=0; i<20; i++)
                  printf("%c", msg2give.data[i]);
               printf("\n");
	     }
            nsim++;
            if (eventptr->eventity == A)
               A_output(msg2give);
             else
               B_output(msg2give);
            }
          else if (eventptr->evtype ==  FROM_LAYER3) {
            pkt2give.seqnum = eventptr->pktptr->seqnum;
            pkt2give.acknum = eventptr->pktptr->acknum;
            pkt2give.checksum = eventptr->pktptr->checksum;
            for (i=0; i<20; i++)
                pkt2give.payload[i] = eventptr->pktptr->payload[i];
	    if (eventptr->eventity ==A)      /* deliver packet by calling */
   	       A_input(pkt2give);            /* appropriate entity */
            else
   	       B_input(pkt2give);
	    free(eventptr->pktptr);          /* free the memory for packet */
            }
          else if (eventptr->evtype ==  TIMER_INTERRUPT) {
            if (eventptr->eventity == A)
	       A_timerinterrupt();
             else
	       B_timerinterrupt();
             }
          else  {
	     printf("INTERNAL PANIC: unknown event type \n");
             }
        free(eventptr);
        }

terminate:
   printf(" Simulator terminated at time %f\n after sending %d msgs from layer5\n",time,nsim);
   printf("Packets received A: %d B %d\n", packets_rcvd_a, packets_rcvd_b);
   return 0;
}



void init()                         /* initialize the simulator */
{
  int i;
  float sum, avg;
  float jimsrand();


   printf("-----  Network Simulator Version 1.1 -------- \n\n");
   printf("Enter the number of messages to simulate: ");
   scanf("%d",&nsimmax);
   printf("Enter  packet loss probability [enter 0.0 for no loss]:");
   scanf("%f",&lossprob);
   printf("Enter packet corruption probability [0.0 for no corruption]:");
   scanf("%f",&corruptprob);
   printf("Enter average time between messages from sender's layer5 [ > 0.0]:");
   scanf("%f",&lambda);
   printf("Enter TRACE:");
   scanf("%d",&TRACE);

   srand(9999);              /* init random number generator */
   sum = 0.0;                /* test random number generator for students */
   for (i=0; i<1000; i++)
      sum=sum+jimsrand();    /* jimsrand() should be uniform in [0,1] */
   avg = sum/1000.0;
   if (avg < 0.25 || avg > 0.75) {
    printf("It is likely that random number generation on your machine\n" );
    printf("is different from what this emulator expects.  Please take\n");
    printf("a look at the routine jimsrand() in the emulator code. Sorry. \n");
    exit(1);
    }

   ntolayer3 = 0;
   nlost = 0;
   ncorrupt = 0;

   time=0.0;                    /* initialize time to 0.0 */
   generate_next_arrival();     /* initialize event list */
}

/****************************************************************************/
/* jimsrand(): return a float in range [0,1].  The routine below is used to */
/* isolate all random number generation in one location.  We assume that the*/
/* system-supplied rand() function return an int in therange [0,mmm]        */
/****************************************************************************/
float jimsrand()
{
  double mmm = RAND_MAX;   /* largest int  - MACHINE DEPENDENT!!!!!!!!   */
  float x;                   /* individual students may need to change mmm */
  x = rand()/mmm;            /* x should be uniform in [0,1] */
  return(x);
}

/********************* EVENT HANDLINE ROUTINES *******/
/*  The next set of routines handle the event list   */
/*****************************************************/

void generate_next_arrival()
{
   double x;
   struct event *evptr;
   float ttime;
   int tempint;

   if (TRACE>2)
       printf("          GENERATE NEXT ARRIVAL: creating new arrival\n");

   x = lambda*jimsrand()*2;  /* x is uniform on [0,2*lambda] */
                             /* having mean of lambda        */
   evptr = (struct event *)malloc(sizeof(struct event));
   evptr->evtime =  time + x;
   evptr->evtype =  FROM_LAYER5;
   if (BIDIRECTIONAL && (jimsrand()>0.5) )
      evptr->eventity = B;
    else
      evptr->eventity = A;
   insertevent(evptr);
}


void insertevent(struct event *p)
{
   struct event *q,*qold;

   if (TRACE>2) {
      printf("            INSERTEVENT: time is %lf\n",time);
      printf("            INSERTEVENT: future time will be %lf\n",p->evtime);
      }
   q = evlist;     /* q points to header of list in which p struct inserted */
   if (q==NULL) {   /* list is empty */
        evlist=p;
        p->next=NULL;
        p->prev=NULL;
        }
     else {
        for (qold = q; q !=NULL && p->evtime > q->evtime; q=q->next)
              qold=q;
        if (q==NULL) {   /* end of list */
             qold->next = p;
             p->prev = qold;
             p->next = NULL;
             }
           else if (q==evlist) { /* front of list */
             p->next=evlist;
             p->prev=NULL;
             p->next->prev=p;
             evlist = p;
             }
           else {     /* middle of list */
             p->next=q;
             p->prev=q->prev;
             q->prev->next=p;
             q->prev=p;
             }
         }
}

void printevlist()
{
  struct event *q;
  int i;
  printf("--------------\nEvent List Follows:\n");
  for(q = evlist; q!=NULL; q=q->next) {
    printf("Event time: %f, type: %d entity: %d\n",q->evtime,q->evtype,q->eventity);
    }
  printf("--------------\n");
}



/********************** Student-callable ROUTINES ***********************/

/* called by students routine to cancel a previously-started timer */
void stoptimer(int AorB)
{
 struct event *q,*qold;

 if (TRACE>2)
    printf("          STOP TIMER: stopping timer at %f\n",time);
/* for (q=evlist; q!=NULL && q->next!=NULL; q = q->next)  */
 for (q=evlist; q!=NULL ; q = q->next)
    if ( (q->evtype==TIMER_INTERRUPT  && q->eventity==AorB) ) {
       /* remove this event */
       if (q->next==NULL && q->prev==NULL)
             evlist=NULL;         /* remove first and only event on list */
          else if (q->next==NULL) /* end of list - there is one in front */
             q->prev->next = NULL;
          else if (q==evlist) { /* front of list - there must be event after */
             q->next->prev=NULL;
             evlist = q->next;
             }
           else {     /* middle of list */
             q->next->prev = q->prev;
             q->prev->next =  q->next;
             }
       free(q);
       return;
     }
  printf("Warning: unable to cancel your timer. It wasn't running.\n");
}


void starttimer(int AorB, float increment)
{

 struct event *q;
 struct event *evptr;

 if (TRACE>2)
    printf("          START TIMER: starting timer at %f\n",time);
 /* be nice: check to see if timer is already started, if so, then  warn */
/* for (q=evlist; q!=NULL && q->next!=NULL; q = q->next)  */
   for (q=evlist; q!=NULL ; q = q->next)
    if ( (q->evtype==TIMER_INTERRUPT  && q->eventity==AorB) ) {
      printf("Warning: attempt to start a timer that is already started\n");
      return;
      }

/* create future event for when timer goes off */
   evptr = (struct event *)malloc(sizeof(struct event));
   evptr->evtime =  time + increment;
   evptr->evtype =  TIMER_INTERRUPT;
   evptr->eventity = AorB;
   insertevent(evptr);
}


/************************** TOLAYER3 ***************/
void tolayer3(int AorB, struct pkt packet)
{
 struct pkt *mypktptr;
 struct event *evptr,*q;
 float lastime, x;
 int i;


 ntolayer3++;

 /* simulate losses: */
 if (jimsrand() < lossprob)  {
      nlost++;
      if (TRACE>0)
	printf("          TOLAYER3: packet being lost\n");
      return;
    }

/* make a copy of the packet student just gave me since he/she may decide */
/* to do something with the packet after we return back to him/her */
 mypktptr = (struct pkt *)malloc(sizeof(struct pkt));
 mypktptr->seqnum = packet.seqnum;
 mypktptr->acknum = packet.acknum;
 mypktptr->checksum = packet.checksum;
 for (i=0; i<20; i++)
    mypktptr->payload[i] = packet.payload[i];
 if (TRACE>2)  {
   printf("          TOLAYER3: seq: %d, ack %d, check: %d ", mypktptr->seqnum,
	  mypktptr->acknum,  mypktptr->checksum);
    for (i=0; i<20; i++)
        printf("%c",mypktptr->payload[i]);
    printf("\n");
   }

/* create future event for arrival of packet at the other side */
  evptr = (struct event *)malloc(sizeof(struct event));
  evptr->evtype =  FROM_LAYER3;   /* packet will pop out from layer3 */
  evptr->eventity = (AorB+1) % 2; /* event occurs at other entity */
  evptr->pktptr = mypktptr;       /* save ptr to my copy of packet */
/* finally, compute the arrival time of packet at the other end.
   medium can not reorder, so make sure packet arrives between 1 and 10
   time units after the latest arrival time of packets
   currently in the medium on their way to the destination */
 lastime = time;
/* for (q=evlist; q!=NULL && q->next!=NULL; q = q->next) */
 for (q=evlist; q!=NULL ; q = q->next)
    if ( (q->evtype==FROM_LAYER3  && q->eventity==evptr->eventity) )
      lastime = q->evtime;
 evptr->evtime =  lastime + 1 + 9*jimsrand();



 /* simulate corruption: */
 if (jimsrand() < corruptprob)  {
    ncorrupt++;
    if ( (x = jimsrand()) < .75)
       mypktptr->payload[0]='Z';   /* corrupt payload */
      else if (x < .875)
       mypktptr->seqnum = 999999;
      else
       mypktptr->acknum = 999999;
    if (TRACE>0)
	printf("          TOLAYER3: packet being corrupted\n");
    }

  if (TRACE>2)
     printf("          TOLAYER3: scheduling arrival on other side\n");
  insertevent(evptr);
}

void tolayer5(int AorB, char datasent[20])
{
  int i;
  if (TRACE>2) {
     printf("          TOLAYER5: data received: ");
     for (i=0; i<20; i++)
        printf("%c",datasent[i]);
     printf("\n");
   }

}
