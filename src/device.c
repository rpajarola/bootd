/*
 * $Id: device.c,v 1.2 2002/07/22 13:03:14 pajarola Exp $
 */

#include "bootd.h"

static device_t *device_new();
static void     device_add(device_t * d);

static device_t *l_devices = NULL;
#ifdef HAVE_GETIFADDRS

#ifdef IFF_LOOPBACK
#define ISLOOPBACK_IFA(ifa) ((ifa)->ifa_flags & IFF_LOOPBACK)
#define ISLOOPBACK_IFR(ifr) ((ifr)->ifr_flags & IFF_LOOPBACK)
#else
#define ISLOOPBACK_IFA(ifa) ((strncmp((ifa)->ifa_name, "lo", 2)==0)&&(isdigit((ifa)->ifa_name[2])||((ifa)->ifa_name[2]==0))
#define ISLOOPBACK_IFR(ifr) ((strncmp((ifr)->ifr_name, "lo", 2)==0)&&(isdigit((ifr)->ifr_name[2])||((ifr)->ifr_name[2]==0))
#endif

/*
 * see /usr/src/contrib/libpcap/inet.c on how to get a list of all devices
 */

void
device_init()
{
	device_t       *d;
	char            buf[512];
	int             i;

	struct ifaddrs *pifa, *nifa;
	struct sockaddr_in *sain;
	struct sockaddr_dl *sadl;
	struct ether_addr *ea;
	if (getifaddrs(&pifa) != 0) {
		log_msg(LOG_ERR | LOG_DEV, "getifaddrs: %s", strerror(errno));
		/* notreached */
	}
	for (nifa = pifa; nifa; nifa = nifa->ifa_next) {
		if ((l_devices == NULL) || ((d = device_search_by_name(nifa->ifa_name)) == NULL)) {
			/* new device */
			if (ISLOOPBACK_IFA(nifa)) {
				continue;
			}
			if (!(nifa->ifa_flags & IFF_UP)) {
				continue;
			}
			d = device_new();
			d->name = strdup(nifa->ifa_name);
			d->flags = nifa->ifa_flags;
			device_add(d);
		}
		switch (nifa->ifa_addr->sa_family) {
		case AF_INET:
			sain = (struct sockaddr_in *) nifa->ifa_addr;
			if (util_ip_valid((ipaddr_t*)&sain->sin_addr)) {
				for (i = 0; i < DEVICE_IP_ADDRS_MAX; i++) {
					if (!util_ip_valid(&(d->ip[i]))) {
						memcpy(&(d->ip[i]), &(sain->sin_addr), sizeof(ipaddr_t));
						break;
					}
				}
			}
			break;
		case AF_LINK:
			sadl = (struct sockaddr_dl *) nifa->ifa_addr;
			ea = (struct ether_addr *) (sadl->sdl_data + sadl->sdl_nlen);
			memcpy(&(d->eth), ea, sizeof(struct ether_addr));
			break;
		default:
			/* unknown address family */
		}
	}
	freeifaddrs(pifa);

	for (d = l_devices; d; d = d->next) {
		buf[0] = 0;
		str_eth(&(d->eth), sizeof(buf) - strlen(buf), buf + strlen(buf));
		for (i = 0; i < DEVICE_IP_ADDRS_MAX; i++) {
			if (util_ip_valid(&(d->ip[i]))) {
				strcat(buf, ", ");
				str_ip(&(d->ip[i]), sizeof(buf) - strlen(buf), buf + strlen(buf));
			}
		}
		log_msg(LOG_DEBUG | LOG_DEV, "%s: %s", d->name, buf);
	}
}
#else				/* HAVE_IFADDRS */
void
device_init()
{
	/* XXX noop */
	device_t       *d;
	d = device_new();
	d->name = strdup("eth0");
	d->flags = 0;
	device_add(d);
}
#endif

static device_t *
device_new()
{
	device_t       *d;
	d = malloc(sizeof(device_t));
	memset(d, 0, sizeof(device_t));
	return d;
}

static void
device_add(device_t * d)
{
	device_t       *n;
	if (l_devices == NULL) {
		l_devices = d;
		d->next = NULL;
	} else {
		for (n = l_devices; n; n = n->next) {
			if (str_is_equal(n->name, d->name)) {
				log_msg(LOG_WARN | LOG_CONF, "duplicate entry for device %s (ignored)", d->name);
				return;
			}
			if (n->next == NULL) {
				n->next = d;
				return;
			}
		}
	}
}

device_t       *
device_search_by_name(char *name)
{
	device_t       *n;
	if (name == NULL) {
		return l_devices;
	}
	if (l_devices == NULL) {
		device_init();
	}
	for (n = l_devices; n; n = n->next) {
		if (str_is_equal(n->name, name)) {
			return n;
		}
	}
	return NULL;
}

device_t       *
device_search_by_eth(etheraddr_t * eth)
{
	device_t       *n;
	if (l_devices == NULL) {
		device_init();
	}
	for (n = l_devices; n; n = n->next) {
		if (memcmp(&(n->eth), eth, sizeof(etheraddr_t)) == 0) {
			return n;
		}
	}
	return NULL;
}

device_t       *
device_search_by_ip(ipaddr_t * ip)
{
	device_t       *n;
	int             i;
	if (l_devices == NULL) {
		device_init();
	}
	for (n = l_devices; n; n = n->next) {
		for (i = 0; i < DEVICE_IP_ADDRS_MAX; i++) {
			if (memcmp(&(n->ip[i]), ip, sizeof(ipaddr_t)) == 0) {
				return n;
			}
		}
	}
	return NULL;
}
