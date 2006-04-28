/*
 * This file is part of libtrace
 *
 * Copyright (c) 2004 The University of Waikato, Hamilton, New Zealand.
 * Authors: Daniel Lawson 
 *          Perry Lorier 
 *          
 * All rights reserved.
 *
 * This code has been developed by the University of Waikato WAND 
 * research group. For further information please see http://www.wand.net.nz/
 *
 * libtrace is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * libtrace is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with libtrace; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * $Id$
 *
 */

#ifndef LIBTRACE_H
#define LIBTRACE_H

/** @file
 *
 * @brief Trace file processing library header
 *
 * @author Daniel Lawson
 * @author Perry Lorier
 *
 * @version $Id$
 *
 * This library provides a per packet interface into a trace file, or a live 
 * captures.  It supports ERF, DAG cards, WAG cards, WAG's event format,
 * pcap etc.
 *
 * @par Usage
 * See the example/ directory in the source distribution for some simple examples
 * @par Linking
 * To use this library you need to link against libtrace by passing -ltrace
 * to your linker. You may also need to link against a version of libpcap 
 * and of zlib which are compiled for largefile support (if you wish to access
 * traces larger than 2 GB). This is left as an exercise for the reader. Debian
 * Woody, at least, does not support large file offsets.
 *
 */

#include <sys/types.h>
#ifdef _MSC_VER
    /* define the following from MSVC's internal types */
    typedef		__int8	int8_t;
    typedef		__int16	int16_t;
    typedef		__int32	int32_t;
    typedef		__int64	int64_t;
    typedef unsigned 	__int8	uint8_t;
    typedef unsigned	__int16 uint16_t;
    typedef unsigned	__int32 uint32_t;
    typedef unsigned	__int64 uint64_t;
	#ifdef BUILDING_DLL
		#define DLLEXPORT __declspec(dllexport)
	#else
		#define DLLEXPORT __declspec(dllimport)
	#endif
	#define DLLLOCAL
#else
	#ifdef HAVE_GCCVISIBILITYPATCH
		#define DLLEXPORT __attribute__ (visibility("default"))
		#define DLLLOCAL __attribute__ (visibility("hidden"))
	#else
		#define DLLEXPORT
		#define DLLLOCAL
	#endif
#endif

#ifdef WIN32
#   include <winsock2.h>
#   include <ws2tcpip.h>
    typedef short sa_family_t;
    /* Make up for a lack of stdbool.h */
#    define bool signed char
#    define false 0
#    define true 1
#    if !defined(ssize_t)
     /* XXX: Not 64-bit safe! */
#    define ssize_t int
#    endif    
#else
#    include <netinet/in.h>
#    include <stdbool.h>
#endif

/** API version as 2 byte hex digits, eg 0xXXYYZZ */
#define LIBTRACE_API_VERSION 0x030000  /* 3.0.00 */

