/*
 * Copyright (c) 2019, Oracle and/or its affiliates. All rights reserved.
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

// Methods to be included as part of the test data structure.

//----------------------------------------------------------------------------------------------------------------------

void resetUndoAccounting() {
  lastUndoStackSize = um->get_undo_stack().size();
  lastRedoStackSize = um->get_redo_stack().size();
}

//----------------------------------------------------------------------------------------------------------------------

void checkOnlyOneUndoAdded() {
  ++lastUndoStackSize;
  $expect(um->get_undo_stack().size()).toEqual(lastUndoStackSize, "Added 1 undo action");

  // Adding new stuff to the undo stack will clear the redo stack.
  lastRedoStackSize = um->get_redo_stack().size();
}

//----------------------------------------------------------------------------------------------------------------------

void checkUndo() {
  $expect(um->get_undo_stack().size()).toEqual(lastUndoStackSize, "Undo stack size");
  $expect(um->get_redo_stack().size()).toEqual(lastRedoStackSize, "Redo stack size");

  // Check that the latest undo action has a description.
  $expect(um->get_action_description()).Not.toEqual("", "Undo action description is set");

  um->undo();
  --lastUndoStackSize;

  // Redo stack should grow by 1 and undo shrink by 1.
  $expect(um->get_redo_stack().size()).toEqual(lastRedoStackSize + 1, "Redo stack size after undo");
  $expect(um->get_undo_stack().size()).toEqual(lastUndoStackSize, "Undo stack size after undo");

  lastRedoStackSize = um->get_redo_stack().size();
}

//----------------------------------------------------------------------------------------------------------------------

void checkRedo() {
  // make sure that the undo/redo stack has the expected size
  $expect(um->get_undo_stack().size()).toEqual(lastUndoStackSize, "Undo stack size");
  $expect(um->get_redo_stack().size()).toEqual(lastRedoStackSize, "Redo stack size");

  um->redo();
  ++lastUndoStackSize;

  $expect(um->get_redo_stack().size()).toEqual(lastRedoStackSize - 1, "Redo stack size after redo");
  $expect(um->get_undo_stack().size()).toEqual(lastUndoStackSize, "Undo stack size after redo");

  lastRedoStackSize = um->get_redo_stack().size();
}

//----------------------------------------------------------------------------------------------------------------------
