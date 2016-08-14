# MailboxAlert
Compact and cheap radio mail alert TX/RX pair

Transmitter current consumption (avg.): 30µA (~1.5 years on a 6LR61)

Receiver current consumption (avg.): 70µA (TBD)

Radio frame format: SSSSAAAACCCC
* S: Sync, 1111
* A: Address, please change
* C: Hamming code (3 data bits)
