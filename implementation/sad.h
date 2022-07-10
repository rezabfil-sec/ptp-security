/**
 * @file sad.h
 * @brief Implements security association database for prong A of Annex P.
 */

#ifndef HAVE_SAD_H
#define HAVE_SAD_H

#include "ddt.h"
#include "toml.h"

/* 0xFF is used to signal no SA, 2^8 - 1 available */
#define MAX_SA  255

/** 
 * Supported hashing algorithms
 * Guide to dummy hashes:
 *  + Suffix A  = Sleep only on appending
 *  + Suffix C  = Sleep only on verifying
 *  + Suffix AC = Sleep on both
 *  + Prefix R = Random sleeping interval 
 */
enum hash_algorithms {
    HMAC_SHA_512_256 = 0x0,
    BLAKE2B          = 0x1,
    BLAKE3           = 0x2,
    DUMMY1A          = 0x3, 
    DUMMY2A          = 0x4,
    DUMMY3A          = 0x5,
    DUMMY1C          = 0x6,
    DUMMY2C          = 0x7,
    DUMMY3C          = 0x8,
    DUMMY1AC         = 0x9,
    DUMMY2AC         = 0xA,
    DUMMY3AC         = 0xB,
    RDUMMY1AC        = 0xC,
    RDUMMY2AC        = 0xD,
    RDUMMY3AC        = 0xE,
    RDUMMY4AC        = 0xF,
};

#define DUMMY1_SLEEP    100000
#define DUMMY2_SLEEP    100000
#define DUMMY3_SLEEP    100000
#define DUMMY4_SLEEP    5000000000

#define DUMMY1_RAND     50000
#define DUMMY2_RAND     500000
#define DUMMY3_RAND     1000000
#define DUMMY4_RAND     50000000


#define RAND_INTERVAL_LOW       0
#define RAND_INTERVAL_HIGH      1

/**
 * Cf. 16.14.4.2, PTPv2.1
 * "[F]or each SA, the received sequenceId shall be monitored for each of the 
 *  applicable message types which have separate sequenceId pools."
 */
struct seqnum {
    UInteger16 sync;
    UInteger16 delay_req;
    UInteger16 pdelay_req;
    UInteger16 announce;
    UInteger16 signaling;
    UInteger16 management;
};

/* direct mapping of sad.toml */
struct security_association {
    unsigned char *key;
    int key_id;
    int key_len;
    enum hash_algorithms hash_algo;
    int hash_len;
    struct seqnum seq;
};

/* SA field keys */
#define FIELD_SA            "security_associations"
#define FIELD_KEY           "key"
#define FIELD_KEY_ID        "key_id"
#define FIELD_KEY_LEN       "key_len"
#define FIELD_HASH_ALGO     "hash_algo"
#define FIELD_HASH_LEN      "hash_len"

/* Holds all SAs read from the SAD definition */
typedef struct {
    struct security_association associations[MAX_SA];
} security_association_db;

/**
 * Reads in the SAD definition.
 **/
void set_sad_table(security_association_db *sad);

/**
 * Provides the Security Association (SA) for a matching SPP.
 **/
struct security_association *query_security_association(UInteger8 spp, security_association_db *sad);

/**
 * Appends the ICV, calculated with the supplied hashing algorithm and parameters.
 **/
void append_icv(struct security_association *sa, unsigned char *out, const unsigned char *m, uint64_t input_len);


/**
 * Checks the supplied ICV with the supplied hashing algorithm and parameters.
 * Returns non-zero on error, zero on success.
 **/
int check_icv(struct security_association *sa, const unsigned char *icv, const unsigned char *in, uint64_t input_len);

/**
 * Checks the message's sequenceID with the last one stored in the association.
 * In case it does not advance (overflows count as advancing), the message is dropped.
 * Otherwise, we increase the currently stored sequenceID.
 */
int check_seqnum(struct security_association *sa, struct ptp_message *m, struct port *p);

#endif