#ifdef __cplusplus 
extern "C" { 
#endif

/* Function does not depend on anything but its
 * parameters, used to hint gcc's optimisations
 */
#if __GNUC__ >= 3 
#  define SIMPLE_FUNCTION __attribute__((pure))
#  define UNUSED __attribute__((unused))
#  define PACKED __attribute__((packed))
#  define CONSTRUCTOR __attribute__((constructor))
#else
#  define SIMPLE_FUNCTION
#  define UNUSED
#  define PACKED __declspec(align(1))
#  define CONSTRUCTOR
#endif
	
/** Opaque structure holding information about an output trace */
typedef struct libtrace_out_t libtrace_out_t;
	
/** Opaque structure holding information about a trace */
typedef struct libtrace_t libtrace_t;
        
/** Opaque structure holding information about a bpf filter */
typedef struct libtrace_filter_t libtrace_filter_t;

/** if a packet has memory allocated
 * If the packet has allocated it's own memory it's buffer_control should
 * be TRACE_CTRL_PACKET, when the packet is destroyed it's memory will be
 * free()'d.  If it's doing zerocopy out of memory owned by something else
 * it should be TRACE_CTRL_EXTERNAL.
 * @note the letters p and e are magic numbers used to detect if the packet
 * wasn't created properly
 */
typedef enum {
	TRACE_CTRL_PACKET='p',
	TRACE_CTRL_EXTERNAL='e' 
} buf_control_t;
/** Structure holding information about a packet */
#define LIBTRACE_PACKET_BUFSIZE 65536

/** The libtrace structure, applications shouldn't be meddling around in here 
 */
typedef struct libtrace_packet_t {
	struct libtrace_t *trace; /**< pointer to the trace */
	void *header;		/**< pointer to the framing header */
	void *payload;		/**< pointer to the link layer */
	buf_control_t buf_control; /**< who owns the memory */
	void *buffer;		/**< allocated buffer */
	size_t size;		/**< trace_get_framing_length()
				 * +trace_get_capture_length() */
	uint32_t type;		/**< rt protocol type for the packet */
} libtrace_packet_t;

/** libtrace error information */
typedef struct trace_err_t{
	int err_num; 		/**< error code */
	char problem[255];	/**< the format, uri etc that caused the error for reporting purposes */
} libtrace_err_t;

/** Enumeration of error codes */
enum {
	/** No Error has occured.... yet. */
	TRACE_ERR_NOERROR 	= 0,
	/** The URI passed to trace_create() is unsupported, or badly formed */
	TRACE_ERR_BAD_FORMAT 	= -1,
	/** The trace failed to initialise */
	TRACE_ERR_INIT_FAILED 	= -2,
	/** Unknown config option */
	TRACE_ERR_UNKNOWN_OPTION= -3,
	/** Option known, but unsupported by this format */
	TRACE_ERR_OPTION_UNAVAIL= -6,
	/** This output uri cannot write packets of this type */
	TRACE_ERR_NO_CONVERSION = -4,
	/** This packet is corrupt, or unusable for the action required */ 
	TRACE_ERR_BAD_PACKET	= -5,
	/** This feature is unsupported */
	TRACE_ERR_UNSUPPORTED	= -7
};

typedef enum {
	TRACE_DLT_NULL = 0,
	TRACE_DLT_EN10MB = 1,
	TRACE_DLT_ATM_RFC1483 = 11,
	TRACE_DLT_IEEE802_11 = 105,
	TRACE_DLT_LINUX_SLL = 113,
	TRACE_DLT_PFLOG = 117
} libtrace_dlt_t ;


/** @name Protocol structures
 * These convenience structures are here as they are portable ways of dealing
 * with various protocols.
 * @{
 */

#ifdef WIN32
#pragma pack(push)
#pragma pack(1)
#endif

/** Structure for dealing with IP packets */
typedef struct libtrace_ip
{
#if BYTE_ORDER == LITTLE_ENDIAN
    uint8_t ip_hl:4;		/**< header length */
    uint8_t ip_v:4;		/**< version */
#elif BYTE_ORDER == BIG_ENDIAN
    uint8_t ip_v:4;		/**< version */
    uint8_t ip_hl:4;		/**< header length */
#else
#   error "Adjust your <bits/endian.h> defines"
#endif
    uint8_t  ip_tos;			/**< type of service */
    uint16_t ip_len;			/**< total length */
    int16_t  ip_id;			/**< identification */
#if BYTE_ORDER == LITTLE_ENDIAN
    uint16_t ip_off:12;		/**< fragment offset */
    uint16_t ip_mf:1;		/**< more fragments flag */
    uint16_t ip_df:1;		/**< dont fragment flag */
    uint16_t ip_rf:1;		/**< reserved fragment flag */
#elif BYTE_ORDER == BIG_ENDIAN
    uint16_t ip_rf:1;
    uint16_t ip_df:1;
    uint16_t ip_mf:1;
    uint16_t ip_off:12;
#else
#   error "Adjust your <bits/endian.h> defines"
#endif
    uint8_t  ip_ttl;			/**< time to live */
    uint8_t  ip_p;			/**< protocol */
    uint16_t ip_sum;			/**< checksum */
    struct in_addr ip_src;		/**< source address */
    struct in_addr ip_dst;		/**< dest address */
} PACKED libtrace_ip_t;

typedef struct libtrace_ip6_ext
{
	uint8_t nxt;
	uint8_t len;
} PACKED libtrace_ip6_ext_t;

/** IPv6 header structure */
typedef struct libtrace_ip6
{ 
    uint32_t flow;
    uint16_t plen;			/**< Payload length */
    uint8_t nxt;			/**< Next header */
    uint8_t hlim;			/**< Hop limit */
    struct in6_addr ip_src;		/**< source address */
    struct in6_addr ip_dst;		/**< dest address */
} PACKED libtrace_ip6_t;

/** Structure for dealing with TCP packets */
typedef struct libtrace_tcp
  {
    uint16_t source;		/**< Source Port */
    uint16_t dest;		/**< Destination port */
    uint32_t seq;		/**< Sequence number */
    uint32_t ack_seq;		/**< Acknowledgement Number */
#  if BYTE_ORDER == LITTLE_ENDIAN
    uint8_t res1:4;	/**< Reserved bits */
    uint8_t doff:4;	/**< data offset */	
    uint8_t fin:1;		/**< FIN */
    uint8_t syn:1;		/**< SYN flag */
    uint8_t rst:1;		/**< RST flag */
    uint8_t psh:1;		/**< PuSH flag */
    uint8_t ack:1;		/**< ACK flag */
    uint8_t urg:1;		/**< URG flag */
    uint8_t res2:2;	/**< Reserved */
#  elif BYTE_ORDER == BIG_ENDIAN
    uint8_t doff:4;	/**< Data offset */
    uint8_t res1:4;	/**< Reserved bits */
    uint8_t res2:2;	/**< Reserved */
    uint8_t urg:1;		/**< URG flag */
    uint8_t ack:1;		/**< ACK flag */
    uint8_t psh:1;		/**< PuSH flag */
    uint8_t rst:1;		/**< RST flag */
    uint8_t syn:1;		/**< SYN flag */
    uint8_t fin:1;		/**< FIN flag */
#  else
#   error "Adjust your <bits/endian.h> defines"
#  endif
    uint16_t window;		/**< Window Size */
    uint16_t check;		/**< Checksum */
    uint16_t urg_ptr;		/**< Urgent Pointer */
} PACKED libtrace_tcp_t;

/** UDP Header for dealing with UDP packets */
typedef struct libtrace_udp {
  uint16_t	source;		/**< Source port */
  uint16_t	dest;		/**< Destination port */
  uint16_t	len;		/**< Length */
  uint16_t	check;		/**< Checksum */
} PACKED libtrace_udp_t;

/** ICMP Header for dealing with icmp packets */
typedef struct libtrace_icmp
{
  uint8_t type;		/**< message type */
  uint8_t code;		/**< type sub-code */
  uint16_t checksum;		/**< checksum */
  union
  {
    struct
    {
      uint16_t	id;
      uint16_t	sequence;
    } echo;			/**< echo datagram */
    uint32_t	gateway;	/**< gateway address */
    struct
    {
      uint16_t	unused;
      uint16_t	mtu;
    } frag;			/**< path mtu discovery */
  } un;				/**< Union for payloads of various icmp codes */
} PACKED libtrace_icmp_t;

/** LLC/SNAP header */
typedef struct libtrace_llcsnap
{
/* LLC */
  uint8_t dsap;			/**< Destination Service Access Point */
  uint8_t ssap;			/**< Source Service Access Point */
  uint8_t control;
/* SNAP */
  uint32_t oui:24;		/**< Organisationally Unique Identifier (scope)*/
  uint16_t type;		/**< Protocol within OUI */
} PACKED libtrace_llcsnap_t;

/** 802.3 frame */
typedef struct libtrace_ether
{
  uint8_t ether_dhost[6];	/**< destination ether addr */
  uint8_t ether_shost[6];	/**< source ether addr */
  uint16_t ether_type;		/**< packet type ID field (next-header) */
} PACKED libtrace_ether_t;

/** 802.1Q frame */
typedef struct libtrace_8021q 
{
  uint16_t vlan_pri:3;	 /**< vlan user priority */
  uint16_t vlan_cfi:1; 	 /**< vlan format indicator, 
				   * 0 for ethernet, 1 for token ring */
  uint16_t vlan_id:12; 	 /**< vlan id */
  uint16_t vlan_ether_type;	 /**< vlan sub-packet type ID field 
				   * (next-header)*/
} PACKED libtrace_8021q_t;

/** ATM cell */
typedef struct libtrace_atm_cell
{
  uint32_t gfc:4;		/**< Generic Flow Control */
  uint32_t vpi:8;		/**< Virtual Path Identifier */
  uint32_t vci:8;		/**< Virtual Channel Identifier */
  uint32_t pt:3;		/**< Payload Type */
  uint32_t clp:1;		/**< Cell Loss Priority */
  uint32_t hec:8;		/**< Header Error Control */
} PACKED libtrace_atm_cell_t;

/** POS header */
typedef struct libtrace_pos
{
 uint16_t header;
 uint16_t ether_type;		/**< ether type */
} PACKED libtrace_pos_t;

/** 802.11 header */
typedef struct libtrace_80211_t {
        uint16_t      protocol:2;
        uint16_t      type:2;
        uint16_t      subtype:4;
        uint16_t      to_ds:1;		/**< Packet to Distribution Service */
        uint16_t      from_ds:1;	/**< Packet from Distribution Service */
        uint16_t      more_frag:1;	/**< Packet has more fragments */
        uint16_t      retry:1;		/**< Packet is a retry */
        uint16_t      power:1;
        uint16_t      more_data:1;
        uint16_t      wep:1;
        uint16_t      order:1;
        uint16_t     duration;
        uint8_t      mac1[6];
        uint8_t      mac2[6];
        uint8_t      mac3[6];
        uint16_t     SeqCtl;
        uint8_t      mac4[6];
} PACKED libtrace_80211_t;

#ifdef WIN32
#pragma pack(pop)
#endif


/*@}*/

/** Prints help information for libtrace 
 *
 * Function prints out some basic help information regarding libtrace,
 * and then prints out the help() function registered with each input module
 */
DLLEXPORT void trace_help();

/** Gets the output format for a given output trace
 *
 * @param libtrace	the output trace to get the name of the format fo
 * @return callee-owned null-terminated char* containing the output format
 *
 */
DLLEXPORT SIMPLE_FUNCTION
char *trace_get_output_format(const libtrace_out_t *libtrace);

/** @name Trace management
 * These members deal with creating, configuring, starting, pausing and
 * cleaning up a trace object
 *@{
 */

/** Create a trace file from a URI
 * 
 * @param uri containing a valid libtrace URI
 * @return opaque pointer to a libtrace_t
 *
 * Valid URI's are:
 *  - erf:/path/to/erf/file
 *  - erf:/path/to/erf/file.gz
 *  - erf:/path/to/rtclient/socket
 *  - erf:-  (stdin)
 *  - dag:/dev/dagcard                  
 *  - pcapint:pcapinterface                (eg: pcap:eth0)
 *  - pcap:/path/to/pcap/file
 *  - pcap:-
 *  - rtclient:hostname
 *  - rtclient:hostname:port
 *  - wag:-
 *  - wag:/path/to/wag/file
 *  - wag:/path/to/wag/file.gz
 *  - wag:/path/to/wag/socket
 *
 *  If an error occured when attempting to open the trace file, an error
 *  trace is returned and trace_get_error should be called to find out
 *  if an error occured, and what that error was.  The trace is created in the
 *  configuration state, you must call trace_start to start the capture.
 */
DLLEXPORT libtrace_t *trace_create(const char *uri);

/** Creates a "dummy" trace file that has only the format type set.
 *
 * @return opaque pointer to a (sparsely initialised) libtrace_t
 *
 * IMPORTANT: Do not attempt to call trace_read_packet or other such functions
 * with the dummy trace. Its intended purpose is to act as a packet->trace for
 * libtrace_packet_t's that are not associated with a libtrace_t structure.
 */
DLLEXPORT libtrace_t *trace_create_dead(const char *uri);

/** Creates a trace output file from a URI. 
 *
 * @param uri	the uri string describing the output format and destination
 * @return opaque pointer to a libtrace_output_t
 * @author Shane Alcock
 *
 * Valid URI's are:
 *  - gzerf:/path/to/erf/file.gz
 *  - gzerf:/path/to/erf/file
 *  - rtserver:hostname
 *  - rtserver:hostname:port
 *
 *  If an error occured when attempting to open the output trace, NULL is returned 
 *  and trace_errno is set. Use trace_perror() to get more information
 */
DLLEXPORT libtrace_out_t *trace_create_output(const char *uri);

/** Start the capture
 * @param libtrace	The trace to start
 * @return 0 on success
 *
 * This does the actual work with starting the trace capture, and applying
 * all the config options.  This may fail.
 */
DLLEXPORT int trace_start(libtrace_t *libtrace);

/** Pause the capture
 * @param libtrace	The trace to pause
 * @return 0 on success
 *
 * This stops a capture in progress and returns you to the configuration
 * state.  Any packets that arrive after trace_pause() has been called
 * will be discarded.  To resume capture, call trace_start().
 */
DLLEXPORT int trace_pause(libtrace_t *libtrace);

/** Start an output trace
 * @param libtrace	The trace to start
 * @return 0 on success
 *
 * This does the actual work with starting a trace for write.  This generally
 * creates the file.
 */
DLLEXPORT int trace_start_output(libtrace_out_t *libtrace);

/** Valid trace capture options */
typedef enum {
	TRACE_OPTION_SNAPLEN, /**< Number of bytes captured */
	TRACE_OPTION_PROMISC, /**< Capture packets to other hosts */
	TRACE_OPTION_FILTER   /**< Apply this filter to all packets recieved */
} trace_option_t;

/** Sets an input config option
 * @param libtrace	the trace object to apply the option to
 * @param option	the option to set
 * @param value		the value to set the option to
 * @return -1 if option configuration failed, 0 otherwise
 * This should be called after trace_create, and before trace_start
 */
DLLEXPORT int trace_config(libtrace_t *libtrace,
		trace_option_t option,
		void *value);

typedef enum {
	TRACE_OPTION_OUTPUT_FILEFLAGS, /**< File flags to open the trace file
					* with.  eg O_APPEND
					*/
	TRACE_OPTION_OUTPUT_COMPRESS   /**< Compression level, eg 6. */
} trace_option_output_t;

/** Sets an output config option
 *
 * @param libtrace	the output trace object to apply the option to
 * @param option	the option to set
 * @param value		the value to set the option to
 * @return -1 if option configuration failed, 0 otherwise
 * This should be called after trace_create_output, and before 
 * trace_start_output
 */
DLLEXPORT int trace_config_output(libtrace_out_t *libtrace, 
		trace_option_output_t option,
		void *value
		);

/** Close a trace file, freeing up any resources it may have been using
 *
 */
DLLEXPORT void trace_destroy(libtrace_t *trace);

/** Close a trace file, freeing up any resources it may have been using
 * @param trace		trace file to be destroyed
 */
DLLEXPORT void trace_destroy_dead(libtrace_t *trace);

/** Close a trace output file, freeing up any resources it may have been using
 * @param trace		the output trace file to be destroyed
 *
 * @author Shane Alcock
 */
DLLEXPORT void trace_destroy_output(libtrace_out_t *trace);

/** Check (and clear) the current error state of an input trace
 * @param trace		the trace file to check the error state on
 * @return Error report
 * This reads and returns the current error state and sets the current error 
 * to "no error".
 */
DLLEXPORT libtrace_err_t trace_get_err(libtrace_t *trace);

/** Return if there is an error
 * @param trace		the trace file to check the error state on
 * This does not clear the error status, and only returns true or false.
 */
DLLEXPORT bool trace_is_err(libtrace_t *trace);

/** Output an error message to stderr and clear the error status.
 * @param trace		the trace with the error to output
 * @param msg		the message to prefix to the error
 * This function does clear the error status.
 */
DLLEXPORT void trace_perror(libtrace_t *trace, const char *msg,...);

/** Check (and clear) the current error state of an output trace
 * @param trace		the output trace file to check the error state on
 * @return Error report
 * This reads and returns the current error state and sets the current error 
 * to "no error".
 */
DLLEXPORT libtrace_err_t trace_get_err_output(libtrace_out_t *trace);

/** Return if there is an error
 * @param trace		the trace file to check the error state on
 * This does not clear the error status, and only returns true or false.
 */
DLLEXPORT bool trace_is_err_output(libtrace_out_t *trace);

/** Output an error message to stderr and clear the error status.
 * @param trace		the trace with the error to output
 * @param msg		the message to prefix to the error
 * This function does clear the error status.
 */
DLLEXPORT void trace_perror_output(libtrace_out_t *trace, const char *msg,...);


/*@}*/

/** @name Reading/Writing packets
 * These members deal with creating, reading and writing packets
 *
 * @{
 */

/** Create a new packet object
 *
 * @return a pointer to an initialised libtrace_packet_t object
 */
DLLEXPORT libtrace_packet_t *trace_create_packet();

/** Copy a packet
 * @param packet	the source packet to copy
 * @return a new packet which has the same content as the source packet
 * @note This always involves a copy, which can be slow.  Use of this 
 * function should be avoided where possible.
 * @par The reason you would want to use this function is that a zerocopied
 * packet from a device is using the devices memory which may be a limited
 * resource.  Copying the packet will cause it to be copied into the systems
 * memory.
 */
DLLEXPORT libtrace_packet_t *trace_copy_packet(const libtrace_packet_t *packet);

/** Destroy a packet object
 *
 * sideeffect: sets packet to NULL
 */
DLLEXPORT void trace_destroy_packet(libtrace_packet_t **packet);


/** Read one packet from the trace into buffer
 *
 * @param trace 	the libtrace opaque pointer
 * @param packet  	the packet opaque pointer
 * @return 0 on EOF, negative value on error, number of bytes read when
 * successful.
 *
 * @note the number of bytes read is usually (but not always) the same as
 * trace_get_framing_length()+trace_get_capture_length() depending on the
 * trace format.
 * @note the trace must have been started with trace_start before calling
 * this function
 */
DLLEXPORT int trace_read_packet(libtrace_t *trace, libtrace_packet_t *packet);

/** Event types 
 * see \ref libtrace_eventobj_t and \ref trace_event
 */
typedef enum {
	TRACE_EVENT_IOWAIT,	/**< Need to block on fd */
	TRACE_EVENT_SLEEP,	/**< Sleep for some time */
	TRACE_EVENT_PACKET,	/**< packet has arrived */
	TRACE_EVENT_TERMINATE	/**< End of trace */
} libtrace_event_t;

/** structure returned by libtrace_event explaining what the current event is */
typedef struct libtrace_eventobj_t {
	libtrace_event_t type; /**< event type (iowait,sleep,packet) */
	int fd;		       /**< if IOWAIT, the fd to sleep on */
	double seconds;	       /**< if SLEEP, the amount of time to sleep for 
				*/
	int size; 	       /**< if PACKET, the value returned from 
				*  trace_read_packet 
				*/
} libtrace_eventobj_t;

/** process a libtrace event
 * @param trace the libtrace opaque pointer
 * @param packet the libtrace_packet opaque pointer
 * @return libtrace_event struct containing the type, and potential
 * 	fd or seconds to sleep on
 *
 * Type can be:
 *  TRACE_EVENT_IOWAIT	Waiting on I/O on fd
 *  TRACE_EVENT_SLEEP	Next event in seconds
 *  TRACE_EVENT_PACKET	Packet arrived in buffer with size size
 *  TRACE_EVENT_TERMINATE Trace terminated (perhaps with an error condition)
 */
DLLEXPORT libtrace_eventobj_t trace_event(libtrace_t *trace,
		libtrace_packet_t *packet);


/** Write one packet out to the output trace
 *
 * @param trace		the libtrace_out opaque pointer
 * @param packet	the packet opaque pointer
 * @return the number of bytes written out, if zero or negative then an error has occured.
 */
DLLEXPORT int trace_write_packet(libtrace_out_t *trace, const libtrace_packet_t *packet);
/*@}*/

/** @name Protocol decodes
 * These functions locate and return a pointer to various headers inside a
 * packet
 * @{
 */

/** get a pointer to the link layer
 * @param packet  	the packet opaque pointer
 *
 * @return a pointer to the link layer, or NULL if there is no link layer
 *
 * @note you should call getLinkType() to find out what type of link layer 
 * this is
 */
DLLEXPORT SIMPLE_FUNCTION
void *trace_get_link(const libtrace_packet_t *packet);

/** get a pointer to the IP header (if any)
 * @param packet  	the packet opaque pointer
 *
 * @return a pointer to the IP header, or NULL if there is not an IP packet
 */
DLLEXPORT SIMPLE_FUNCTION
libtrace_ip_t *trace_get_ip(libtrace_packet_t *packet);

/** Gets a pointer to the transport layer header (if any)
 * @param packet        a pointer to a libtrace_packet structure
 * @param[out] proto	transport layer protocol
 *
 * @return a pointer to the transport layer header, or NULL if there is no header
 *
 * @note proto may be NULL if proto is unneeded.
 */
DLLEXPORT void *trace_get_transport(libtrace_packet_t *packet, uint8_t *proto, 
		uint32_t *remaining);

/** Gets a pointer to the payload given a pointer to the IP header
 * @param ip            The IP Header
 * @param[out] proto	An output variable of the IP protocol
 * @param[in,out] remaining Updated with the number of bytes remaining
 *
 * @return a pointer to the transport layer header, or NULL if header isn't present.
 *
 * Remaining may be NULL.  If Remaining is not NULL it must point to the number
 * of bytes captured of the IP header and beyond.  It will be updated after this
 * function to the number of bytes remaining after the IP header (and any IP options)
 * have been removed.
 *
 * proto may be NULL if not needed.
 *
 * @note This is similar to trace_get_transport_from_ip in libtrace2
 */
DLLEXPORT void *trace_get_payload_from_ip(libtrace_ip_t *ip, uint8_t *proto,
		uint32_t *remaining);

/** Gets a pointer to the payload given a pointer to a tcp header
 * @param tcp           The tcp Header
 * @param[in,out] remaining Updated with the number of bytes remaining
 *
 * @return a pointer to the tcp payload, or NULL if the payload isn't present.
 *
 * Remaining may be NULL.  If Remaining is not NULL it must point to the number
 * of bytes captured of the TCP header and beyond.  It will be updated after this
 * function to the number of bytes remaining after the TCP header (and any TCP options)
 * have been removed.
 *
 * @note This is similar to trace_get_transport_from_ip in libtrace2
 */
DLLEXPORT void *trace_get_payload_from_tcp(libtrace_tcp_t *tcp, uint32_t *remaining);

/** Gets a pointer to the payload given a pointer to a udp header
 * @param udp           The udp Header
 * @param[in,out] remaining Updated with the number of bytes remaining
 *
 * @return a pointer to the udp payload, or NULL if the payload isn't present.
 *
 * Remaining may be NULL.  If Remaining is not NULL it must point to the number
 * of bytes captured of the TCP header and beyond.  It will be updated after this
 * function to the number of bytes remaining after the TCP header (and any TCP options)
 * have been removed.
 *
 * @note This is similar trace_get_transport_from_ip in libtrace2
 */
DLLEXPORT void *trace_get_payload_from_udp(libtrace_udp_t *udp, uint32_t *remaining);

/** Gets a pointer to the payload given a pointer to a icmp header
 * @param icmp          The icmp Header
 * @param[in,out] remaining Updated with the number of bytes remaining
 *
 * @return a pointer to the icmp payload, or NULL if the payload isn't present.
 *
 * Remaining may be NULL.  If Remaining is not NULL it must point to the number
 * of bytes captured of the TCP header and beyond.  It will be updated after this
 * function to the number of bytes remaining after the TCP header (and any TCP options)
 * have been removed.
 *
 * @note This is similar to trace_get_payload_from_icmp in libtrace2
 */
DLLEXPORT void *trace_get_payload_from_icmp(libtrace_icmp_t *icmp, uint32_t *skipped);

/** get a pointer to the TCP header (if any)
 * @param packet  	the packet opaque pointer
 *
 * @return a pointer to the TCP header, or NULL if there is not a TCP packet
 */
DLLEXPORT SIMPLE_FUNCTION
libtrace_tcp_t *trace_get_tcp(libtrace_packet_t *packet);

/** get a pointer to the TCP header (if any) given a pointer to the IP header
 * @param ip		The IP header
 * @param[in,out] remaining Updated with the number of bytes remaining
 *
 * @return a pointer to the TCP header, or NULL if this is not a TCP packet
 *
 * Remaining may be NULL.  If Remaining is not NULL it must point to the number
 * of bytes captured of the TCP header and beyond.  It will be updated after this
 * function to the number of bytes remaining after the TCP header (and any TCP options)
 * have been removed.
 *
 * @note The last parameter has changed from libtrace2
 */
DLLEXPORT SIMPLE_FUNCTION
libtrace_tcp_t *trace_get_tcp_from_ip(libtrace_ip_t *ip, uint32_t *remaining);

/** get a pointer to the UDP header (if any)
 * @param packet  	the packet opaque pointer
 *
 * @return a pointer to the UDP header, or NULL if this is not a UDP packet
 */
DLLEXPORT SIMPLE_FUNCTION
libtrace_udp_t *trace_get_udp(libtrace_packet_t *packet);

/** get a pointer to the UDP header (if any) given a pointer to the IP header
 * @param 	ip	The IP header
 * @param[in,out] remaining Updated with the number of bytes remaining
 *
 * @return a pointer to the UDP header, or NULL if this is not an UDP packet
 *
 * Remaining may be NULL.  If Remaining is not NULL it must point to the number
 * of bytes captured of the TCP header and beyond.  It will be updated after this
 * function to the number of bytes remaining after the TCP header (and any TCP options)
 * have been removed.
 *
 * @note Beware the change from libtrace2 from skipped to remaining
 */
DLLEXPORT SIMPLE_FUNCTION
libtrace_udp_t *trace_get_udp_from_ip(libtrace_ip_t *ip,uint32_t *remaining);

/** get a pointer to the ICMP header (if any)
 * @param packet  	the packet opaque pointer
 *
 * @return a pointer to the ICMP header, or NULL if this is not a ICMP packet
 */
DLLEXPORT SIMPLE_FUNCTION
libtrace_icmp_t *trace_get_icmp(libtrace_packet_t *packet);

/** get a pointer to the ICMP header (if any) given a pointer to the IP header
 * @param ip		The IP header
 * @param[in,out] remaining Updated with the number of bytes remaining
 *
 * @return a pointer to the ICMP header, or NULL if this is not an ICMP packet
 *
 * Remaining may be NULL.  If Remaining is not NULL it must point to the number
 * of bytes captured of the TCP header and beyond.  It will be updated after this
 * function to the number of bytes remaining after the TCP header (and any TCP options)
 * have been removed.
 *
 * @note Beware the change from libtrace2 from skipped to remaining
 */
DLLEXPORT SIMPLE_FUNCTION
libtrace_icmp_t *trace_get_icmp_from_ip(libtrace_ip_t *ip,uint32_t *remaining);

/** Get the destination MAC addres
 * @param packet  	the packet opaque pointer
 * @return a pointer to the destination mac, (or NULL if there is no 
 * destination MAC)
 */
DLLEXPORT SIMPLE_FUNCTION
uint8_t *trace_get_destination_mac(libtrace_packet_t *packet);

/** Get the source MAC addres
 * @param packet  	the packet opaque pointer
 * @return a pointer to the source mac, (or NULL if there is no source MAC)
 */
SIMPLE_FUNCTION
uint8_t *trace_get_source_mac(libtrace_packet_t *packet);

/** Get the source addres
 * @param packet  	the packet opaque pointer
 * @param addr		a pointer to a sockaddr to store the address in, or NULL to use
 * 			static storage.
 * @return NULL if there is no source address, or a sockaddr holding a v4 or v6 address
 */
DLLEXPORT SIMPLE_FUNCTION
struct sockaddr *trace_get_source_address(const libtrace_packet_t *packet,
		struct sockaddr *addr);

/** Get the source addres
 * @param packet  	the packet opaque pointer
 * @param addr		a pointer to a sockaddr to store the address in, or NULL to use
 * 			static storage.
 * @return NULL if there is no source address, or a sockaddr holding a v4 or v6 address
 */
DLLEXPORT SIMPLE_FUNCTION
struct sockaddr *trace_get_destination_address(const libtrace_packet_t *packet,
		struct sockaddr *addr);

/*@}*/

/** parse an ip or tcp option
 * @param[in,out] ptr	the pointer to the current option
 * @param[in,out] len	the length of the remaining buffer
 * @param[out] type	the type of the option
 * @param[out] optlen 	the length of the option
 * @param[out] data	the data of the option
 *
 * @return bool true if there is another option (and the fields are filled in)
 *               or false if this was the last option.
 *
 * This updates ptr to point to the next option after this one, and updates
 * len to be the number of bytes remaining in the options area.  Type is updated
 * to be the code of this option, and data points to the data of this option,
 * with optlen saying how many bytes there are.
 *
 * @note Beware of fragmented packets.
 */
DLLEXPORT int trace_get_next_option(unsigned char **ptr,int *len,
			unsigned char *type,
			unsigned char *optlen,
			unsigned char **data);


/** @name Time
 * These functions deal with time that a packet arrived and return it
 * in various formats
 * @{
 */
/** Get the current time in DAG time format 
 * @param packet  	the packet opaque pointer
 *
 * @return a 64 bit timestamp in DAG ERF format (upper 32 bits are the seconds
 * past 1970-01-01, the lower 32bits are partial seconds)
 * @author Daniel Lawson
 */
DLLEXPORT SIMPLE_FUNCTION
uint64_t trace_get_erf_timestamp(const libtrace_packet_t *packet);

/** Get the current time in struct timeval
 * @param packet  	the packet opaque pointer
 *
 * @return time that this packet was seen in a struct timeval
 * @author Daniel Lawson
 * @author Perry Lorier
 */ 
DLLEXPORT SIMPLE_FUNCTION
struct timeval trace_get_timeval(const libtrace_packet_t *packet);

/** Get the current time in floating point seconds
 * @param packet  	the packet opaque pointer
 *
 * @return time that this packet was seen in 64bit floating point seconds
 * @author Daniel Lawson
 * @author Perry Lorier
 */
DLLEXPORT SIMPLE_FUNCTION
double trace_get_seconds(const libtrace_packet_t *packet);

/** Seek within a trace
 * @param trace		trace to seek
 * @param seconds	time to seek to
 * @return 0 on success.
 * Make the next packet read to be the first packet to occur at or after the
 * time searched for.  This must be called in the configuration state (ie,
 * before trace_start() or after trace_pause().
 * @note This function may be extremely slow.
 */
DLLEXPORT int trace_seek_seconds(libtrace_t *trace, double seconds);

/** Seek within a trace
 * @param trace		trace to seek
 * @param tv		time to seek to
 * @return 0 on success.
 * Make the next packet read to be the first packet to occur at or after the
 * time searched for.  This must be called in the configuration state (ie,
 * before trace_start() or after trace_pause().
 * @note This function may be extremely slow.
 */
DLLEXPORT int trace_seek_timeval(libtrace_t *trace, struct timeval tv);

/** Seek within a trace
 * @param trace		trace to seek
 * @param ts		erf timestamp
 * @return 0 on success.
 * Make the next packet read to be the first packet to occur at or after the
 * time searched for.  This must be called in the configuration state (ie,
 * before trace_start() or after trace_pause().
 * @note This function may be extremely slow.
 */
DLLEXPORT int trace_seek_erf_timestamp(libtrace_t *trace, uint64_t ts);

/*@}*/

/** @name Sizes
 * This section deals with finding or setting the various different lengths
 * a packet can have
 * @{
 */
/** Get the size of the packet in the trace
 * @param packet  	the packet opaque pointer
 * @return the size of the packet in the trace
 * @author Perry Lorier
 * @note Due to this being a header capture, or anonymisation, this may not
 * be the same size as the original packet.  See get_wire_length() for the
 * original size of the packet.
 * @note This can (and often is) different for different packets in a trace!
 * @note This is sometimes called the "snaplen".
 * @note The return size refers to the network-level payload of the packet and
 * does not include any capture headers. For example, an Ethernet packet with
 * an empty TCP packet will return sizeof(ethernet_header) + sizeof(ip_header)
 * + sizeof(tcp_header).
 */
DLLEXPORT SIMPLE_FUNCTION
size_t trace_get_capture_length(const libtrace_packet_t *packet);

/** Get the size of the packet as it was seen on the wire.
 * @param packet  	the packet opaque pointer
 * @return the size of the packet as it was on the wire.
 * @note Due to the trace being a header capture, or anonymisation this may
 * not be the same as the Capture Len.
 * @note trace_getwire_length \em{includes} FCS.
 */ 
DLLEXPORT SIMPLE_FUNCTION
size_t trace_get_wire_length(const libtrace_packet_t *packet);

/** Get the length of the capture framing headers.
 * @param packet  	the packet opaque pointer
 * @return the size of the packet as it was on the wire.
 * @author Perry Lorier
 * @author Daniel Lawson
 * @note this length corresponds to the difference between the size of a 
 * captured packet in memory, and the captured length of the packet
 */ 
DLLEXPORT SIMPLE_FUNCTION
size_t trace_get_framing_length(const libtrace_packet_t *packet);

/** Truncate ("snap") the packet at the suggested length
 * @param packet	the packet opaque pointer
 * @param size		the new length of the packet
 * @return the new capture length of the packet, or the original capture
 * length of the packet if unchanged
 */
DLLEXPORT size_t trace_set_capture_length(libtrace_packet_t *packet, size_t size);

/*@}*/


/** Link layer types
 * This enumates the various different link types that libtrace understands
 */
typedef enum { 
       TRACE_TYPE_HDLC_POS = 1, 
       TRACE_TYPE_ETH,			/**< 802.3 style Ethernet */
       TRACE_TYPE_ATM,
       TRACE_TYPE_AAL5,
       TRACE_TYPE_80211,		/**< 802.11 frames */
       TRACE_TYPE_NONE,
       TRACE_TYPE_LINUX_SLL,		/**< Linux "null" framing */
       TRACE_TYPE_PFLOG,		/**< FreeBSD's PFlug */
       TRACE_TYPE_POS,
       TRACE_TYPE_80211_PRISM = 12
     } libtrace_linktype_t;

/** Get the type of the link layer
 * @param packet  	the packet opaque pointer
 * @return libtrace_linktype_t
 * @author Perry Lorier
 * @author Daniel Lawson
 */
DLLEXPORT SIMPLE_FUNCTION
libtrace_linktype_t trace_get_link_type(const libtrace_packet_t *packet);

/** Set the direction flag, if it has one
 * @param packet  	the packet opaque pointer
 * @param direction	the new direction (0,1,2,3)
 * @return a signed value containing the direction flag, or -1 if this is not supported
 * @author Daniel Lawson
 */
DLLEXPORT int8_t trace_set_direction(libtrace_packet_t *packet, int8_t direction);

/** Get the direction flag, if it has one
 * @param packet  	the packet opaque pointer
 * @return a signed value containing the direction flag, or -1 if this is not supported
 * The direction is defined as 0 for packets originating locally (ie, outbound)
 * and 1 for packets originating remotely (ie, inbound).
 * Other values are possible, which might be overloaded to mean special things
 * for a special trace.
 * @author Daniel Lawson
 */
DLLEXPORT SIMPLE_FUNCTION
int8_t trace_get_direction(const libtrace_packet_t *packet);

/** @name BPF
 * This section deals with using Berkley Packet Filters
 * @{
 */
/** setup a BPF filter
 * @param filterstring a char * containing the bpf filter string
 * @return opaque pointer pointer to a libtrace_filter_t object
 * @author Daniel Lawson
 * @note The filter is not actually compiled at this point, so no correctness
 * tests are performed here. trace_bpf_setfilter will always return ok, but
 * if the filter is poorly constructed an error will be generated when the 
 * filter is actually used
 */
DLLEXPORT SIMPLE_FUNCTION
libtrace_filter_t *trace_bpf_setfilter(const char *filterstring);

/** apply a BPF filter
 * @param filter 	the filter opaque pointer
 * @param packet	the packet opaque pointer
 * @return 1 if the filter matches, 0 if it doesn't.
 * @note Due to the way BPF filters are built, the filter is not actually
 * compiled until the first time trace_bpf_filter is called. If your filter is
 * incorrect, it will generate an error message and assert, exiting the
 * program. This behaviour may change to more graceful handling of this error
 * in the future.
 */
DLLEXPORT int trace_bpf_filter(libtrace_filter_t *filter,
		const libtrace_packet_t *packet);

/** destory of BPF filter
 * @param filter 	the filter opaque pointer
 * Deallocate all the resources associated with a BPF filter
 */
DLLEXPORT void trace_destroy_bpf(libtrace_filter_t *filter);
/*@}*/

/** @name Portability
 * This section has functions that causes annoyances to portability for one
 * reason or another.
 * @{
 */

/** Convert an ethernet address to a string 
 * @param addr 	Ethernet address in network byte order
 * @param buf	Buffer to store the ascii representation, or NULL
 * @return buf, or if buf is NULL then a statically allocated buffer.
 *
 * This function is similar to the GNU ether_ntoa_r function, with a few
 * minor differences.  if NULL is passed as buf, then the function will
 * use an internal static buffer, if NULL isn't passed then the function
 * will use that buffer instead.
 *
 * @note the type of addr isn't struct ether_addr as it is with ether_ntoa_r,
 * however it is bit compatible so that a cast will work.
 */ 
DLLEXPORT char *trace_ether_ntoa(const uint8_t *addr, char *buf);

/** Convert a string to an ethernet address
 * @param buf	Ethernet address in hex format delimited with :'s.
 * @param addr	buffer to store the binary representation, or NULL
 * @return addr, or if addr is NULL, then a statically allocated buffer.
 *
 * This function is similar to the GNU ether_aton_r function, with a few
 * minor differences.  if NULL is passed as addr, then the function will
 * use an internal static buffer, if NULL isn't passed then the function will 
 * use that buffer instead.
 * 
 * @note the type of addr isn't struct ether_addr as it is with ether_aton_r,
 * however it is bit compatible so that a cast will work.
 */
DLLEXPORT uint8_t *trace_ether_aton(const char *buf, uint8_t *addr);

/*@}*/


/** Which port is the server port */
typedef enum {
	USE_DEST, 	/**< Destination port is the server port */
	USE_SOURCE	/**< Source port is the server port */
} serverport_t;

/** Get the source port
 * @param packet	the packet to read from
 * @return a port in \em HOST byte order, or equivilent to ports for this
 * protocol, or 0 if this protocol has no ports.
 * @author Perry Lorier
 */
DLLEXPORT SIMPLE_FUNCTION
uint16_t trace_get_source_port(const libtrace_packet_t *packet);

/** Get the destination port
 * @param packet	the packet to read from
 * @return a port in \em HOST byte order, or equivilent to ports for this
 * protocol, or 0 if this protocol has no ports.
 * @author Perry Lorier
 */
DLLEXPORT SIMPLE_FUNCTION
uint16_t trace_get_destination_port(const libtrace_packet_t *packet);

/** hint at the server port in specified protocol
 * @param protocol	the IP layer protocol, eg 6 (tcp), 17 (udp)
 * @param source	the source port from the packet
 * @param dest		the destination port from the packet
 * @return one of USE_SOURCE or USE_DEST depending on which one you should use
 * @note ports must be in \em HOST byte order!
 * @author Daniel Lawson
 */
DLLEXPORT SIMPLE_FUNCTION
int8_t trace_get_server_port(uint8_t protocol, uint16_t source, uint16_t dest);

/** Takes a uri and splits it into a format and uridata component. 
 * Primarily for internal use but made available for external use.
 * @param uri		the uri to be parsed
 * @param format	destination location for the format component of the uri 
 * @return 0 if an error occured, otherwise return the uridata component
 * @author Shane Alcock
 */
DLLEXPORT const char *trace_parse_uri(const char *uri, char **format);

/** RT protocol base format identifiers
 * This is used to say what kind of packet is being sent over the rt protocol
 */ 
enum base_format_t {
        TRACE_FORMAT_ERF          =1,
        TRACE_FORMAT_PCAP         =2,
        TRACE_FORMAT_PCAPFILE     =3,
        TRACE_FORMAT_WAG          =4,
        TRACE_FORMAT_RT           =5,
        TRACE_FORMAT_LEGACY_ATM   =6,
	TRACE_FORMAT_LEGACY_POS	  =7,
	TRACE_FORMAT_LEGACY_ETH   =8,
	TRACE_FORMAT_LINUX_NATIVE =9
};

/** Gets the framing header type for a given packet.
 * @param packet	the packet opaque pointer
 * @return the format of the packet
 */
enum base_format_t trace_get_format(struct libtrace_packet_t *packet);

#ifdef __cplusplus
} /* extern "C" */
#endif /* #ifdef __cplusplus */
#endif /* LIBTRACE_H_ */
