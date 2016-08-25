.PHONY:all debug release test

all: debug

debug:
	blade build ... -p debug --verbose

release:
	blade build ... --verbose

test:
	blade test -p debug --verbose
