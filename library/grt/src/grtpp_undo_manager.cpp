/*
 * Copyright (c) 2007, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include "base/log.h"

#include "grtpp_undo_manager.h"
#include "base/string_utilities.h"

#include <iostream>
#include <time.h>

#ifdef _MSC_VER
#undef max
#endif

DEFAULT_LOG_DOMAIN("Undo manager")

using namespace grt;
using namespace base;

static bool debug_undo = false;

/** For a list, try getting the object that owns it. Returns null if its not owned
 */
static ObjectRef owner_of_list(const BaseListRef &list) {
  internal::OwnedList *olist = dynamic_cast<internal::OwnedList *>(list.valueptr());

  if (olist)
    return ObjectRef(olist->owner_of_owned_list());

  return ObjectRef();
}

/** For a dict, try getting the object that owns it. Returns null if its not owned
 */
static ObjectRef owner_of_dict(const DictRef &dict) {
  internal::OwnedDict *odict = dynamic_cast<internal::OwnedDict *>(dict.valueptr());

  if (odict)
    return ObjectRef(odict->owner_of_owned_dict());

  return ObjectRef();
}

static bool find_member_for_list(const MetaClass::Member *member, const internal::Object *object,
                                 const internal::List *list, std::string *ret_member_name) {
  if (member->type.base.type == ListType &&
      object->get_metaclass()->get_member_value(object, member->name).valueptr() == list) {
    *ret_member_name = member->name;
    return false;
  }
  return true;
}

/** Get the name of the object member the list belongs to
 */
static std::string member_for_object_list(const ObjectRef &object, const BaseListRef &list) {
  MetaClass *meta = object.get_metaclass();
  std::string name;

  meta->foreach_member(std::bind(&find_member_for_list, std::placeholders::_1, (internal::Object *)object.valueptr(),
                                 (internal::List *)list.valueptr(), &name));
  return name;
}

static bool find_member_for_dict(const MetaClass::Member *member, const ObjectRef &object, const DictRef &dict,
                                 std::string *ret_member_name) {
  if (member->type.base.type == DictType &&
      object.get_metaclass()->get_member_value((internal::Object *)object.valueptr(), member->name) == dict) {
    *ret_member_name = member->name;
    return false;
  }
  return true;
}

/** Get the name of the object member the dict belongs to
 */
static std::string member_for_object_dict(const ObjectRef &object, const DictRef &dict) {
  MetaClass *meta = object.get_metaclass();
  std::string name;

  meta->foreach_member(std::bind(&find_member_for_dict, std::placeholders::_1, object, dict, &name));

  return name;
}

//---------------------------------------------------------------------------------------------------

void UndoAction::set_description(const std::string &description) {
  _description = description;
}

//---------------------------------------------------------------------------------------------------

void SimpleUndoAction::dump(std::ostream &out, int indent) const {
  out << strfmt("%*s custom_action ", indent, "") << ": " << _description << std::endl;
}

//---------------------------------------------------------------------------------------------------

UndoObjectChangeAction::UndoObjectChangeAction(const ObjectRef &object, const std::string &member)
  : _object(object), _member(member) {
  _value = _object.get_member(_member);
  debug_undo = getenv("DEBUG_UNDO") != 0;
}

UndoObjectChangeAction::UndoObjectChangeAction(const ObjectRef &object, const std::string &member,
                                               const ValueRef &value)
  : _object(object), _member(member), _value(value) {
}

void UndoObjectChangeAction::undo(UndoManager *owner) {
  // owner->add_undo(new UndoObjectChangeAction(_object, _member));
  // owner->set_action_description(description());

  grt::GRT::get()->start_tracking_changes();
  _object.set_member(_member, _value);
  owner->set_action_description(description());
  grt::GRT::get()->stop_tracking_changes();
}

void UndoObjectChangeAction::dump(std::ostream &out, int indent) const {
  std::string new_value;

  if (_object.get_metaclass()->get_member_info(_member)->type.base.type == ObjectType)
    new_value = ObjectRef::cast_from(_object.get_member(_member)).id();
  else
    new_value = _object.get_member(_member).debugDescription().c_str();

  out << strfmt("%*s change_object ", indent, "") << _object.class_name() << "::" << _member << " <" << _object.id()
      << "> ->" << new_value << ": " << description() << std::endl;
}

