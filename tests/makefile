DYTEST="LD_LIBRARY_PATH=../../libs DYLD_LIBRARY_PATH=../../libs ./dynamic/"
DOTEST=./

tests:
	# Test u8record and u8xrecode from latin to utf8
	${DOTEST}u8recode latin1 utf8 < data/latin1.text > tmp/utf8.text
	diff data/utf8.text tmp/utf8.text
	${DOTEST}u8xrecode latin1 utf8 < data/latin1.text > tmp/utf8.text
	diff data/utf8.text tmp/utf8.text
	# Test going from utf8 to latin
	${DOTEST}u8xrecode utf8 latin1 < data/utf8.text > tmp/latin1.text
	diff data/latin1.text tmp/latin1.text
	${DOTEST}u8recode utf8 latin1 < data/utf8.text > tmp/latin1.text
	diff data/latin1.text tmp/latin1.text
	# Test roundtrip from latin1 through latin3
	${DOTEST}u8xrecode latin1 latin3 < data/latin1.text > tmp/latin3.text
	${DOTEST}u8xrecode latin3 latin1 < tmp/latin3.text > tmp/latin1.text
	diff data/latin1.text tmp/latin1.text

	u8xrecode latin3 utf8 < tmp/latin3.text > tmp/utf8.text
	diff data/utf8.text tmp/utf8.text
dytests:
	make DOTEST=${DYTEST} tests

clean:
	rm -f tmp/*.text
