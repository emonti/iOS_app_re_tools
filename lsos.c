/* lsos - list open sockets
 * author: eric monti 10/14/2010
 *
 * This is a half-assed open socket listing tool based on lsof for systems
 * that support libproc (i.e. darwin). This was written because the lsof 
 * port from Cydia stopped working on recent idevices i've tried it on and
 * i didn't feel like setting up my opensource dev environment to re-build
 * and debug it from the source.
 */

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <strings.h>
#include <assert.h>
#include <arpa/inet.h>
#include <libproc.h>

/*
 * IPv6_2_IPv4()  -- macro to define the address of an IPv4 address contained
 *                 in an IPv6 address
 */

#define	IPv6_2_IPv4(v6)	(((uint8_t *)((struct in6_addr *)v6)->s6_addr)+12)

void dump_proc_sockets(pid_t pid);
void dump_all_proc_sockets(void);
void dump_fdinfo(char *cmd, pid_t pid, int fd);
char * get_protocol(int p);

int
main(int argc, char **argv)
{
  int i;

  if (argc < 2)
    dump_all_proc_sockets();
  else
    for (i=1; i < argc; i++)
      dump_proc_sockets((pid_t) atoi(argv[i]));

  return(0);
}

void
dump_all_proc_sockets()
{
  void *buf=NULL;
  int sz1, sz2;
  pid_t *pid;

  sz1 = proc_listpids(PROC_ALL_PIDS, 0, NULL, 0);
  assert (sz1 > 0);

  buf = (void *) malloc(sz1);
  assert(buf != NULL);

  sz2 = proc_listpids(PROC_ALL_PIDS, 0, buf, sz1);
  assert(sz2 > 0);

  pid = (int *) buf;

  for(; pid < (int *)(buf+sz2); pid++) {
    if (*(pid) == 0)
      continue;

    dump_proc_sockets(*(pid));
  }
}

void
dump_proc_sockets(pid_t pid)
{
  int ret, i, bret;
  char * buf;
  size_t bufsz;
  struct proc_taskallinfo taskinfo;
  struct proc_fdinfo * pfd, pfde;

  ret = proc_pidinfo(pid, PROC_PIDTASKALLINFO, 0, &taskinfo, sizeof(struct proc_taskallinfo));
  assert(ret == sizeof(struct proc_taskallinfo));

  bufsz = (sizeof(struct proc_fdinfo) * taskinfo.pbsd.pbi_nfiles);
  buf = (void *) malloc(bufsz);
  assert(buf != 0);
  bzero(buf, bufsz);

  bret = proc_pidinfo(pid, PROC_PIDLISTFDS, 0, buf, bufsz);
  assert(bret >= sizeof(struct proc_fdinfo));

  /* process each file descriptor looking for sockets */
  pfd = (struct proc_fdinfo *) buf;
  for(; pfd < (struct proc_fdinfo *)(buf+bret); pfd++)
    if(pfd->proc_fdtype == PROX_FDTYPE_SOCKET)
      dump_fdinfo(
          ((taskinfo.pbsd.pbi_name[0] != '\0')? taskinfo.pbsd.pbi_name : taskinfo.pbsd.pbi_comm),
          pid, 
          pfd->proc_fd);

  free(buf);
}

