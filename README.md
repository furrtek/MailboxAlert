# MailboxAlert
Compact and cheap radio mail alert TX/RX pair

Transmitter current consumption (avg.): 30�A (~1.5 years on a 6LR61)
Receiver current consumption (avg.): 70�A (TBD)

Radio frame format: SSSSAAAACCCC
*S: Sync, 1111
*A: Address, please change
*C: Hamming code (3 data bits)