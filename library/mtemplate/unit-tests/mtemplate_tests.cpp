/*
 * Copyright (c) 2016, 2017, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301  USA
 */

#include "wb_helpers.h"

#include "base/utf8string.h"
#include "mtemplate/template.h"
#include <fstream>

using namespace base;

TEST_MODULE(mtemplate_test, "mtemplate");

std::map<std::string, base::utf8string> language_details_map = {
  {"english", base::utf8string("I can eat glass and it doesn't hurt me. ")},
  {"Sanskrit", base::utf8string("काचं शक्नोम्यत्तुम् । नोपहिनस्ति माम् ॥")},
  {"Sanskrit (standard transcription)", base::utf8string("kācaṃ śaknomyattum; nopahinasti mām.")},
  {"Greek (polytonic)", base::utf8string("Μπορῶ νὰ φάω σπασμένα γυαλιὰ χωρὶς νὰ πάθω τίποτα.")},
  {"Spanish", base::utf8string("Puedo comer vidrio, no me hace daño.")},
  {"Portuguese", base::utf8string("Posso comer vidro, não me faz mal.")},
  {"Cornish", base::utf8string("Mý a yl dybry gwéder hag éf ny wra ow ankenya.")},
  {"Welsh", base::utf8string("Dw i'n gallu bwyta gwydr, 'dyw e ddim yn gwneud dolur i mi.")},
  {"Irish", base::utf8string("Is féidir liom gloinne a ithe. Ní dhéanann sí dochar ar bith dom.")},
  {"Anglo-Saxon (Runes)", base::utf8string("ᛁᚳ᛫ᛗᚨᚷ᛫ᚷᛚᚨᛋ᛫ᛖᚩᛏᚪᚾ᛫ᚩᚾᛞ᛫ᚻᛁᛏ᛫ᚾᛖ᛫ᚻᛖᚪᚱᛗᛁᚪᚧ᛫ᛗᛖ᛬")},
  {"Swedish", base::utf8string("Jag kan äta glas utan att skada mig.")},
  {"Czech", base::utf8string("Mohu jíst sklo, neublíží mi.")},
  {"Slovak", base::utf8string("Môžem jesť sklo. Nezraní ma.")},
  {"Polish", base::utf8string("Mogę jeść szkło i mi nie szkodzi.")},
  {"Russian", base::utf8string("Я могу есть стекло, оно мне не вредит.")},
  {"Hindi", base::utf8string("मैं काँच खा सकता हूँ और मुझे उससे कोई चोट नहीं पहुंचती.")},
  {"Tamil", base::utf8string("நான் கண்ணாடி சாப்பிடுவேன், அதனால் எனக்கு ஒரு கேடும் வராது.")},
  {"Chinese", base::utf8string("我能吞下玻璃而不伤身体。")},
  {"Japanese", base::utf8string("私はガラスを食べられます。それは私を傷つけません。")}};

bool compare_file_contents(const std::string &filename1, const std::string &filename2) {
  std::ifstream file1(filename1, std::ifstream::binary | std::ifstream::ate);
  std::ifstream file2(filename2, std::ifstream::binary | std::ifstream::ate);

  if (file1.fail() || file2.fail())
    return false; // file problem

  if (file1.tellg() != file2.tellg())
    return false; // size mismatch

  // seek back to beginning and use std::equal to compare contents
  file1.seekg(0, std::ifstream::beg);
  file2.seekg(0, std::ifstream::beg);
  return std::equal(std::istreambuf_iterator<char>(file1.rdbuf()), std::istreambuf_iterator<char>(),
                    std::istreambuf_iterator<char>(file2.rdbuf()));
}

