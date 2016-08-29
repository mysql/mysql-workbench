#include <base/utf8string.h>
#include <utility>
#include "wb_helpers.h"

using namespace base;


TEST_MODULE(utf8string_test, "utf8string");


/*
 * Testing empyty constructors
 */
TEST_FUNCTION(5)
{
  base::utf8string str1;
  ensure_equals("TEST 5.1: has length", str1.length(), (size_t)0);
  ensure_equals("TEST 5.2: has bytes", str1.bytes(), (size_t)0);
  ensure_equals("TEST 5.3: empty", str1.empty(), true);

  base::utf8string str2 = "";
  ensure_equals("TEST 5.4: has length", str2.length(), (size_t)0);
  ensure_equals("TEST 5.5: has bytes", str2.bytes(), (size_t)0);
  ensure_equals("TEST 5.6: empty", str2.empty(), true);
}

/*
 * Testing utf8string created from char*
 */
TEST_FUNCTION(10)
{
  base::utf8string str = u8"zażółć";
  ensure_equals("TEST 10.1: has length", str.length(), (size_t)6);
  ensure_equals("TEST 10.2: has bytes", str.bytes(), (size_t)10);
  ensure_equals("TEST 10.3: empty", str.empty(), false);
  str = u8"ὕαλον ϕαγεῖν";
  ensure_equals("TEST 10.4: has length", str.length(), (size_t)12);
  ensure_equals("TEST 10.5: has bytes", str.bytes(), (size_t)25);
  ensure_equals("TEST 10.6: empty", str.empty(), false);
  str = u8"Я могу есть стекло";
  ensure_equals("TEST 10.7: has length", str.length(), (size_t)18);
  ensure_equals("TEST 10.8: has bytes", str.bytes(), (size_t)33);
  ensure_equals("TEST 10.9: empty", str.empty(), false);
  str = u8"هذا لا يؤلمني";
  ensure_equals("TEST 10.10: has length", str.length(), (size_t)13);
  ensure_equals("TEST 10.11: has bytes", str.bytes(), (size_t)24);
  ensure_equals("TEST 10.12: empty", str.empty(), false);
  str = u8"我能吞";
  ensure_equals("TEST 10.13: has length", str.length(), (size_t)3);
  ensure_equals("TEST 10.14: has bytes", str.bytes(), (size_t)9);
  ensure_equals("TEST 10.15: empty", str.empty(), false);
}

/*
 * Testing utf8string created from std::string
 */
TEST_FUNCTION(15)
{
   base::utf8string str = std::string("zażółć");
   ensure_equals("TEST 15.1: has length", str.length(), (size_t)6);
   ensure_equals("TEST 15.2: has bytes", str.bytes(), (size_t)10);
   ensure_equals("TEST 15.3: empty", str.empty(), false);
   str = std::string("ὕαλον ϕαγεῖν");
   ensure_equals("TEST 15.4: has length", str.length(), (size_t)12);
   ensure_equals("TEST 15.5: has bytes", str.bytes(), (size_t)25);
   ensure_equals("TEST 15.6: empty", str.empty(), false);
   str = std::string("Я могу есть стекло");
   ensure_equals("TEST 15.7: has length", str.length(), (size_t)18);
   ensure_equals("TEST 15.8: has bytes", str.bytes(), (size_t)33);
   ensure_equals("TEST 15.9: empty", str.empty(), false);
   str = std::string("هذا لا يؤلمني");
   ensure_equals("TEST 15.10: has length", str.length(), (size_t)13);
   ensure_equals("TEST 15.11: has bytes", str.bytes(), (size_t)24);
   ensure_equals("TEST 15.12: empty", str.empty(), false);
   str = std::string("我能吞");
   ensure_equals("TEST 15.13: has length", str.length(), (size_t)3);
   ensure_equals("TEST 15.14: has bytes", str.bytes(), (size_t)9);
   ensure_equals("TEST 15.15: empty", str.empty(), false);
}

