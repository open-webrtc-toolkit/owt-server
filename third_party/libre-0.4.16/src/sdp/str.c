/**
 * @file sdp/str.c  SDP strings
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <re_types.h>
#include <re_fmt.h>
#include <re_list.h>
#include <re_sa.h>
#include <re_sdp.h>


const char sdp_attr_fmtp[]     = "fmtp";      /**< fmtp                 */
const char sdp_attr_maxptime[] = "maxptime";  /**< maxptime             */
const char sdp_attr_ptime[]    = "ptime";     /**< ptime                */
const char sdp_attr_rtcp[]     = "rtcp";      /**< rtcp                 */
const char sdp_attr_rtpmap[]   = "rtpmap";    /**< rtpmap               */

const char sdp_media_audio[]   = "audio";     /**< Media type 'audio'   */
const char sdp_media_video[]   = "video";     /**< Media type 'video'   */
const char sdp_media_text[]    = "text";      /**< Media type 'text'    */

const char sdp_proto_rtpavp[]  = "RTP/AVP";   /**< RTP Profile          */
const char sdp_proto_rtpsavp[] = "RTP/SAVP";  /**< Secure RTP Profile   */


/**
 * Get the SDP media direction name
 *
 * @param dir Media direction
 *
 * @return Name of media direction
 */
const char *sdp_dir_name(enum sdp_dir dir)
{
	switch (dir) {

	case SDP_INACTIVE: return "inactive";
	case SDP_RECVONLY: return "recvonly";
	case SDP_SENDONLY: return "sendonly";
	case SDP_SENDRECV: return "sendrecv";
	default:           return "??";
	}
}


/**
 * Get the SDP bandwidth name
 *
 * @param type Bandwidth type
 *
 * @return Bandwidth name
 */
const char *sdp_bandwidth_name(enum sdp_bandwidth type)
{
	switch (type) {

	case SDP_BANDWIDTH_CT:   return "CT";
	case SDP_BANDWIDTH_AS:   return "AS";
	case SDP_BANDWIDTH_RS:   return "RS";
	case SDP_BANDWIDTH_RR:   return "RR";
	case SDP_BANDWIDTH_TIAS: return "TIAS";
	default:                 return "??";
	}
}
