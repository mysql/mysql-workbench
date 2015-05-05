add_definitions(-Wno-missing-braces -Wno-missing-field-initializers -Wno-char-subscripts -DGTK -DSCI_LEXER -DSCI_NAMESPACE)

include_directories(.
    ../include
    ../src
    ../lexlib
    ${GTK2_INCLUDE_DIRS}
)

add_library(wbscintilla
    scintilla-marshal.c
    ../src/Selection.cxx
    ../lexlib/Accessor.cxx
    ../lexlib/CharacterSet.cxx
    ../lexlib/LexerBase.cxx
    ../lexlib/LexerModule.cxx
    ../lexlib/LexerNoExceptions.cxx
    ../lexlib/LexerSimple.cxx
    ../lexlib/StyleContext.cxx
    ../lexlib/WordList.cxx
    ../lexlib/PropSetSimple.cxx
    ../lexlib/CharacterCategory.cxx
    ../src/CharClassify.cxx
    ../src/Decoration.cxx
    ../src/Catalogue.cxx
    ../src/Document.cxx
    ../src/CallTip.cxx
    ../src/ScintillaBase.cxx
    ../src/ContractionState.cxx
    ../src/Editor.cxx
    ../src/ExternalLexer.cxx
    ./PlatGTK.cxx
    ../src/KeyMap.cxx
    ../src/LineMarker.cxx
    ../src/PositionCache.cxx
    ./ScintillaGTK.cxx
    ../src/CellBuffer.cxx
    ../src/ViewStyle.cxx
    ../src/RESearch.cxx
    ../src/RunStyles.cxx
    ../src/Style.cxx
    ../src/Indicator.cxx
    ../src/AutoComplete.cxx
    ../src/UniConversion.cxx
    ../src/XPM.cxx
    ../src/PerLine.cxx
    ../src/CaseConvert.cxx
    ../src/CaseFolder.cxx
    ../src/EditModel.cxx
	../src/EditView.cxx
	../src/MarginView.cxx
    ../lexers/LexCPP.cxx
    ../lexers/LexHTML.cxx
    ../lexers/LexMSSQL.cxx
    ../lexers/LexMySQL.cxx
    ../lexers/LexPython.cxx
    ../lexers/LexSQL.cxx
)

target_link_libraries(wbscintilla ${GTK2_LIBRARIES})

install(TARGETS wbscintilla DESTINATION ${WB_INSTALL_LIB_DIR})
