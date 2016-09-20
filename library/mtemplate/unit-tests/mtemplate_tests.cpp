/* 
 * Copyright (c) 2016, Oracle and/or its affiliates. All rights reserved.
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

#include <base/utf8string.h>
#include "../dictionary.h"
#include "mtemplate/template.h"


using namespace base;


TEST_MODULE(mtemplate_test, "mtemplate");


std::map<std::string, base::utf8string> language_details_map = 
{
  {"english", base::utf8string(u8"I can eat glass and it doesn't hurt me. ")}
, {"Sanskrit", base::utf8string(u8"काचं शक्नोम्यत्तुम् । नोपहिनस्ति माम् ॥")}
, {"Sanskrit (standard transcription)", base::utf8string(u8"kācaṃ śaknomyattum; nopahinasti mām.")}
, {"Greek (polytonic)", base::utf8string(u8"Μπορῶ νὰ φάω σπασμένα γυαλιὰ χωρὶς νὰ πάθω τίποτα.")}
, {"Spanish", base::utf8string(u8"Puedo comer vidrio, no me hace daño.")}
, {"Portuguese", base::utf8string(u8"Posso comer vidro, não me faz mal.")}
, {"Cornish", base::utf8string(u8"Mý a yl dybry gwéder hag éf ny wra ow ankenya.")}
, {"Welsh", base::utf8string(u8"Dw i'n gallu bwyta gwydr, 'dyw e ddim yn gwneud dolur i mi.")}
, {"Irish", base::utf8string(u8"Is féidir liom gloinne a ithe. Ní dhéanann sí dochar ar bith dom.")}
, {"Anglo-Saxon (Runes)", base::utf8string(u8"ᛁᚳ᛫ᛗᚨᚷ᛫ᚷᛚᚨᛋ᛫ᛖᚩᛏᚪᚾ᛫ᚩᚾᛞ᛫ᚻᛁᛏ᛫ᚾᛖ᛫ᚻᛖᚪᚱᛗᛁᚪᚧ᛫ᛗᛖ᛬")}
, {"Swedish", base::utf8string(u8"Jag kan äta glas utan att skada mig.")}
, {"Czech", base::utf8string(u8"Mohu jíst sklo, neublíží mi.")}
, {"Slovak", base::utf8string(u8"Môžem jesť sklo. Nezraní ma.")}
, {"Polish", base::utf8string(u8"Mogę jeść szkło i mi nie szkodzi.")}
, {"Russian", base::utf8string(u8"Я могу есть стекло, оно мне не вредит.")}
, {"Hindi", base::utf8string(u8"मैं काँच खा सकता हूँ और मुझे उससे कोई चोट नहीं पहुंचती.")}
, {"Tamil", base::utf8string(u8"நான் கண்ணாடி சாப்பிடுவேன், அதனால் எனக்கு ஒரு கேடும் வராது.")}
, {"Chinese", base::utf8string(u8"我能吞下玻璃而不伤身体。")}
, {"Japanese", base::utf8string(u8"私はガラスを食べられます。それは私を傷つけません。")}
};


// string escaper for CSV tokens, encloses fields with " if needed, depending on the separator
struct CSVTokenQuoteModifier : public mtemplate::Modifier
{
  virtual base::utf8string modify(const base::utf8string &input, const base::utf8string arg = "")
  {
    base::utf8string search_for = " \"\t\r\n";
    base::utf8string result = input;
    
    if (arg == "=comma")
      search_for += ',';
    else if (arg == "=tab")
      search_for = '\t';        //  TODO: verify if this argument is ever used, since it is in the generic searches
    else if (arg == "=semicolon")
      search_for += ';';
    else
      search_for += ';';
    
    if (input.find_first_of(search_for) != std::string::npos)
    {
      base::replaceString(result, "\"", "\"\"");
      result = base::utf8string("\"") + result + base::utf8string("\"");
    }
    
    return result;
  }
};

TEST_FUNCTION(1)
{
  mtemplate::Template *pre_template = mtemplate::GetTemplate("data/mtemplate/CSV_semicolon.pre.tpl");
  mtemplate::Template *data_template = mtemplate::GetTemplate("data/mtemplate/CSV_semicolon.tpl");
  mtemplate::TemplateOutputFile output("test_output/test1.csv");
  mtemplate::DictionaryInterface *pre_dictionary = mtemplate::CreateMainDictionary();
  mtemplate::DictionaryInterface *data_dictionary = mtemplate::CreateMainDictionary();
  
  mtemplate::Modifier::addModifier<CSVTokenQuoteModifier>("csv_quote");

//   pre_dictionary->setValue("COLUMN_separator", ",");
  
  pre_dictionary->addSectionDictionary("COLUMN")->setValue("COLUMN_NAME", "Language");
  pre_dictionary->addSectionDictionary("COLUMN")->setValue("COLUMN_NAME", "Phrase");
  
  pre_template->expand(pre_dictionary, &output);
  
  
  
  for (auto item : language_details_map)
  {
    mtemplate::DictionaryInterface *row_dictionary = data_dictionary->addSectionDictionary("ROW");
    row_dictionary->setValue("ROW_separator", "\n");
    mtemplate::DictionaryInterface *field_dictionary_col1 = row_dictionary->addSectionDictionary("FIELD");
    field_dictionary_col1->setValue("FIELD_VALUE", item.first);
    mtemplate::DictionaryInterface *field_dictionary_col2 = row_dictionary->addSectionDictionary("FIELD");
    field_dictionary_col2->setValue("FIELD_VALUE", item.second);
  }
  
  data_template->expand(data_dictionary, &output);
  
}


END_TESTS