void dump_fdinfo(char *cmd, pid_t pid, int fd) {
  char lbuf[256], fbuf[256];
  struct socket_fdinfo si;
  int fam, ret;
  unsigned char *la=NULL, *fa=NULL;
  int lp, fp;

  ret = proc_pidfdinfo(pid, fd, PROC_PIDFDSOCKETINFO, &si, sizeof(si));
  assert(ret > 0);

  fam = si.psi.soi_family;

  if((fam == AF_INET) || (fam == AF_INET6)) {
    char ipv = (fam == AF_INET) ? '4' : '6';
    char *proto = get_protocol(si.psi.soi_protocol);

    if (proto == NULL) {
      printf("%s(%d) fd:%d, proto=IPv%c, trans=UNKNOWN(%d)\n", cmd, pid, fd, ipv, si.psi.soi_kind);
      return;
    }

    printf("%s(%d) fd=%d, proto=IPv%c, trans=%s", cmd, pid, fd, ipv, proto);

    if (fam == AF_INET) {
      // Get IPv4 address information.
      if (si.psi.soi_kind == SOCKINFO_TCP) {
        // Get information for a TCP socket.
        la = (unsigned char *)&si.psi.soi_proto.pri_tcp.tcpsi_ini.insi_laddr.ina_46.i46a_addr4;
        lp = (int)ntohs(si.psi.soi_proto.pri_tcp.tcpsi_ini.insi_lport);
        fa = (unsigned char *)&si.psi.soi_proto.pri_tcp.tcpsi_ini.insi_faddr.ina_46.i46a_addr4;
        fp = (int)ntohs(si.psi.soi_proto.pri_tcp.tcpsi_ini.insi_fport);
      } else {
        // Get information for a non-TCP socket.
        la = (unsigned char *)&si.psi.soi_proto.pri_in.insi_laddr.ina_46.i46a_addr4;
        lp = (int)ntohs(si.psi.soi_proto.pri_in.insi_lport);
        fa = (unsigned char *)&si.psi.soi_proto.pri_in.insi_faddr.ina_46.i46a_addr4;
        fp = (int)ntohs(si.psi.soi_proto.pri_in.insi_fport);
      }

print_ipv4:
      if(!inet_ntop(fam, la, lbuf, sizeof(lbuf)-1))
        printf(", (error: can't format local ipv6 address)");
      else
        printf(", local=%s:%d", lbuf, lp);

      if (!inet_ntop(fam, fa, fbuf, sizeof(fbuf)-1))
        printf(", (error: can't format remote ipv6 address)");
      else
        printf(", remote=%s:%d", fbuf, fp);

    } else {
      // Get IPv6 address information
      if (si.psi.soi_kind == SOCKINFO_TCP)
      {
        // Get TCP socket information.
        la = (unsigned char *)&si.psi.soi_proto.pri_tcp.tcpsi_ini.insi_laddr.ina_6;
        lp = (int)ntohs(si.psi.soi_proto.pri_tcp.tcpsi_ini.insi_lport);
        fa = (unsigned char *)&si.psi.soi_proto.pri_tcp.tcpsi_ini.insi_faddr.ina_6;
        fp = (int)ntohs(si.psi.soi_proto.pri_tcp.tcpsi_ini.insi_fport);
      } else {
        // Get non-TCP socket information.
        la = (unsigned char *)&si.psi.soi_proto.pri_in.insi_laddr.ina_6;
        lp = (int)ntohs(si.psi.soi_proto.pri_in.insi_lport);
        fa = (unsigned char *)&si.psi.soi_proto.pri_in.insi_faddr.ina_6;
        fp = (int)ntohs(si.psi.soi_proto.pri_in.insi_fport);
      }
      if ((la && IN6_IS_ADDR_V4MAPPED((struct in6_addr *)la)) || (fa && IN6_IS_ADDR_V4MAPPED((struct in6_addr *)fa))) {
        // Adjust IPv4 addresses mapped in IPv6 addresses.
        fam = AF_INET;
        if (la)
          la = (unsigned char *)IPv6_2_IPv4(la);
        if (fa)
          fa = (unsigned char *)IPv6_2_IPv4(fa);

        goto print_ipv4;
      }

      if(!inet_ntop(fam, la, lbuf, sizeof(lbuf)-1))
        printf(", (error: can't format local ipv6 address)");
      else
        printf(", local=[%s]:%d", lbuf, lp);

      if (!inet_ntop(fam, fa, fbuf, sizeof(fbuf)-1))
        printf(", (error: can't format remote ipv6 address)");
      else
        printf(", remote=[%s]:%d", fbuf, fp);
    }

    printf("\n");
  }

}

