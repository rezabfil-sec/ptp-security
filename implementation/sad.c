/**
 * @file sad.c
 */
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sodium.h>
#include <blake2.h>

#include "blake3.h"
#include "print.h"
#include "toml.h"
#include "msg.h"
#include "port.h"
#include "port_private.h"
#include "sad.h"

#define SAD_PATH    "/etc/linuxptp/sad.toml"

/* assuming char is between 0 and f */
static char convert_hex_char(char c)
{
    if (c >= 'a')
        return (c - 'a') + 10;

    return c - '0';
}

static int get_toml_int(const char *field, toml_table_t *table)
{
    toml_datum_t val = toml_int_in(table, field);

    if (val.ok)
        return val.u.i;

    return 0;
}

static unsigned char *convert_hex_key(char *hex_key)
{
    unsigned char *key;
    int len;
    len = strlen(hex_key);

    key = malloc(len / 2);
    if (!key) {
        pr_err("error allocating memory for key");
        return NULL;
    }

    for (int i = 0, key_iter = 0; i < (len / 2); i++, key_iter += 2) {
        key[i] = (convert_hex_char(hex_key[key_iter]) << 4) |
                 (convert_hex_char(hex_key[key_iter + 1]));
    }

    return key;
}

static unsigned char *get_key(const char *field, toml_table_t *table)
{
    toml_datum_t key_hex = toml_string_in(table, field);
    if (!key_hex.ok) {
        pr_err("error reading in key from SAD at %s", SAD_PATH);
        return NULL;
    }

    unsigned char *ret = convert_hex_key(key_hex.u.s);
    free(key_hex.u.s);

    return ret;
}

static void retrieve_association(struct security_association *sa, toml_table_t *table)
{
    sa->key = get_key(FIELD_KEY, table);
    sa->key_id = get_toml_int(FIELD_KEY_ID, table);
    sa->key_len = get_toml_int(FIELD_KEY_LEN, table);
    sa->hash_algo = get_toml_int(FIELD_HASH_ALGO, table);
    sa->hash_len = get_toml_int(FIELD_HASH_LEN, table);
    memset(&sa->seq, 0x0, sizeof(struct seqnum));
}

static void fill_sad(security_association_db *sad, toml_table_t *sad_table)
{
    toml_array_t *assocs;
    int size;

    assocs = toml_array_in(sad_table, FIELD_SA);
    size = toml_array_nelem(assocs);

    for (int i = 0; i < size; i++) {
        retrieve_association(&sad->associations[i], toml_table_at(assocs, i));
    }

    /* seed random sleep for dummies */
    srand(time(NULL));
}

static void hash_blake2(struct security_association *sa, unsigned char *out, const unsigned char *m, uint64_t input_len)
{
    blake2b_state b2;
    blake2b_init_key(&b2, sa->hash_len, sa->key, sa->key_len);
    blake2b_update(&b2, m, input_len);
    blake2b_final(&b2, out, sa->hash_len);
}

static int check_blake2(struct security_association *sa, const unsigned char *icv, const unsigned char *in, uint64_t input_len)
{
    uint8_t out[sa->hash_len];
    hash_blake2(sa, out, in, input_len);

    return strncmp((const char *)out, (const char *)icv, sa->hash_len);
}

static void hash_blake3(struct security_association *sa, unsigned char *out, const unsigned char *m, uint64_t input_len)
{
    blake3_hasher b3;
    blake3_hasher_init_keyed(&b3, sa->key);
    blake3_hasher_update(&b3, m, input_len);
    blake3_hasher_finalize(&b3, out, sa->hash_len);
}

static int check_blake3(struct security_association *sa, const unsigned char *icv, const unsigned char *in, uint64_t input_len)
{
    uint8_t out[sa->hash_len];
    hash_blake3(sa, out, in, input_len);

    return strncmp((const char *)out, (const char *)icv, sa->hash_len);
}

static int dummy_sleep(long nsec)
{
    struct timespec req;

    req.tv_sec = 0;
    req.tv_nsec = nsec;

    nanosleep(&req, NULL);

    return 0;
}

static int dummy_sleep_rand(long sleep, long random_sleep)
{
    long slp = sleep + (long)((double)rand() / ((double)RAND_MAX + 1) * random_sleep);
    
    struct timespec req;
    req.tv_sec = 0;
    req.tv_nsec = slp;

    nanosleep(&req, NULL);

    return 0;
}

static inline int has_advanced(UInteger16 seq_old, UInteger16 seq_new) 
{
    return ((seq_old + 1) == seq_new) 
        || ((seq_old == UINT16_MAX) && seq_new == 0);
}

static int check_delay_resp_seqnum(UInteger16 seq_new, struct port *p) 
{
    struct ptp_message *stored_req;

    /* from port.c:2101 */
    /* ports might send multiple delay_requests in case they get dropped */
    TAILQ_FOREACH(stored_req, &p->delay_req, list) {
		if (seq_new == ntohs(stored_req->delay_req.hdr.sequenceId)) {
			return 0;
		}
	}

    return 0;
    /* !!! uncomment for real checking !!! */
    // return -1;
}

/* check should not be required here since it is handled by port_syfufsm (port.c:1364) */
static int check_fup_seqnum(UInteger16 seq_new, struct port *p) 
{
    return 0;
}

