# Compilation settings:
CFLAGS=-Wall -std=gnu99  $(MYCFLAGS)
LDFLAGS=-lm  $(MYLDFLAGS)

OPTCFLAGS=$(CFLAGS) -O3 -march=native -DNDEBUG
DBGCFLAGS=$(CFLAGS) -ggdb3

# Benchmarking settings:
RUNS=4
ITERS=20


######


.PHONY: all clean benchmark evaluate
all: live-opt live-dbg


live-opt: main.c
	$(CC) $(OPTCFLAGS) $(LDFLAGS) -o $@ $^

live-dbg: main.c 
	$(CC) $(DBGCFLAGS) $(LDFLAGS) -o $@ $^


clean:
	rm -f *.o *.s live-opt live-dbg


#benchmark: live-opt
#	@{ echo; \
#		top -b -n 5 | head -n 5; \
#		echo; \
#		echo '>>> Will compute time needed for $(ITERS) iterations averaged over $(RUNS) runs.'; \
#		echo '>>> The measured mean time, its probable lower and upper bounds and S.D.'; \
#		echo '>>> are printed out. See the header of statistics.awk for details.'; } >&2
#	@(for i in `seq 1 $(RUNS)`; do \
#		./live-opt $(ITERS) test16384.pbm output.pbm | tee /dev/stderr; \
#	done) | awk -f statistics.awk
#
#evaluate: live-opt
#	for s in 64 1024; do ./live-opt 1 test$$s.pbm output.pbm && display output.pbm; done >/dev/null
#	# Consider the optimistic estimate
#	echo -n "Name: " >&2; read name; make -s benchmark | { read m o p s; echo $$m $$o $$p $$s >&2; echo $$name $$o; }
