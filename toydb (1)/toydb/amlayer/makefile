a.out : am.o amfns.o amsearch.o aminsert.o amstack.o amglobals.o ../pflayer/pflayer.o main.o amscan.o amprint.o
	cc am.o amfns.o amsearch.o aminsert.o  amstack.o amglobals.o ../pflayer/pflayer.o main.o amscan.o amprint.o 

amlayer.o : am.o amfns.o amsearch.o aminsert.o amstack.o amglobals.o amscan.o amprint.o
	ld -r am.o amfns.o amsearch.o aminsert.o  amstack.o amglobals.o amscan.o amprint.o  -o amlayer.o

am.o : am.c am.h pf.h
	cc -c am.c

amfns.o : amfns.c am.h pf.h
	cc -c amfns.c

amsearch.o : amsearch.c am.h pf.h
	cc -c amsearch.c

aminsert.o : aminsert.c am.h pf.h
	cc -c aminsert.c

amscan.o : amscan.c am.h pf.h
	cc -c amscan.c

amstack.o : amstack.c am.h pf.h
	cc -c amstack.c

amglobals.o : amglobals.c am.h
	cc -c amglobals.c

amprint.o : amprint.c am.h pf.h 
	cc -c amprint.c
	
main.o : main.c am.h pf.h 
	cc -c main.c

# Test programs
test1 : test1.o misc.o amlayer.o ../pflayer/pflayer.o
	cc test1.o misc.o amlayer.o ../pflayer/pflayer.o -o test1

test2 : test2.o misc.o amlayer.o ../pflayer/pflayer.o
	cc test2.o misc.o amlayer.o ../pflayer/pflayer.o -o test2

test3 : test3.o misc.o amlayer.o ../pflayer/pflayer.o
	cc test3.o misc.o amlayer.o ../pflayer/pflayer.o -o test3

test_objective3 : test_objective3.o misc.o amlayer.o ../pflayer/pflayer.o
	cc test_objective3.o misc.o amlayer.o ../pflayer/pflayer.o -o test_objective3

test1.o : test1.c am.h testam.h
	cc -c test1.c

test2.o : test2.c am.h testam.h
	cc -c test2.c

test3.o : test3.c am.h testam.h
	cc -c test3.c

test_objective3.o : test_objective3.c am.h testam.h
	cc -c test_objective3.c

misc.o : misc.c am.h pf.h testam.h
	cc -c misc.c

# Clean targets
clean:
	rm -f *.o test1 test2 test3 test_objective3 a.out
	rm -f testrel.* student_method*.0


