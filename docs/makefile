RONN_MANUAL       = libu8
RONN_ORGANIZATION = beingmeta.com

PAGES=u8run.1 u8_xsbust.1 u8_gitversion.1 u8_gitpatchcount.1 u8_debchangelog.1 u8_aptversion.1 u8_gitbranch.1 u8_gitcommit.1 u8_install_shared.1

man/%: ronn/%.ronn
	ronn --roff < $< > $@
man.html/%.html: ronn/%.ronn
	ronn --html < $< > $@
man.html.include/%.html: ronn/%.ronn
	ronn --fragment < $< > $@

default: manpages htmlpages htmlinclude

man manpages:
	@for page in ${PAGES}; do make man/$${page}; done

html htmlpages man.html:
	@for page in ${PAGES}; do make man.html/$${page}.html; done

htmlinclude man.html.include:
	@for page in ${PAGES}; do make man.html.include/$${page}.html; done

.PHONY: man manpages man.html man.html.include

clean:
	rm man/*.*
	rm man.html/*.html
	rm man.html.include/*.html