//---------------------------------------------------------------------------------------------------

UndoListInsertAction::UndoListInsertAction(const BaseListRef &list, size_t index) : _list(list), _index(index) {
}

void UndoListInsertAction::undo(UndoManager *owner) {
  if (_index == BaseListRef::npos) {
    // Remove last entry in the list, if there is one.
    if (_list.count() > 0) {
      grt::GRT::get()->start_tracking_changes();
      _list.remove(_list.count() - 1);
      owner->set_action_description(description());
      grt::GRT::get()->stop_tracking_changes();
    } else {
      // The list should not be empty actually if we get to this point. Investigate...
      std::cerr << "INTERNAL INCONSISTENCY: UndoListInsertAction: Invalid undo record ";
      dump(std::cerr, 1);
    }
  } else {
    grt::GRT::get()->start_tracking_changes();
    _list.remove(_index);
    owner->set_action_description(description());
    grt::GRT::get()->stop_tracking_changes();
  }
}

void UndoListInsertAction::dump(std::ostream &out, int indent) const {
  ObjectRef owner = owner_of_list(_list);

  out << strfmt("%*s insert_list ", indent, "");

  if (owner.is_valid())
    out << owner.class_name() << "::" << member_for_object_list(owner, _list)
        << strfmt("[%i]", (int)(_index == BaseListRef::npos ? -1 : _index)) << " <" << owner.id() << ">";
  else
    out << "<unowned list>" << strfmt("%p", _list.valueptr())
        << strfmt("[%i]", (int)(_index == BaseListRef::npos ? -1 : _index));

  out << ": " << description() << std::endl;
}

//---------------------------------------------------------------------------------------------------

UndoListReorderAction::UndoListReorderAction(const BaseListRef &list, size_t oindex, size_t nindex)
  : _list(list), _oindex(oindex), _nindex(nindex) {
}

void UndoListReorderAction::undo(UndoManager *owner) {
  /*
  owner->add_undo(new UndoListReorderAction(_list, _nindex, _oindex));
  owner->set_action_description(description());
  _list.reorder(_nindex, _oindex);
   */
  grt::GRT::get()->start_tracking_changes();
  _list.reorder(_nindex, _oindex);
  owner->set_action_description(description());
  grt::GRT::get()->stop_tracking_changes();
}

void UndoListReorderAction::dump(std::ostream &out, int indent) const {
  std::string change(strfmt("[%i]->[%i]", (int)(_oindex == BaseListRef::npos ? -1 : _oindex),
                            (int)(_nindex == BaseListRef::npos ? -1 : _nindex)));
  ObjectRef owner = owner_of_list(_list);

  out << strfmt("%*s reorder_list ", indent, "");

  if (owner.is_valid())
    out << owner.class_name() << "." << member_for_object_list(owner, _list) << change << " <" << owner.id() << ">";
  else
    out << "<unowned list>" << strfmt("%p", _list.valueptr()) << change;

  out << ": " << description() << std::endl;
}

//---------------------------------------------------------------------------------------------------

UndoListSetAction::UndoListSetAction(const BaseListRef &list, size_t index) : _list(list), _index(index) {
  _value = list.get(index);
}

void UndoListSetAction::undo(UndoManager *owner) {
  /*
  owner->add_undo(new UndoListSetAction(_list, _index));
  owner->set_action_description(description());
  _list.gset(_index, _value);
   */
  grt::GRT::get()->start_tracking_changes();
  _list.gset(_index, _value);
  owner->set_action_description(description());
  grt::GRT::get()->stop_tracking_changes();
}

void UndoListSetAction::dump(std::ostream &out, int indent) const {
  ObjectRef owner = owner_of_list(_list);

  out << strfmt("%*s set_list ", indent, "");

  if (owner.is_valid())
    out << owner.class_name() << "." << member_for_object_list(owner, _list)
        << strfmt("[%i]", (int)(_index == BaseListRef::npos ? -1 : _index)) << " <" << owner.id() << ">";
  else
    out << "<unowned list>" << strfmt("%p", _list.valueptr())
        << strfmt("[%i]", (int)(_index == BaseListRef::npos ? -1 : _index));

  out << ": " << description() << std::endl;
}

//---------------------------------------------------------------------------------------------------

