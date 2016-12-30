
distdir = nfddos
export DYLD_LIBRARY_PATH := libnf/src/.libs/

main:
	lex config.l
	bison -d config.y
	gcc -O3 msgs.c nfddos.c lex.yy.c config.tab.c config.c daemonize.c histcounter.c db.c action.c -Wall \
	    -I libnf -I libnf/include -L libnf/src/.libs  -lnf  -lpq -pthread \
	    -o nfddos

run:
	./nfddos -F 
	

clean:
	rm netflow

dist:  
	mkdir $(distdir)
	cp -R Makefile README conf.h main.c dataflow.* msgs.* sysdep.h $(distdir)
	find $(distdir) -name .svn -delete
	tar czf $(distdir).tar.gz $(distdir)
	rm -rf $(distdir)

