#define _POSIX_C_SOURCE 200112L
#define _DEFAULT_SOURCE

#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdint.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netdb.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h> // open(...), write(...)

// For Sync Hardware Clock (Using Linux kernel API)
#include <linux/rtc.h>
#include <sys/ioctl.h>

const uint64_t    NTP_Offset = 2208988800ULL; // First day UNIX
const char* const NTP_Server = "0.in.pool.ntp.org"; // NTP Server URL for India
const char* const NTP_Port   = "123"; // By Default NTP Servers Use This Port

struct NTP_Packet {
	/*
		First Byte Is Called Header And Contains:
		LI   = Leap Indicator, 2 Bits
		VN   = Version Number, 3 Bits
		Mode =           Mode, 3 Bits
	*/
	uint8_t  Header;
	uint8_t  Stratum;
	uint8_t  Poll;      // Max interval between successive messages
	uint8_t  Precision; // Precision of local clock

	uint32_t RootDelay;
	uint32_t RootDispersion;
	uint32_t ReferenceID;

	uint32_t ReferenceTimestamp_Seconds;
	uint32_t ReferenceTimestamp_Fraction;

	uint32_t OriginTimestamp_Seconds;
	uint32_t OriginTimestamp_Fraction;

	uint32_t ReceiveTimestamp_Seconds;
	uint32_t ReceiveTimestamp_Fraction;

	uint32_t TransmitTimeStamp_Seconds;
	uint32_t TransmitTimeStamp_Fraction;
};

int FetchNTP_Packet(struct NTP_Packet* pkt, const char* const server, const char* const port) {
	struct addrinfo hints;
	bzero(&hints, sizeof(hints));
	hints.ai_family = AF_UNSPEC;     // Allow IPV4 & IPV6
	hints.ai_socktype = SOCK_DGRAM;  // UDP Socket
	hints.ai_flags = 0;              // No Extra Flags
	hints.ai_protocol = IPPROTO_UDP; // Only Return UDP Sockets

	struct addrinfo* result = NULL;
	int AddrInfoResult = getaddrinfo(server, port, &hints, &result);
	if (AddrInfoResult != 0) {
		printf("Error: getaddrinfo(...) - %s\n", gai_strerror(AddrInfoResult));
		return 1;
	}

	/* getaddrinfo() returns a list of address structures.
	Try each address until we successfully connect(2).
	If socket(2) (or connect(2)) fails, we (close the socket)
	and try the next address. */

	struct addrinfo* rp = NULL;
	int SocketFD = -1;
	for (rp = result; rp != NULL; rp = rp->ai_next) {
		SocketFD = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		if (SocketFD == -1)
			continue;

		if (connect(SocketFD, rp->ai_addr, rp->ai_addrlen) != -1) break; // Success

		close(SocketFD);
	}

	if (rp == NULL) {
		printf("Error: couldn't connect to the NTP server\n");
		freeaddrinfo(result);
		return 1;
	}
	freeaddrinfo(result);

	int NumBytesWritten = write(SocketFD, pkt, sizeof(*pkt));
	if (NumBytesWritten < 0) {
		printf("Error: write(...) - %s\n", strerror(errno));
		close(SocketFD);
		return 1;
	} else if (NumBytesWritten != sizeof(*pkt)) {
		printf("Error: write(...) - expected to write %ld bytes, wrote %d bytes instead\n", sizeof(*pkt), NumBytesWritten);
		close(SocketFD);
		return 1;
	}

	int NumBytesRead = read(SocketFD, pkt, sizeof(*pkt));
	if (NumBytesRead < 0) {
		printf("Error: read(...) - %s\n", strerror(errno));
		close(SocketFD);
		return 1;
	} else if (NumBytesRead != sizeof(*pkt)) {
		printf("Error: read(...) - expected to read %ld bytes, read %d bytes instead\n", sizeof(*pkt), NumBytesRead);
		close(SocketFD);
		return 1;
	}

	pkt->TransmitTimeStamp_Seconds = htonl(pkt->TransmitTimeStamp_Seconds);
	pkt->TransmitTimeStamp_Fraction = htonl(pkt->TransmitTimeStamp_Fraction);

	close(SocketFD);
	return 0;
}

int main(void) {
	#define BUILD_BUG_ON(condition) ((void)sizeof(char[1 - 2*!!(condition)]))
		BUILD_BUG_ON(sizeof(struct NTP_Packet) != 48);
	#undef BUILD_BUG_ON

	struct NTP_Packet MyPacket;
	bzero(&MyPacket, sizeof(MyPacket));

	/*
		LI     = 0 (00)  Corresponds To No Warning Of An Impending Leap Second To Be Inserted/Deleted
		VN     = 4 (010) Corresponds To NTP v4
		Mode   = 3 (011) Corresponds To Client Mode
		Header = 0x13 (00 010 011)
	*/
	MyPacket.Header = 0x13;

	if (FetchNTP_Packet(&MyPacket, NTP_Server, NTP_Port) != 0) return 1;

	struct timeval time_ntp = {
		(time_t)(MyPacket.TransmitTimeStamp_Seconds - NTP_Offset), 0
	};
	struct timeval time_local;
	if (gettimeofday(&time_local, NULL) < 0) {
		printf("Error: gettimeofday(...) - %s\n", strerror(errno));
		return 1;
	}

	int fd = 0;
	struct rtc_time rt;
	struct tm tp_tm;

	if (gmtime_r(&time_ntp.tv_sec, &tp_tm) == NULL) {
		printf("Error: gmtime_r(...) - %s\n", strerror(errno));
		return 1;
	}

	rt.tm_sec  = tp_tm.tm_sec;
	rt.tm_min  = tp_tm.tm_min;
	rt.tm_hour = tp_tm.tm_hour;
	rt.tm_mday = tp_tm.tm_mday;
	rt.tm_mon  = tp_tm.tm_mon;
	rt.tm_year = tp_tm.tm_year;

	fd = open("/dev/rtc", O_RDONLY);
	if (fd < 0) {
		printf("Error: open(...) - %s\n", strerror(errno));
		return 1;
	}
	if (ioctl(fd, RTC_SET_TIME, &rt) < 0) {
		printf("Error: ioctl(...) - %s\n", strerror(errno));
		return 1;
	}
	if (close(fd) < 0) {
		printf("Error: close(...) - %s\n", strerror(errno));
		return 1;
	}

	printf("Time: %s", ctime(&time_ntp.tv_sec));
	printf("Your system was %ld seconds off\n", time_local.tv_sec - time_ntp.tv_sec);

	return 0;
}

