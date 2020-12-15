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


#ifndef __mpulse_rtable_h__
#define __mpulse_rtable_h__

#include <assert.h>
#include <sys/types.h>
#include <config.h>
#include <lib/bsd-list.h>
#include <scheduler.h>

#define CURRENT_TIME    Scheduler::instance().clock()
#define INFINITY2        0xff

#define NO_RESERV 0
#define RESERV 1


/*
  Route Table Entry
*/

class mpulse_rt_entry {
        friend class mpulse_rtable;
        friend class MPULSE;
 public:
        mpulse_rt_entry();
        ~mpulse_rt_entry();
	
 protected:
        LIST_ENTRY(mpulse_rt_entry) rt_link;

        nsaddr_t        rt_dst;
        u_int32_t       rt_seqno;		// to keep a fresh route
        u_int16_t       rt_hops;       		// hop count
        nsaddr_t        rt_nexthop;    		// next hop IP address
        u_int8_t        rt_flags;
};


/*
  The Routing Table
*/

class mpulse_rtable {
 public:
	mpulse_rtable() { LIST_INIT(&rthead); }

        mpulse_rt_entry*	head() { return rthead.lh_first; }

        mpulse_rt_entry*	rt_add(nsaddr_t id);
	mpulse_rt_entry*	rt_add(nsaddr_t dst, u_int32_t seqno, 
					u_int8_t hops, nsaddr_t src, 
					u_int8_t flag); 

        void			rt_delete(nsaddr_t id);
	void			rt_delete(mpulse_rt_entry *rt);
        mpulse_rt_entry*	rt_lookup(nsaddr_t id);

 private:
        LIST_HEAD(mpulse_rthead, mpulse_rt_entry) rthead;
};

#endif /* _mpulse__rtable_h__ */
