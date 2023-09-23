BUILDDIR = $(PWD)/build
SRCDIR = $(PWD)/


all :  $(BUILDDIR)/Makefile
	+make -C $(BUILDDIR)
	cp -f $(BUILDDIR)/demo demo

$(BUILDDIR)/Makefile:
	mkdir -p $(BUILDDIR)
	cd $(BUILDDIR)/; cmake $(SRCDIR)

clean :
	rm -rf $(BUILDDIR)/
	rm -f demo
	rm -f *.dot
	rm -f llvm-demo.tar
	find -name "*~"| xargs rm -rf
