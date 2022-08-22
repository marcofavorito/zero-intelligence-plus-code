

OBJS = random.o sd.o agent.o tdat.o ddat.o expctl.o
LIBS = -lm
HDRS = sd.h agent.h tdat.h ddat.h max.h expctl.h random.h
# CFLAGS = -O
CFLAGS = -ggdb
CC = cc

all: smith

smith: smith.o ${OBJS} ; ${CC} ${CFLAGS} smith.o ${OBJS} ${LIBS} -o $@

expctl.o: max.h random.h expctl.h

sd.o: random.h agent.h max.h

agent.o: random.h agent.h

tdat.o: random.h agent.h max.h tdat.h

ddat.o: random.h agent.h max.h ddat.h

smith.o : random.h agent.h max.h ddat.h tdat.h expctl.h

random.o : random.h

.o: ${HDRS} ; ${CC} -c ${CFLAGS} $<

clean:
	rm -f *.o smith
	rm -f *.xg
	rm -f *.fig

