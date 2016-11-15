include_directories(.
    ../include
    ../src
    ../lexlib
    ${GTK3_INCLUDE_DIRS}
)

set(SCINTILLA_SOURCES
    scintilla-marshal.c
    PlatGTK.cxx
    ScintillaGTK.cxx
    ../src/AutoComplete.cxx
    ../src/CallTip.cxx
    ../src/CaseConvert.cxx
    ../src/CaseFolder.cxx
    ../src/Catalogue.cxx
    ../src/CellBuffer.cxx
    ../src/CharClassify.cxx
    ../src/ContractionState.cxx
    ../src/Decoration.cxx
    ../src/Document.cxx
    ../src/EditModel.cxx
    ../src/Editor.cxx
    ../src/EditView.cxx
    ../src/ExternalLexer.cxx
    ../src/Indicator.cxx
    ../src/KeyMap.cxx
    ../src/LineMarker.cxx
    ../src/MarginView.cxx
    ../src/PerLine.cxx
    ../src/PositionCache.cxx
    ../src/RESearch.cxx
    ../src/RunStyles.cxx
    ../src/ScintillaBase.cxx
    ../src/Selection.cxx
    ../src/Style.cxx
    ../src/UniConversion.cxx
    ../src/ViewStyle.cxx
    ../src/XPM.cxx
    ../lexlib/Accessor.cxx
    ../lexlib/CharacterCategory.cxx
    ../lexlib/CharacterSet.cxx
    ../lexlib/LexerBase.cxx
    ../lexlib/LexerModule.cxx
    ../lexlib/LexerNoExceptions.cxx
    ../lexlib/LexerSimple.cxx
    ../lexlib/PropSetSimple.cxx
    ../lexlib/StyleContext.cxx
    ../lexlib/WordList.cxx
    ../lexers/LexA68k.cxx
    ../lexers/LexAbaqus.cxx
    ../lexers/LexAda.cxx
    ../lexers/LexAPDL.cxx
    ../lexers/LexAsm.cxx
    ../lexers/LexAsn1.cxx
    ../lexers/LexASY.cxx
    ../lexers/LexAU3.cxx
    ../lexers/LexAVE.cxx
    ../lexers/LexAVS.cxx
    ../lexers/LexBaan.cxx
    ../lexers/LexBash.cxx
    ../lexers/LexBasic.cxx
    ../lexers/LexBatch.cxx
    ../lexers/LexBibTeX.cxx
    ../lexers/LexBullant.cxx
    ../lexers/LexCaml.cxx
    ../lexers/LexCLW.cxx
    ../lexers/LexCmake.cxx
    ../lexers/LexCOBOL.cxx
    ../lexers/LexCoffeeScript.cxx
    ../lexers/LexConf.cxx
    ../lexers/LexCPP.cxx
    ../lexers/LexCrontab.cxx
    ../lexers/LexCsound.cxx
    ../lexers/LexCSS.cxx
    ../lexers/LexD.cxx
    ../lexers/LexDiff.cxx
    ../lexers/LexDMAP.cxx
    ../lexers/LexDMIS.cxx
    ../lexers/LexECL.cxx
    ../lexers/LexEiffel.cxx
    ../lexers/LexErlang.cxx
    ../lexers/LexErrorList.cxx
    ../lexers/LexEScript.cxx
    ../lexers/LexFlagship.cxx
    ../lexers/LexForth.cxx
    ../lexers/LexFortran.cxx
    ../lexers/LexGAP.cxx
    ../lexers/LexGui4Cli.cxx
    ../lexers/LexHaskell.cxx
    ../lexers/LexHex.cxx
    ../lexers/LexHTML.cxx
    ../lexers/LexInno.cxx
    ../lexers/LexJSON.cxx
    ../lexers/LexKix.cxx
    ../lexers/LexKVIrc.cxx
    ../lexers/LexLaTeX.cxx
    ../lexers/LexLisp.cxx
    ../lexers/LexLout.cxx
    ../lexers/LexLua.cxx
    ../lexers/LexMagik.cxx
    ../lexers/LexMake.cxx
    ../lexers/LexMarkdown.cxx
    ../lexers/LexMatlab.cxx
    ../lexers/LexMetapost.cxx
    ../lexers/LexMMIXAL.cxx
    ../lexers/LexModula.cxx
    ../lexers/LexMPT.cxx
    ../lexers/LexMSSQL.cxx
    ../lexers/LexMySQL.cxx
    ../lexers/LexNimrod.cxx
    ../lexers/LexNsis.cxx
    ../lexers/LexNull.cxx
    ../lexers/LexOpal.cxx
    ../lexers/LexOScript.cxx
    ../lexers/LexPascal.cxx
    ../lexers/LexPB.cxx
    ../lexers/LexPerl.cxx
    ../lexers/LexPLM.cxx
    ../lexers/LexPO.cxx
    ../lexers/LexPOV.cxx
    ../lexers/LexPowerPro.cxx
    ../lexers/LexPowerShell.cxx
    ../lexers/LexProgress.cxx
    ../lexers/LexProps.cxx
    ../lexers/LexPS.cxx
    ../lexers/LexPython.cxx
    ../lexers/LexR.cxx
    ../lexers/LexRebol.cxx
    ../lexers/LexRegistry.cxx
    ../lexers/LexRuby.cxx
    ../lexers/LexRust.cxx
    ../lexers/LexScriptol.cxx
    ../lexers/LexSmalltalk.cxx
    ../lexers/LexSML.cxx
    ../lexers/LexSorcus.cxx
    ../lexers/LexSpecman.cxx
    ../lexers/LexSpice.cxx
    ../lexers/LexSQL.cxx
    ../lexers/LexSTTXT.cxx
    ../lexers/LexTACL.cxx
    ../lexers/LexTADS3.cxx
    ../lexers/LexTAL.cxx
    ../lexers/LexTCL.cxx
    ../lexers/LexTCMD.cxx
    ../lexers/LexTeX.cxx
    ../lexers/LexTxt2tags.cxx
    ../lexers/LexVB.cxx
    ../lexers/LexVerilog.cxx
    ../lexers/LexVHDL.cxx
    ../lexers/LexVisualProlog.cxx
    ../lexers/LexYAML.cxx
)

