# ntpc
A tiny NTP client, NTP v4 Spec Compliant (RFC5905)

---
## Features

- [x] NTP v4
- [x] Sets the hardware clock
- [ ] Time Difference
- [ ] Leap Second

---
## Notes

- Any type of authentication will **not** be implemented to keep the client tiny.
- Currently the client only works only on Linux based OSes due to using kernel calls for syncing hardware clock, support for other OSes will be added in future.

---
## References
- NTP v4 Standard: https://datatracker.ietf.org/doc/html/rfc5905 ([Click Here To View Local Copy In Repo](./RFC5905.txt))
- A Bit Info About A NTP Packet: https://www.meinbergglobal.com/english/info/ntp-packet.htm
- https://man7.org/linux/man-pages/man4/rtc.4.html
- https://scaryreasoner.wordpress.com/2009/02/28/checking-sizeof-at-compile-time/

---
# Thanks

