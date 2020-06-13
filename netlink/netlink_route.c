#include <stdio.h>
#include <string.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <linux/if_addr.h>

static int netlink_socket()
{
	int ret;
	int sock;
	struct sockaddr_nl snl;

	sock = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
	if(sock < 0)
	{
		printf("%s-%d:Failed to create netlink sock.\n", __func__, __LINE__);
		return -1;
	}	
	
	ret = fcntl(sock, F_SETFL, O_NONBLOCK);
	if(ret < 0)
	{
		printf("%s-%d:Failed to set netlink sock nonblock.\n", __func__, __LINE__);
		return -1;	
	}

	memset(&snl, 0, sizeof(struct sockaddr_nl));
	snl.nl_family = AF_NETLINK;
	snl.nl_groups = RTMGRP_IPV4_IFADDR;
	ret = bind(sock, (struct sockaddr *)&snl, sizeof(snl));
	if(ret < 0)
	{
		printf("%s-%d:bind netlink sock error, sock=%d.\n",
				__func__, __LINE__, sock);
		return -1;
	}

	return sock;
}

static int netlink_addr(struct nlmsghdr *h)
{
	int len;
	struct ifaddrmsg *ifa;
	struct rtattr *tb[IFA_MAX + 1];
	struct rtattr *rta;
	void *addr1;
	void *addr2;
	unsigned int addr;

	if(!h)
		return -1;

	ifa = NLMSG_DATA(h);	
	if(ifa->ifa_family != AF_INET)
	{
		printf("%s:addr error.\n", __func__);
		return -1;
	}

	len = h->nlmsg_len - NLMSG_LENGTH(sizeof(struct ifaddrmsg));
	if(len < 0)
	{
		printf("%s:len is error\n", __func__);
		return -1;
	}

	rta = IFA_RTA(ifa);
	memset(tb, 0, sizeof(tb));
	while(RTA_OK(rta, len))
	{
		if(rta->rta_type <= IFA_MAX)
			tb[rta->rta_type] = rta;

		rta = RTA_NEXT(rta, len);
	}

	if(tb[IFA_ADDRESS] == NULL)
		tb[IFA_ADDRESS] = tb[IFA_LOCAL];

	if(tb[IFA_LOCAL])
		addr1 = RTA_DATA(tb[IFA_LOCAL]);
	if(tb[IFA_ADDRESS])
		addr2 = RTA_DATA(tb[IFA_ADDRESS]);

	if(addr1)
	{
		memcpy(&addr, addr1, 32);
		printf("%s:prefixlen=%d addr1=%#X\n",
				__func__, ifa->ifa_prefixlen, addr);
	}

	if(addr2)
	{
		memcpy(&addr, addr2, 32);
		printf("%s:prefixlen=%d addr2=%#X\n",
				__func__, ifa->ifa_prefixlen, addr);
	}

	return 0;
}

int main()
{
	int netlink_sock;
	int sock_max, ret, status;
	char data[4096];
	fd_set readfds, readfds_temp;

	netlink_sock = netlink_socket();
	if(netlink_sock < 0)
		return -1;

	FD_ZERO(&readfds);
	FD_SET(netlink_sock, &readfds);

	sock_max = netlink_sock + 1;

	while(1)
	{
		readfds_temp = readfds;

		ret = select(sock_max, &readfds_temp, NULL, NULL, NULL);
		if(ret == -1)
			perror("select");
		else if(ret)
		{
			if(FD_ISSET(netlink_sock, &readfds_temp))
			{
				struct iovec iov = {data, sizeof(data)};
				struct sockaddr_nl snl;
				struct msghdr msg = {(void *)&snl, sizeof(snl), &iov, 1, NULL, 0, 0};	
				struct nlmsghdr *h;

				status = recvmsg(netlink_sock, &msg, 0);	
				if(status < 0)
				{
					printf("%s-%d:recv netlink msg error, sock=%d\n",
							__func__, __LINE__, netlink_sock);
					continue;
				}

				if(snl.nl_pid != 0)
				{
					printf("%s-%d:msg is not kernel.\n", __func__, __LINE__);
					continue;
				}

				if(status == 0)
				{
					printf("%s-%d:EOF\n", __func__, __LINE__);
					continue;
				}

				if(msg.msg_namelen != sizeof(snl))
				{
					printf("%s-%d:sender address length error:length %d\n",
							__func__, __LINE__, msg.msg_namelen);
					continue;
				}

				for(h = (struct nlmsghdr *)data; NLMSG_OK(h, status); h = NLMSG_NEXT(h, status))
				{
					if(h->nlmsg_type == RTM_NEWADDR)
					{
						netlink_addr(h);
					}
				}
			}
		}	
	}	
}