add_library(wbscintilla
  ${SCINTILLA_SOURCES})

set(NEW_WB_CFLAGS ${WB_CFLAGS})
list(REMOVE_ITEM NEW_WB_CFLAGS -Werror)
list(APPEND NEW_WB_CFLAGS -DGTK -DSCI_LEXER -DSCI_NAMESPACE -Wno-deprecated-register)
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  list(APPEND NEW_WB_CFLAGS -DNO_CXX11_REGEX)
endif()
target_compile_options(wbscintilla PRIVATE ${NEW_WB_CFLAGS})
target_link_libraries(wbscintilla ${GTK3_LIBRARIES})


foreach(ITEM in ${SCINTILLA_SOURCES})
  get_filename_component(FILEEXT ${ITEM} EXT)
  if ("${FILEEXT}" STREQUAL ".cxx")
    set_source_files_properties(${ITEM} PROPERTIES COMPILE_FLAGS -std=c++11)
  endif()
endforeach(ITEM in ${SCINTILLA_SOURCES})

if(BUILD_FOR_TESTS)
  target_link_libraries(wbscintilla gcov)
endif()

if(COMMAND cotire)
    set_target_properties(wbscintilla PROPERTIES
        COTIRE_PREFIX_HEADER_IGNORE_PATH "${PRECOMPILED_HEADERS_EXCLUDE_PATHS}")
    
  cotire(wbscintilla)
endif()

install(TARGETS wbscintilla DESTINATION ${WB_INSTALL_LIB_DIR})
