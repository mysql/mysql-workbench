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

#include "SymbolTable.h"

using namespace parsers;

//----------------- Symbol ---------------------------------------------------------------------------------------------

Symbol::Symbol(std::string const &aName) : name(aName){};

void Symbol::clear() {
}

void Symbol::setParent(Symbol *parent) {
  this->parent = parent;
}

Symbol *Symbol::getRoot() const {
  Symbol *run = parent;
  while (run != nullptr) {
    if (run->parent == nullptr || dynamic_cast<SymbolTable *>(run->parent) != nullptr)
      return run;
    run = run->parent;
  }
  return run;
}

std::vector<Symbol const *> Symbol::getSymbolPath() const {
  std::vector<Symbol const *> result;
  const Symbol *run = this;
  while (run != nullptr) {
    result.push_back(run);
    if (run->parent == nullptr || dynamic_cast<SymbolTable *>(run->parent) != nullptr)
      break;
    run = run->parent;
  }
  return result;
}

std::string Symbol::qualifiedName(std::string const &separator, bool full) const {
  std::string result = name;
  Symbol *run = parent;
  while (run != nullptr) {
    result = run->name + separator + result;
    if (!full || run->parent == nullptr || dynamic_cast<SymbolTable *>(run->parent) != nullptr)
      break;
    run = run->parent;
  }
  return result;
}

//----------------- TypedSymbol ----------------------------------------------------------------------------------------

TypedSymbol::TypedSymbol(std::string const &name, Type *aType) : Symbol(name), type(aType) {
};

//----------------- ScopedSymbol ---------------------------------------------------------------------------------------

ScopedSymbol::ScopedSymbol(std::string const &name) : Symbol(name) {
};

void ScopedSymbol::clear() {
  children.clear();
}

void ScopedSymbol::addAndManageSymbol(Symbol *symbol) {
  children.emplace_back(symbol);
  symbol->setParent(this);
}

Symbol *ScopedSymbol::resolve(std::string const &name, bool localOnly) const {
  for (auto &child : children) {
    if (child->name == name)
      return child.get();
  }

  // Nothing found locally. Let the parent continue.
  if (!localOnly) {
    ScopedSymbol *scopedParent = dynamic_cast<ScopedSymbol *>(parent);
    if (scopedParent != nullptr)
      return scopedParent->resolve(name, true);
  }

  return nullptr;
}

std::vector<TypedSymbol *> ScopedSymbol::getTypedSymbols(bool localOnly) const {
  std::vector<TypedSymbol *> result = getSymbolsOfType<TypedSymbol>();

  if (!localOnly) {
    ScopedSymbol *scopedParent = dynamic_cast<ScopedSymbol *>(parent);
    if (scopedParent != nullptr) {
      auto localList = scopedParent->getTypedSymbols(true);
      result.insert(result.end(), localList.begin(), localList.end());
    }
  }

  return result;
}

std::vector<std::string> ScopedSymbol::getTypedSymbolNames(bool localOnly) const {
  std::vector<std::string> result;

  for (auto &child : children) {
    TypedSymbol *typedChild = dynamic_cast<TypedSymbol *>(child.get());
    if (typedChild != nullptr)
      result.push_back(typedChild->name);
  }

  if (!localOnly) {
    ScopedSymbol *scopedParent = dynamic_cast<ScopedSymbol *>(parent);
    if (scopedParent != nullptr) {
      auto localList = scopedParent->getTypedSymbolNames(true);
      result.insert(result.end(), localList.begin(), localList.end());
    }
  }
  
  return result;
}

std::vector<Type *> ScopedSymbol::getTypes(bool localOnly) const {
  std::vector<Type *> result = getSymbolsOfType<Type>();

  if (!localOnly) {
    ScopedSymbol *scopedParent = dynamic_cast<ScopedSymbol *>(parent);
    if (scopedParent != nullptr) {
      auto localList = scopedParent->getTypes(true);
      result.insert(result.end(), localList.begin(), localList.end());
    }
  }
  
  return result;
}

std::vector<ScopedSymbol *> ScopedSymbol::getDirectScopes() const {
  return getSymbolsOfType<ScopedSymbol>();
}

