target: udpreceiver1.c udpreceiver2.c udpreceiver3.c udpsender1.c udpsender2.c udpsender3.c
	gcc udpsender1.c -o sender_sigalrm
	gcc udpreceiver1.c -o receiver_sigalrm
	gcc udpsender2.c -o sender_select
	gcc udpreceiver2.c -o receiver_select
	gcc udpsender3.c -o sender_sockopt
	gcc udpreceiver3.c -o receiver_sockopt

clean:
	rm sender_sigalrm receiver_sigalrm sender_select receiver_select sender_sockopt receiver_sockopt