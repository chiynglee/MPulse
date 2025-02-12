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

The AODV code developed by the CMU/MONARCH group was optimized and tuned by Samir Das and Mahesh Marina, University of Cincinnati. The work was partially done in Sun Microsystems.

*/

#ifndef __mpulse_h__
#define __mpulse_h__

#include <cmu-trace.h>
#include <priqueue.h>
#include "mpulse_rtable.h"
#include "mpulse_rqueue.h"
#include <classifier/classifier-port.h>

/*
  Causes AODV to apply a "smoothing" function to the link layer feedback
  that is generated by 802.11.  In essence, it requires that RT_MAX_ERROR
  errors occurs within a window of RT_MAX_ERROR_TIME before the link
  is considered bad.
*/

//#define DEBUG
#define MPULSE_LINK_LAYER_DETECTION

class MPULSE;

// Should be set by the user using best guess (conservative) 
#define NETWORK_DIAMETER    30   // 30 hops
#define INTER_HOP	3        // inter pulse propagation's hop count

#define NO_DELAY -1.0       // no delay 
#define ARP_DELAY 0.01      // fixed delay to keep arp happy
#define ARP_JITTER 0.01

#define CNT_WAKEUP_PERIOD	14	// the number of wakeup periods
#define CNT_WAKEUP_INTERVAL	16	// the number of wakeup intervals

/* Full Pulse Time */
#define SYNC_ERR		0.052 // 52 ms //0.05	// 5 ms
#define FLOOD_TIME		0.384 // 384 ms //0.450   // 450 ms //0.532   // 532 ms //0.22	// 220 ms
#define RESERVATION_TIME	0.100 // 100 ms //0.120 // 120 ms
#define PULSE_PERIOD		SYNC_ERR + FLOOD_TIME + RESERVATION_TIME	// 400 ms
/* Wake up Time */
#define WAKEUP_PERIOD		RESERVATION_TIME + SYNC_ERR	// 150 ms
/* Inter Pulse Time */
#define INTER_FLOOD_TIME	0.1		// 100 ms
#define INTER_PULSE_PERIOD	SYNC_ERR + INTER_FLOOD_TIME + RESERVATION_TIME	// 250 ms

/* interval */
#define WAKEUP_INTERVAL		5		// 5000 ms ( 5 s )
#define PULSE_INTERVAL		PULSE_PERIOD + (WAKEUP_PERIOD * CNT_WAKEUP_PERIOD) \
				 + (CNT_WAKEUP_INTERVAL * WAKEUP_INTERVAL) \
				 + INTER_PULSE_PERIOD	// 82720 ms (83 seconds)
#define INTER_PULSE_INTERVAL	PULSE_INTERVAL - INTER_PULSE_PERIOD		// 80720 ms

/* Pulse Flood Delay Time */
#define FLOOD_JITTER		0.01		// 10 ms (retransmission jitter)
#define FLOOD_DELAY		0.02		// 20 ms (retransmission delay)

#define NO_PKT		0
#define HAVE_PKT	1

// value of timeFlag
#define TIME_INIT 0
#define TIME_SYNC 1
#define TIME_FLOOD 2
#define TIME_RES 3
#define TIME_SLEEP 4
#define TIME_INTER_FLOOD 5

#define SINK_ID 0

// energy model
#define MP_SLEEP 1
#define MP_IDLE 0


class MPULSETimer : public Handler {
       friend class MPULSE;
public:
       MPULSETimer(MPULSE* a) : agent(a) 
             { cnt_sleep = 0; timeFlag = 0; }
       void	handle(Event*);
protected:
       int timeFlag;
       int cnt_sleep;
private:
       MPULSE  *agent;
       Event	intr;
};


/*
  The Routing Agent
*/
class MPULSE: public Agent {

  /*
   * make some friends first 
   */
        friend class mpulse_rt_entry;
        friend class MPULSETimer;

 public:
        MPULSE(nsaddr_t id);

        void		recv(Packet *p, Handler *);
        void		mpulse_init();

		void        Sleep();
		void        WakeUp();

 protected:
        int             command(int, const char *const *);
        int             initialized() { return 1 && target_; }
	EnergyModel*    energy() { return node_info->energy_model(); }

        /*
         * Route Table Management
         */
        void            rt_expired();
        void            rt_initRes();
        void            rt_resolve(Packet *p);
        void            rt_update(mpulse_rt_entry *rt, u_int32_t seqnum,
		     	  	u_int16_t metric, nsaddr_t nexthop,
		      		u_int8_t flags);
 public:
        void            rt_ll_failed(Packet *p);
 protected:
        void            enque(mpulse_rt_entry *rt, Packet *p);
        Packet*         deque(mpulse_rt_entry *rt);

        /*
         * Packet TX Routines
         */
        void            forward(mpulse_rt_entry *rt, Packet *p, double delay);
        void            sendPulse();
        void            sendReservation();
        void            sendData();

        /*
         * Packet RX Routines
         */
        void            recvMPULSE(Packet *p);
        void            recvPulsePacket(Packet *p);
        void            recvReservPacket(Packet *p);

	/*
	 * delay management
	 */
        double          RetransmitDelay(double delay_time);

	/*
	 * address list structure management
	 */
        void         insert_list(struct list_node *ln, nsaddr_t addr);
        int          find_list(struct list_node *ln, nsaddr_t addr);
        void         delete_list(struct list_node *ln, nsaddr_t addr);
        void         delete_list_all(struct list_node *ln);

        nsaddr_t        index;                  // IP Address of this node
        u_int32_t       seqno;                  // Sequence Number

        mpulse_rtable         rthead;                 // routing table

        /*
         * Timer
         */
        MPULSETimer	mptimer;

        /*
         * Routing Table
         */
        mpulse_rtable          rtable;
        /*
         *  A "drop-front" queue used by the routing layer to buffer
         *  packets to which it does not have a route.
         */
        mpulse_rqueue         rqueue;

        /*
         * A mechanism for logging the contents of the routing
         * table.
         */
        Trace           *logtarget;

        /*
         * A pointer to the network interface queue that sits
         * between the "classifier" and the "link layer".
         */
        PriQueue        *ifqueue;

        /*
         * Logging stuff
         */
        void            log_link_del(nsaddr_t dst);
        void            log_link_broke(Packet *p);
        void            log_link_kept(nsaddr_t dst);

        /* for passing packets up to agents */
        PortClassifier *dmux_;

        int recvPulsePKT;	// received pulse flood
        int recvResPKT;		// received reservation

		Node *node_info;

		int node_state;
};

#endif /* _mpulse_h__ */