UndoListRemoveAction::UndoListRemoveAction(const BaseListRef &list, const ValueRef &value)
  : _list(list), _value(value) {
  bool found = false;
  for (size_t c = list.count(), i = 0; i < c; i++) {
    if (list[i].valueptr() == value.valueptr()) {
      _index = i;
      found = true;
      break;
    }
  }
  // if the undo action is added after the value is deleted, it wont work
  if (!found)
    throw std::logic_error("attempt to add invalid undo operation");
}

UndoListRemoveAction::UndoListRemoveAction(const BaseListRef &list, size_t index)
  : _list(list), _value(list.get(index)), _index(index) {
}

void UndoListRemoveAction::undo(UndoManager *owner) {
  grt::GRT::get()->start_tracking_changes();
  _list.ginsert(_value, _index);
  owner->set_action_description(description());
  grt::GRT::get()->stop_tracking_changes();
}

void UndoListRemoveAction::dump(std::ostream &out, int indent) const {
  ObjectRef owner = owner_of_list(_list);

  out << strfmt("%*s remove_list ", indent, "");

  if (owner.is_valid())
    out << owner.class_name() << "." << member_for_object_list(owner, _list)
        << strfmt("[%i]", (int)(_index == BaseListRef::npos ? -1 : _index)) << " <" << owner.id() << ">";
  else
    out << "<unowned list>" << strfmt("%p", _list.valueptr())
        << strfmt("[%i]", (int)(_index == BaseListRef::npos ? -1 : _index));

  out << ": " << description() << std::endl;
}

//---------------------------------------------------------------------------------------------------

UndoDictSetAction::UndoDictSetAction(const DictRef &dict, const std::string &key) : _dict(dict), _key(key) {
  if (_dict.has_key(key)) {
    _value = _dict.get(_key);
    _had_value = true;
  } else
    _had_value = false;
}

void UndoDictSetAction::undo(UndoManager *owner) {
  if (_had_value) {
    grt::GRT::get()->start_tracking_changes();
    _dict.set(_key, _value);
    owner->set_action_description(description());
    grt::GRT::get()->stop_tracking_changes();
  } else {
    grt::GRT::get()->start_tracking_changes();
    _dict.remove(_key);
    owner->set_action_description(description());
    grt::GRT::get()->stop_tracking_changes();
  }
}

void UndoDictSetAction::dump(std::ostream &out, int indent) const {
  ObjectRef owner = owner_of_dict(_dict);

  out << strfmt("%*s set_dict ", indent, "");

  if (owner.is_valid())
    out << owner.class_name() << "." << member_for_object_dict(owner, _dict) << strfmt("[%s]", _key.c_str()) << " <"
        << owner.id() << ">";
  else
    out << "<unowned list>" << strfmt("%p", _dict.valueptr()) << strfmt("[%s]", _key.c_str());

  out << ": " << description() << std::endl;
}

//---------------------------------------------------------------------------------------------------

UndoDictRemoveAction::UndoDictRemoveAction(const DictRef &dict, const std::string &key) : _dict(dict), _key(key) {
  if (_dict.has_key(key)) {
    _value = _dict.get(_key);
    _had_value = true;
  } else
    _had_value = false;
}

void UndoDictRemoveAction::undo(UndoManager *owner) {
  if (_had_value) {
    grt::GRT::get()->start_tracking_changes();
    _dict.set(_key, _value);
    owner->set_action_description(description());
    grt::GRT::get()->stop_tracking_changes();
  } else {
    // nop
    owner->add_undo(new UndoDictRemoveAction(_dict, _key));
    owner->set_action_description(description());
  }
}

void UndoDictRemoveAction::dump(std::ostream &out, int indent) const {
  ObjectRef owner = owner_of_dict(_dict);

  out << strfmt("%*s remove_dict ", indent, "");

  if (owner.is_valid())
    out << owner.class_name() << "." << member_for_object_dict(owner, _dict) << strfmt("[%s]", _key.c_str()) << " <"
        << owner.id() << ">";
  else
    out << "<unowned list>" << strfmt("%p", _dict.valueptr()) << strfmt("[%s]", _key.c_str());

  out << ": " << description() << std::endl;
}

//---------------------------------------------------------------------------------------------------

UndoGroup::UndoGroup() {
  _is_open = true;
}

