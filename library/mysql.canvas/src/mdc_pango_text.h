/*
 *  mdc_pango_text.h
 *  MySQLWorkbench
 *
 *  Created by Alfredo Kojima on 9/Jul/09.
 *  Copyright 2009 Sun Microsystems Inc. All rights reserved.
 *
 */

#ifndef _MDC_PANGO_TEXT_H_
#define _MDC_PANGO_TEXT_H_

#include <string>

class TextParagraph {
public:
  TextParagraph(const std::string &text);

  void stroke();
};

#endif