// string escaper for CSV tokens, encloses fields with " if needed, depending on the separator
struct CSVTokenQuoteModifier : public mtemplate::Modifier {
  virtual base::utf8string modify(const base::utf8string &input, const base::utf8string arg = "") {
    base::utf8string search_for = " \"\t\r\n";
    base::utf8string result = input;

    if (arg == "=comma")
      search_for += ',';
    else if (arg == "=tab")
      search_for = '\t'; //  TODO: verify if this argument is ever used, since it is in the generic searches
    else if (arg == "=semicolon")
      search_for += ';';
    else
      search_for += ';';

    if (input.find_first_of(search_for) != std::string::npos) {
      base::replaceString(result, "\"", "\"\"");
      result = base::utf8string("\"") + result + base::utf8string("\"");
    }

    return result;
  }
};

struct SQLQuoteModifier : public mtemplate::Modifier {
  virtual base::utf8string modify(const base::utf8string &input, const base::utf8string arg = "") {
    return base::utf8string("\"") + input + base::utf8string("\"");
  }
};

TEST_FUNCTION(1) {
  base::create_directory("test_output", 0700);
  //    This test creates a CSV file from a template + the data above. Also tests the usage of a modifier
  {
    //    setup modifiers
    mtemplate::Modifier::addModifier<CSVTokenQuoteModifier>("csv_quote");

    //    create output streams
    mtemplate::TemplateOutputFile output("test_output/test_result.csv");

    { //   Header of the files
      mtemplate::Template *template_csv = mtemplate::GetTemplate("data/mtemplate/CSV_semicolon.pre.tpl");

      mtemplate::DictionaryInterface *dictionary = mtemplate::CreateMainDictionary();

      dictionary->addSectionDictionary("COLUMN")->setValue("COLUMN_NAME", "Language");
      dictionary->addSectionDictionary("COLUMN")->setValue("COLUMN_NAME", "Phrase");

      template_csv->expand(dictionary, &output);
    }

    { //   data
      mtemplate::Template *template_data = mtemplate::GetTemplate("data/mtemplate/CSV_semicolon.tpl");

      for (auto item : language_details_map) {
        mtemplate::DictionaryInterface *data_dictionary = mtemplate::CreateMainDictionary();
        mtemplate::DictionaryInterface *row_dictionary = data_dictionary->addSectionDictionary("ROW");

        mtemplate::DictionaryInterface *field_dictionary_col1 = row_dictionary->addSectionDictionary("FIELD");
        field_dictionary_col1->setValue("FIELD_VALUE", item.first);

        mtemplate::DictionaryInterface *field_dictionary_col2 = row_dictionary->addSectionDictionary("FIELD");
        field_dictionary_col2->setValue("FIELD_VALUE", item.second);

        template_data->expand(data_dictionary, &output);
      }
    }
  }

  ensure_true("Comparing CSV file",
              compare_file_contents("data/mtemplate/test_result.csv", "test_output/test_result.csv"));
}

TEST_FUNCTION(2) {
  {
    mtemplate::SetGlobalValue("INDENT", "\t");

    //    create output streams
    mtemplate::TemplateOutputFile output_json("test_output/test_result.json");

    //   Header of the files (no data)
    mtemplate::GetTemplate("data/mtemplate/JSON.pre.tpl")->expand(nullptr, &output_json);

    { //   data
      mtemplate::Template *data_template_json = mtemplate::GetTemplate("data/mtemplate/JSON.tpl");

      for (auto item : language_details_map) {
        mtemplate::DictionaryInterface *data_dictionary = mtemplate::CreateMainDictionary();
        mtemplate::DictionaryInterface *row_dictionary = data_dictionary->addSectionDictionary("ROW");

        mtemplate::DictionaryInterface *field_dictionary_col1 = row_dictionary->addSectionDictionary("FIELD");
        field_dictionary_col1->setValue("FIELD_NAME", "Language");
        field_dictionary_col1->setValue("FIELD_VALUE", item.first);

        mtemplate::DictionaryInterface *field_dictionary_col2 = row_dictionary->addSectionDictionary("FIELD");
        field_dictionary_col2->setValue("FIELD_NAME", "Phrase");
        field_dictionary_col2->setValue("FIELD_VALUE", item.second);

        data_template_json->expand(data_dictionary, &output_json);
      }
    }

    //   Footer for the files (no data)
    mtemplate::GetTemplate("data/mtemplate/JSON.post.tpl")->expand(nullptr, &output_json);
  }
  ensure_true("Comparing JSON file",
              compare_file_contents("data/mtemplate/test_result.json", "test_output/test_result.json"));
}

