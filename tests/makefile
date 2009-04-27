tests:
	u8recode latin1 utf8 < latin1.text > tmp/utf8.text
	diff utf8.text tmp/utf8.text
	u8xrecode latin1 utf8 < latin1.text > tmp/utf8.text
	diff utf8.text tmp/utf8.text
	u8xrecode utf8 latin1 < utf8.text > tmp/latin1.text
	diff latin1.text tmp/latin1.text
	u8xrecode latin1 latin3 < latin1.text > tmp/latin3.text
	u8xrecode latin3 latin1 < tmp/latin3.text > tmp/latin1.text
	diff latin1.text tmp/latin1.text
	u8xrecode latin3 utf8 < tmp/latin3.text > tmp/utf8.text
	diff utf8.text tmp/utf8.text
