#include <stdio.h>
#include <stdlib.h>
#include <net/if.h>
#include <linux/netlink.h>

#include "utils.h"
#include "ip_common.h"
#include "libnetlink.h"


static void hexdump(const void *data, size_t size) {
	size_t i;
	for (i = 0; i < size; i++) {
		uint8_t c = ((uint8_t *)data)[i];
		printf("%02X", c);
		if (i < size - 1) {
			printf(" ");
		}
	}
}

int main(int argc, char *argv[])
{
	struct rtnl_handle rth = { .fd = -1 };
	// Get index of interface
	int ifindex;
	if (argc == 1) {
		ifindex = 1;
	} else if (argc == 2) {
		ifindex = if_nametoindex(argv[1]);
		if (ifindex == 0) {
			perror("Interface does not exitst");
			return -1;
		}
	} else {
		printf("Usage: %s <name>\n", argv[0]);
		return -1;
	}
	
	// Connect to the kernel's routing tables (rtnetlink)
	if (rtnl_open(&rth, 0) < 0) {
		perror("Cannot open netlink socket");
		return -1;
	}
	// Create rtnl request
	struct iplink_req req = {
		.n.nlmsg_len = NLMSG_LENGTH(sizeof(struct ifinfomsg)),
		.n.nlmsg_flags = NLM_F_REQUEST,
		.n.nlmsg_type = RTM_GETLINK,
		.i.ifi_family = AF_UNSPEC,
		.i.ifi_index = ifindex
	};
	// Send request
	struct nlmsghdr *answer;
	if (rtnl_talk(&rth, &req.n, &answer) < 0) {
		perror("Cannot send link request");
		return -1;
	}
	// Print response
	struct ifinfomsg *ifi = NLMSG_DATA(answer);
	struct rtattr *rta = IFLA_RTA(ifi);
	int len = answer->nlmsg_len - NLMSG_HDRLEN - sizeof(struct ifinfomsg);
	int i = 0;
	while (RTA_OK(rta, len)) {
		printf("%3i:", i);
		switch (rta->rta_type) {
		case IFLA_IFNAME:
			printf(" IFLA_IFNAME: %s", (char *)RTA_DATA(rta));
			break;
		default:
			printf(" type: %3i, data: ", rta->rta_type);
			if (rta->rta_len <= 16) {
				hexdump(RTA_DATA(rta), rta->rta_len);
			} else {
				printf("<too big>");
			}
			printf(", len: %i", rta->rta_len);
			break;
		}
		printf("\n");
		rta = RTA_NEXT(rta, len);
		i++;
	}
	// Close rtnetlink connection
	rtnl_close(&rth);
	return 0;
}