#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// Matt Stropkey
// Project 2 CS1652 Part 1
// June 26, 2019

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

#define BIDIRECTIONAL 0

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
    char payload[20];
};

void tolayer5(int AorB, char datasent[20]);
void starttimer(int AorB, float increment);
void tolayer3(int AorB, struct pkt packet);
void stoptimer(int AorB);
/********* STUDENTS WRITE THE NEXT SEVEN ROUTINES *********/

int seqnum_a;          // used to hold the current sequence number for a
int seqnum_b;          // used to hold the current sequence number for b
int acknum_a;          // used to hold value of last sent ack by a
int acknum_b;          // used to hold the value of the last sent ack by b
int expected_seq_b;    // used to hold expected sequence value for a packet coming from a
int expected_seq_a;    // used to hold expected sequence value for a packet coming from b
struct pkt* mypktptr_a;   // pointer to the current packet being sent by a
struct pkt* mypktptr_b;   // pointer to the current packet being sent by b
int packet_valid_b;       // used by b as a boolean
int packet_ack_a;         // used by a to indicate when a ack was received from b
struct prev_pkt prev_pkt;         // used to store values of previuos packet for b
int duplicate_rcvd;     // boolean for receiver if duplicate packet received
float time = 0.000;     // holds time, moved from below
float start_time = 0.000;    // holds the start time of when packet sent
float return_time = 0.000;   // holds return time of when response received from receiver
float rtt = 10.000;      // holds the average round trip time
float rtt_dev = 0.000;  // holds the average deviation in round trip time
float timer_value = 0.000;    // holds actual timer value passed to timer
const float ALPHA = 0.125;    // used for RTT calculation
const float BETA = 0.25;     // used for RTT deviation calculation
float sample_rtt = 0.000;    // used to hold current rtt
int packet_lost;             // boolean to hold if timeout for packet occurred
int update_rtt;              // boolean to update rtt(need this and packet last because will not update if retrans and not lost)
int unacked_packet = 0;      // boolean used to indicate if there is a unacknowledged packet


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
void calculate_rtt()
{
    // get the absolute value of the difference between the current rtt and the previous calculated average
    float absolute_dif = fabsf(sample_rtt - rtt);
    // get the updated average rtt
    rtt = ((1-ALPHA) * rtt) + (ALPHA * sample_rtt);
    // get the updated deviation in the average rtt
    rtt_dev = ((1-BETA) * rtt_dev) + (BETA * absolute_dif);
    // get the updated value to set the timer to
    timer_value = rtt + (4 * rtt_dev);
}

/* called from layer 5, passed the data to be sent to other side */
void A_output(struct msg message)
{
    // if there is already an unacknowledged packet, just return
    if(unacked_packet)
    {
        printf("Warning: a message was discarded as there is aleady a unacknowledged message\n");       // for testing
        return;
    }
    int i;  // for the for loop
    mypktptr_a = (struct pkt *)malloc(sizeof(struct pkt));       // allocate memory for a packet to send
    // 1. get the current sequence number for a
    mypktptr_a->seqnum = seqnum_a;                     // set the packets sequence number(0 or 1)
    // set the acknowledgement number to 0, will always be 0 for a as a is not acknowledging anything
    mypktptr_a->acknum = 0;
    // copy the message into the packet
    for (i=0; i<20; i++)
       mypktptr_a->payload[i] = message.data[i];
    // set the checksum for the packet
    mypktptr_a->checksum = generate_checksum(mypktptr_a);
    // set the start time so that the new rtt can be calculated
    start_time = time;
    // start the timer
    starttimer(0, timer_value);
    // set boolean that there is a unacknowledged packet
    unacked_packet = 1;
    // set boolean that as of right now, rtt should be updated as the packet has not been retransmitted
    update_rtt = 1;
    // set boolean that a timeout has not occurred, thus a packet was not lost
    packet_lost = 0;
    // boolean set to indicate packet has not been acknowledged yet
    packet_ack_a = 0;
    // send the packet to layer 3
    printf("Sending packet with sequence number %d at A at time %f\n", mypktptr_a->seqnum, time);
    tolayer3(0, *mypktptr_a);
}

// function resends the packet if it was not acknowledged
// or if a timeout occurred
void resend_packet_a()
{
    // if packet has yet to be acknowledged, resend it
    // set boolean to update rtt to false so as packet being retransmitted
    update_rtt = 0;
    // if the packet has not been acknowledged yet
    if(!packet_ack_a)
    {
        // if the packet has been lost, restart the timer with 2 times the value of the timer_value
        if(packet_lost)
        {
            starttimer(0, 2*timer_value);
        }
        printf("Resending packet for A with sequence number %d\n", mypktptr_a->seqnum);
        // resend the packet
        tolayer3(0, *mypktptr_a);
    }
}

