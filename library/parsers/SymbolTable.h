/*
 * Copyright (c) 2016, 2018, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is also distributed with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms, as
 * designated in a particular file or component or in included license
 * documentation. The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have included with MySQL.
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#pragma once

#include "parsers-common.h"

#include <set>
#include <memory>

// A simple symbol table implementation, tailored towards code completion.

namespace antlr4 {
  class ParserRuleContext;
}

namespace parsers {

  class Type;

  enum TypeKind {
    Integer,
    Float,
    String,
    Bool,
    Date,
  };

  enum MemberVisibility {
    Invalid,
    Public,
    Protected,
    Private,
    Library,
  };

  // The root of the symbol table class hierarchy: a symbol can be any managable entity (like a block), not only
  // things like variables or classes.
  class PARSERS_PUBLIC_TYPE Symbol {
  public:
    std::string name; // The name of the scope or empty if anonymous.

    antlr4::ParserRuleContext *context = nullptr; // Reference to the parse tree which defines this symbol.

    Symbol(std::string const &aName = "");
    virtual ~Symbol();

    virtual void clear();
    void setParent(Symbol *parent);
    Symbol *getRoot() const; // Get the outermost entity (below the symbol table) that holds us.

    // Returns the the next enclosing parent of the given type.
    template <typename T>
    T *getParentOfType() const {
      Symbol *run = parent;
      while (run != nullptr) {
        T *castParent = dynamic_cast<T *>(run);
        if (castParent != nullptr)
          return castParent;
        run = run->parent;
      }
      return nullptr;
    }

    // The list of symbols from this one up to root.
    std::vector<Symbol const *> getSymbolPath() const;

    // Create a qualified identifier from this symbol and its parent.
    // If `full` is true then all parents are traversed, instead only the direct one.
    std::string qualifiedName(std::string const &separator = ".", bool full = false) const;

  protected:
    Symbol *parent = nullptr; // The enclosing entity.
  };

  // A symbol with an attached type (variables, fields etc.).
  class PARSERS_PUBLIC_TYPE TypedSymbol : public Symbol {
  public:
    const Type *type = nullptr;

    TypedSymbol(std::string const &name, Type const *aType);
  };

  // A symbol with a scope (so it can have child symbols).
  class PARSERS_PUBLIC_TYPE ScopedSymbol : public Symbol {
  public:
    virtual void clear() override;

    void addAndManageSymbol(Symbol *symbol); // Takes over ownership.

    template <typename T>
    std::vector<T *> getSymbolsOfType() const {
      std::vector<T *> result;
      for (auto &child : children) {
        T *castChild = dynamic_cast<T *>(child.get());
        if (castChild != nullptr)
          result.push_back(castChild);
      }

      return result;
    }

    // Retrieval functions for this scope or any of the parent scopes (conditionally).
    virtual Symbol *resolve(std::string const &name, bool localOnly = false);

    // Returns all accessible symbols that have a type assigned.
    std::vector<TypedSymbol *> getTypedSymbols(bool localOnly = true) const;

    // The names of all accessible symbols with a type.
    std::vector<std::string> getTypedSymbolNames(bool localOnly = true) const;

    std::vector<Type *> getTypes(bool localOnly = true) const; // The types accessible in this scope.

    // Returns all direct child symbols with a scope (e.g. classes in a module).
    std::vector<ScopedSymbol *> getDirectScopes() const;

    // Returns symbols from this and all nested scopes in the order they were defined.
    std::vector<Symbol *> getAllSymbols() const;

    // Like getAllSymbols but only the names (sorted alpabetically).
    std::set<std::string> getAllSymbolNames() const;

  protected:
    ScopedSymbol(const ScopedSymbol&) = delete;
    ScopedSymbol& operator=(const ScopedSymbol&) = delete;

    std::vector<std::unique_ptr<Symbol>> children; // All child symbols in definition order.

    ScopedSymbol(std::string const &name = "");
  };

  class PARSERS_PUBLIC_TYPE VariableSymbol : public TypedSymbol {
  public:
    VariableSymbol(std::string const &name, Type const *type);
  };

  class PARSERS_PUBLIC_TYPE ParameterSymbol : public VariableSymbol {};

  // A standalone function/procedure/rule.
  class PARSERS_PUBLIC_TYPE RoutineSymbol : public ScopedSymbol {
  public:
    const Type *returnType; // Can be null if result is void.

    RoutineSymbol(std::string const &name, Type const *aReturnType);

    std::vector<VariableSymbol *> getVariables(bool localOnly = true) const;
    std::vector<ParameterSymbol *> getParameters(bool localOnly = true) const;
  };

  // A routine which belongs to a class or other outer container structure.
  class PARSERS_PUBLIC_TYPE MethodSymbol : public RoutineSymbol {
  public:
    enum Flags {
      None = 0,
      Virtual = 1,
      Const = 2,
      Overwritten = 4,
      SetterOrGetter = 8, // Distinguished by the return type.
      Explicit = 16,      // Special flag used e.g. in C++ for explicit c-tors.
    } methodFlags = None;

    MemberVisibility visibility = MemberVisibility::Invalid;

    MethodSymbol(std::string const &name, Type const *returnType);
  };

  class PARSERS_PUBLIC_TYPE FieldSymbol : public VariableSymbol {
  public:
    MemberVisibility visibility = MemberVisibility::Invalid;

    MethodSymbol *setter = nullptr;
    MethodSymbol *getter = nullptr;

    FieldSymbol(std::string const &name, Type const *type);
  };

  // The base interface class for types.
  // Implemented in fundamental types, type definitions and type aliases.
  class PARSERS_PUBLIC_TYPE Type {
  public:
    const std::string name;
    const Type *baseType = nullptr; // The super type of this type or empty if this is a fundamental type.
                                    // Also used as the target type for type aliases and typedefs.

    Type(std::string const &name, Type const *base = nullptr);
  };

  // One class for all "built-in" types.
  class PARSERS_PUBLIC_TYPE FundamentalType : Type {
  public:
    const TypeKind kind;

    static const Type *INTEGER_TYPE;
    static const Type *FLOAT_TYPE;
    static const Type *STRING_TYPE;
    static const Type *BOOL_TYPE;
    static const Type *DATE_TYPE;

    FundamentalType(std::string const &name, TypeKind kind);
  };

  // Classes and structs.
  class PARSERS_PUBLIC_TYPE ClassSymbol : virtual public ScopedSymbol, virtual public Type {
  public:
    bool isStruct = false;

    // Usually only one member, unless your language supports multiple inheritance.
    std::vector<ClassSymbol *> superClasses;

    ClassSymbol(std::string const &name, ClassSymbol *aSuperClass);

    std::vector<MethodSymbol *> getMethods(bool includeInherited = false) const; // Returns a list of all methods.
    std::vector<FieldSymbol *> getFields(bool includeInherited = false) const;   // Returns all fields.
  };

  class PARSERS_PUBLIC_TYPE ArrayType : public Type {
  public:
    const Type *elementType;
    size_t size = 0; // > 0 if fixed length.

    ArrayType(std::string const &name, Type const *elemType, size_t aSize = 0);
  };

  // An alias for another type.
  class PARSERS_PUBLIC_TYPE TypeAlias : public Type {
  public:
    TypeAlias(std::string const &name, Type const *target);
  };

  // A few more types for databases.
  class PARSERS_PUBLIC_TYPE CatalogSymbol : public ScopedSymbol {
  public:
    CatalogSymbol(std::string const &name) : ScopedSymbol(name) {
    }
  };

  class PARSERS_PUBLIC_TYPE SchemaSymbol : public ScopedSymbol {
  public:
    SchemaSymbol(std::string const &name) : ScopedSymbol(name) {
    }
  };

  class PARSERS_PUBLIC_TYPE TableSymbol : public ScopedSymbol {
  public:
    TableSymbol(std::string const &name) : ScopedSymbol(name) {
    }
  };

  class PARSERS_PUBLIC_TYPE ViewSymbol : public ScopedSymbol {
  public:
    ViewSymbol(std::string const &name) : ScopedSymbol(name) {
    }
  };

  class PARSERS_PUBLIC_TYPE EventSymbol : public ScopedSymbol {
  public:
    EventSymbol(std::string const &name) : ScopedSymbol(name) {
    }
  };

  class PARSERS_PUBLIC_TYPE ColumnSymbol : public TypedSymbol {
  public:
    ColumnSymbol(std::string const &name, Type const *type) : TypedSymbol(name, type) {
    }
  };

  class PARSERS_PUBLIC_TYPE IndexSymbol : public Symbol { // Made of columns, but doesn't contain them. Hence not a scope.
  public:
    IndexSymbol(std::string const &name) : Symbol(name) {
    }
  };

  class PARSERS_PUBLIC_TYPE PrimaryKeySymbol : public Symbol { // ditto
  public:
    PrimaryKeySymbol(std::string const &name) : Symbol(name) {
    }
  };

  class PARSERS_PUBLIC_TYPE ForeignKeySymbol : public Symbol { // ditto
  public:
    ForeignKeySymbol(std::string const &name) : Symbol(name) {
    }
  };

  class PARSERS_PUBLIC_TYPE StoredRoutineSymbol : public RoutineSymbol {
  public:
    StoredRoutineSymbol(std::string const &name, Type const *returnType) : RoutineSymbol(name, returnType) {
    }
  };

  class PARSERS_PUBLIC_TYPE TriggerSymbol : public ScopedSymbol {
  public:
    TriggerSymbol(std::string const &name) : ScopedSymbol(name) {
    }
  };

  class PARSERS_PUBLIC_TYPE UdfSymbol : public Symbol { // No body nor parameter info.
  public:
    UdfSymbol(std::string const &name) : Symbol(name) {
    }
  };

  class PARSERS_PUBLIC_TYPE EngineSymbol : public Symbol {
  public:
    EngineSymbol(std::string const &name) : Symbol(name) {
    }
  };

  class PARSERS_PUBLIC_TYPE TableSpaceSymbol : public Symbol {
  public:
    TableSpaceSymbol(std::string const &name) : Symbol(name) {
    }
  };

  class PARSERS_PUBLIC_TYPE LogfileGroupSymbol : public Symbol {
  public:
    LogfileGroupSymbol(std::string const &name) : Symbol(name) {
    }
  };

  class PARSERS_PUBLIC_TYPE CharsetSymbol : public Symbol {
  public:
    CharsetSymbol(std::string const &name) : Symbol(name) {
    }
  };

  class PARSERS_PUBLIC_TYPE CollationSymbol : public Symbol {
  public:
    CollationSymbol(std::string const &name) : Symbol(name) {
    }
  };

  class PARSERS_PUBLIC_TYPE UserVariableSymbol : public VariableSymbol {
  public:
    UserVariableSymbol(std::string const &name, Type const *type) : VariableSymbol(name, type) {
    }
  };

  class PARSERS_PUBLIC_TYPE SystemVariableSymbol : public Symbol {
  public:
    SystemVariableSymbol(std::string const &name) : Symbol(name) {
    }
  };

  // The main class managing all the symbols for a top level entity like a file, library or similar.
  // This class is thread safe for all symbol manipulations.
  class PARSERS_PUBLIC_TYPE SymbolTable : public ScopedSymbol {
  public:
    SymbolTable();
    virtual ~SymbolTable();

    // Lock/unlock can be used recursively, but must be balanced of course.
    void lock();
    void unlock();

    void addDependencies(std::vector<SymbolTable *> const &newDependencies);

    // The returned symbol instance is managed by this table.
    template <typename T, typename... Args>
    T *addNewSymbol(ScopedSymbol *parent, Args &&... args)  {
      T *result = new T(args...);

      lock();
      if (parent == nullptr) {
        this->addAndManageSymbol(result);
      } else {
        parent->addAndManageSymbol(result);
      }
      unlock();
      return result;
    }

    template <typename T>
    std::vector<T *> getSymbolsOfType(ScopedSymbol *parent = nullptr) {
      std::vector<T *> result;

      lock();
      if (parent == nullptr || parent == this) {
        for (auto &child : children) {
          T *castChild = dynamic_cast<T *>(child.get());
          if (castChild != nullptr)
            result.push_back(castChild);
        }

        for (SymbolTable *table : _dependencies) {
          auto subList = table->getSymbolsOfType<T>();
          result.insert(result.end(), subList.begin(), subList.end());
        }
      } else {
        result = parent->getSymbolsOfType<T>();
      }

      unlock();
      return result;
    }

    virtual Symbol *resolve(std::string const &name, bool localOnly = false) override;

  private:
    // Other symbol information available to this instance.
    std::vector<SymbolTable *> _dependencies;

    class Private;
    Private *_d;
  };

} // namespace parsers
