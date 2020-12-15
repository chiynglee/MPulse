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


#include "pulse_rtable.h"

/*
  The Routing Table
*/

pulse_rt_entry::pulse_rt_entry()
{

 rt_dst = 0;
 rt_seqno = 0;
 rt_hops = INFINITY2;
 rt_nexthop = 0;
 rt_flags = NO_RESERV;

};


pulse_rt_entry::~pulse_rt_entry()
{
}

/*
  The Routing Table
*/

pulse_rt_entry*
pulse_rtable::rt_lookup(nsaddr_t id)
{
pulse_rt_entry *rt = rthead.lh_first;

 for(; rt; rt = rt->rt_link.le_next) {
   if(rt->rt_dst == id)
     break;
 }
 return rt;

}

void
pulse_rtable::rt_delete(nsaddr_t id)
{
 pulse_rt_entry *rt = rt_lookup(id);

 if(rt) {
   LIST_REMOVE(rt, rt_link);
   delete rt;
 }

}

void
pulse_rtable::rt_delete(pulse_rt_entry* rt)
{
 if(rt) {
   LIST_REMOVE(rt, rt_link);
   delete rt;
 }
}

pulse_rt_entry*
pulse_rtable::rt_add(nsaddr_t id)
{
pulse_rt_entry *rt;

 assert(rt_lookup(id) == 0);
 rt = new pulse_rt_entry;
 assert(rt);
 rt->rt_dst = id;
 LIST_INSERT_HEAD(&rthead, rt, rt_link);
 return rt;
}

pulse_rt_entry*
pulse_rtable::rt_add(nsaddr_t dst, u_int32_t seqno, 
		      u_int8_t hops, nsaddr_t src, 
		      u_int8_t flag)
{
 pulse_rt_entry *rt;

 assert(rt_lookup(dst) == 0);
 rt = new pulse_rt_entry;
 assert(rt);

 rt->rt_dst = dst;
 rt->rt_seqno = seqno;
 rt->rt_hops = hops;
 rt->rt_nexthop = src;
 rt->rt_flags = flag;

 LIST_INSERT_HEAD(&rthead, rt, rt_link);

 return rt;
}

