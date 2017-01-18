/*
 * Copyright (c) 2010, 2017, Oracle and/or its affiliates. All rights reserved.
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

#include "wf_base.h"
#include "wf_view.h"
#include "wf_webbrowser.h"

using namespace System::ComponentModel;
using namespace System::Windows::Forms;

using namespace MySQL;
using namespace MySQL::Forms;

//----------------- MformsWebBrowser ---------------------------------------------------------------

ref class MformsWebBrowser : public System::Windows::Forms::WebBrowser {
public:
  mforms::WebBrowser *backend;

  virtual void OnDocumentCompleted(WebBrowserDocumentCompletedEventArgs ^ args) override {
    __super ::OnDocumentCompleted(args);
    backend->document_loaded(NativeToCppString(args->Url->AbsoluteUri));
  }

  //------------------------------------------------------------------------------------------------

  virtual void OnNewWindow(CancelEventArgs ^ args) override {
    __super ::OnNewWindow(args);

    HtmlElement ^ element = Document->ActiveElement;
    String ^ url = element->GetAttribute("href");

    // We don't want to let IE open an external IE window if a new window is requested.
    // Instead we cancel the action here and open the default browser.
    mforms::Utilities::open_url(NativeToCppString(url));
    args->Cancel = true;
  }

  //------------------------------------------------------------------------------------------------

  virtual void OnStatusTextChanged(EventArgs ^ args) override {
    mforms::App::get()->set_status_text(NativeToCppString(StatusText));
  }

  //------------------------------------------------------------------------------------------------

  virtual void OnNavigating(WebBrowserNavigatingEventArgs ^ args) override {
    if (backend->on_link_clicked(NativeToCppString(args->Url->ToString())))
      args->Cancel = true;
  }
};

//----------------- WebBrowserWrapper -----------------------------------------------------------------

WebBrowserWrapper::WebBrowserWrapper(mforms::View *backend) : ViewWrapper(backend) {
}

//-------------------------------------------------------------------------------------------------

bool WebBrowserWrapper::create(mforms::WebBrowser *backend) {
  WebBrowserWrapper *wrapper = new WebBrowserWrapper(backend);
  MformsWebBrowser ^ browser = WebBrowserWrapper::Create<MformsWebBrowser>(backend, wrapper);
  browser->backend = backend;
  browser->ScriptErrorsSuppressed = true; // No script error popup please.

  return true;
}

//-------------------------------------------------------------------------------------------------

void WebBrowserWrapper::set_html(mforms::WebBrowser *backend, const std::string &code) {
  System::Windows::Forms::WebBrowser ^ browser =
    WebBrowserWrapper::GetManagedObject<System::Windows::Forms::WebBrowser>(backend);
  browser->DocumentText = CppStringToNative(code);
}

//-------------------------------------------------------------------------------------------------

void WebBrowserWrapper::navigate(mforms::WebBrowser *backend, const std::string &url) {
  System::Windows::Forms::WebBrowser ^ browser =
    WebBrowserWrapper::GetManagedObject<System::Windows::Forms::WebBrowser>(backend);
  browser->Navigate(CppStringToNative(url));
}

//-------------------------------------------------------------------------------------------------

std::string WebBrowserWrapper::get_document_title(mforms::WebBrowser *backend) {
  System::Windows::Forms::WebBrowser ^ browser =
    WebBrowserWrapper::GetManagedObject<System::Windows::Forms::WebBrowser>(backend);
  return NativeToCppString(browser->DocumentTitle);
}

//-------------------------------------------------------------------------------------------------

void WebBrowserWrapper::init() {
  mforms::ControlFactory *f = mforms::ControlFactory::get_instance();

  f->_webbrowser_impl.create = &WebBrowserWrapper::create;
  f->_webbrowser_impl.set_html = &WebBrowserWrapper::set_html;
  f->_webbrowser_impl.navigate = &WebBrowserWrapper::navigate;
  f->_webbrowser_impl.get_document_title = &WebBrowserWrapper::get_document_title;
}

//-------------------------------------------------------------------------------------------------
