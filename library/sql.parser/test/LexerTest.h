#pragma once

class LexerTestException {};

class LexerTest
{
  void cppunit_assert(bool cond);

  void stmtscanTest();
  void basicLexerTest();
  void sqlTest();

  void basicParserTest();

  void fileParse(const char* fileName);


public:
  LexerTest() {}
  virtual ~LexerTest() {}

  void testLexer();

  void stringParse(const char* str);
};