UndoGroup::~UndoGroup() {
  for (std::list<UndoAction *>::reverse_iterator iter = _actions.rbegin(); iter != _actions.rend(); ++iter)
    delete *iter;
}

void UndoGroup::undo(UndoManager *owner) {
  owner->begin_undo_group();
  for (std::list<UndoAction *>::reverse_iterator iter = _actions.rbegin(); iter != _actions.rend(); ++iter) {
    (*iter)->undo(owner);
  }

  owner->end_undo_group();
  owner->set_action_description(UndoAction::description());
}

void UndoGroup::trim() {
  std::list<UndoAction *>::iterator next, iter;
  next = _actions.begin();
  // delete closed groups that are empty or have a single action
  while (next != _actions.end()) {
    UndoGroup *subgroup = dynamic_cast<UndoGroup *>(*next);
    iter = next++;

    if (subgroup && !subgroup->is_open()) {
      subgroup->trim();

      if (subgroup->_actions.size() == 1) {
        UndoAction *content = subgroup->_actions.front();
        subgroup->_actions.clear();
        delete subgroup;

        // replace the group with the content
        *iter = content;
      } else if (subgroup->empty()) {
        // remove this subgroup
        _actions.erase(iter);
        delete subgroup;
      }
    }
  }
}

UndoGroup *UndoGroup::get_deepest_open_subgroup(UndoGroup **parent) {
  if (!_actions.empty()) {
    UndoGroup *group = dynamic_cast<UndoGroup *>(_actions.back());
    if (group && group->is_open()) {
      if (parent)
        *parent = this;
      return group->get_deepest_open_subgroup(parent);
    }
  }
  return _is_open ? this : 0;
}

void UndoGroup::close() {
  // close the topmost open undo group
  UndoGroup *group = get_deepest_open_subgroup();
  if (group)
    group->_is_open = false;
  else
    logWarning("trying to close already closed undo group\n");
}

void UndoGroup::add(UndoAction *op) {
  // add the action to the topmost open undo group
  UndoGroup *subgroup = get_deepest_open_subgroup();

  if (subgroup)
    subgroup->_actions.push_back(op);
  else
    throw std::logic_error("trying to add an action to a closed undo group");
}

bool UndoGroup::empty() const {
  return _actions.empty();
}

void UndoGroup::set_description(const std::string &description) {
  if (!_actions.empty() && _is_open) {
    UndoGroup *subgroup = dynamic_cast<UndoGroup *>(_actions.back());
    if (subgroup) {
      if (subgroup->_is_open)
        _actions.back()->set_description(description);
      else
        subgroup->set_description(description);
      return;
    } else
      _actions.back()->set_description(description);
  }

  if (!_is_open)
    UndoAction::set_description(description);
}

std::string UndoGroup::description() const {
  if (!_actions.empty() && _is_open) {
    UndoGroup *subgroup = dynamic_cast<UndoGroup *>(_actions.back());
    if (subgroup && subgroup->_is_open) {
      return _actions.back()->description();
    }
  }
  return UndoAction::description();
}

void UndoGroup::dump(std::ostream &out, int indent) const {
  out << strfmt("%*s group%s { ", indent, "", _is_open ? "(open)" : "") << std::endl;
  for (std::list<UndoAction *>::const_iterator iter = _actions.begin(); iter != _actions.end(); ++iter) {
    (*iter)->dump(out, indent + 2);
  }
  out << strfmt("%*s }", indent, "") << ": " << description() << std::endl;
}

//---------------------------------------------------------------------------------------------------

UndoManager::UndoManager() {
  _undo_log = 0;
  _is_undoing = false;
  _is_redoing = false;
  _undo_limit = 0;
  _blocks = 0;
}

UndoManager::~UndoManager() {
  _changed_signal.disconnect_all_slots(); // prevent emission in reset()
  reset();
}

void UndoManager::enable_logging_to(std::ostream *stream) {
  char buf[30];
  time_t t = time(NULL);

  _undo_log = stream;

  *_undo_log << "***** Starting Undo Log at " <<
#ifdef _MSC_VER
    ctime_s(buf, sizeof(buf), &t)
#else
    ctime_r(&t, buf)
#endif
             << " *****" << std::endl;
}

void UndoManager::lock() const {
  _mutex.lock();
}