/*
 * Testing utf8string copy constructor
 */
TEST_FUNCTION(20)
{
  base::utf8string str1 = std::string("zażółć");
  base::utf8string str2 = str1;
  ensure_equals("TEST 20.1: has length", str2.length(), (size_t)6);
  ensure_equals("TEST 20.2: has bytes", str2.bytes(), (size_t)10);
  ensure_equals("TEST 20.3: empty", str2.empty(), false);

}

/*
 * Testing utf8string operator[]
 */
TEST_FUNCTION(25)
{
  base::utf8string str = std::string("zażółć");
  base::utf8string::utf8char res1("ó");
  base::utf8string::utf8char res2("ć");
  ensure_equals("TEST 25.1: index of 3", str[3], res1);
  ensure_equals("TEST 25.2: index of 5", str[5], res2);
}

/*
 * Testing utf8string substr
 */
TEST_FUNCTION(30)
{
  base::utf8string str = std::string("zażółć");
  base::utf8string res1 = "żółć";
  base::utf8string res2 = "aż";
  ensure_equals("TEST 30.1: substr 2", str.substr(2), res1);
  ensure_equals("TEST 30.2: substr 1,2", str.substr(1, 2), res2);
}

/*
 * Testing utf8string operators +, +=, =, ==, !=
 */
TEST_FUNCTION(35)
{
  base::utf8string str1 = std::string("zażółć");
  base::utf8string str2 = std::string("gęślą");
  base::utf8string result = std::string("zażółćgęślą");

  ensure_equals("TEST 35.1: operator +", str1 + str2, result);
  str1 += str2;
  ensure_equals("TEST 35.2: operator +=", str1, result);
  str1 = str2;
  ensure_equals("TEST 35.3: operator =", str1, str2);
  ensure_equals("TEST 35.4: operator ==", str1 == str2, true);
  ensure_equals("TEST 35.5: operator !=", str1 != result, true);
}

/*
 * Testing functions c_str, toString, toWString
 */
TEST_FUNCTION(40)
{
   base::utf8string str = std::string("zażółć");
   ensure_equals("TEST 40.1: c_str", strcmp(str.c_str(), "zażółć"), 0);
   ensure_equals("TEST 40.2: toString", str.toString() == std::string("zażółć"), true);
   ensure_equals("TEST 40.3: toWString", str.toWString() == std::wstring(L"zażółć"), true);
}

/*
 * Testing move constructor and operator=
 */
TEST_FUNCTION(45)
{
   base::utf8string str1(std::string("za") + std::string("żółć")) ;
   ensure_equals("TEST 45.1: has length", str1.length(), (size_t)6);
   ensure_equals("TEST 45.2: has bytes", str1.bytes(), (size_t)10);
   ensure_equals("TEST 45.3: empty", str1.empty(), false);

   base::utf8string str2 = std::move(str1);
   ensure_equals("TEST 45.4: str2 has length", str2.length(), (size_t)6);
   ensure_equals("TEST 45.5: str2 has bytes", str2.bytes(), (size_t)10);
   ensure_equals("TEST 45.6: str2 empty", str2.empty(), false);
}

/*
 * Testing trim* functions
 */
TEST_FUNCTION(50)
{
   ensure_equals("TEST 50.1: has length", base::utf8string("  zażółć    ").trim_left().length(), (size_t)10);
   ensure_equals("TEST 50.2: has bytes", base::utf8string("  zażółć    ").trim_left().bytes(), (size_t)14);
   ensure_equals("TEST 50.3: has length", base::utf8string("  zażółć    ").trim_right().length(), (size_t)8);
   ensure_equals("TEST 50.4: has bytes", base::utf8string("  zażółć    ").trim_right().bytes(), (size_t)12);
   ensure_equals("TEST 50.5: has length", base::utf8string("  zażółć    ").trim().length(), (size_t)6);
   ensure_equals("TEST 50.6: has bytes", base::utf8string("  zażółć    ").trim().bytes(), (size_t)10);
}

