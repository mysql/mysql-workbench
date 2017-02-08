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

void reset_undo_accounting() {
  last_undo_stack_height = um->get_undo_stack().size();
  last_redo_stack_height = um->get_redo_stack().size();
}

void check_only_one_undo_added_(const std::string &loc) {
  ++last_undo_stack_height;
  ensure_equals(loc + ": added 1 undo action", um->get_undo_stack().size(), last_undo_stack_height);

  // adding new stuff to the undo stack will clear the redo stack
  last_redo_stack_height = um->get_redo_stack().size();
}
#define check_only_one_undo_added() check_only_one_undo_added_(G_STRINGIFY(__LINE__))

// helper member functions
void check_undo_(const std::string &loc) {
  ensure_equals(loc + ":undo stack size", um->get_undo_stack().size(), last_undo_stack_height);
  ensure_equals(loc + ":redo stack size", um->get_redo_stack().size(), last_redo_stack_height);

  // check that the latest undo action has a description
  ensure(loc + ":undo action description is set", um->get_action_description() != "");

  // perform undo
  um->undo();

  --last_undo_stack_height;
  // redo stack should grow by 1 and undo shrink by 1
  ensure_equals(loc + ":redo stack size after undo", um->get_redo_stack().size(), last_redo_stack_height + 1);
  ensure_equals(loc + ":undo stack size after undo", um->get_undo_stack().size(), last_undo_stack_height);

  last_redo_stack_height = um->get_redo_stack().size();
}
#define check_undo() check_undo_(G_STRINGIFY(__LINE__))

void check_redo_(const std::string &loc) {
  // make sure that the undo/redo stack has the expected size
  ensure_equals(loc + ":undo stack size", um->get_undo_stack().size(), last_undo_stack_height);
  ensure_equals(loc + ":redo stack size", um->get_redo_stack().size(), last_redo_stack_height);

  // perform redo
  um->redo();

  ++last_undo_stack_height;
  ensure_equals(loc + ":redo stack size after redo", um->get_redo_stack().size(), last_redo_stack_height - 1);
  ensure_equals(loc + ":undo stack size after redo", um->get_undo_stack().size(), last_undo_stack_height);

  last_redo_stack_height = um->get_redo_stack().size();
}
#define check_redo() check_redo_(G_STRINGIFY(__LINE__))
