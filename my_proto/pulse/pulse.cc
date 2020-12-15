/*
Copyright (c) 1997, 1998 Carnegie Mellon University.  All Rights
Reserved. 

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.
3. The name of the author may not be used to endorse or promote products
derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

The AODV code developed by the CMU/MONARCH group was optimized and tuned by Samir Das and Mahesh Marina, University of Cincinnati. The work was partially done in Sun Microsystems. Modified for gratuitous replies by Anant Utgikar, 09/16/02.

*/

#include "pulse.h"
#include "pulse_packet.h"
#include <random.h>
#include <cmu-trace.h>

#define max(a,b)        ( (a) > (b) ? (a) : (b) )
#define CURRENT_TIME    Scheduler::instance().clock()

#ifdef DEBUG
static int num_pulse = 0;
#endif


/*
  TCL Hooks
*/
int hdr_pulse::offset_;
static class PULSEHeaderClass : public PacketHeaderClass {
public:
        PULSEHeaderClass() : PacketHeaderClass("PacketHeader/PULSE",
                                              sizeof(hdr_all_pulse)) {
	  bind_offset(&hdr_pulse::offset_);
	} 
} class_rtProtoPULSE_hdr;

static class PULSEclass : public TclClass {
public:
        PULSEclass() : TclClass("Agent/PULSE") {}
        TclObject* create(int argc, const char*const* argv) {
          assert(argc == 5);
	  return (new PULSE((nsaddr_t) Address::instance().str2addr(argv[4])));
        }
} class_rtProtoPULSE;

int
PULSE::command(int argc, const char*const* argv) {
  Tcl& tcl = Tcl::instance();

  if(argc == 2) {
    if(strncasecmp(argv[1], "id", 2) == 0) {
      tcl.resultf("%d", index);
      return TCL_OK;
    }
    
    if(strncasecmp(argv[1], "start-pulse", 2) == 0) {
      ptimer.handle((Event*) 0);
      return TCL_OK;
     }               
  }
  else if(argc == 3) {
    if(strcmp(argv[1], "index") == 0) {
      index = atoi(argv[2]);
      return TCL_OK;
    }

    else if(strcmp(argv[1], "log-target") == 0 || strcmp(argv[1], "tracetarget") == 0) {
      logtarget = (Trace*) TclObject::lookup(argv[2]);
      if(logtarget == 0)
	return TCL_ERROR;
      return TCL_OK;
    }
    else if(strcmp(argv[1], "drop-target") == 0) {
    int stat = rqueue.command(argc,argv);
      if (stat != TCL_OK) return stat;
      return Agent::command(argc, argv);
    }
    else if(strcmp(argv[1], "if-queue") == 0) {
    ifqueue = (PriQueue*) TclObject::lookup(argv[2]);
      
      if(ifqueue == 0)
	return TCL_ERROR;
      return TCL_OK;
    }
    else if (strcmp(argv[1], "port-dmux") == 0) {
      dmux_ = (PortClassifier *)TclObject::lookup(argv[2]);
	  if (dmux_ == 0) {
		fprintf (stderr, "%s: %s lookup of %s failed\n", __FILE__,
		argv[1], argv[2]);
		return TCL_ERROR;
	  }
	  return TCL_OK;
    }
	else if (strcmp(argv[1], "pulse-node") == 0) {
	  node_info = (Node *)tcl.lookup(argv[2]);
	  return TCL_OK;
	}
  }
  return Agent::command(argc, argv);
}


/* 
   Constructor
*/
PULSE::PULSE(nsaddr_t id) : Agent(PT_PULSE),
			  ptimer(this), rqueue() {
  index = id;
  seqno = 0;

  logtarget = 0;
  ifqueue = 0;

  recvPulsePKT = NO_PKT;   // 1 : received at least one pulse 
  recvResPKT = NO_PKT;     // 1 : received at least one reservation

  node_info = 0;
  node_state = P_IDLE;
}

