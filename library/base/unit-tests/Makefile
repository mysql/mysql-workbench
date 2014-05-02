TEST_TIMEOUT=60
top_srcdir=../../../

#COV_CFLAGS=-fprofile-arcs -ftest-coverage

CXXFLAGS=-Wall -Wno-deprecated -Wno-unknown-pragmas -ggdb3 -fpermissive $(COV_CFLAGS)
INCLUDES=-Iinclude `mysql_config --cflags` \
	-I$(top_srcdir)/\
	-I$(top_srcdir)/testing/tut/include\
	-I$(top_srcdir)/modules/interfaces\
	-I$(top_srcdir)/modules\
	-I$(top_srcdir)/modules/wb.model/src\
	-I$(top_srcdir)/generated/\
	-I$(top_srcdir)/library/base/\
	-DENABLE_LUA_MODULES -DEXCLUDE_MYSQL_SUPPORT \
	 `pkg-config glib-2.0 libxml-2.0 sigc++-2.0 cairo --cflags` `pcre-config --cflags` \
	-DIMAGE_TESTS ${FLAGS}
#	 `pkg-config glib-2.0 libxml-2.0 sigc++-2.0 cairo glitz-glx --cflags` `pcre-config --cflags` 


%.o: %.cpp
	g++ $< -c $(INCLUDES) $(CXXFLAGS) -DTEST_STANDALONE

%.o: $(top_srcdir)/testing/tut/source/%.cpp
	g++ $< -c $(INCLUDES) $(CXXFLAGS) -DTEST_STANDALONE

TUT_FILES=test_helpers.o  main.o 

base_test: base_test.o $(TUT_FILES)
	g++ $< $(TUT_FILES) ../.libs/libwbbase.so -o$@ `pkg-config sigc++-2.0 --libs`


clean:
	rm *.o
