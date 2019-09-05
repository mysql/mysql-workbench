/*
 * Copyright (c) 2018, 2019, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is also distributed with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms, as
 * designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have included with MySQL.
 * This program is distributed in the hope that it will be useful,  but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "base/utf8string.h"
#include "mtemplate/template.h"
#include "base/string_utilities.h"
#include <fstream>

#include "casmine.h"

using namespace casmine;

namespace {

$ModuleEnvironment() {};

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

$TestData {
  std::string outputDir = CasmineContext::get()->outputDir();
  std::string dataDir = CasmineContext::get()->tmpDataDir();

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
    {"Japanese", base::utf8string("私はガラスを食べられます。それは私を傷つけません。")}
  };

};

$describe("mtemplate") {

  $it("Create CSV from template", [this]() {
    //    This test creates a CSV file from a template + the data above. Also tests the usage of a modifier
    {
      //    setup modifiers
      mtemplate::Modifier::addModifier<CSVTokenQuoteModifier>("csv_quote");

      //    create output streams
      mtemplate::TemplateOutputFile output(data->outputDir + "/test_result.csv");

      { //   Header of the files
        mtemplate::Template *template_csv = mtemplate::GetTemplate(data->dataDir + "/mtemplate/CSV_semicolon.pre.tpl");

        mtemplate::DictionaryInterface *dictionary = mtemplate::CreateMainDictionary();

        dictionary->addSectionDictionary("COLUMN")->setValue("COLUMN_NAME", "Language");
        dictionary->addSectionDictionary("COLUMN")->setValue("COLUMN_NAME", "Phrase");

        template_csv->expand(dictionary, &output);
      }

      { //   data
        mtemplate::Template *template_data = mtemplate::GetTemplate(data->dataDir + "/mtemplate/CSV_semicolon.tpl");

        for (auto item : data->language_details_map) {
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

    $expect(compare_file_contents(data->dataDir + "/mtemplate/test_result.csv", data->outputDir + "/test_result.csv")).toBeTrue();
  });

  $it("Create JSON from template", [this]() {
    {
      mtemplate::SetGlobalValue("INDENT", "\t");

      //    create output streams
      mtemplate::TemplateOutputFile output_json(data->outputDir + "/test_result.json");

      //   Header of the files (no data)
      mtemplate::GetTemplate(data->dataDir + "/mtemplate/JSON.pre.tpl")->expand(nullptr, &output_json);

      { //   data
        mtemplate::Template *data_template_json = mtemplate::GetTemplate(data->dataDir + "/mtemplate/JSON.tpl");

        for (auto item : data->language_details_map) {
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
      mtemplate::GetTemplate(data->dataDir + "/mtemplate/JSON.post.tpl")->expand(nullptr, &output_json);
    }
    $expect(compare_file_contents(data->dataDir + "/mtemplate/test_result.json", data->outputDir + "/test_result.json")).toBeTrue();
  });

  $it("Create SQL from template", [this]() {
    {
      mtemplate::SetGlobalValue("TABLE_NAME", "some_table");

      //    setup modifiers
      mtemplate::Modifier::addModifier<SQLQuoteModifier>("sql_quote");

      //    create output streams
      mtemplate::TemplateOutputFile output_json(data->outputDir + "/test_result.sql");

      { //   Header of the files
        mtemplate::Template *header_json = mtemplate::GetTemplate(data->dataDir + "/mtemplate/SQL_inserts.pre.tpl");
        mtemplate::DictionaryInterface *dictionary = mtemplate::CreateMainDictionary();
        header_json->expand(dictionary, &output_json);
      }

      { //   data
        mtemplate::Template *data_template_json = mtemplate::GetTemplate(data->dataDir + "/mtemplate/SQL_inserts.tpl");

        for (auto item : data->language_details_map) {
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

    $expect(compare_file_contents(data->dataDir + "/mtemplate/test_result.sql", data->outputDir + "/test_result.sql")).toBeTrue();
  });

  $it("Create HTML from template", [this]() {
    { // Need outer braces to make output_json flush its data to disk.
      //    create output streams
      mtemplate::TemplateOutputFile output_json(data->outputDir + "/test_result.html");

      { //   Header of the files
        std::unique_ptr<mtemplate::Template> template_json(mtemplate::GetTemplate(data->dataDir + "/mtemplate/HTML.pre.tpl"));
        std::unique_ptr<mtemplate::DictionaryInterface> dictionary(mtemplate::CreateMainDictionary());
        dictionary->setValueAndShowSection("COLUMN_NAME", "Language", "COLUMN");
        dictionary->setValueAndShowSection("COLUMN_NAME", "Phrase", "COLUMN");
        template_json->expand(dictionary.get(), &output_json);
      }

      { //   data
        std::unique_ptr<mtemplate::Template> data_template_json(mtemplate::GetTemplate(data->dataDir + "/mtemplate/HTML.tpl"));

        for (auto item : data->language_details_map) {
          std::unique_ptr<mtemplate::DictionaryInterface> data_dictionary(mtemplate::CreateMainDictionary());
          std::unique_ptr<mtemplate::DictionaryInterface> row_dictionary(data_dictionary->addSectionDictionary("ROW"));

          row_dictionary->setValueAndShowSection("FIELD_VALUE", item.first, "FIELD");
          row_dictionary->setValueAndShowSection("FIELD_VALUE", item.second, "FIELD");

          data_template_json->expand(data_dictionary.get(), &output_json);
        }
      }

      //   Footer for the files(no data)
      mtemplate::GetTemplate(data->dataDir + "/mtemplate/HTML.post.tpl")->expand(nullptr, &output_json);
    }

    $expect(compare_file_contents(data->dataDir + "/mtemplate/test_result.html", data->outputDir + "/test_result.html")).toBeTrue();
  });

}

}
