DIRS = . scorefiles

all local_install clean lean debug installsrc install profile:
	@for i in $(DIRS); do \
		(cd ./$$i; echo "$$i:"; \
		make -f Makefile $(MAKE_FLAGS) $@) || exit $? ; done
	

	