void UndoManager::unlock() const {
  _mutex.unlock();
}

void UndoManager::disable() {
  _blocks++;
}

void UndoManager::enable() {
  if (_blocks == 0)
    return;
  _blocks--;
}

void UndoManager::set_undo_limit(size_t limit) {
  _undo_limit = limit;

  trim_undo_stack();
}

void UndoManager::trim_undo_stack() {
  lock();
  if (_undo_limit > 0)
    _undo_stack.erase(_undo_stack.begin(), _undo_stack.begin() + std::max(0, (int)(_undo_stack.size() - _undo_limit)));
  unlock();
}

bool UndoManager::can_undo() const {
  lock();
  bool empty = _undo_stack.empty();
  unlock();

  return !empty;
}

bool UndoManager::can_redo() const {
  lock();
  bool empty = _redo_stack.empty();
  unlock();

  return !empty;
}

std::string UndoManager::undo_description() const {
  std::string d;
  lock();
  if (can_undo())
    d = _undo_stack.back()->description();
  unlock();
  return d;
}

std::string UndoManager::redo_description() const {
  std::string d;
  lock();
  if (can_redo())
    d = _redo_stack.back()->description();
  unlock();
  return d;
}

UndoAction *UndoManager::get_latest_closed_undo_action() const {
  lock();

  std::deque<UndoAction *>::const_reverse_iterator action = _undo_stack.rbegin();

  while (action != _undo_stack.rend()) {
    UndoGroup *group = dynamic_cast<UndoGroup *>(*action);
    if (!group || !group->is_open()) {
      unlock();
      return *action;
    }
    ++action;
  }

  unlock();
  return 0;
}

UndoAction *UndoManager::get_latest_undo_action() const {
  lock();
  if (_undo_stack.empty()) {
    unlock();
    return 0;
  }
  UndoAction *action = _undo_stack.back();

  UndoGroup *group = dynamic_cast<UndoGroup *>(action);
  while (group && group->is_open() && !group->empty()) {
    action = group->get_actions().back();
    group = dynamic_cast<UndoGroup *>(action);
  }
  unlock();
  return action;
}

void UndoManager::reset() {
  lock();
  for (std::deque<UndoAction *>::iterator iter = _undo_stack.begin(); iter != _undo_stack.end(); ++iter)
    delete *iter;
  _undo_stack.clear();

  for (std::deque<UndoAction *>::iterator iter = _redo_stack.begin(); iter != _redo_stack.end(); ++iter)
    delete *iter;
  _redo_stack.clear();

  unlock();
  _changed_signal();
}

bool UndoManager::empty() const {
  return _undo_stack.empty() && _redo_stack.empty();
}

UndoGroup *UndoManager::begin_undo_group(UndoGroup *group) {
  if (_blocks > 0) { // if blocked, delete the group given and return 0
    delete group;
    return 0;
  }

  // if no group given, just create the default type
  if (!group)
    group = new UndoGroup();

  logDebug3("begin undo group: %s\n", group->description().c_str());

  add_undo(group);

  return group;
}

/** Closes an open undo group in the stack and sets its description
 *
 * @param description for the undo group
 * @param trim whether undo groups with a single action should be removed and replaced
 * with the containing action
 *
 * @return true if the undo group was closed and added to the undo stack
 */
bool UndoManager::end_undo_group(const std::string &description, bool trim) {
  if (_blocks > 0)
    return false;

  UndoGroup *group = 0;
  std::deque<UndoAction *> *stack;

  if (_is_undoing)
    stack = &_redo_stack;
  else
    stack = &_undo_stack;

  if (stack->empty())
    throw std::logic_error("unmatched undo group (undo stack is empty)");

  group = dynamic_cast<UndoGroup *>(stack->back());
  if (!group)
    throw std::logic_error("unmatched undo group");

  if (group->empty()) {
    stack->pop_back();
    delete group;
    if (getenv("DEBUG_UNDO"))
      g_message("undo group '%s' was empty, so it was deleted", description.c_str());

    return false;
  } else {
    group->close();
    if (!description.empty())
      group->set_description(description);

    if (!group->is_open() && _undo_log && _undo_log->good())
      group->dump(*_undo_log);

    if (description != "cancelled")
      _changed_signal();
    /* have to 1st merge or check for signal_apply from the deleted groups
    if (!getenv("DEBUG_UNDO"))
    {
      if (trim)
        group->trim();

      // if the group has a single action, we can remove the group and leave the action
      if (trim && !group->is_open() && group->get_actions().size() == 1)
      {
        UndoAction *action= group->get_actions().front();
        if (action->description().empty() || action->description() == group->description())
        {
          action->set_description(group->description());
          stack->pop_back();
          stack->push_back(action);
          group->get_actions().clear();
          delete group;
        }
      }
    }
     */
    logDebug3("end undo group: %s\n", description.c_str());
    return true;
  }
}

