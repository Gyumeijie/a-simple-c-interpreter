CC = gcc
CFLAGS = -m32
OBJDIR = .
TARGET = main
CSOURCES = ${shell find  ${SRCDIR} -name \*.c}
OBJECTS = ${shell for obj in ${CSOURCES:.c=.o}; do echo ${OBJDIR}/`basename $$obj`;done}

${OBJDIR}/%.o: %.c
	${CC} -c ${CFLAGS} ${CPPFLAGS} $< -o $@

${TARGET}: ${OBJECTS} 
	${CC}  ${CFLAGS} ${LDFLAGS} ${OBJECTS} -o $@

clean:
	rm -f *.o



