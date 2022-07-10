/**
 * @file spd.h
 * @brief Implements helper interfaces between PTP ports and the security databases.
 */

#ifndef HAVE_AUTH_H
#define HAVE_AUTH_H

#include "port.h"
#include "port_private.h"
#include "spd.h"
#include "sad.h"

/**
 * Appends the Authentication TLV to a PTP message, if indicated by the security policy.
 * @param p      The port the message is to be sent on. For TCs: the ingress port.
 * @param m      The message to be modified.
 * @param policy The associated security policy.
 * @return       Zero on success, non-zero otherwise.
 */
int authentication_append(struct port *p, struct ptp_message *m, struct security_policy *policy);

/**
 * Calculates the ICV for an existing Authentication TLV, if indicated by the security policy.
 * @param p      The port the message is to be sent on. For TCs: the ingress port.
 * @param m      The message to be modified.
 * @param policy The associated security policy.
 * @return       Zero on success, non-zero otherwise.
 */
int protect_message(struct port *p, struct ptp_message *m, struct security_policy *policy);

/**
 * Verifies the ICV of the given PTP message, if indicated by the security policy.
 * @param p      The port the message is to be sent on. For TCs: the ingress port.
 * @param m      The message to be modified.
 * @param policy The associated security policy.
 * @return       Zero on success, non-zero otherwise.
 */
int verify_icv(struct port *p, struct ptp_message *m, struct security_policy *policy);

#endif