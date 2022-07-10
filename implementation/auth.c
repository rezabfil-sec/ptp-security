/**
 * @file auth.c
 */

#include "auth.h"
#include "print.h"

#define SAD_PATH    "/etc/linuxptp/sad.toml"

int authentication_append(struct port *p, struct ptp_message *m, struct security_policy *policy)
{
	struct security_association *sa;
	struct authentication_tlv *auth;
	struct tlv_extra *extra;

	if (!policy) {
		return -1;
	}

	/* only immediate processing currently supported */
	if (policy->spp_immediate == SPP_NO_SECURITY) {
		return 0;
	}

	sa = query_security_association(policy->spp_immediate, clock_get_sad(p->clock));
	if (!sa) {
		return -1;
	}

	extra = msg_tlv_append(m, (sizeof(*auth) + sa->hash_len));
	if (!extra) {
		return -1;
	}

	/* build the AUTHENTICATION TLV */
	auth = (struct authentication_tlv *) extra->tlv;
	auth->type = TLV_AUTHENTICATION;
	auth->length = (sizeof(*auth) - sizeof(auth->type) - sizeof(auth->length)) + sa->hash_len;
	auth->spp = policy->spp_immediate;
	auth->secParamIndicator = SEC_PARAM_INDICATOR(NOT_PRESENT, NOT_PRESENT, NOT_PRESENT);
	memcpy(auth->keyId, &sa->key_id, sizeof(auth->keyId));

	return 0;
}

int protect_message(struct port *p, struct ptp_message *m, struct security_policy *policy) 
{
	struct security_association *sa;
	int len;

	/* only immediate processing currently supported */
	if (policy->spp_immediate == SPP_NO_SECURITY) {
		return 0;
	}

	sa = query_security_association(policy->spp_immediate, clock_get_sad(p->clock));
	if (!sa) {
		return -1;
	}
	
	/* As per the standard, the ICV field is not considered when hashing */
	len = ntohs(m->header.messageLength) - sa->hash_len;
	append_icv(sa, (unsigned char *)m + len, (unsigned char *)m, len);

	return 0;
}

int verify_icv(struct port *p, struct ptp_message *m, struct security_policy *policy)
{
	struct security_association *sa;
	int len, icv_invalid, seq_invalid;

	/* only immediate processing currently supported */
	if (policy->spp_immediate == SPP_NO_SECURITY) {
		return 0;
	}

	sa = query_security_association(policy->spp_immediate, clock_get_sad(p->clock));
	if (!sa) {
		return -1;
	}

	/* As per the standard, the ICV field is not considered when hashing */
	len = ntohs(m->header.messageLength) - sa->hash_len;

	icv_invalid = check_icv(sa, (unsigned char *)m + len, (unsigned char *)m, len);

	if (icv_invalid) {
		pr_err("%s: wrong ICV attached to message (SPP (Immediate): %d)", p->log_name, policy->spp_immediate);
		p->stats.invalidPA++;
		return -1;
	}

	/* ICV checks out - check seqnum next to mitigate replayability */
	seq_invalid = check_seqnum(sa, m, p);

	if (seq_invalid) {
		pr_err("%s: seqnum did not advance!", p->log_name);
		p->stats.invalidPA++;
		return -1;
	}

	p->stats.validPA++;
	return 0;
}