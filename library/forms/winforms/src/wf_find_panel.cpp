/*
 * Copyright (c) 2011, 2017, Oracle and/or its affiliates. All rights reserved.
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
#include "wf_find_panel.h"

#include "base/log.h"

using namespace System::Windows::Forms;

using namespace MySQL;
using namespace MySQL::Forms;
using namespace MySQL::Utilities;

DEFAULT_LOG_DOMAIN(DOMAIN_MFORMS_WRAPPER)

//--------------------------------------------------------------------------------------------------

ref class MformsFindPanel : public MySQL::Utilities::FindPanel, public MySQL::Utilities::IFindPanel {
public:
  mforms::FindPanel *panel;

  //------------------------------------------------------------------------------------------------

  size_t DoFindReplace(mforms::FindPanelAction action) {
    mforms::CodeEditor *editor = panel->get_editor();

    if (editor == NULL) // Should never happen.
      return 0;

    std::string search_text = NativeToCppStringRaw(SearchText);
    std::string replace_text = NativeToCppStringRaw(ReplaceText);

    mforms::FindFlags flags = mforms::FindDefault;
    if (!IgnoreCase)
      flags |= mforms::FindMatchCase;
    if (Wrap)
      flags |= mforms::FindWrapAround;
    if (WholeWords)
      flags |= mforms::FindWholeWords;
    if (RegularExpression)
      flags |= mforms::FindRegex;

    switch (action) {
      case mforms::FindNext:
        return editor->find_and_highlight_text(search_text, flags, true, false);
        break;
      case mforms::FindPrevious:
        return editor->find_and_highlight_text(search_text, flags, true, true);
        break;
      case mforms::FindAndReplace:
        return editor->find_and_replace_text(search_text, replace_text, flags, false);
        break;
      case mforms::ReplaceAll:
        return editor->find_and_replace_text(search_text, replace_text, flags, true);
        break;
    }

    return 0;
  }

  //------------------------------------------------------------------------------------------------

  virtual void Close() {
    panel->get_editor()->hide_find_panel();
  }

  //------------------------------------------------------------------------------------------------

  virtual int FindReplaceAction(MySQL::Utilities::FindPanelAction action) {
    return (int)DoFindReplace((mforms::FindPanelAction)action);
  }

  //-----------------------------------------------------------------------------------------------
};

//----------------- FindPanelWrapper ---------------------------------------------------------------

FindPanelWrapper::FindPanelWrapper(mforms::FindPanel *backend) : ViewWrapper(backend) {
}

//--------------------------------------------------------------------------------------------------

bool FindPanelWrapper::create(mforms::FindPanel *backend) {
  FindPanelWrapper *wrapper = new FindPanelWrapper(backend);
  MformsFindPanel ^ findPanel = FindPanelWrapper::Create<MformsFindPanel>(backend, wrapper);
  findPanel->Backend = findPanel; // One way to inject functionality across managed boundaries.
  findPanel->panel = backend;

  return true;
}

//--------------------------------------------------------------------------------------------------

size_t FindPanelWrapper::perform_action(mforms::FindPanel *backend, mforms::FindPanelAction action) {
  MformsFindPanel ^ findPanel = FindPanelWrapper::GetManagedObject<MformsFindPanel>(backend);
  return findPanel->DoFindReplace(action);
}

//--------------------------------------------------------------------------------------------------

void FindPanelWrapper::focus(mforms::FindPanel *backend) {
  MformsFindPanel ^ findPanel = FindPanelWrapper::GetManagedObject<MformsFindPanel>(backend);
  findPanel->FocusSearchField();
}

//--------------------------------------------------------------------------------------------------

void FindPanelWrapper::enable_replace(mforms::FindPanel *backend, bool flag) {
  MformsFindPanel ^ findPanel = FindPanelWrapper::GetManagedObject<MformsFindPanel>(backend);
  findPanel->ShowReplace = flag;
}

//--------------------------------------------------------------------------------------------------

void FindPanelWrapper::init() {
  mforms::ControlFactory *f = mforms::ControlFactory::get_instance();

  f->_findpanel_impl.create = &FindPanelWrapper::create;
  f->_findpanel_impl.perform_action = &FindPanelWrapper::perform_action;
  f->_findpanel_impl.focus = &FindPanelWrapper::focus;
  f->_findpanel_impl.enable_replace = &FindPanelWrapper::enable_replace;
}

//--------------------------------------------------------------------------------------------------
