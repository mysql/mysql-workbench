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

#pragma once

// A simple symbol table implementation, tailored towards code completion.

namespace antlr4 {
  class ParserRuleContext;
}

namespace parsers {

  class Type;

  enum MemberVisibility {
    Invalid,
    Public,
    Protected,
    Private,
    Library,
  };

  // The root of the symbol table class hierarchy: a symbol can be any managable entity (like a block), not only
  // things like variables or classes.
  class Symbol {
  public:
    std::string name; // The name of the scope or empty if anonymous.

    antlr4::ParserRuleContext *context = nullptr; // Reference to the parse tree which defines this symbol.

    Symbol(std::string const &aName = "");

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
  class TypedSymbol : public Symbol {
  public:
    Type *type = nullptr;

    TypedSymbol(std::string const &name, Type *aType);
  };

  // A symbol with a scope (so it can have child symbols).
  class ScopedSymbol : public Symbol {
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
    virtual Symbol *resolve(std::string const &name, bool localOnly = false) const;

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
    std::vector<std::unique_ptr<Symbol>> children; // All child symbols in definition order.

    ScopedSymbol(std::string const &name = "");
  };

  class VariableSymbol : public TypedSymbol {
  public:
    VariableSymbol(std::string const &name, Type *type);
  };

  class ParameterSymbol : public VariableSymbol {};

  // A standalone function/procedure/rule.
  class RoutineSymbol : public ScopedSymbol {
  public:
    Type *returnType; // Can be null if result is void.

    RoutineSymbol(std::string const &name, Type *aReturnType);

    std::vector<VariableSymbol *> getVariables(bool localOnly = true) const;
    std::vector<ParameterSymbol *> getParameters(bool localOnly = true) const;
  };

  // A routine which belongs to a class or other outer container structure.
  class MethodSymbol : public RoutineSymbol {
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

    MethodSymbol(std::string const &name, Type *returnType);
  };

  class FieldSymbol : public VariableSymbol {
  public:
    MemberVisibility visibility = MemberVisibility::Invalid;

    MethodSymbol *setter = nullptr;
    MethodSymbol *getter = nullptr;

    FieldSymbol(std::string const &name, Type *type);
  };

  // A small root type class. Used for full types and type aliases.
  class Type : public Symbol {
  public:
    Type *baseType; // The 'parent' type of this type or null if this is a fundamental type.
    // Also used as the target type for type aliases.

    Type(std::string const &name, Type *base = nullptr);
  };

  // A type with concrete properties.
  class FullType : public Type {
  public:
    enum FundamentalType {
      Invalid,   // Not known/initialized.
      Inherited, // baseType must be valid.
      Integer,
      Float,
      String,
    } fundamentalType = Invalid;

    enum ReferenceType {
      Pointer,   // Default for most languages ("Type*" in C++).
      Reference, // "Type&" in C++
      Instance,  // "Type" as such
    } referenceType = Pointer;

    size_t storageSize = 0; // For numeric types only (bits).
    bool isSigned = false;

    FullType(std::string const &name);
  };

  // Classes and structs.
  class ClassSymbol : virtual public ScopedSymbol, virtual public Type {
  public:
    bool isStruct = false;

    // Usually only one member, unless your language supports multiple inheritance.
    std::vector<ClassSymbol *> superClasses;

    ClassSymbol(std::string const &name, ClassSymbol *aSuperClass);

    std::vector<MethodSymbol *> getMethods(bool includeInherited = false) const; // Returns a list of all methods.
    std::vector<FieldSymbol *> getFields(bool includeInherited = false) const;   // Returns all fields.
  };

  class ArrayType : public Type {
  public:
    Type *elementType;
    size_t size = 0; // > 0 if fixed length.

    ArrayType(std::string const &name, Type *elemType, size_t aSize = 0);
  };

  // An alias for another type.
  class TypeAlias : public Type {
  public:
    TypeAlias(std::string const &name, Type *target);
  };