/*
 * Testing functions toUpper, toLower, truncate, toCaseFold, validate
 */
TEST_FUNCTION(55)
{
   ensure_equals("TEST 55.1: toUpper",  base::utf8string("zażółć").toUpper(), base::utf8string("ZAŻÓŁĆ"));
   ensure_equals("TEST 55.1: toLower",  base::utf8string("ZAŻÓŁĆ").toLower(), base::utf8string("zażółć"));
   ensure_equals("TEST 55.2: truncate",  base::utf8string("zażółć").truncate(5), base::utf8string("zaż..."));
   ensure_equals("TEST 55.3: truncate",  base::utf8string("zażółć").truncate(7), base::utf8string("zażółć"));
   ensure_equals("TEST 55.4: tocaseFold",  base::utf8string("zAżóŁć").toCaseFold(), base::utf8string("zażółć"));
   ensure_equals("TEST 55.5: validate",  base::utf8string("grüßen").validate(), true);
}

/*
 * Testing functions contains*
 */
TEST_FUNCTION(60)
{
   base::utf8string str = std::string("zażółć");
   ensure_equals("TEST 60.1: starts_with", str.starts_with("za"), true);
   ensure_equals("TEST 60.2: starts_with", str.starts_with("kk"), false);
   ensure_equals("TEST 60.3: starts_with", str.starts_with("toolongstring"), false);
   ensure_equals("TEST 60.4: ends_with", str.ends_with("ółć"), true);
   ensure_equals("TEST 60.5: ends_with", str.ends_with("ÓŁa"), false);
   ensure_equals("TEST 60.6: ends_with", str.ends_with("toolongstring"), false);
   ensure_equals("TEST 60.7: contains", str.contains("żół"), true);
   ensure_equals("TEST 60.8: contains", str.contains("ŻÓŁ"), false);
   ensure_equals("TEST 60.9: contains", str.contains("ŻÓŁ", false), true);
   ensure_equals("TEST 60.10: contains", str.contains("", false), false);
}

/*
 * Testing functions charIndexToByteOffset, byteOffsetToCharIndex
 */
TEST_FUNCTION(65)
{
   base::utf8string str = std::string("zażółć");
   ensure_equals("TEST 65.1: charIndexToByteOffset", str.charIndexToByteOffset(2), (size_t)2);
   ensure_equals("TEST 65.2: charIndexToByteOffset", str.charIndexToByteOffset(3), (size_t)4);
   ensure_equals("TEST 65.3: charIndexToByteOffset", str.charIndexToByteOffset(4), (size_t)6);
   ensure_equals("TEST 65.4: byteOffsetToCharIndex", str.byteOffsetToCharIndex(2), (size_t)2);
   ensure_equals("TEST 65.5: byteOffsetToCharIndex", str.byteOffsetToCharIndex(5), (size_t)4);
   ensure_equals("TEST 65.6: byteOffsetToCharIndex", str.byteOffsetToCharIndex(6), (size_t)4);
}

/*
 * Testing itertaor
 */
TEST_FUNCTION(70)
{
   base::utf8string str = std::string("zażółć");
   base::utf8string::iterator it = str.begin();
   ensure_equals("TEST 70.1: begin", *it, base::utf8string::utf8char("z"));
   ensure_equals("TEST 70.1: == begin", it == str.begin(), true);
   ensure_equals("TEST 70.2: != end", it != str.end(), true);
   ++it;
   ensure_equals("TEST 70.3: ++it", *it, base::utf8string::utf8char("a"));
   for (size_t i = 0; i < 5; i++) {
     ++it;
   }
   ensure_equals("TEST 70.4: ==end", it == str.end(), true);
   --it;
   ensure_equals("TEST 70.5: --it", *it, base::utf8string::utf8char("ć"));
}

END_TESTS
