/*
 * =====================================================================================
 *
 *       Filename:  netlink_user.c
 *
 *    Description:  netlink user
 *
 *        Version:  1.0
 *        Created:  2020年04月09日 22时39分23秒 CST
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  xuequankuan (), 
 *   Organization:  
 *
 * =====================================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <string.h>
#include <linux/netlink.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>

#define NETLINK_TEST	30
#define MSG_LEN		125
#define MAX_PLOAD	125

typedef struct _user_msg_info
{
	struct nlmsghdr hdr;
	char msg[MSG_LEN];
}user_msg_info;

int main(int argc, char **argv)
{
	int ret;
	int skfd;
	socklen_t len;
	user_msg_info u_info;
	struct nlmsghdr *nlh = NULL;
	struct sockaddr_nl saddr, daddr;
	char *umsg = "hello netlink!!";

	/* create netlink socket */
	skfd = socket(AF_NETLINK, SOCK_RAW, NETLINK_TEST);
	if(-1 == skfd)
	{
		perror("create netlink socket error\n");
		return -1;
	}

	memset(&saddr, 0, sizeof(saddr));
	saddr.nl_family = AF_NETLINK;
	saddr.nl_pid = 100;
	saddr.nl_groups = 0;
	if(bind(skfd, (struct sockaddr *)&saddr, sizeof(saddr)))
	{
		perror("bind() error\n");
		close(skfd);
		return -1;
	}
	
	memset(&daddr, 0, sizeof(daddr));
	daddr.nl_family = AF_NETLINK;
	daddr.nl_pid = 0;	//to kernel
	daddr.nl_groups = 0;

	nlh = (struct nlmsghdr *)malloc(NLMSG_SPACE(MAX_PLOAD));
	memset(nlh, 0, sizeof(struct nlmsghdr));
	nlh->nlmsg_len = NLMSG_SPACE(MAX_PLOAD);
	nlh->nlmsg_flags = 0;
	nlh->nlmsg_type = 0;
	nlh->nlmsg_seq = 0;
	nlh->nlmsg_pid = saddr.nl_pid;	//self port

	memcpy(NLMSG_DATA(nlh), umsg, strlen(umsg));
	ret = sendto(skfd, nlh, nlh->nlmsg_len, 0, (struct sockaddr *)&daddr, sizeof(struct sockaddr_nl));
	if(!ret)
	{
		perror("sendto error\n");
		close(skfd);
		exit(-1);
	}	
	printf("send kernel:%s\n", umsg);
	
	memset(&u_info, 0, sizeof(u_info));
	len = sizeof(struct sockaddr_nl);
	ret = recvfrom(skfd, &u_info, sizeof(user_msg_info), 0, (struct sockaddr *)&daddr, &len);
	if(!ret)
	{
		perror("recv from kernel error\n");
		close(skfd);
		exit(-1);
	}
	printf("from kernel:%s\n", u_info.msg);
	
	close(skfd);
	free((void *)nlh);

	return 0;
}