/*
  Timer
*/
void
PULSETimer::handle(Event*) {
#ifdef DEBUG
 fprintf(stderr, "Timer %d expired at node %d.\n", timeFlag, agent->index);
#endif

// pulse flood period
  switch(timeFlag) {
  case TIME_INIT : 
	timeFlag = TIME_SYNC;	// sync error
	agent->WakeUp();
	agent->pulse_init();
	Scheduler::instance().schedule(this, &intr, SYNC_ERR);
	break;
  case TIME_SYNC : 
 	if(cnt_sleep == 0 || cnt_sleep >= CNT_WAKEUP_INTERVAL) {
		timeFlag = TIME_FLOOD;	// flood propagation
		if(agent->index == SINK_ID)
			agent->sendPulse();
		Scheduler::instance().schedule(this, &intr, FLOOD_TIME);
	}
	else {
		timeFlag = TIME_RES;	// reservation in wakeup period
		if(agent->index != SINK_ID && !(agent->rqueue.empty())) {
			agent->sendReservation();
		}
		Scheduler::instance().schedule(this, &intr, RESERVATION_TIME);
	}
	break;
  case TIME_FLOOD :
	timeFlag = TIME_RES;	// reservation after flood & inter flood
	if(agent->index != SINK_ID && !(agent->rqueue.empty())) {
		agent->sendReservation();
	}
	Scheduler::instance().schedule(this, &intr, RESERVATION_TIME);
	break;
  case TIME_RES :
	timeFlag = TIME_SLEEP;	// sleep & data send
	cnt_sleep += 1;
	if(agent->index != SINK_ID) {
		if(agent->recvResPKT == NO_PKT) { // go to sleep
			agent->Sleep();
		}
		else if(!(agent->rqueue.empty())) {
			agent->sendData();
		}
	}
	Scheduler::instance().schedule(this, &intr, WAKEUP_INTERVAL);
	break;
  case TIME_SLEEP :
  	agent->WakeUp();
	if(cnt_sleep >= CNT_WAKEUP_INTERVAL) {
		agent->pulse_init();
		cnt_sleep = 0;
	}
	else
		agent->rt_initRes();
	timeFlag = TIME_SYNC;	// sync error after sleep
	Scheduler::instance().schedule(this, &intr, SYNC_ERR);
	break;
  }
}


// pulse control variable initalize
void 
PULSE::pulse_init() {
  recvPulsePKT = NO_PKT;
  recvResPKT = NO_PKT;
  rt_expired();
}

/*
  Retransmission time
*/
double 
PULSE::RetransmitDelay(double delay_time) {
  double jitter = 0;
  jitter = (FLOOD_JITTER * Random::uniform()) + delay_time;
  return jitter;
}


void 
PULSE::WakeUp()
{
 node_state = P_IDLE;
 node_info->energy_model()->set_node_sleep(P_IDLE);
}

void 
PULSE::Sleep()
{
 node_state = P_SLEEP;
 node_info->energy_model()->set_node_sleep(P_SLEEP);
}

/*
  Link Failure Management Functions
*/
static void
pulse_rt_failed_callback(Packet *p, void *arg) {
  ((PULSE*) arg)->rt_ll_failed(p);
}

/*
 * This routine is invoked when the link-layer reports a route failed.
 */
void
PULSE::rt_ll_failed(Packet *p) {
struct hdr_cmn *ch = HDR_CMN(p);
struct hdr_ip *ih = HDR_IP(p);
pulse_rt_entry *rt;
//nsaddr_t broken_hop = ch->next_hop_;

#ifndef PULSE_LINK_LAYER_DETECTION
 drop(p, DROP_RTR_MAC_CALLBACK);
#else 

 /*
  * Non-data packets and Broadcast Packets can be dropped.
  */
  if(! DATA_PACKET(ch->ptype()) ||
     (u_int32_t) ih->daddr() == IP_BROADCAST) {
    drop(p, DROP_RTR_MAC_CALLBACK);
    return;
  }
  log_link_broke(p);
  if((rt = rtable.rt_lookup(ih->daddr())) == 0) {
    drop(p, DROP_RTR_MAC_CALLBACK);
    return;
  }
  log_link_del(ch->next_hop_);

/*  if(ch->num_forwards() < rt->rt_hops) {
    if(ptimer.timeFlag == TIME_SLEEP && rt->rt_flags == RESERV) {
      forward(rt, p, ARP_DELAY * Random::uniform());
      return;
    }
  }
*/
  drop(p, DROP_RTR_MAC_CALLBACK);
#endif // LINK LAYER DETECTION
}

void
PULSE::rt_update(pulse_rt_entry *rt, u_int32_t seqnum, u_int16_t metric,
	       	nsaddr_t nexthop, u_int8_t flags) {
 rt->rt_seqno = seqnum;
 rt->rt_hops = metric;
 rt->rt_nexthop = nexthop;
 if(flags == 0 || flags == 1)
   rt->rt_flags = flags;
}

