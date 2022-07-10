/**
 * @file spd.h
 * @brief Implements security policy database for prong A of Annex P.
 */

#ifndef HAVE_SPD_H
#define HAVE_SPD_H

#include "ddt.h"
#include "msg.h"
#include "toml.h"

#define SPP_NO_SECURITY     0xFF

#define NUM_MSG     10

enum msg_types {
    MSG_SYNC,
    MSG_DELAY_REQ,
    MSG_PDELAY_REQ,
    MSG_PDELAY_RESP,
    MSG_FOLLOW_UP,
    MSG_DELAY_RESP,
    MSG_PDELAY_RESP_FOLLOW_UP,
    MSG_ANNOUNCE,
    MSG_SIGNALING,
    MSG_MANAGEMENT
};

/* Direct mapping of spd.toml */
struct security_policy {
    UInteger8 spp_immediate;
    UInteger8 spp_delayed;
};

/* SPD field keys */
#define FIELD_SPP_IMMEDIATE         "spp_immediate"
#define FIELD_SPP_DELAYED           "spp_delayed"

/* Holds all security policies from spd.toml */
typedef struct {
    struct security_policy policies[NUM_MSG];
} security_policy_db;

/**
 * Reads in the SPD definition from the TOML configuration.
 **/
void set_spd_table(security_policy_db *spd);

/**
 * Provides the security policy regarding an AUTHENTICATION TLV 
 * by querying with policy-limiting fields.
 * Policy-limiting fields may include: 
 * sourcePortIdentity, domainNumber and messageType,
 * as well as majorSdoId and minorSdoId (both not supported by linuxptp as of yet).
 **/
struct security_policy *query_security_policy(struct ptp_message *m, security_policy_db *spd);

#endif