void UndoManager::cancel_undo_group() {
  std::deque<UndoAction *> *stack;
  // undo the deepest open undo group
  if (_is_undoing)
    stack = &_redo_stack;
  else
    stack = &_undo_stack;

  UndoGroup *group = 0;
  UndoGroup *parent = 0;
  UndoGroup *subgroup = 0;

  if (!stack->empty() && ((group = dynamic_cast<UndoGroup *>(stack->back())))) {
    subgroup = group->get_deepest_open_subgroup(&parent);
    if (!subgroup)
      subgroup = group;
  }

  // close (deepest open) group and undo it to revert half-changes
  if (end_undo_group("cancelled")) {
    // disable undo registration to avoid getting a redo action after undo
    disable();

    if (group) {
      // undo the deepest open undo group
      subgroup->undo(this);

      lock();
      // if this was the top-level undo group, delete it from the stack
      if (subgroup == group) {
        stack->pop_back();
        delete group;
      } else {
        g_assert(parent->get_actions().back() == subgroup);
        delete subgroup;
        parent->get_actions().pop_back();
      }
      unlock();
    }
    enable();
  }
}

void UndoManager::set_action_description(const std::string &descr) {
  if (_blocks > 0)
    return; // added by tax (instructed by alfredo)

  lock();
  if (_is_undoing) {
    if (!_redo_stack.empty())
      _redo_stack.back()->set_description(descr);
  } else {
    if (!_undo_stack.empty())
      _undo_stack.back()->set_description(descr);
  }
  unlock();
  _changed_signal();
}

std::string UndoManager::get_action_description() const {
  if (_is_undoing)
    return _redo_stack.back()->description();
  else
    return _undo_stack.back()->description();
}

std::string UndoManager::get_running_action_description() const {
  if (_is_redoing)
    return _redo_stack.back()->description();
  else if (_is_undoing)
    return _undo_stack.back()->description();
  return "";
}

void UndoManager::undo() {
  if (_is_undoing)
    throw std::logic_error("unexpected nested undo"); // is this allowed??

  lock();
  if (can_undo()) {
    UndoAction *cmd = _undo_stack.back();
    _is_undoing = true;
    unlock();

    if (debug_undo) {
      std::cout << "UNDOING: ";
      cmd->dump(std::cout, 0);
    }
    cmd->undo(this);

    lock();
    _is_undoing = false;

    _undo_stack.pop_back();
    unlock();

    _undo_signal(cmd);

    delete cmd;
  } else
    unlock();
}

void UndoManager::redo() {
  if (_is_redoing)
    throw std::logic_error("unexpected nested redo"); // is this allowed??

  lock();
  if (can_redo()) {
    UndoAction *cmd = _redo_stack.back();
    _is_redoing = true;
    unlock();

    cmd->undo(this);

    lock();
    _is_redoing = false;

    _redo_stack.pop_back();
    unlock();

    _redo_signal(cmd);

    delete cmd;
  } else
    unlock();
}

void UndoManager::add_undo(UndoAction *cmd) {
  if (_blocks > 0) {
    delete cmd;
    return;
  }

  lock();
  if (_is_undoing) {
    bool flag = false;
    if (!_redo_stack.empty()) {
      UndoGroup *group = dynamic_cast<UndoGroup *>(_redo_stack.back());
      if (group && group->is_open()) {
        group->add(cmd);
        flag = true;
      }
    }
    if (!flag)
      _redo_stack.push_back(cmd);
  } else {
    bool flag = false;
    if (!_undo_stack.empty()) {
      UndoGroup *group = dynamic_cast<UndoGroup *>(_undo_stack.back());
      if (group && group->is_open()) {
        group->add(cmd);
        flag = true;
      }
    }
    if (!flag) {
      if (debug_undo && !dynamic_cast<UndoGroup *>(cmd))
        logDebug2("added undo action that's not a group to top");
      _undo_stack.push_back(cmd);
      trim_undo_stack();
    }

    // if we're not undoing neither redoing, then reset the redo stack
    if (!_is_redoing) {
      for (std::deque<UndoAction *>::iterator iter = _redo_stack.begin(); iter != _redo_stack.end(); ++iter)
        delete *iter;
      _redo_stack.clear();
    }
  }
  unlock();

  UndoGroup *ugrp = dynamic_cast<UndoGroup *>(cmd);
  if (ugrp && !ugrp->is_open())
    _changed_signal();
}