/* when the time reaches pulse period,
   routing table will be cleaned. */
void PULSE::rt_expired() {
  pulse_rt_entry *rt, *rtn;

  for(rt = rtable.head(); rt; rt = rtn) {
    rtn = rt->rt_link.le_next;
    rtable.rt_delete(rt);
  }
}

void PULSE::rt_initRes() {
 pulse_rt_entry *rt, *rtn;

 for(rt = rtable.head(); rt; rt = rtn) {
   rtn = rt->rt_link.le_next;
   rt->rt_flags = NO_RESERV;
 }
 
 recvResPKT = NO_PKT;
}

/*
  Route Handling Functions
  ( when a data packet is received )
*/
void
PULSE::rt_resolve(Packet *p) {
struct hdr_cmn *ch = HDR_CMN(p);
struct hdr_ip *ih = HDR_IP(p);
pulse_rt_entry *rt;

#ifdef DEBUG
   fprintf(stderr, "%s: Receive packet (node %d)\n", __FUNCTION__, index);
#endif

 //  Set the transmit failure callback.  That
 //  won't change.
 ch->xmit_failure_ = pulse_rt_failed_callback;
 ch->xmit_failure_data_ = (void*) this;

 rt = rtable.rt_lookup(ih->daddr());
/*
 if(rt == 0) {
   #ifdef DEBUG
   fprintf(stderr, "%s - ( %d ): I don't know where is a destination.",
                                __FUNCTION__, index);
   #endif
   if(ih->saddr() == index)
     rqueue.enque(p);
   else
     Packet::free(p);
   return;
 }

 if( ptimer.timeFlag == TIME_SLEEP && rt->rt_flags == RESERV) {
   if(node_state == P_SLEEP) {
     #ifdef DEBUG
     fprintf(stderr, "%s: I am sleeping. (node %d)\n", __FUNCTION__, index);
     #endif

     Packet::free(p);
     return;
   }
   
//   forward(rt, p, NO_DELAY);
   rqueue.enque(p);
   sendData();
 }
 else {
   if(ih->saddr() == index) {
     #ifdef DEBUG
     fprintf(stderr, "%lf: %s: Enqueue a packet (node %d)(timer = %d, flag = %d)\n", 
                             CURRENT_TIME, __FUNCTION__, index, ptimer.timeFlag, rt->rt_flags);
     #endif
     rqueue.enque(p);
   }
   else {
     Packet::free(p);
   }
 }
*/
 #ifdef DEBUG
 fprintf(stderr, "%lf: %s: Enqueue a packet (node %d)(timer = %d)\n", 
                         CURRENT_TIME, __FUNCTION__, index, ptimer.timeFlag);
 #endif
 rqueue.enque(p);

 if(ptimer.timeFlag == TIME_SLEEP && node_state == P_IDLE) {
   if(rt != 0 && rt->rt_flags == RESERV)
     sendData();
 }

 #ifdef DEBUG
 if(rt == 0) {
   fprintf(stderr, "%s - ( %d ): I don't know where is a destination.",
                                __FUNCTION__, index);
   return ;
 }
 else if(node_state == P_SLEEP) {
   fprintf(stderr, "%s: I am sleeping. (node %d)\n", __FUNCTION__, index);
   return ;
 }
 else {
   fprintf(stderr, "%s: %d is not Data sending time.\n", __FUNCTION__, index);
   return ;
 }
 #endif
}

/*
  Packet Reception Routines
*/