void B_output(struct msg message)
{
    int i;  // for the for loop
    mypktptr_b = (struct pkt *)malloc(sizeof(struct pkt));       // allocate memory for a packet to send

    // if correct packet received, update values
    if(packet_valid_b && !duplicate_rcvd)
    {
        // flip the acknowledgement number for B
        if(acknum_b == 0)
        {
            acknum_b = 1;
        }
        else
        {
            acknum_b = 0;
        }
        // flip the expected sequence number for B
        if(expected_seq_b == 0)
        {
            expected_seq_b = 1;
        }
        else
        {
            expected_seq_b = 0;
        }
    }

    // set the packets acknowledgement number to the packet sequence number that was received
    mypktptr_b->acknum = acknum_b;
    // set the sequence number to 0, a does not need to worry about sequence number, just ack
    mypktptr_b->seqnum = 0;
    // copy the message into the packet( wherher ACK or NACK )
    for (i=0; i<20; i++)
        mypktptr_b->payload[i] = message.data[i];
    // set the checksum for the packet
    mypktptr_b->checksum = generate_checksum(mypktptr_b);
    // send the packet to the other side
    tolayer3(1, *mypktptr_b);
    // free the memory for the packet
    free(mypktptr_b);
}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(struct pkt packet)
{
    // boolean to indicate if correct sequence number detected
    int seq_valid = 0;
    // boolean to indicate if valid checksum detected
    int valid_checksum = 0;
    // boolean to indicate if valid acknowedgement number detected
    int valid_ack = 0;
    // used to hold

    // if a acknowledgement comes in when not waiting for one, just return
    if(!unacked_packet)
    {
        printf("Received acknowledgement in A but not waiting on any at time %f\n", time);       // for testing
        return;
    }
    // stop the timer as response received from reciever
    // check the sequence number, will always be 0 when coming from receiver
    // as only need to consider if acknowledge number is the packet sequence number that
    // was sent
    if(packet.seqnum == 0)
    {
        //printf("Sequence number valid from B\n");                                 // for testing
        seq_valid = 1;
    }

    // if sequence is valid and checksum is correct and the acknowedgement number matches
    // the sequence number of the packet sent originally
    if(seq_valid && (generate_checksum(&packet) == packet.checksum) && packet.acknum == seqnum_a)
    {
        // set boolean that the checksum is valid
        valid_checksum = 1;
        // if the return message only has 3 characters, see if it is ACK
        if(strlen(packet.payload) == 3)
        {
            // if the sent message was ACK, the packet was acknowledged
            if(!strcmp(packet.payload, "ACK"))
            {
                // set that acknowledgement is valid
                valid_ack = 1;
            }
        }

    }
    // if the packet is in fact valid
    if(seq_valid && valid_checksum && valid_ack)
    {
        // flip the sequence number for a if pakcet was acknowledged
        if(seqnum_a == 0)
        {
            seqnum_a = 1;
        }
        else
        {
            seqnum_a = 0;
        }

        // if the packet did not have to be retransmitted, update the rtt
        stoptimer(0);
        if(update_rtt)
        {
            // set return time
            return_time = time;
            // set the current rtt for this packet
            sample_rtt = return_time -start_time;
            // caculate the new rtt to get the new timer value
            calculate_rtt();
        }
        // set boolean that packet is acknowledged
        packet_ack_a = 1;
        free(mypktptr_a);              // free the packet from memory
        // set the boolean that there are not any unacknowledged packets
        unacked_packet = 0;
        printf("The valid packet acknowedgement was received by A at %f\n", time);    // for testing
        return;
    }
    else      // if the packet was not acknowledged, or packet was invalid
    {
        // set that the packet was not acknowledged
        packet_ack_a = 0;
        printf("Invalid packet acknowledgement received by A\n");   // for testing
        stoptimer(0);
        packet_lost = 1;
        resend_packet_a();
    }
}

/* called when A's timer goes off */
void A_timerinterrupt()
{
    // do not want to update rtt because if 2 ACK packets arrive after, will mess up timer
    packet_lost = 1;
    printf("Timer interrupt in A at %f\n", time);               // for testing
    // call method to resend packet
    resend_packet_a();
}

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init()
{
    seqnum_a = 0;                   // set the initial packet sequence number to 0
    acknum_a = 0;                   // set initial acknowledge number to 0
    expected_seq_a = 0;             // set to 0, should always be 0
    packet_ack_a = 0;               // set that there are currently no unacknowledged packets
    timer_value = 10.000;           // initialize the tiemr to 10.0000
}