static int check_pdelay_seqnum(UInteger16 seq_new, struct port *p) 
{
    UInteger16 expected;
    expected = ntohs(p->peer_delay_req->header.sequenceId);

    return 0;

    /* !!! uncomment for real checking !!! */
    // return expected != seq_new;
}

static int check_advancing_seqnum(UInteger16 *seq_old, UInteger16 seq_new) 
{
    if (has_advanced(*seq_old, seq_new)) {
        *seq_old = seq_new;
        return 0;
    }

    return 0;

    /* !!! uncomment for real checking !!! */
    // return -1;
}

void set_sad_table(security_association_db *sad)
{
    FILE *f;
    toml_table_t *sad_table;
    char err[200];

    f = fopen(SAD_PATH, "r");
    if (!f) {
        pr_err("Couldn't open SAD at %s. %s", SAD_PATH, strerror(errno));
    }

    sad_table = toml_parse_file(f, err, sizeof(err));
    fclose(f);

    if (!sad_table) {
        pr_err("Error parsing SAD definition at %s", SAD_PATH);
    }

    fill_sad(sad, sad_table);
}

struct security_association *query_security_association(UInteger8 spp, security_association_db *sad)
{
    return &sad->associations[spp];
}

void append_icv(struct security_association *sa, unsigned char *out, const unsigned char *m, uint64_t input_len) 
{
    switch (sa->hash_algo) {
        case HMAC_SHA_512_256:
            crypto_auth_hmacsha512256(out, m, input_len, sa->key);
            break;
        case BLAKE2B:
            hash_blake2(sa, out, m, input_len);
            break;
        case BLAKE3:
            hash_blake3(sa, out, m, input_len);
            break;
        case DUMMY1C:
        case DUMMY2C:
        case DUMMY3C:
            break;
        case DUMMY1A:
        case DUMMY1AC:
            dummy_sleep(DUMMY1_SLEEP);
            break;
        case DUMMY2A:
        case DUMMY2AC:
            dummy_sleep(DUMMY2_SLEEP);
            break;
        case DUMMY3A:
        case DUMMY3AC:
            dummy_sleep(DUMMY3_SLEEP);
            break;
        case RDUMMY1AC:
            dummy_sleep_rand(DUMMY1_SLEEP, DUMMY1_RAND);
            break;
        case RDUMMY2AC:
            dummy_sleep_rand(DUMMY2_SLEEP, DUMMY2_RAND);
            break;
        case RDUMMY3AC:
            dummy_sleep_rand(DUMMY3_SLEEP, DUMMY3_RAND);
            break;
        case RDUMMY4AC:
            dummy_sleep_rand(DUMMY4_SLEEP, DUMMY4_RAND);
            break;
        default:
            break;
    }
}

int check_icv(struct security_association *sa, const unsigned char *icv, const unsigned char *in, uint64_t input_len) 
{
    switch (sa->hash_algo) {
        case HMAC_SHA_512_256:
            return crypto_auth_hmacsha512256_verify(icv, in, input_len, sa->key);
        case BLAKE2B:
            return check_blake2(sa, icv, in, input_len);
        case BLAKE3:
            return check_blake3(sa, icv, in, input_len);
        case DUMMY1A:
        case DUMMY2A:
        case DUMMY3A:
            return 0;
        case DUMMY1C:
        case DUMMY1AC:
            return dummy_sleep(DUMMY1_SLEEP);
        case DUMMY2C:
        case DUMMY2AC:
            return dummy_sleep(DUMMY2_SLEEP);
        case DUMMY3C:
        case DUMMY3AC:
            return dummy_sleep(DUMMY3_SLEEP);
        case RDUMMY1AC:
            return dummy_sleep_rand(DUMMY1_SLEEP, DUMMY1_RAND);
        case RDUMMY2AC:
            return dummy_sleep_rand(DUMMY2_SLEEP, DUMMY2_RAND);
        case RDUMMY3AC:
            return dummy_sleep_rand(DUMMY3_SLEEP, DUMMY3_RAND);
        case RDUMMY4AC:
            return dummy_sleep_rand(DUMMY4_SLEEP, DUMMY4_RAND);
        default:
             return -1;
    }
}

int check_seqnum(struct security_association *sa, struct ptp_message *m, struct port *p) 
{
    int type;
    UInteger16 new;

    new = ntohs(m->header.sequenceId);
    type = msg_type(m);

    switch (type) {
        case SYNC:
            return check_advancing_seqnum(&sa->seq.sync, new);
        case DELAY_REQ:
            return check_advancing_seqnum(&sa->seq.delay_req, new);
        case PDELAY_REQ:
            return check_advancing_seqnum(&sa->seq.pdelay_req, new);
        case SIGNALING:
            return check_advancing_seqnum(&sa->seq.signaling, new);
        case MANAGEMENT:
            return check_advancing_seqnum(&sa->seq.management, new);
        case ANNOUNCE:
            return check_advancing_seqnum(&sa->seq.announce, new);
        /* the following aren't checked for advancing, but are associated with the initial request */
        case DELAY_RESP:
            return check_delay_resp_seqnum(new, p);
        case FOLLOW_UP:
            return check_fup_seqnum(new, p);
        case PDELAY_RESP:
        case PDELAY_RESP_FOLLOW_UP:
            return check_pdelay_seqnum(new, p);
    }

    return 0;
}