TEST_FUNCTION(3) {
  {
    mtemplate::SetGlobalValue("TABLE_NAME", "some_table");

    //    setup modifiers
    mtemplate::Modifier::addModifier<SQLQuoteModifier>("sql_quote");

    //    create output streams
    mtemplate::TemplateOutputFile output_json("test_output/test_result.sql");

    { //   Header of the files
      mtemplate::Template *header_json = mtemplate::GetTemplate("data/mtemplate/SQL_inserts.pre.tpl");
      mtemplate::DictionaryInterface *dictionary = mtemplate::CreateMainDictionary();
      header_json->expand(dictionary, &output_json);
    }

    { //   data
      mtemplate::Template *data_template_json = mtemplate::GetTemplate("data/mtemplate/SQL_inserts.tpl");

      for (auto item : language_details_map) {
        mtemplate::DictionaryInterface *data_dictionary = mtemplate::CreateMainDictionary();
        mtemplate::DictionaryInterface *row_dictionary = data_dictionary->addSectionDictionary("ROW");

        mtemplate::DictionaryInterface *field_dictionary_col1 = row_dictionary->addSectionDictionary("FIELD");
        field_dictionary_col1->setValue("FIELD_NAME", "Language");
        field_dictionary_col1->setValue("FIELD_VALUE", item.first);

        mtemplate::DictionaryInterface *field_dictionary_col2 = row_dictionary->addSectionDictionary("FIELD");
        field_dictionary_col2->setValue("FIELD_NAME", "Phrase");
        field_dictionary_col2->setValue("FIELD_VALUE", item.second);

        data_template_json->expand(data_dictionary, &output_json);
      }
    }
  }

  ensure_true("Comparing SQL file",
              compare_file_contents("data/mtemplate/test_result.sql", "test_output/test_result.sql"));
}

TEST_FUNCTION(4) {
  {
    //    create output streams
    mtemplate::TemplateOutputFile output_json("test_output/test_result.html");

    { //   Header of the files
      mtemplate::Template *template_json = mtemplate::GetTemplate("data/mtemplate/HTML.pre.tpl");
      mtemplate::DictionaryInterface *dictionary = mtemplate::CreateMainDictionary();
      dictionary->setValueAndShowSection("COLUMN_NAME", "Language", "COLUMN");
      dictionary->setValueAndShowSection("COLUMN_NAME", "Phrase", "COLUMN");
      template_json->expand(dictionary, &output_json);
    }

    { //   data
      mtemplate::Template *data_template_json = mtemplate::GetTemplate("data/mtemplate/HTML.tpl");

      for (auto item : language_details_map) {
        mtemplate::DictionaryInterface *data_dictionary = mtemplate::CreateMainDictionary();
        mtemplate::DictionaryInterface *row_dictionary = data_dictionary->addSectionDictionary("ROW");

        row_dictionary->setValueAndShowSection("FIELD_VALUE", item.first, "FIELD");
        row_dictionary->setValueAndShowSection("FIELD_VALUE", item.second, "FIELD");

        data_template_json->expand(data_dictionary, &output_json);
      }
    }

    //   Footer for the files(no data)
    mtemplate::GetTemplate("data/mtemplate/HTML.post.tpl")->expand(nullptr, &output_json);
  }

  ensure_true("Comparing HTML file",
              compare_file_contents("data/mtemplate/test_result.html", "test_output/test_result.html"));
}

END_TESTS