void
PULSE::recv(Packet *p, Handler*) {
struct hdr_cmn *ch = HDR_CMN(p);
struct hdr_ip *ih = HDR_IP(p);

 assert(initialized());
 // XXXXX NOTE: use of incoming flag has been depracated; In order to track direction of pkt flow, direction_ in hdr_cmn is used instead. see packet.h for details.

 if(ch->ptype() == PT_PULSE) {
   ih->ttl_ -= 1;
   recvPULSE(p);
   return;
 }

 /*
  *  Must be a packet I'm originating...
  */
 if((ih->saddr() == index) && (ch->num_forwards() == 0)) {
 /*
  * Add the IP Header
  */
   // Added by Parag Dadhania && John Novatnack to handle broadcasting
   if ( (u_int32_t)ih->daddr() != IP_BROADCAST)
     ih->ttl_ = NETWORK_DIAMETER;
 }
 /*
  *  I received a packet that I sent.  Probably
  *  a routing loop.
  */
 else if(ih->saddr() == index) {
   drop(p, DROP_RTR_ROUTE_LOOP);
   return;
 }
 /*
  *  Packet I'm forwarding...
  */
 else {
 /*
  *  Check the TTL.  If it is zero, then discard.
  */
   if(--ih->ttl_ == 0) {
     drop(p, DROP_RTR_TTL);
     return;
   }
 }
// Added by Parag Dadhania && John Novatnack to handle broadcasting
 if ( (u_int32_t)ih->daddr() != IP_BROADCAST)
   rt_resolve(p);
 else
   forward((pulse_rt_entry*) 0, p, NO_DELAY);
}


void
PULSE::recvPULSE(Packet *p) {
 struct hdr_pulse *ph = HDR_PULSE(p);

 assert(HDR_IP (p)->sport() == RT_PORT);
 assert(HDR_IP (p)->dport() == RT_PORT);

 /*
  * Incoming Packets.
  */
 switch(ph->ph_type) {

 case PULSETYPE_PULSE:
   recvPulsePacket(p);
   break;

 case PULSETYPE_RESERV:
   recvReservPacket(p);
   break;
        
 default:
   fprintf(stderr, "Invalid PULSE type (%x)\n", ph->ph_type);
   exit(1);
 }
}


void
PULSE::recvPulsePacket(Packet *p) {
struct hdr_ip *ih = HDR_IP(p);
struct hdr_pulse_pulse *pul = HDR_PULSE_PULSE(p);

  /*
   * Drop if:
   *      - no flood propagation time
   *      - I'm the source
   *      - I'm the sink
   */

  if(ptimer.timeFlag != TIME_FLOOD) {
#ifdef DEBUG
    fprintf(stderr, "(%d) - %s: Not Flood Propagation Time\n", index, __FUNCTION__);
#endif
    Packet::free(p);
    return;
  }

  if(ih->saddr() == index) {
#ifdef DEBUG
    fprintf(stderr, "%s: got my own PULSE\n", __FUNCTION__);
#endif // DEBUG
    Packet::free(p);
    return;
  }

  if(index == SINK_ID) {
#ifdef DEBUG
    fprintf(stderr, "(%d) - %s: I am the sink node\n", index, __FUNCTION__);
#endif
    Packet::free(p);
    return;
  }

#ifdef DEBUG
  fprintf(stderr, "%s: node %d received from node %d.\n", 
                               __FUNCTION__, index, ih->saddr());
#endif

 /* 
  * We are either going to forward the REQUEST or generate a
  * REPLY. Before we do anything, we make sure that the REVERSE
  * route is in the route table.
  */

  pulse_rt_entry *rt0; // rt0 is the reverse route 
   
  rt0 = rtable.rt_lookup(pul->pul_src);
  if(rt0 == 0) { /* if not in the route table */
  // create an entry for the reverse route.
    rt0 = rtable.rt_add(pul->pul_src);
  }
  
  if ( (pul->pul_seqno > rt0->rt_seqno) ||
  	((pul->pul_seqno == rt0->rt_seqno) && 
  	(pul->pul_metric < rt0->rt_hops)) ) {
  // If we have a fresher seq no. or lesser #hops for the 
  // same seq no., update the rt entry. Else don't bother.
    rt_update(rt0, pul->pul_seqno, pul->pul_metric, ih->saddr(), NO_RESERV);
    recvPulsePKT = HAVE_PKT;

    if(ptimer.timeFlag == TIME_FLOOD) {
      ih->saddr() = index;
      ih->daddr() = IP_BROADCAST;
      pul->pul_metric += 1;

      double delay = pul->pul_timestamp;
      pul->pul_timestamp = RetransmitDelay(delay);

      #ifdef DEBUG
      fprintf(stderr, "Pulse Flood packet is forwarded from node %d.\n", index);
      #endif   // DEBUG

      forward((pulse_rt_entry*) 0, p, FLOOD_JITTER * Random::uniform());
    }
    else {
      if(ptimer.timeFlag == TIME_RES && !rqueue.empty()) {
        sendReservation();
      }
      Packet::free(p);
    }
  }
  else {
    #ifdef DEBUG
    fprintf(stderr, "%s: discarding pulse.(node=%d)\n", __FUNCTION__, index);
    #endif // DEBUG
    Packet::free(p);
    return;
  }
}

