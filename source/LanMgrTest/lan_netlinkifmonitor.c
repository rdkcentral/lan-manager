#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <net/if.h>

// Buffer size for netlink messages
#define NL_BUFSIZE 8192

// Logging macro
#define monitor_log(fmt, ...)   fprintf(stderr, "[DEBUG] " fmt "\n", ##__VA_ARGS__)


// Helper: parse attributes
static void parse_attributes(struct rtattr *tb[], int max, struct rtattr *rta, int len) {
    while (RTA_OK(rta, len)) {
        if (rta->rta_type <= max)
            tb[rta->rta_type] = rta;
        rta = RTA_NEXT(rta, len);
    }
}

// Function to handle link events
void handle_link_event(struct nlmsghdr *nlh) {
    struct ifinfomsg *ifi = NLMSG_DATA(nlh);
    int len = nlh->nlmsg_len - NLMSG_LENGTH(sizeof(*ifi));
    struct rtattr *tb[IFLA_MAX + 1];
    memset(tb, 0, sizeof(tb));

    parse_attributes(tb, IFLA_MAX, IFLA_RTA(ifi), len);

    const char *ifname = NULL;
    if (tb[IFLA_IFNAME]) {
        ifname = (char *)RTA_DATA(tb[IFLA_IFNAME]);
    }

    if (nlh->nlmsg_type == RTM_NEWLINK) {
        printf("Interface added/updated: %s (index %d)\n", 
               ifname ? ifname : "unknown", ifi->ifi_index);
    } else if (nlh->nlmsg_type == RTM_DELLINK) {
        printf("Interface deleted: %s (index %d)\n", 
               ifname ? ifname : "unknown", ifi->ifi_index);
    }
}

int main() {
    int sockfd;
    struct sockaddr_nl sa;
    char buf[NL_BUFSIZE];

    // Create netlink socket
    sockfd = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
    if (sockfd < 0) {
        perror("socket");
        return 1;
    }

    memset(&sa, 0, sizeof(sa));
    sa.nl_family = AF_NETLINK;
    sa.nl_groups = RTMGRP_LINK; // Listen for link events

    if (bind(sockfd, (struct sockaddr *)&sa, sizeof(sa)) < 0) {
        perror("bind");
        close(sockfd);
        return 1;
    }

    printf("Listening for interface create/delete events...\n");

    while (1) {
        int len = recv(sockfd, buf, sizeof(buf), 0);
        if (len < 0) {
            if (errno == EINTR) continue;
            perror("recv");
            break;
        }

        struct nlmsghdr *nlh;
        for (nlh = (struct nlmsghdr *)buf; NLMSG_OK(nlh, len); nlh = NLMSG_NEXT(nlh, len)) {
            if (nlh->nlmsg_type == NLMSG_DONE)
                break;
            if (nlh->nlmsg_type == NLMSG_ERROR) {
                fprintf(stderr, "Received netlink error\n");
                continue;
            }
            handle_link_event(nlh);
        }
    }

    close(sockfd);
    return 0;
}
