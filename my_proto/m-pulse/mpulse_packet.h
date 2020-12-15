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


#ifndef __mpulse_packet_h__
#define __mpulse_packet_h__

/* =====================================================================
   Packet Formats...
   ===================================================================== */
#define MPULSETYPE_PULSE  	0x01
#define MPULSETYPE_RESERV   0x02

/*
 * PULSE Routing Protocol Header Macros
 */
#define HDR_MPULSE(p)			((struct hdr_mpulse*)hdr_mpulse::access(p))
#define HDR_MPULSE_PULSE(p)  		((struct hdr_mpulse_pulse*)hdr_mpulse::access(p))
#define HDR_MPULSE_RESERV(p)		((struct hdr_mpulse_reserv*)hdr_mpulse::access(p))

struct list_node {
	nsaddr_t		address;
	struct list_node	*prev;
	struct list_node	*next;
};

/*
 * General MPULSE Header - shared by all formats
 */
struct hdr_mpulse {
        u_int8_t        mph_type;

	// Header access methods
	static int offset_; // required by PacketHeaderManager
	inline static int& offset() { return offset_; }
	inline static hdr_mpulse* access(const Packet* p) {
		return (hdr_mpulse*) p->access(offset_);
	}
};

struct hdr_mpulse_pulse {
        u_int8_t        pul_type;	// Packet Type
        u_int8_t        reserved;
        u_int8_t	pul_ttl;	// ttl
        u_int8_t        pul_metric;   // Hop Count

        u_int32_t       pul_seqno;   // Sequence Number
        double		pul_timestamp;   // accumulated delay time
        nsaddr_t        pul_src;     // Source Address (sink address)
 
  inline int size() { 
	int sz = 0;
  
  	sz = 4*sizeof(u_int8_t)		// pul_type + reserved + ttl + pul_metric
	     + sizeof(double)		// pul_timestamp
	     + sizeof(nsaddr_t)		// pul_src
	     + sizeof(u_int32_t);	// pul_seqno

  	assert (sz >= 0);
	return sz;
  }
};

struct hdr_mpulse_reserv {
        u_int8_t	res_type;        // Packet Type
        u_int16_t	res_length;	// total length of reservation packet
        u_int8_t	res_metric;	// Hop Count (node's distace from pulse source)

        nsaddr_t	   res_dst;         // source address of pulse packet (sink addresss)
        struct list_node  *res_src;         // Source Address (address of sending or forwarding node)
						
  inline int size() {
	int sz = 0;
	int cnt = 0;
	struct list_node *pnt = res_src;

	for( ; pnt != NULL; cnt++)
		pnt = pnt->next;

  	sz = 4*sizeof(u_int8_t)		// res_type + res_length + res_metric
	     + sizeof(nsaddr_t);	// res_dst

	// res_src
	if(cnt == 0)
		sz += sizeof(struct list_node*);
	else
		sz += cnt * sizeof(struct list_node);

  	assert (sz >= 0);
	return sz;
  }

  inline void add_srcNode(nsaddr_t addr) {
	struct list_node *src = new struct list_node;

	src->address = addr;
	src->next = 0;
	src->prev = 0;

	if(res_src != 0) {
		src->next = res_src;
		res_src->prev = src;
	}

	res_src = src;
  }
};

// for size calculation of header-space reservation
union hdr_all_mpulse {
  hdr_mpulse          	mphd;
  hdr_mpulse_pulse  	mppul;
  hdr_mpulse_reserv     mpres;
};

#endif /* __mpulse_packet_h__ */