void
PULSE::recvReservPacket(Packet *p) {
#ifdef DEBUG
struct hdr_cmn *ch = HDR_CMN(p);
#endif

struct hdr_ip *ih = HDR_IP(p);
struct hdr_pulse_reserv *res = HDR_PULSE_RESERV(p);
pulse_rt_entry *rt, *rt0;
	
 if(ih->saddr() == index) {
   #ifdef DEBUG
   fprintf(stderr, "%s: Got my own reservation.\n", __FUNCTION__);
   #endif
   Packet::free(p);
   return;
 }

 // not reservation time
 if(ptimer.timeFlag != TIME_RES) {
   #ifdef DEBUG
   fprintf(stderr, "%s: The Time is not Reservation time.\n", __FUNCTION__);
   #endif
   Packet::free(p);
   return;
 }

 rt = rtable.rt_lookup(res->res_dst);	// search route
 if(rt == 0) {
   #ifdef DEBUG
   fprintf(stderr, "%s: Does not receive Pulse Flood.\n", __FUNCTION__);
   #endif
   Packet::free(p);
   return;
 }

#ifdef DEBUG
 fprintf(stderr, "(%d) - %s: received a Reservation from (%d)\n", 
                                  index, __FUNCTION__, ch->prev_hop_);
#endif // DEBUG

 /*
  * Add a forward route table entry. 
  */
 if ( rt->rt_flags == NO_RESERV ) {
   rt->rt_flags = RESERV;

   if(recvResPKT == NO_PKT)
     recvResPKT = HAVE_PKT;

   struct list_node *ln = res->res_src;
   if (ih->daddr() == index) { // If I am the original source
				               // add all reverse route
     while(ln != 0) {
       if( (rt0 = rtable.rt_lookup(ln->address)) == 0 ) {
          rt0 = rtable.rt_add(ln->address);
       }
       rt_update(rt0, rt->rt_seqno, res->res_metric, ln->address, RESERV);
       ln = ln->next;
     }

     Packet::free(p);
     return;
   }

   // If I am a sensor node, then add a reverse route.
   if( (rt0 = rtable.rt_lookup(ln->address)) == 0 ) {
     rt0 = rtable.rt_add(ln->address);
   }
   rt_update(rt0, rt->rt_seqno, res->res_metric, ln->address, RESERV);

   if(ptimer.timeFlag == TIME_RES) {	// forward for reservation time
     pulse_rt_entry *rt_route = rtable.rt_lookup(ih->daddr()); 
     // If the rt is up, forward
     if(rt_route && (rt_route->rt_hops != INFINITY2)) {
        assert (rt_route->rt_flags == RESERV);

        res->res_metric -= 1;
        res->add_srcNode(index);
        res->res_length = res->size();

        #ifdef DEBUG
        fprintf(stderr, "%s: forwarding Reseration from (%d) to (%d).\n",
                                       __FUNCTION__, index, rt_route->rt_nexthop);
        #endif // DEBUG

        forward(rt_route, p, NO_DELAY);
     }
     else {
        #ifdef DEBUG
        fprintf(stderr, "%s: I don't know a route.\n", __FUNCTION__);
        #endif
        Packet::free(p);
        return;
     }
   }
   else {	// not reservation time
     if(ptimer.timeFlag == TIME_SLEEP && 
          !rqueue.empty() && 
          recvResPKT == HAVE_PKT) {
       sendData();
     }
     Packet::free(p); // reservation pkt drop
   }
 }
 else {	// if reservation pkt is old, then drop.
   #ifdef DEBUG
   fprintf(stderr, "%s: reservation pkt is old.\n", __FUNCTION__);
   #endif
   Packet::free(p);
 }
}

/*
   Packet Transmission Routines
*/

