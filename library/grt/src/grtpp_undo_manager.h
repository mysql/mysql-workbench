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

#pragma once

#include "grt.h"

#include <deque>
#include <boost/signals2.hpp>
#include <ostream>

namespace grt {

  class UndoManager;

  class MYSQLGRT_PUBLIC UndoAction {
    std::string _description;

  public:
    virtual ~UndoAction(){};

    virtual void set_description(const std::string &description);

    virtual void undo(UndoManager *owner) = 0;
    virtual std::string description() const {
      return _description;
    }

    virtual void dump(std::ostream &out, int indent = 0) const = 0;
  };

  class MYSQLGRT_PUBLIC SimpleUndoAction : public UndoAction {
    std::string _description;

    std::function<void()> _undo_slot;

  public:
    SimpleUndoAction(const std::function<void()> &undoslot) : _undo_slot(undoslot){};

    virtual void dump(std::ostream &out, int indent = 0) const;

    virtual void undo(UndoManager *owner) {
      _undo_slot();
    }
  };

  class MYSQLGRT_PUBLIC UndoObjectChangeAction : public UndoAction {
  protected:
    ObjectRef _object;
    std::string _member;
    ValueRef _value;

  public:
    UndoObjectChangeAction(const ObjectRef &object, const std::string &member);
    UndoObjectChangeAction(const ObjectRef &object, const std::string &member, const ValueRef &value);

    virtual void undo(UndoManager *owner);

    const ObjectRef &get_object() const {
      return _object;
    }
    const std::string &get_member() const {
      return _member;
    }

    virtual void dump(std::ostream &out, int indent = 0) const;
  };

  class MYSQLGRT_PUBLIC UndoListInsertAction : public UndoAction {
    BaseListRef _list;
    size_t _index;

  public:
    UndoListInsertAction(const BaseListRef &list, size_t index = BaseListRef::npos);

    virtual void undo(UndoManager *owner);

    virtual void dump(std::ostream &out, int indent = 0) const;
  };

  class MYSQLGRT_PUBLIC UndoListSetAction : public UndoAction {
    BaseListRef _list;
    size_t _index;
    ValueRef _value;

  public:
    UndoListSetAction(const BaseListRef &list, size_t index);

    virtual void undo(UndoManager *owner);

    virtual void dump(std::ostream &out, int indent = 0) const;
  };

  class MYSQLGRT_PUBLIC UndoListReorderAction : public UndoAction {
    BaseListRef _list;
    size_t _oindex;
    size_t _nindex;

  public:
    UndoListReorderAction(const BaseListRef &list, size_t oindex, size_t nindex);

    virtual void undo(UndoManager *owner);
    virtual void dump(std::ostream &out, int indent = 0) const;
  };

  class MYSQLGRT_PUBLIC UndoListRemoveAction : public UndoAction {
    BaseListRef _list;
    ValueRef _value;
    size_t _index;

  public:
    UndoListRemoveAction(const BaseListRef &list, const ValueRef &value);
    UndoListRemoveAction(const BaseListRef &list, size_t index);

    virtual void undo(UndoManager *owner);
    virtual void dump(std::ostream &out, int indent = 0) const;
  };

  class MYSQLGRT_PUBLIC UndoDictSetAction : public UndoAction {
    DictRef _dict;
    std::string _key;
    ValueRef _value;
    bool _had_value;

  public:
    UndoDictSetAction(const DictRef &dict, const std::string &key);

    virtual void undo(UndoManager *owner);
    virtual void dump(std::ostream &out, int indent = 0) const;
  };

  class MYSQLGRT_PUBLIC UndoDictRemoveAction : public UndoAction {
    DictRef _dict;
    std::string _key;
    ValueRef _value;
    bool _had_value;

  public:
    UndoDictRemoveAction(const DictRef &dict, const std::string &key);

    virtual void undo(UndoManager *owner);
    virtual void dump(std::ostream &out, int indent = 0) const;
  };

  class MYSQLGRT_PUBLIC UndoGroup : public UndoAction {
    std::list<UndoAction *> _actions;
    bool _is_open;

  public:
    UndoGroup();
    virtual ~UndoGroup();

    void trim();
    void close();
    inline bool is_open() {
      return _is_open;
    }

    virtual void set_description(const std::string &description);
    virtual std::string description() const;

    virtual void undo(UndoManager *owner);

    virtual void dump(std::ostream &out, int indent = 0) const;

    void add(UndoAction *op);
    bool empty() const;

    virtual bool matches_group(UndoGroup *group) const {
      return false;
    }

    UndoGroup *get_deepest_open_subgroup(UndoGroup **parent = 0);

    std::list<UndoAction *> &get_actions() {
      return _actions;
    }
  };

  //----------------------------------------------------------------------

  class MYSQLGRT_PUBLIC UndoManager {
  public:
    typedef boost::signals2::signal<void(UndoAction *)> UndoSignal;
    typedef boost::signals2::signal<void(UndoAction *)> RedoSignal;

    UndoManager();
    virtual ~UndoManager();

    void enable_logging_to(std::ostream *stream);

    bool can_undo() const;
    bool can_redo() const;
    std::string undo_description() const;
    std::string redo_description() const;

    void set_undo_limit(size_t limit);
    size_t get_undo_limit() const {
      return _undo_limit;
    }

    void disable();
    void enable();
    bool is_enabled() const {
      return _blocks == 0;
    }

    void reset();
    bool empty() const;

    bool is_undoing() const {
      return _is_undoing;
    }
    bool is_redoing() const {
      return _is_redoing;
    }

    virtual void undo();
    virtual void redo();

    // the optional group to be used will become owned by the undo manager
    UndoGroup *begin_undo_group(UndoGroup *group = 0);
    bool end_undo_group(const std::string &description = "", bool trim = false);
    void cancel_undo_group();

    virtual void add_undo(UndoAction *cmd);
    virtual void add_simple_undo(const std::function<void()> &slot);
    void set_action_description(const std::string &descr);
    std::string get_action_description() const;

    UndoAction *get_latest_undo_action() const;
    UndoAction *get_latest_closed_undo_action() const;

    std::string get_running_action_description() const;

    UndoSignal *signal_undo() {
      return &_undo_signal;
    };
    RedoSignal *signal_redo() {
      return &_redo_signal;
    };

    boost::signals2::signal<void()> *signal_changed() {
      return &_changed_signal;
    }

    void dump_undo_stack();
    void dump_redo_stack();

  public:
    std::deque<UndoAction *> &get_undo_stack() {
      return _undo_stack;
    }
    std::deque<UndoAction *> &get_redo_stack() {
      return _redo_stack;
    }
    void lock() const;
    void unlock() const;

  protected:
    mutable base::RecMutex _mutex;
    std::ostream *_undo_log;

    std::deque<UndoAction *> _undo_stack;
    std::deque<UndoAction *> _redo_stack;

    size_t _undo_limit;

    int _blocks;
    bool _is_undoing;
    bool _is_redoing;

    UndoSignal _undo_signal;
    RedoSignal _redo_signal;
    boost::signals2::signal<void()> _changed_signal;

    void trim_undo_stack();
  };

  struct MYSQLGRT_PUBLIC AutoUndo {
  public:
    UndoGroup *group;

    AutoUndo(bool noop = false);
    AutoUndo(UndoGroup *use_group, bool noop = false);

    ~AutoUndo();

    void set_description_for_last_action(const std::string &s);
    void cancel();
    void end_or_cancel_if_empty(const std::string &descr);
    void end(const std::string &descr);

  private:
    bool _valid;
  };
};
