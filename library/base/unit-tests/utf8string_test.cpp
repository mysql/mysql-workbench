#include <base/utf8string.h>
#include <utility>
#include "wb_helpers.h"

using namespace base;


TEST_MODULE(utf8string_test, "utf8string");

const base::utf8string GlobalTestStringEnglish(u8"This is a lazy test");    //  string for non-unicode tests
const base::utf8string GlobalTestStringPolish(u8"zażółć");
const base::utf8string GlobalTestStringGreek(u8"ὕαλον ϕαγεῖν");
const base::utf8string GlobalTestStringRussian(u8"Я могу есть стекло");
const base::utf8string GlobalTestStringArabic(u8"هذا لا يؤلمني");
const base::utf8string GlobalTestStringChinese(u8"我可以吞下茶");
const base::utf8string GlobalTestStringJapanese(u8"私はお茶を飲み込むことができます");
const base::utf8string GlobalTestStringPortuguese(u8"Há açores e cães ávidos no chão");

struct lang_string_details
{
  const base::utf8string _text;
  size_t _length;
  size_t _bytes;
};

// u8"هذا لا يؤلمني"


std::map<std::string, lang_string_details> LanguageStrings = 
{
  { "english"   , {base::utf8string(u8"This is a lazy test"), 19, 19} }
, { "polish"    , {base::utf8string(u8"zażółć"), 6, 10 } }
, { "greek"     , {base::utf8string(u8"ὕαλον ϕαγεῖν"), 12, 25 } }
, { "russian"   , {base::utf8string(u8"Я могу есть стекло"), 18, 33 } }      
//, { "arabic"    , {base::utf8string(u8"هذا لا يؤلمني")    }
, { "chinese"   , {base::utf8string(u8"我可以吞下茶"), 6, 18 } }
, { "japanese"  , {base::utf8string(u8"私はお茶を飲み込むことができます"), 16, 48 } }
, { "portuguese", {base::utf8string(u8"Há açores e cães ávidos no chão"), 31, 36 } }
};

/*
 * Testing empty constructors
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

TEST_FUNCTION(6)
{
  ensure_equals("TEST 6.1 - utf8string(const utf8string &str, size_t pos, size_t len)", base::utf8string(LanguageStrings["polish"]._text, 1, 4), base::utf8string(u8"ażół"));
  ensure_equals("TEST 6.2 - utf8string(const utf8string &s)", base::utf8string(LanguageStrings["polish"]._text), LanguageStrings["polish"]._text);
  ensure_equals("TEST 6.3 - utf8string(size_t size, char c)", base::utf8string(10, 'a'), "aaaaaaaaaa");
  ensure_equals("TEST 6.3 - utf8string(size_t size, utf8char c)", base::utf8string(10, base::utf8string::utf8char(u8"ł")), "łłłłłłłłłł");

}

/*
 * Testing utf8string created from char*
 */