std::vector<Symbol *> ScopedSymbol::getAllSymbols() const {
  std::vector<Symbol *> result;

  for (auto &child : children) {
    result.push_back(child.get());
  }

  ScopedSymbol *scopedParent = dynamic_cast<ScopedSymbol *>(parent);
  if (scopedParent != nullptr) {
    auto localList = scopedParent->getAllSymbols();
    result.insert(result.end(), localList.begin(), localList.end());
  }

  return result;
}

std::set<std::string> ScopedSymbol::getAllSymbolNames() const {
  std::set<std::string> result;

  for (auto &child : children) {
    result.insert(child->name);
  }

  ScopedSymbol *scopedParent = dynamic_cast<ScopedSymbol *>(parent);
  if (scopedParent != nullptr) {
    auto localList = scopedParent->getAllSymbolNames();
    result.insert(localList.begin(), localList.end());
  }

  return result;
}

//----------------- VariableSymbol -------------------------------------------------------------------------------------

VariableSymbol::VariableSymbol(std::string const &name, Type *type) : TypedSymbol(name, type) {
}

//----------------- ParameterSymbol ------------------------------------------------------------------------------------

//----------------- RoutineSymbol --------------------------------------------------------------------------------------

RoutineSymbol::RoutineSymbol(std::string const &name, Type *aReturnType)
  : ScopedSymbol(name), returnType(aReturnType){};

std::vector<VariableSymbol *> RoutineSymbol::getVariables(bool localOnly) const {
  return getSymbolsOfType<VariableSymbol>();
}

std::vector<ParameterSymbol *> RoutineSymbol::getParameters(bool localOnly) const {
  return getSymbolsOfType<ParameterSymbol>();
}

//----------------- MethodSymbol ---------------------------------------------------------------------------------------

MethodSymbol::MethodSymbol(std::string const &name, Type *returnType) : RoutineSymbol(name, returnType) {
}

//----------------- FieldSymbol ----------------------------------------------------------------------------------------

FieldSymbol::FieldSymbol(std::string const &name, Type *type) : VariableSymbol(name, type) {
}

//----------------- Type -----------------------------------------------------------------------------------------------

Type::Type(std::string const &name, Type *base) : Symbol(name), baseType(base) {
}

//----------------- FullType -------------------------------------------------------------------------------------------

FullType::FullType(std::string const &name) : Type(name) {
}

//----------------- ClassSymbol ----------------------------------------------------------------------------------------

ClassSymbol::ClassSymbol(std::string const &name, ClassSymbol *aSuperClass)
  : ScopedSymbol(name), Type(name), superClasses({aSuperClass}) {
}

std::vector<MethodSymbol *> ClassSymbol::getMethods(bool includeInherited) const {
  return getSymbolsOfType<MethodSymbol>();
}

std::vector<FieldSymbol *> ClassSymbol::getFields(bool includeInherited) const {
  return getSymbolsOfType<FieldSymbol>();
}

//----------------- ArrayType ------------------------------------------------------------------------------------------

ArrayType::ArrayType(std::string const &name, Type *elemType, size_t aSize)
  : Type(name), elementType(elemType), size(aSize) {
}

//----------------- TypeAlias ------------------------------------------------------------------------------------------

TypeAlias::TypeAlias(std::string const &name, Type *target) : Type(name, target) {
}

//----------------- SymbolTable ----------------------------------------------------------------------------------------

SymbolTable::SymbolTable() : ScopedSymbol() {
}

void SymbolTable::addDependencies(std::vector<SymbolTable *> const &newDependencies) {
  // No duplicate check takes place.
  _dependencies.insert(_dependencies.end(), newDependencies.begin(), newDependencies.end());
}

Symbol *SymbolTable::resolve(std::string const &name, bool localOnly) const {
  Symbol *result = ScopedSymbol::resolve(name, localOnly);

  if (result == nullptr && !localOnly) {
    for (auto dependency : _dependencies) {
      result = dependency->resolve(name, false);
      if (result != nullptr)
        break;
    }
  }

  return result;
}