void UndoManager::add_simple_undo(const std::function<void()> &slot) {
  add_undo(new SimpleUndoAction(slot));
}

void UndoManager::dump_undo_stack() {
  for (std::deque<UndoAction *>::iterator iter = _undo_stack.begin(); iter != _undo_stack.end(); ++iter)
    (*iter)->dump(std::cout);
}

void UndoManager::dump_redo_stack() {
  for (std::deque<UndoAction *>::iterator iter = _redo_stack.begin(); iter != _redo_stack.end(); ++iter)
    (*iter)->dump(std::cout);
}

//----------------- AutoUndo -------------------------------------------------------------------------------------------

AutoUndo::AutoUndo(bool noop) {
  _valid = true;
  if (!noop)
    group = grt::GRT::get()->begin_undoable_action();
  else
    group = nullptr;
}

//----------------------------------------------------------------------------------------------------------------------

AutoUndo::AutoUndo(UndoGroup *use_group, bool noop) : group(nullptr) {
  _valid = true;
  if (noop) {
    delete use_group;
    use_group = nullptr;
  } else {
    // check if the group can be merged into the previous one and if so, just drop it
    if (!grt::GRT::get()->get_undo_manager()->get_undo_stack().empty()) {
      UndoGroup *last_group = dynamic_cast<UndoGroup *>(grt::GRT::get()->get_undo_manager()->get_undo_stack().back());

      if (last_group && use_group->matches_group(last_group)) {
        delete use_group;
        use_group = nullptr;
      }
    }

    if (use_group != nullptr)
      group = grt::GRT::get()->begin_undoable_action(use_group);
  }
}

//----------------------------------------------------------------------------------------------------------------------

AutoUndo::~AutoUndo() {
  if (_valid && group != nullptr) {
    const char *tmp;
    // check if the currently open undo group is not empty, in that case we warn about it
    // cancel() should be explicitly called if the cancellation is intentional
    if ((tmp = getenv("DEBUG_UNDO"))) {
      UndoGroup *group = dynamic_cast<UndoGroup *>(grt::GRT::get()->get_undo_manager()->get_latest_undo_action());

      if (group && group->is_open())
        logWarning("automatically cancelling unclosed undo group\n");
    }

    cancel();
  }
}

//----------------------------------------------------------------------------------------------------------------------

void AutoUndo::set_description_for_last_action(const std::string &s) {
  if (_valid && group != nullptr) {
    UndoAction *action = grt::GRT::get()->get_undo_manager()->get_latest_undo_action();

    action->set_description(s);
  }
}

//----------------------------------------------------------------------------------------------------------------------

void AutoUndo::cancel() {
  if (_valid) {
    if (group != nullptr)
      grt::GRT::get()->cancel_undoable_action();
    _valid = false;
  } else
    throw std::logic_error("Trying to cancel an already finished undo action");
}

//----------------------------------------------------------------------------------------------------------------------

void AutoUndo::end_or_cancel_if_empty(const std::string &descr) {
  if (_valid) {
    if (group == nullptr)
      return;

    if (!group->empty())
      grt::GRT::get()->end_undoable_action(descr);
    else
      grt::GRT::get()->cancel_undoable_action();
    _valid = false;
  } else
    throw std::logic_error("Trying to end an already finished undo action");
}

void AutoUndo::end(const std::string &descr) {
  if (_valid) {
    if (group != nullptr)
      grt::GRT::get()->end_undoable_action(descr);
    _valid = false;
  } else
    throw std::logic_error("Trying to end an already finished undo action");
}

//----------------------------------------------------------------------------------------------------------------------
