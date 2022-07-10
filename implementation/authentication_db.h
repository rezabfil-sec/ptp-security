/**
 * @file authentication_db.h
 * @brief Definition of a structure to hold the SPD and SAD tables.
 */

#ifndef HAVE_AUTHENTICATION_DB_H
#define HAVE_AUTHENTICATION_DB_H

#include "toml.h"
#include "sad.h"
#include "spd.h"

struct authentication_db {
    security_policy_db spd;
    security_association_db sad;
};

#endif