char * get_protocol(int p) {
  char *s;

  switch (p) {

#if	defined(IPPROTO_TCP)
  case IPPROTO_TCP:
      s = "TCP";
      break;
#endif	/* defined(IPPROTO_TCP) */

#if	defined(IPPROTO_UDP)
  case IPPROTO_UDP:
      s = "UDP";
      break;
#endif	/* defined(IPPROTO_UDP) */

#if	defined(IPPROTO_IP)
# if	!defined(IPPROTO_HOPOPTS) || IPPROTO_IP!=IPPROTO_HOPOPTS
  case IPPROTO_IP:
      s = "IP";
      break;
# endif	/* !defined(IPPROTO_HOPOPTS) || IPPROTO_IP!=IPPROTO_HOPOPTS */
#endif	/* defined(IPPROTO_IP) */

#if	defined(IPPROTO_ICMP)
  case IPPROTO_ICMP:
      s = "ICMP";
      break;
#endif	/* defined(IPPROTO_ICMP) */

#if	defined(IPPROTO_ICMPV6)
  case IPPROTO_ICMPV6:
      s = "ICMPV6";
      break;
#endif	/* defined(IPPROTO_ICMPV6) */

#if	defined(IPPROTO_IGMP)
  case IPPROTO_IGMP:
      s = "IGMP";
      break;
#endif	/* defined(IPPROTO_IGMP) */

#if	defined(IPPROTO_GGP)
  case IPPROTO_GGP:
      s = "GGP";
      break;
#endif	/* defined(IPPROTO_GGP) */

#if	defined(IPPROTO_EGP)
  case IPPROTO_EGP:
      s = "EGP";
      break;
#endif	/* defined(IPPROTO_EGP) */

#if	defined(IPPROTO_PUP)
  case IPPROTO_PUP:
      s = "PUP";
      break;
#endif	/* defined(IPPROTO_PUP) */

#if	defined(IPPROTO_IDP)
  case IPPROTO_IDP:
      s = "IDP";
      break;
#endif	/* defined(IPPROTO_IDP) */

#if	defined(IPPROTO_ND)
  case IPPROTO_ND:
      s = "ND";
      break;
#endif	/* defined(IPPROTO_ND) */

#if	defined(IPPROTO_RAW)
  case IPPROTO_RAW:
      s = "RAW";
      break;
#endif	/* defined(IPPROTO_RAW) */

#if	defined(IPPROTO_HELLO)
  case IPPROTO_HELLO:
      s = "HELLO";
      break;
#endif	/* defined(IPPROTO_HELLO) */

#if	defined(IPPROTO_PXP)
  case IPPROTO_PXP:
      s = "PXP";
      break;
#endif	/* defined(IPPROTO_PXP) */

#if	defined(IPPROTO_RAWIP)
  case IPPROTO_RAWIP:
      s = "RAWIP";
      break;
#endif	/* defined(IPPROTO_RAWIP) */

#if	defined(IPPROTO_RAWIF)
  case IPPROTO_RAWIF:
      s = "RAWIF";
      break;
#endif	/* defined(IPPROTO_RAWIF) */

#if	defined(IPPROTO_HOPOPTS)
  case IPPROTO_HOPOPTS:
      s = "HOPOPTS";
      break;
#endif	/* defined(IPPROTO_HOPOPTS) */

#if	defined(IPPROTO_IPIP)
  case IPPROTO_IPIP:
      s = "IPIP";
      break;
#endif	/* defined(IPPROTO_IPIP) */

#if	defined(IPPROTO_ST)
  case IPPROTO_ST:
      s = "ST";
      break;
#endif	/* defined(IPPROTO_ST) */

#if	defined(IPPROTO_PIGP)
  case IPPROTO_PIGP:
      s = "PIGP";
      break;
#endif	/* defined(IPPROTO_PIGP) */

#if	defined(IPPROTO_RCCMON)
  case IPPROTO_RCCMON:
      s = "RCCMON";
      break;
#endif	/* defined(IPPROTO_RCCMON) */

#if	defined(IPPROTO_NVPII)
  case IPPROTO_NVPII:
      s = "NVPII";
      break;
#endif	/* defined(IPPROTO_NVPII) */

#if	defined(IPPROTO_ARGUS)
  case IPPROTO_ARGUS:
      s = "ARGUS";
      break;
#endif	/* defined(IPPROTO_ARGUS) */

#if	defined(IPPROTO_EMCON)
  case IPPROTO_EMCON:
      s = "EMCON";
      break;
#endif	/* defined(IPPROTO_EMCON) */

#if	defined(IPPROTO_XNET)
  case IPPROTO_XNET:
      s = "XNET";
      break;
#endif	/* defined(IPPROTO_XNET) */

#if	defined(IPPROTO_CHAOS)
  case IPPROTO_CHAOS:
      s = "CHAOS";
      break;
#endif	/* defined(IPPROTO_CHAOS) */

#if	defined(IPPROTO_MUX)
  case IPPROTO_MUX:
      s = "MUX";
      break;
#endif	/* defined(IPPROTO_MUX) */

#if	defined(IPPROTO_MEAS)
  case IPPROTO_MEAS:
      s = "MEAS";
      break;
#endif	/* defined(IPPROTO_MEAS) */

#if	defined(IPPROTO_HMP)
  case IPPROTO_HMP:
      s = "HMP";
      break;
#endif	/* defined(IPPROTO_HMP) */

#if	defined(IPPROTO_PRM)
  case IPPROTO_PRM:
      s = "PRM";
      break;
#endif	/* defined(IPPROTO_PRM) */

#if	defined(IPPROTO_TRUNK1)
  case IPPROTO_TRUNK1:
      s = "TRUNK1";
      break;
#endif	/* defined(IPPROTO_TRUNK1) */

#if	defined(IPPROTO_TRUNK2)
  case IPPROTO_TRUNK2:
      s = "TRUNK2";
      break;
#endif	/* defined(IPPROTO_TRUNK2) */

#if	defined(IPPROTO_LEAF1)
  case IPPROTO_LEAF1:
      s = "LEAF1";
      break;
#endif	/* defined(IPPROTO_LEAF1) */

#if	defined(IPPROTO_LEAF2)
  case IPPROTO_LEAF2:
      s = "LEAF2";
      break;
#endif	/* defined(IPPROTO_LEAF2) */

#if	defined(IPPROTO_RDP)
  case IPPROTO_RDP:
      s = "RDP";
      break;
#endif	/* defined(IPPROTO_RDP) */

#if	defined(IPPROTO_IRTP)
  case IPPROTO_IRTP:
      s = "IRTP";
      break;
#endif	/* defined(IPPROTO_IRTP) */

#if	defined(IPPROTO_TP)
  case IPPROTO_TP:
      s = "TP";
      break;
#endif	/* defined(IPPROTO_TP) */

#if	defined(IPPROTO_BLT)
  case IPPROTO_BLT:
      s = "BLT";
      break;
#endif	/* defined(IPPROTO_BLT) */

#if	defined(IPPROTO_NSP)
  case IPPROTO_NSP:
      s = "NSP";
      break;
#endif	/* defined(IPPROTO_NSP) */

#if	defined(IPPROTO_INP)
  case IPPROTO_INP:
      s = "INP";
      break;
#endif	/* defined(IPPROTO_INP) */

#if	defined(IPPROTO_SEP)
  case IPPROTO_SEP:
      s = "SEP";
      break;
#endif	/* defined(IPPROTO_SEP) */

#if	defined(IPPROTO_3PC)
  case IPPROTO_3PC:
      s = "3PC";
      break;
#endif	/* defined(IPPROTO_3PC) */

#if	defined(IPPROTO_IDPR)
  case IPPROTO_IDPR:
      s = "IDPR";
      break;
#endif	/* defined(IPPROTO_IDPR) */

#if	defined(IPPROTO_XTP)
  case IPPROTO_XTP:
      s = "XTP";
      break;
#endif	/* defined(IPPROTO_XTP) */

#if	defined(IPPROTO_DDP)
  case IPPROTO_DDP:
      s = "DDP";
      break;
#endif	/* defined(IPPROTO_DDP) */

#if	defined(IPPROTO_CMTP)
  case IPPROTO_CMTP:
      s = "CMTP";
      break;
#endif	/* defined(IPPROTO_CMTP) */

#if	defined(IPPROTO_TPXX)
  case IPPROTO_TPXX:
      s = "TPXX";
      break;
#endif	/* defined(IPPROTO_TPXX) */

#if	defined(IPPROTO_IL)
  case IPPROTO_IL:
      s = "IL";
      break;
#endif	/* defined(IPPROTO_IL) */

#if	defined(IPPROTO_IPV6)
  case IPPROTO_IPV6:
      s = "IPV6";
      break;
#endif	/* defined(IPPROTO_IPV6) */

#if	defined(IPPROTO_SDRP)
  case IPPROTO_SDRP:
      s = "SDRP";
      break;
#endif	/* defined(IPPROTO_SDRP) */

#if	defined(IPPROTO_ROUTING)
  case IPPROTO_ROUTING:
      s = "ROUTING";
      break;
#endif	/* defined(IPPROTO_ROUTING) */

#if	defined(IPPROTO_FRAGMENT)
  case IPPROTO_FRAGMENT:
      s = "FRAGMNT";
      break;
#endif	/* defined(IPPROTO_FRAGMENT) */

#if	defined(IPPROTO_IDRP)
  case IPPROTO_IDRP:
      s = "IDRP";
      break;
#endif	/* defined(IPPROTO_IDRP) */

#if	defined(IPPROTO_RSVP)
  case IPPROTO_RSVP:
      s = "RSVP";
      break;
#endif	/* defined(IPPROTO_RSVP) */

#if	defined(IPPROTO_GRE)
  case IPPROTO_GRE:
      s = "GRE";
      break;
#endif	/* defined(IPPROTO_GRE) */

#if	defined(IPPROTO_MHRP)
  case IPPROTO_MHRP:
      s = "MHRP";
      break;
#endif	/* defined(IPPROTO_MHRP) */

#if	defined(IPPROTO_BHA)
  case IPPROTO_BHA:
      s = "BHA";
      break;
#endif	/* defined(IPPROTO_BHA) */

#if	defined(IPPROTO_ESP)
  case IPPROTO_ESP:
      s = "ESP";
      break;
#endif	/* defined(IPPROTO_ESP) */

#if	defined(IPPROTO_AH)
  case IPPROTO_AH:
      s = "AH";
      break;
#endif	/* defined(IPPROTO_AH) */

#if	defined(IPPROTO_INLSP)
  case IPPROTO_INLSP:
      s = "INLSP";
      break;
#endif	/* defined(IPPROTO_INLSP) */

#if	defined(IPPROTO_SWIPE)
  case IPPROTO_SWIPE:
      s = "SWIPE";
      break;
#endif	/* defined(IPPROTO_SWIPE) */

#if	defined(IPPROTO_NHRP)
  case IPPROTO_NHRP:
      s = "NHRP";
      break;
#endif	/* defined(IPPROTO_NHRP) */

#if	defined(IPPROTO_NONE)
  case IPPROTO_NONE:
      s = "NONE";
      break;
#endif	/* defined(IPPROTO_NONE) */

#if	defined(IPPROTO_DSTOPTS)
  case IPPROTO_DSTOPTS:
      s = "DSTOPTS";
      break;
#endif	/* defined(IPPROTO_DSTOPTS) */

#if	defined(IPPROTO_AHIP)
  case IPPROTO_AHIP:
      s = "AHIP";
      break;
#endif	/* defined(IPPROTO_AHIP) */

#if	defined(IPPROTO_CFTP)
  case IPPROTO_CFTP:
      s = "CFTP";
      break;
#endif	/* defined(IPPROTO_CFTP) */

#if	defined(IPPROTO_SATEXPAK)
  case IPPROTO_SATEXPAK:
      s = "SATEXPK";
      break;
#endif	/* defined(IPPROTO_SATEXPAK) */

#if	defined(IPPROTO_KRYPTOLAN)
  case IPPROTO_KRYPTOLAN:
      s = "KRYPTOL";
      break;
#endif	/* defined(IPPROTO_KRYPTOLAN) */

#if	defined(IPPROTO_RVD)
  case IPPROTO_RVD:
      s = "RVD";
      break;
#endif	/* defined(IPPROTO_RVD) */

#if	defined(IPPROTO_IPPC)
  case IPPROTO_IPPC:
      s = "IPPC";
      break;
#endif	/* defined(IPPROTO_IPPC) */

#if	defined(IPPROTO_ADFS)
  case IPPROTO_ADFS:
      s = "ADFS";
      break;
#endif	/* defined(IPPROTO_ADFS) */

#if	defined(IPPROTO_SATMON)
  case IPPROTO_SATMON:
      s = "SATMON";
      break;
#endif	/* defined(IPPROTO_SATMON) */

#if	defined(IPPROTO_VISA)
  case IPPROTO_VISA:
      s = "VISA";
      break;
#endif	/* defined(IPPROTO_VISA) */

#if	defined(IPPROTO_IPCV)
  case IPPROTO_IPCV:
      s = "IPCV";
      break;
#endif	/* defined(IPPROTO_IPCV) */

#if	defined(IPPROTO_CPNX)
  case IPPROTO_CPNX:
      s = "CPNX";
      break;
#endif	/* defined(IPPROTO_CPNX) */

#if	defined(IPPROTO_CPHB)
  case IPPROTO_CPHB:
      s = "CPHB";
      break;
#endif	/* defined(IPPROTO_CPHB) */

#if	defined(IPPROTO_WSN)
  case IPPROTO_WSN:
      s = "WSN";
      break;
#endif	/* defined(IPPROTO_WSN) */

#if	defined(IPPROTO_PVP)
  case IPPROTO_PVP:
      s = "PVP";
      break;
#endif	/* defined(IPPROTO_PVP) */

#if	defined(IPPROTO_BRSATMON)
  case IPPROTO_BRSATMON:
      s = "BRSATMN";
      break;
#endif	/* defined(IPPROTO_BRSATMON) */

#if	defined(IPPROTO_WBMON)
  case IPPROTO_WBMON:
      s = "WBMON";
      break;
#endif	/* defined(IPPROTO_WBMON) */

#if	defined(IPPROTO_WBEXPAK)
  case IPPROTO_WBEXPAK:
      s = "WBEXPAK";
      break;
#endif	/* defined(IPPROTO_WBEXPAK) */

#if	defined(IPPROTO_EON)
  case IPPROTO_EON:
      s = "EON";
      break;
#endif	/* defined(IPPROTO_EON) */

#if	defined(IPPROTO_VMTP)
  case IPPROTO_VMTP:
      s = "VMTP";
      break;
#endif	/* defined(IPPROTO_VMTP) */

#if	defined(IPPROTO_SVMTP)
  case IPPROTO_SVMTP:
      s = "SVMTP";
      break;
#endif	/* defined(IPPROTO_SVMTP) */

#if	defined(IPPROTO_VINES)
  case IPPROTO_VINES:
      s = "VINES";
      break;
#endif	/* defined(IPPROTO_VINES) */

#if	defined(IPPROTO_TTP)
  case IPPROTO_TTP:
      s = "TTP";
      break;
#endif	/* defined(IPPROTO_TTP) */

#if	defined(IPPROTO_IGP)
  case IPPROTO_IGP:
      s = "IGP";
      break;
#endif	/* defined(IPPROTO_IGP) */

#if	defined(IPPROTO_DGP)
  case IPPROTO_DGP:
      s = "DGP";
      break;
#endif	/* defined(IPPROTO_DGP) */

#if	defined(IPPROTO_TCF)
  case IPPROTO_TCF:
      s = "TCF";
      break;
#endif	/* defined(IPPROTO_TCF) */

#if	defined(IPPROTO_IGRP)
  case IPPROTO_IGRP:
      s = "IGRP";
      break;
#endif	/* defined(IPPROTO_IGRP) */

#if	defined(IPPROTO_OSPFIGP)
  case IPPROTO_OSPFIGP:
      s = "OSPFIGP";
      break;
#endif	/* defined(IPPROTO_OSPFIGP) */

#if	defined(IPPROTO_SRPC)
  case IPPROTO_SRPC:
      s = "SRPC";
      break;
#endif	/* defined(IPPROTO_SRPC) */

#if	defined(IPPROTO_LARP)
  case IPPROTO_LARP:
      s = "LARP";
      break;
#endif	/* defined(IPPROTO_LARP) */

#if	defined(IPPROTO_MTP)
  case IPPROTO_MTP:
      s = "MTP";
      break;
#endif	/* defined(IPPROTO_MTP) */

#if	defined(IPPROTO_AX25)
  case IPPROTO_AX25:
      s = "AX25";
      break;
#endif	/* defined(IPPROTO_AX25) */

#if	defined(IPPROTO_IPEIP)
  case IPPROTO_IPEIP:
      s = "IPEIP";
      break;
#endif	/* defined(IPPROTO_IPEIP) */

#if	defined(IPPROTO_MICP)
  case IPPROTO_MICP:
      s = "MICP";
      break;
#endif	/* defined(IPPROTO_MICP) */

#if	defined(IPPROTO_SCCSP)
  case IPPROTO_SCCSP:
      s = "SCCSP";
      break;
#endif	/* defined(IPPROTO_SCCSP) */

#if	defined(IPPROTO_ETHERIP)
  case IPPROTO_ETHERIP:
      s = "ETHERIP";
      break;
#endif	/* defined(IPPROTO_ETHERIP) */

#if	defined(IPPROTO_ENCAP)
# if	!defined(IPPROTO_IPIP) || IPPROTO_IPIP!=IPPROTO_ENCAP
  case IPPROTO_ENCAP:
      s = "ENCAP";
      break;
# endif	/* !defined(IPPROTO_IPIP) || IPPROTO_IPIP!=IPPROTO_ENCAP */
#endif	/* defined(IPPROTO_ENCAP) */

#if	defined(IPPROTO_APES)
  case IPPROTO_APES:
      s = "APES";
      break;
#endif	/* defined(IPPROTO_APES) */

#if	defined(IPPROTO_GMTP)
  case IPPROTO_GMTP:
      s = "GMTP";
      break;
#endif	/* defined(IPPROTO_GMTP) */

#if	defined(IPPROTO_DIVERT)
  case IPPROTO_DIVERT:
      s = "DIVERT";
      break;
#endif	/* defined(IPPROTO_DIVERT) */

  default:
      s = (char *) NULL;
  }
  return(s);
}