  // A few more types for databases.
  class CatalogSymbol : public ScopedSymbol {
  public:
    CatalogSymbol(std::string const &name) : ScopedSymbol(name) {
    }
  };

  class SchemaSymbol : public ScopedSymbol {
  public:
    SchemaSymbol(std::string const &name) : ScopedSymbol(name) {
    }
  };

  class TableSymbol : public ScopedSymbol {
  public:
    TableSymbol(std::string const &name) : ScopedSymbol(name) {
    }
  };

  class ViewSymbol : public ScopedSymbol {
  public:
    ViewSymbol(std::string const &name) : ScopedSymbol(name) {
    }
  };

  class EventSymbol : public ScopedSymbol {
  public:
    EventSymbol(std::string const &name) : ScopedSymbol(name) {
    }
  };

  class ColumnSymbol : public TypedSymbol {
  public:
    ColumnSymbol(std::string const &name, Type *type) : TypedSymbol(name, type) {
    }
  };

  class IndexSymbol : public Symbol { // Made of columns, but doesn't contain them. Hence not a scope.
  public:
    IndexSymbol(std::string const &name) : Symbol(name) {
    }
  };

  class PrimaryKeySymbol : public Symbol { // ditto
  public:
    PrimaryKeySymbol(std::string const &name) : Symbol(name) {
    }
  };

  class ForeignKeySymbol : public Symbol { // ditto
  public:
    ForeignKeySymbol(std::string const &name) : Symbol(name) {
    }
  };

  class StoredRoutineSymbol : public RoutineSymbol {
  public:
    StoredRoutineSymbol(std::string const &name, Type *returnType) : RoutineSymbol(name, returnType) {
    }
  };

  class TriggerSymbol : public ScopedSymbol {
  public:
    TriggerSymbol(std::string const &name) : ScopedSymbol(name) {
    }
  };

  class UdfSymbol : public Symbol { // No body nor parameter info.
  public:
    UdfSymbol(std::string const &name) : Symbol(name) {
    }
  };

  class EngineSymbol : public Symbol {
  public:
    EngineSymbol(std::string const &name) : Symbol(name) {
    }
  };

  class TableSpaceSymbol : public Symbol {
  public:
    TableSpaceSymbol(std::string const &name) : Symbol(name) {
    }
  };

  class LogfileGroupSymbol : public Symbol {
  public:
    LogfileGroupSymbol(std::string const &name) : Symbol(name) {
    }
  };

  class CharsetSymbol : public Symbol {
  public:
    CharsetSymbol(std::string const &name) : Symbol(name) {
    }
  };

  class CollationSymbol : public Symbol {
  public:
    CollationSymbol(std::string const &name) : Symbol(name) {
    }
  };

  class UserVariableSymbol : public VariableSymbol {
  public:
    UserVariableSymbol(std::string const &name, Type *type) : VariableSymbol(name, type) {
    }
  };

  class SystemVariableSymbol : public Symbol {
  public:
    SystemVariableSymbol(std::string const &name) : Symbol(name) {
    }
  };

  // The main class managing all the symbols for a top level entity like a file, library or similar.
  class SymbolTable : public ScopedSymbol {
  public:
    SymbolTable();

    void addDependencies(std::vector<SymbolTable *> const &newDependencies);

    template <typename T, typename... Args>
    T *addNewSymbol(ScopedSymbol *parent, Args &&... args)  {
      T *result = new T(args...);
       if (parent == nullptr) {
         this->addAndManageSymbol(result);
       } else {
         parent->addAndManageSymbol(result);
       }
      return result;
    }

    template <typename T>
    std::vector<T *> getSymbolsOfType(ScopedSymbol *parent = nullptr) const {
      std::vector<T *> result;
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
        return result;
      } else {
        return parent->getSymbolsOfType<T>();
      }
    }

    virtual Symbol *resolve(std::string const &name, bool localOnly = false) const override;

  private:
    // Other symbol information available to this instance.
    std::vector<SymbolTable *> _dependencies;
  };

} // namespace parsers