// In the reveive queue, packets that don't exist destination in routing table must be drpped.
void
PULSE::forward(pulse_rt_entry *rt, Packet *p, double delay) {
struct hdr_cmn *ch = HDR_CMN(p);
struct hdr_ip *ih = HDR_IP(p);

 if(ih->ttl_ == 0) {
#ifdef DEBUG
  fprintf(stderr, "%s: calling drop\n", __PRETTY_FUNCTION__);
#endif // DEBUG
  drop(p, DROP_RTR_TTL);
  return;
 }

 if(node_state == P_SLEEP) {
      #ifdef DEBUG
     fprintf(stderr, "%s: I am sleeping. (node %d)\n", __FUNCTION__, index);
     #endif

     Packet::free(p);
     return;
 }

 if (ch->ptype() != PT_PULSE && ch->direction() == hdr_cmn::UP &&
	((u_int32_t)ih->daddr() == IP_BROADCAST)
		|| (ih->daddr() == here_.addr_)) {
	dmux_->recv(p,0);
	return;
 }

 if (rt) {
   assert(rt->rt_flags == RESERV);

   #ifdef DEBUG
   fprintf(stderr, "%s: changing next hop to (%d)\n", __FUNCTION__, rt->rt_nexthop);
   #endif // DEBUG

   ch->next_hop_ = rt->rt_nexthop;
   ch->addr_type() = NS_AF_INET;
   ch->direction() = hdr_cmn::DOWN;       //important: change the packet's direction
 }
 else { // if it is a broadcast packet
   assert(ih->daddr() == (nsaddr_t) IP_BROADCAST);
   ch->addr_type() = NS_AF_NONE;
   ch->direction() = hdr_cmn::DOWN;       //important: change the packet's direction
 }

 if(delay > 0.0) {
   Scheduler::instance().schedule(target_, p, delay);
 }
 else {
 // Not a broadcast packet, no delay, send immediately
   Scheduler::instance().schedule(target_, p, 0.);
 }
}


void
PULSE::sendPulse() {
// Allocate a Pulse Flood packet 
Packet *p = Packet::alloc();
struct hdr_cmn *ch = HDR_CMN(p);
struct hdr_ip *ih = HDR_IP(p);
struct hdr_pulse_pulse *pul = HDR_PULSE_PULSE(p);

 if(ptimer.timeFlag != TIME_FLOOD) {
#ifdef DEBUG
   fprintf(stderr, "%s: Time is not Flood time.\n", __FUNCTION__);
#endif
   Packet::free(p);
   return;
 }

#ifdef DEBUG
  fprintf(stderr, "(%2d) - %2d sending Pulse Flood.\n", ++num_pulse, index);
#endif // DEBUG

 ch->ptype() = PT_PULSE;
 ch->size() = IP_HDR_LEN + pul->size();
 ch->iface() = -2;
 ch->error() = 0;
 ch->addr_type() = NS_AF_NONE;
 ch->prev_hop_ = index;

 ih->ttl_ = NETWORK_DIAMETER;
 ih->saddr() = index;
 ih->daddr() = IP_BROADCAST;
 ih->sport() = RT_PORT;
 ih->dport() = RT_PORT;

 // Fill up pulse flood packet fields. 
 pul->pul_type = PULSETYPE_PULSE;
 pul->pul_metric = 1;
 pul->pul_seqno = ++seqno;
 pul->pul_timestamp = FLOOD_DELAY;
 pul->pul_src = index;

 Scheduler::instance().schedule(target_, p, 0.);
}

