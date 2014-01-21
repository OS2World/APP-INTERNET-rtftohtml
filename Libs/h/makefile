all:	rtf.h stdcharnames.h

rtf.h:  rtf.h.dist standard-names
	awk -f stdnames.awk standard-names|cat rtf.h.dist - > $@

clean::
	rm -f rtf.h stdcharnames.h

stdcharnames.h:: rtf.h.dist standard-names
	sed -e '/^$$/d' -e '/^#/d' -e 's/.*/ "&",/' \
                standard-names > $@


