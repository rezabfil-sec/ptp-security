/**
 * @file spd.c
 */

#include <stdlib.h>
#include <errno.h>

#include "spd.h"
#include "print.h"

#define SPD_PATH    "/etc/linuxptp/spd.toml"

const char *msg_names[] = {
    "SYNC",
    "DELAY_REQ",
    "PDELAY_REQ",
    "PDELAY_RESP",
    "FOLLOW_UP",
    "DELAY_RESP",
    "PDELAY_RESP_FOLLOW_UP",
    "ANNOUNCE",
    "SIGNALING",
    "MANAGEMENT"
};

static int get_toml_int(const char *field, toml_table_t *table)
{
    toml_datum_t val = toml_int_in(table, field);

    if (val.ok)
        return val.u.i;

    return 0;
}

static void retrieve_policy(struct security_policy *policy, toml_table_t *table)
{
    policy->spp_immediate = get_toml_int(FIELD_SPP_IMMEDIATE, table);
    policy->spp_delayed = get_toml_int(FIELD_SPP_DELAYED, table);
}

static void fill_spd(security_policy_db *spd, toml_table_t *spd_table)
{
    for (int i = 0; i < NUM_MSG; i++) {
        retrieve_policy(&spd->policies[i], toml_table_in(spd_table, msg_names[i]));
    }
} 

void set_spd_table(security_policy_db *spd)
{
    FILE *f;
    toml_table_t *spd_table;
    char err[200];

    f = fopen(SPD_PATH, "r");
    if (!f) {
        pr_err("couldn't open SPD at %s. %s", SPD_PATH, strerror(errno));
    }

    spd_table = toml_parse_file(f, err, sizeof(err));
    fclose(f);

    if (!spd_table) {
        pr_err("error parsing SPD definition at %s", SPD_PATH);
    }

    fill_spd(spd, spd_table);
}

struct security_policy *query_security_policy(struct ptp_message *m, security_policy_db *spd)
{
    int type;

    type = msg_type(m);

    switch (type) {
        case SYNC:
            return &spd->policies[MSG_SYNC];
        case DELAY_REQ:
            return &spd->policies[MSG_DELAY_REQ];
        case PDELAY_REQ:
            return &spd->policies[MSG_PDELAY_REQ];
        case PDELAY_RESP:
            return &spd->policies[MSG_PDELAY_RESP];
        case FOLLOW_UP:
            return &spd->policies[MSG_FOLLOW_UP];
        case DELAY_RESP:
            return &spd->policies[MSG_DELAY_RESP];
        case PDELAY_RESP_FOLLOW_UP:
            return &spd->policies[MSG_PDELAY_RESP_FOLLOW_UP];
        case SIGNALING:
            return &spd->policies[MSG_SIGNALING];
        case MANAGEMENT:
            return &spd->policies[MSG_MANAGEMENT];
        case ANNOUNCE:
            return &spd->policies[MSG_ANNOUNCE];
        default:
            return NULL;
    }
}