void
PULSE::sendReservation() {
Packet *p = Packet::alloc();
struct hdr_cmn *ch = HDR_CMN(p);
struct hdr_ip *ih = HDR_IP(p);
struct hdr_pulse_reserv *res = HDR_PULSE_RESERV(p);
pulse_rt_entry *rt = rtable.head();
struct list_node *sentBefore = 0;      // sent already to this address
double delay = 0.0;

 if(ptimer.timeFlag != TIME_RES) {
#ifdef DEBUG
   fprintf(stderr, "%s: Time is not Reservation time.\n", __FUNCTION__);
#endif
   Packet::free(p);
   return;
 }

 if(recvPulsePKT == NO_PKT) {
#ifdef DEBUG
   fprintf(stderr, "%s: Not received Pulse Flood Packet.\n", __FUNCTION__);
#endif
   Packet::free(p);
   return;
 }

 if(rqueue.empty()) {
#ifdef DEBUG
   fprintf(stderr, "%s: No data sending.\n", __FUNCTION__);
#endif
   Packet::free(p);
   return;
 }

 for( ; rt; rt = rt->rt_link.le_next) {
   // if res pkt is not sent yet and data is ready, then send a res pkt.
   if(!(find_list(sentBefore, rt->rt_dst)) && (rqueue.find(rt->rt_dst))) {
     // saving a already sent address
     insert_list(sentBefore, rt->rt_dst);

     #ifdef DEBUG
     fprintf(stderr, "sending Reservation from (%d) to (%d) at %.2f\n", 
                              index, rt->rt_nexthop, Scheduler::instance().clock());
     #endif // DEBUG

     assert(rt);

     rt->rt_flags = RESERV;
     recvResPKT = HAVE_PKT;

     res->res_type = PULSETYPE_RESERV;
     res->res_metric = rt->rt_hops - 1;
     res->res_dst = rt->rt_dst;
     res->res_src = 0;
     res->add_srcNode(index);
     res->res_length = res->size();

     ch->ptype() = PT_PULSE;
     ch->size() = IP_HDR_LEN + res->size();
     ch->iface() = -2;
     ch->error() = 0;
     ch->addr_type() = NS_AF_INET;
     ch->next_hop_ = rt->rt_nexthop;
     ch->prev_hop_ = index;
     ch->direction() = hdr_cmn::DOWN;

     ih->saddr() = index;
     ih->daddr() = rt->rt_dst;
     ih->sport() = RT_PORT;
     ih->dport() = RT_PORT;
     ih->ttl_ = NETWORK_DIAMETER;

     delay = RetransmitDelay(0.0);
     Scheduler::instance().schedule(target_, p, delay);
   }
 }

 if(sentBefore != 0) {
   delete_list_all(sentBefore);	// delete sent addresses
 }
}

void
PULSE::sendData() {
Packet *buf_pkt;
double delay = 0.0;
pulse_rt_entry *rt = rtable.head();

 if(ptimer.timeFlag != TIME_SLEEP) {
#ifdef DEBUG
   fprintf(stderr, "%s: The time is not wake up interval (sleep).\n", __FUNCTION__);
#endif
 return;
 }

 if(recvResPKT == NO_PKT) {
#ifdef DEBUG
   fprintf(stderr, "%s: Not received Reservation packet.\n", __FUNCTION__);
#endif
   return;
 }

 if(rqueue.empty()) {
#ifdef DEBUG
   fprintf(stderr, "%s: No data packet.\n", __FUNCTION__);
#endif
   return;
 }

 for( ; rt; rt = rt->rt_link.le_next) {
   while((buf_pkt = rqueue.deque(rt->rt_dst))) {
     if(rt->rt_hops != INFINITY2 && rt->rt_flags == RESERV) {
       assert (rt->rt_flags == RESERV);
       forward(rt, buf_pkt, delay);
       delay += (ARP_JITTER * Random::uniform()) + ARP_DELAY;
       buf_pkt = 0;	// pointer initalize
     }
   }
 }
}

/*
 * address list structure management
 */
void
PULSE::insert_list(struct list_node *ln, nsaddr_t addr) {
 struct list_node *node = new struct list_node;

 node->address = addr;
 node->next = 0;
 node->prev = 0;

 if(ln != 0) {
   node->next = ln;
   ln->prev = node;
 }

 ln = node;
}

int
PULSE::find_list(struct list_node *ln, nsaddr_t addr) {
 struct list_node *pnt = ln;

 if(pnt == 0)
   return 0;

 for( ; pnt; pnt = pnt->next) {
   if(pnt->address == addr)
     return 1;
 }

 return 0;
}

void
PULSE::delete_list(struct list_node *ln, nsaddr_t addr) {
 struct list_node *pnt = ln;
 struct list_node *tmp = 0;

 while(pnt != 0) {
   if(pnt->address == addr) {	// delete at the head
     if(pnt->prev == 0) {
       ln = pnt->next;
       ln->prev = 0;
       delete pnt;
       pnt = ln;
     }
     else if(pnt->next == 0) {	// delete at the tail
       pnt->prev->next = 0;
       delete pnt;
       pnt = 0;
     }
     else {	// delete at the middle
       tmp = pnt->next;
       pnt->prev->next = pnt->next;
       pnt->next->prev = pnt->prev;
       delete pnt;
       pnt = tmp;
     }
   }
   else {
     pnt = pnt->next;
   }
 }
}

void
PULSE::delete_list_all(struct list_node *ln) {
 struct list_node *pnt = ln;

 while(pnt != 0) {
   ln = pnt->next;
   ln->prev = 0;
   delete pnt;
   pnt = ln;
 }
}