TEST_FUNCTION(10)
{
//   std::string test_name_base = "TEST 10.";
//   for(auto iter : LanguageStrings)
//   {
//     std::cout << iter.first << std::endl;
//     ensure_equals("TEST 10.1: has length", GlobalTestStringEnglish.length(), (size_t)19);
//     ensure_equals("TEST 10.2: has bytes", GlobalTestStringEnglish.bytes(), (size_t)19);
//     ensure_equals("TEST 10.3: empty", GlobalTestStringEnglish.empty(), false);
//   
// //     ++count;
//   }
  ensure_equals("TEST 10.1: has length", GlobalTestStringEnglish.length(), (size_t)19);
  ensure_equals("TEST 10.2: has bytes", GlobalTestStringEnglish.bytes(), (size_t)19);
  ensure_equals("TEST 10.3: empty", GlobalTestStringEnglish.empty(), false);

  ensure_equals("TEST 10.1: has length", GlobalTestStringPolish.length(), (size_t)6);
  ensure_equals("TEST 10.2: has bytes", GlobalTestStringPolish.bytes(), (size_t)10);
  ensure_equals("TEST 10.3: empty", GlobalTestStringPolish.empty(), false);

  ensure_equals("TEST 10.4: has length", GlobalTestStringGreek.length(), (size_t)12);
  ensure_equals("TEST 10.5: has bytes", GlobalTestStringGreek.bytes(), (size_t)25);
  ensure_equals("TEST 10.6: empty", GlobalTestStringGreek.empty(), false);

  ensure_equals("TEST 10.7: has length", GlobalTestStringRussian.length(), (size_t)18);
  ensure_equals("TEST 10.8: has bytes", GlobalTestStringRussian.bytes(), (size_t)33);
  ensure_equals("TEST 10.9: empty", GlobalTestStringRussian.empty(), false);

  ensure_equals("TEST 10.10: has length", GlobalTestStringArabic.length(), (size_t)13);
  ensure_equals("TEST 10.11: has bytes", GlobalTestStringArabic.bytes(), (size_t)24);
  ensure_equals("TEST 10.12: empty", GlobalTestStringArabic.empty(), false);

  ensure_equals("TEST 10.13: has length", GlobalTestStringChinese.length(), (size_t)6);
  ensure_equals("TEST 10.14: has bytes", GlobalTestStringChinese.bytes(), (size_t)18);
  ensure_equals("TEST 10.15: empty", GlobalTestStringChinese.empty(), false);

  ensure_equals("TEST 10.16: has length", GlobalTestStringJapanese.length(), (size_t)16);
  ensure_equals("TEST 10.17: has bytes", GlobalTestStringJapanese.bytes(), (size_t)48);
  ensure_equals("TEST 10.18: empty", GlobalTestStringJapanese.empty(), false);

  ensure_equals("TEST 10.19: has length", GlobalTestStringPortuguese.length(), (size_t)31);
  ensure_equals("TEST 10.20: has bytes", GlobalTestStringPortuguese.bytes(), (size_t)36);
  ensure_equals("TEST 10.21: empty", GlobalTestStringPortuguese.empty(), false);
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
   ensure_equals("TEST 40.2: toString", str.to_string() == std::string("zażółć"), true);
   ensure_equals("TEST 40.3: toWString", str.to_wstring() == std::wstring(L"zażółć"), true);
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
   ensure_equals("TEST 55.1: toUpper",  base::utf8string("zażółć").to_upper(), base::utf8string("ZAŻÓŁĆ"));
   ensure_equals("TEST 55.1: toLower",  base::utf8string("ZAŻÓŁĆ").to_lower(), base::utf8string("zażółć"));
   
   ensure_equals("TEST 55.4: tocaseFold",  base::utf8string("zAżóŁć").to_case_fold(), base::utf8string("zażółć"));
   ensure_equals("TEST 55.5: validate",  base::utf8string("grüßen").validate(), true);
}

/*
 * Testing functions truncate, substr, left, right
 */
TEST_FUNCTION(56)
{
   ensure_equals("TEST 56.1: truncate",  base::utf8string("zażółć").truncate(0), base::utf8string("..."));
   ensure_equals("TEST 56.1: truncate",  base::utf8string("zażółć").truncate(1), base::utf8string("z..."));
   ensure_equals("TEST 56.1: truncate",  base::utf8string("zażółć").truncate(2), base::utf8string("za..."));
   ensure_equals("TEST 56.2: truncate",  base::utf8string("zażółć").truncate(3), "zażółć");
   ensure_equals("TEST 56.3: truncate",  base::utf8string("zażółć").truncate(4), "zażółć");
   ensure_equals("TEST 56.4: truncate",  base::utf8string("zażółć").truncate(5), "zażółć");
   ensure_equals("TEST 56.5: truncate",  base::utf8string("zażółć").truncate(6), "zażółć");
   ensure_equals("TEST 56.6: truncate",  base::utf8string("zażółć").truncate(7), "zażółć");
   
   ensure_equals("TEST 56.6: left",  base::utf8string("zażółć").left(0), "");
   ensure_equals("TEST 56.6: left",  base::utf8string("zażółć").left(1), "z");
   ensure_equals("TEST 56.6: left",  base::utf8string("zażółć").left(2), "za");
   ensure_equals("TEST 56.6: left",  base::utf8string("zażółć").left(3), "zaż");
   ensure_equals("TEST 56.6: left",  base::utf8string("zażółć").left(4), "zażó");
   ensure_equals("TEST 56.6: left",  base::utf8string("zażółć").left(5), "zażół");
   ensure_equals("TEST 56.6: left",  base::utf8string("zażółć").left(6), "zażółć");
   ensure_equals("TEST 56.6: left",  base::utf8string("zażółć").left(7), "zażółć");
   
   ensure_equals("TEST 56.6: right",  base::utf8string("zażółć").right(0), "");
   ensure_equals("TEST 56.6: right",  base::utf8string("zażółć").right(1), "ć");
   ensure_equals("TEST 56.6: right",  base::utf8string("zażółć").right(2), "łć");
   ensure_equals("TEST 56.6: right",  base::utf8string("zażółć").right(3), "ółć");
   ensure_equals("TEST 56.6: right",  base::utf8string("zażółć").right(4), "żółć");
   ensure_equals("TEST 56.6: right",  base::utf8string("zażółć").right(5), "ażółć");
   ensure_equals("TEST 56.6: right",  base::utf8string("zażółć").right(6), "zażółć");
   ensure_equals("TEST 56.6: right",  base::utf8string("zażółć").right(7), "zażółć");
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
