#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <net/if.h>

#define NL_BUFSIZE 8192
#define MAX_INTERFACES 32   // Max interfaces we allow as arguments

// Store interface list
static const char *monitored_ifaces[MAX_INTERFACES];
static int monitored_count = 0;

// Helper: parse attributes
static void parse_attributes(struct rtattr *tb[], int max, struct rtattr *rta, int len) {
    while (RTA_OK(rta, len)) {
        if (rta->rta_type <= max)
            tb[rta->rta_type] = rta;
        rta = RTA_NEXT(rta, len);
    }
}

// ✅ Callback function when interface comes UP
void on_interface_up(const char *ifname, int ifindex) {
    printf(">>> CALLBACK: Interface %s (index %d) is UP <<<\n", ifname, ifindex);
}

// Function to check if interface is in monitored list
static int is_monitored(const char *ifname) {
    for (int i = 0; i < monitored_count; i++) {
        if (strcmp(monitored_ifaces[i], ifname) == 0) {
            return 1;
        }
    }
    return 0;
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

    if (!ifname || !is_monitored(ifname)) {
        return; // Ignore interfaces we don't care about
    }

    if (nlh->nlmsg_type == RTM_NEWLINK) {
        if (ifi->ifi_flags & IFF_UP) {
            on_interface_up(ifname, ifi->ifi_index);
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <iface1> [iface2] ... [ifaceN]\n", argv[0]);
        return 1;
    }

    // Save monitored interfaces
    monitored_count = argc - 1;
    if (monitored_count > MAX_INTERFACES) {
        fprintf(stderr, "Too many interfaces (max %d)\n", MAX_INTERFACES);
        return 1;
    }
    for (int i = 1; i < argc; i++) {
        monitored_ifaces[i-1] = argv[i];
    }

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

    printf("Monitoring %d interfaces for UP events:\n", monitored_count);
    for (int i = 0; i < monitored_count; i++) {
        printf("  - %s\n", monitored_ifaces[i]);
    }

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