/* Note that with simplex transfer from a-to-B, there is no B_output() */

/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(struct pkt packet)
{
    // boolean to indicate if sequence number is valid
    int seq_valid = 0;
    // boolean to indicate if the checksum is valid
    int valid_checksum = 0;
    // boolean for B side to indicate packet not valid
    packet_valid_b = 0;
    // set that a duplicate packet was not received
    duplicate_rcvd = 0;
    // for the for loop
    int i;
    // struct for the reply from the receiver
    struct msg reply_msg;

    // check to see if the sequence number in the packet is the next packet expected
    if(packet.seqnum == expected_seq_b)
    {
        seq_valid = 1;
    }

    // if sequence valid, checksum correct, and the acknowledgement is correct
    if(seq_valid && generate_checksum(&packet) == packet.checksum && packet.acknum == 0)
    {
        valid_checksum = 1;
    }
    // if the packet is in fact valid
    if(seq_valid && valid_checksum)
    {
        // set message to acknowledge
        for(i = 0; i < 20; i++)
        {
            // if first character, set to A
            if(i == 0)
                reply_msg.data[i] = 'A';
            // if second character, set to C
            if(i == 1)
                reply_msg.data[i] = 'C';
            // if third characater, set to K
            if(i == 2)
                reply_msg.data[i] = 'K';
            // if any other character, set to null terminator
            if(i != 0 && i != 1 && i != 2)
                reply_msg.data[i] = '\0';
        }
        tolayer5(1, packet.payload);
        printf("Sending acknowledgement for packet %d in B\n", packet.seqnum);
        // set the boolean that the packet is valid
        packet_valid_b = 1;
        // set this packet as the last received one for when duplicates are received
        prev_pkt.seqnum = packet.seqnum;
        prev_pkt.checksum = packet.checksum;
        prev_pkt.acknum = packet.acknum;
        for(i = 0; i < 20; i++)
        {
            prev_pkt.payload[i] = packet.payload[i];
        }
    }
    else              // packet is incorrect or a duplicate
    {
        // send previous ack if duplicate received
        if(packet.seqnum == prev_pkt.seqnum)
        {
            if(packet.checksum == prev_pkt.checksum)
            {
                if(packet.acknum == prev_pkt.acknum)
                {
                    int duplicate = 1;
                    for(i = 0; i < 20; i++)
                    {
                        if(packet.payload[i] != prev_pkt.payload[i])
                        {
                            duplicate = 0;
                            break;
                        }
                    }
                    if(duplicate)
                    {
                        printf("Resending previous ACK again as a duplicate packet was received by B\n");     // for testing
                        // set that a duplicate was received
                        duplicate_rcvd = 1;
                        for(i = 0; i < 20; i++)
                        {
                            // if first character, set to A
                            if(i == 0)
                                reply_msg.data[i] = 'A';
                            // if second character, set to C
                            if(i == 1)
                                reply_msg.data[i] = 'C';
                            // if third characater, set to K
                            if(i == 2)
                                reply_msg.data[i] = 'K';
                            // if any other character, set to null terminator
                            if(i != 0 && i != 1 && i != 2)
                                reply_msg.data[i] = '\0';
                        }
                    }

                }
            }
        }
        // set message to NACK if not a duplicate
        if(!duplicate_rcvd)
        {
            for(i = 0; i < 20; i++)
            {
                // if first character, set to A
                if(i == 0)
                    reply_msg.data[i] = 'N';
                // if second character, set to C
                if(i == 1)
                    reply_msg.data[i] = 'A';
                // if third characater, set to K
                if(i == 2)
                    reply_msg.data[i] = 'C';
                if(i == 3)
                    reply_msg.data[i] = 'K';
                // if any other character, set to null terminator
                if(i != 0 && i != 1 && i != 2 && i != 3)
                    reply_msg.data[i] = '\0';
            }
            printf("Sending a negative acknowledgement in B\n");
        }

    }
    // send the message to be output
    B_output(reply_msg);
}

/* called when B's timer goes off */
// do not think a timer is needed for part 1 as only a is sending data to b
// if a's timout occurs, will cause a to resend the packet that b tried to acknowledge
void B_timerinterrupt()
{
}

/* the following rouytine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init()
{
    seqnum_b = 0;           // set the initial packet sequence number to 0
    acknum_b = 1;           // set to 1 as will flip if correct packet received
    expected_seq_b = 0;     // set the expected sequence number to 0
    packet_valid_b = 0;     // used as boolean as to whether b found received packet as valid
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
int nsim = 0;              /* number of messages from 5 to 4 so far */
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
