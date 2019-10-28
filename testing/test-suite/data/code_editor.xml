<?xml version="1.0" encoding="utf-8"?>
<languages>
  <!--
    Settings for scintilla-based code editors in MySQL Workbench.
    
    For a list of possible style identifiers see the SciLexer.h file.
    Keyword list identifiers are taken from the various Lex*.cpp files in Scintilla.
  -->

  <language name="SCLEX_MYSQL">
    <!-- This is the base language setting. It's usually not directly used, but provides values shared by
         more specialized MySQL versions. -->
    
    <!-- Lexer properties -->
    <property name="fold" value="1" />
    <property name="fold.compact" value="0" />
    <property name="fold.comment" value="1" />

    <!-- Editor settings -->
    <setting name="usetabs" value="1" />
    <setting name="tabwidth" value="4" />
    <setting name="indentation" value="4" />

    <!-- Keep in mind to use the same list names as defined in the lexers (e.g. LexMySQL.cpp). -->
    <keywords name="Major Keywords">
      <!-- Keywords that can start a statement. No longer used. -->
    </keywords>

    <keywords name="Keywords">
      <!-- Normal keywords. No longer kept here but come from the keywords_list.h file provided by the server itself. -->
    </keywords>

    <keywords name="Procedure keywords">
      begin end comment
    </keywords>

    <keywords name="User Keywords 1">
      delimiter
    </keywords>

    <!-- These two lists are not used for syntax highlighting but assemble keywords from the other lists that serve a special purpose. -->
    <keywords name="User Keywords 2"> <!-- Keywords that are allowed at the start of an expression. -->
      binary case cast convert exists interval match not row
    </keywords>

    <keywords name="User Keywords 3"> <!-- Like user 2 but those that can appear between two expressions. -->
      all and any as between escape false in is like or regexp sounds true unknown xor
    </keywords>
    
    <style id="0" fore-color-light="#284444" fore-color-dark="#808a8c" /> <!-- SCE_MYSQL_DEFAULT -->

    <style id="1" fore-color-light="#0987cb" fore-color-dark="#0a99e5" /> <!-- SCE_MYSQL_COMMENT -->
    <style id="2" fore-color-light="#0987cb" fore-color-dark="#0a99e5" /> <!-- SCE_MYSQL_COMMENTLINE -->
    <style id="21" back-color-light="#F0F0F0" back-color-dark="#404040" /> <!-- SCE_MYSQL_HIDDENCOMMAND -->

    <style id="3" fore-color-light="#63bf8d" fore-color-dark="#63bf8d" /> <!-- SCE_MYSQL_VARIABLE -->
    <style id="4" fore-color-light="#45aa73" fore-color-dark="#45aa73" /> <!-- SCE_MYSQL_SYSTEMVARIABLE -->
    <style id="5" fore-color-light="#45aa73" fore-color-dark="#45aa73" /> <!-- SCE_MYSQL_KNOWNSYSTEMVARIABLE -->

    <style id="6" fore-color-light="#cc6c00" fore-color-dark="#e57a00" /> <!-- SCE_MYSQL_NUMBER -->
    <style id="12" fore-color-light="#dd7a00" fore-color-dark="#f28600" /> <!-- SCE_MYSQL_SQSTRING -->
    <style id="13" fore-color-light="#dd7a00" fore-color-dark="#f28600" /> <!-- SCE_MYSQL_DQSTRING -->

    <style id="7" bold="Yes" fore-color-light="#007FBF" /> <!-- SCE_MYSQL_MAJORKEYWORD -->
    <style id="8" bold="Yes" fore-color-light="#007FBF" /> <!-- SCE_MYSQL_KEYWORD -->
    <style id="15" fore-color-light="#7d7d63" fore-color-dark="#7db27d" /> <!-- SCE_MYSQL_FUNCTION -->
    <style id="10" fore-color-light="#7d7d63" fore-color-dark="#7db27d" /> <!-- SCE_MYSQL_PROCEDUREKEYWORD -->
    <style id="14" bold="Yes" /> <!-- SCE_MYSQL_OPERATOR -->

    <style id="16" fore-color-light="#000000" fore-color-dark="#FFFFFF" /> <!-- SCE_MYSQL_IDENTIFIER -->
    <style id="17" fore-color-light="#993a3e" fore-color-dark="#e5454c" /> <!-- SCE_MYSQL_QUOTEDIDENTIFIER -->

    <style id="22" fore-color-light="#FFFFFF" back-color-light="#A0A0A0" fore-color-dark="#000000" back-color-dark="#404040" bold="Yes" /> <!-- SCE_MYSQL_PLACEHOLDER -->

    <style id="18" fore-color-light="#007F00" fore-color-dark="#00b200" bold="yes"/> <!-- SCE_MYSQL_USER1 -->

    <!-- All styles again in their variant in a hidden command (with a 0x40 offset). -->
    <style id="65" fore-color-light="#0987cb" back-color-light="#F0F0F0" fore-color-dark="#0a99e5" back-color-dark="#404040" /> <!-- SCE_MYSQL_COMMENT -->
    <style id="66" fore-color-light="#0987cb" back-color-light="#F0F0F0" fore-color-dark="#0a99e5" back-color-dark="#404040" /> <!-- SCE_MYSQL_COMMENTLINE -->

    <style id="67" fore-color-light="#63bf8d" back-color-light="#F0F0F0" fore-color-dark="#63bf8d" back-color-dark="#404040" /> <!-- SCE_MYSQL_VARIABLE -->
    <style id="68" fore-color-light="#45aa73" back-color-light="#F0F0F0" fore-color-dark="#45aa73" back-color-dark="#404040" /> <!-- SCE_MYSQL_SYSTEMVARIABLE -->
    <style id="69" fore-color-light="#45aa73" back-color-light="#F0F0F0" fore-color-dark="#45aa73" back-color-dark="#404040" /> <!-- SCE_MYSQL_KNOWNSYSTEMVARIABLE -->

    <style id="70" fore-color-light="#cc6c00" back-color-light="#F0F0F0" fore-color-dark="#e57a00" back-color-dark="#404040" /> <!-- SCE_MYSQL_NUMBER -->
    <style id="76" fore-color-light="#dd7a00" back-color-light="#F0F0F0" fore-color-dark="#f28600" back-color-dark="#404040" /> <!-- SCE_MYSQL_SQSTRING -->
    <style id="77" fore-color-light="#dd7a00" back-color-light="#F0F0F0" fore-color-dark="#f28600" back-color-dark="#404040" /> <!-- SCE_MYSQL_DQSTRING -->

    <style id="71" back-color-light="#F0F0F0" back-color-dark="#404040" bold="Yes" /> <!-- SCE_MYSQL_MAJORKEYWORD -->
    <style id="72" back-color-light="#F0F0F0" back-color-dark="#404040" bold="Yes"/> <!-- SCE_MYSQL_KEYWORD -->
    <style id="79" fore-color-light="#7d7d63" back-color-light="#F0F0F0" fore-color-dark="#7db27d" back-color-dark="#404040" /> <!-- SCE_MYSQL_FUNCTION -->
    <style id="74" fore-color-light="#7d7d63" back-color-light="#F0F0F0" fore-color-dark="#7db27d" back-color-dark="#404040" /> <!-- SCE_MYSQL_PROCEDUREKEYWORD -->
    <style id="78" back-color-light="#F0F0F0" back-color-dark="#284444" bold="Yes" /> <!-- SCE_MYSQL_OPERATOR -->

    <style id="80" fore-color-light="#000000" back-color-light="#F0F0F0" fore-color-dark="#FFFFFF" back-color-dark="#404040" /> <!-- SCE_MYSQL_IDENTIFIER -->
    <style id="81" fore-color-light="#993a3e" back-color-light="#F0F0F0" fore-color-dark="#e5454c" back-color-dark="#404040" /> <!-- SCE_MYSQL_QUOTEDIDENTIFIER -->

    <style id="86" fore-color-light="#FFFFFF" back-color-light="#A0A0A0" fore-color-dark="#284444" back-color-dark="#404040" bold="Yes" /> <!-- SCE_MYSQL_PLACEHOLDER -->

    <style id="82" fore-color-light="#007F00" back-color-light="#F0F0F0" fore-color-dark="#00b200" back-color-dark="#404040" bold="yes"/> <!-- SCE_MYSQL_USER1 -->

    <!-- Various other styles -->
    <style id="34" back-color-light="#ffee55" back-color-dark="#ffee55" /> <!-- STYLE_BRACELIGHT -->
    <style id="35" back-color-light="#ff7855" back-color-dark="#ff7855" /> <!-- STYLE_BRACELBAD -->
  </language>

  <language name="SCLEX_MYSQL_56">
    <keywords name="Major Keywords">
    </keywords>

    <keywords name="Keywords">
    </keywords>

  </language> <!-- SCLEX_MYSQL_56 -->

  <language name="SCLEX_MYSQL_57">
    <keywords name="Major Keywords">
    </keywords>

    <keywords name="Keywords">
    </keywords>

  </language> <!-- SCLEX_MYSQL_57 -->

  <language name="SCLEX_MYSQL_80">
    <keywords name="Major Keywords">
    </keywords>

    <keywords name="Keywords">
    </keywords>

  </language> <!-- SCLEX_MYSQL_80 -->

  <language name="SCLEX_PYTHON">

    <property name="tab.timmy.whinge.level" value="1" />

    <!-- Editor settings -->
    <setting name="usetabs" value="0" />
    <setting name="tabwidth" value="4" />
    <setting name="indentation" value="4" />

    <!-- Keywords from http://docs.python.org/release/2.3.5/ref/keywords.html -->
    <keywords name="Keywords">
      and del for is raise
      assert elif from lambda return
      break else global not try 
      class except if or while 
      continue exec import pass yield
      def finally in print
      as none
    </keywords>

    <style id="0" fore-color-light="#000000"  fore-color-dark="#808a8c"/> <!-- SCE_P_DEFAULT -->

    <style id="12" fore-color-light="#097BF7" /> <!-- SCE_P_COMMENTBLOCK -->
    <style id="1" fore-color-light="#097BF7" /> <!-- SCE_P_COMMENTLINE -->

    <style id="2" fore-color-light="#3EAAFF" fore-color-dark="#736ca3"/> <!-- SCE_P_NUMBER -->
    <style id="3" fore-color-light="#3EAAFF" fore-color-dark="#ffff3f"/> <!-- SCE_P_STRING -->
    <style id="4" fore-color-light="#3EAAFF" /> <!-- SCE_P_CHARACTER -->
    <style id="6" fore-color-light="#3EAAFF" /> <!-- SCE_P_TRIPLE -->
    <style id="7" fore-color-light="#6D4A27" /> <!-- SCE_P_TRIPLEDOUBLE -->

    <style id="5" fore-color-light="#C00000" bold="Yes" fore-color-dark="#a46f3f"/> <!-- SCE_P_WORD -->
    <style id="8" fore-color-light="#003690" bold="Yes" fore-color-dark="#a0d975" /> <!-- SCE_P_CLASSNAME -->
    <style id="9" fore-color-light="#7F0000" bold="Yes" fore-color-dark="#a0d975" /> <!-- SCE_P_DEFNAME -->

    <style id="10" fore-color-light="#000000" bold="Yes" fore-color-dark="#a46f3f"/> <!-- SCE_P_OPERATOR -->

    <style id="11" fore-color-light="#000000" fore-color-dark="#FFFFFF"/> <!-- SCE_P_IDENTIFIER -->

    <!-- Various other styles -->
    <style id="34" back-color-light="#ffee55" back-color-dark="#ffee55" /> <!-- STYLE_BRACELIGHT -->
    <style id="35" back-color-light="#ff7855" back-color-dark="#ff7855" /> <!-- STYLE_BRACELBAD -->
  </language>

  <language name="SCLEX_HTML">
    <!-- This is a complex lexer with several sublanguages (html, xml, java script, php script, vb sript, python)-->

    <setting name="usetabs" value="0" />
    <setting name="tabwidth" value="4" />
    <setting name="indentation" value="4" />

    <keywords name="HTML elements and attributes">
      !doctype a abbr accept-charset accept accesskey acronym action address align alink alt applet archive area axis
      b background base basefont bdo bgcolor big blockquote body border br button caption cellpadding cellspacing center
      char charoff charset checkbox checked cite class classid clear code codebase codetype col colgroup color cols colspan
      compact content coords data datafld dataformatas datapagesize datasrc datetime dd declare defer del dfn dir disabled
      div dl dt em enctype event face fieldset file font for form frame frameborder frameset h1 h2 h3 h4 h5 h6 head headers
      height hidden hr href hreflang hspace html http-equiv i id iframe image img input ins isindex ismap kbd label lang
      language leftmargin legend li link longdesc map marginwidth marginheight maxlength media menu meta method multiple
      name noframes nohref noresize noscript noshade nowrap object ol onblur onchange onclick ondblclick onfocus onkeydown
      onkeypress onkeyup onload onmousedown onmousemove onmouseover onmouseout onmouseup optgroup option onreset onselect
      onsubmit onunload p param password profile pre prompt public q radio readonly rel reset rev rows rowspan rules s samp
      scheme scope script select selected shape size small span src standby start strike strong style sub submit summary sup
      tabindex table target tbody td text textarea tfoot th thead title topmargin tr tt type u ul usemap valign value valuetype
      var version vlink vspace width xml xmlns
    </keywords>

    <keywords name="JavaScript keywords">
      abstract boolean break byte case catch char class const continue debugger default delete do double else enum export
      extends final finally float for function goto if implements import in instanceof int interface long native new package
      private protected public return short static super switch synchronized this throw throws transient try typeof var void
      volatile while with true false prototype
    </keywords>
      
    <keywords name="VBScript keywords">
      addhandler addressof andalso alias and ansi as assembly attribute auto begin boolean byref byte byval call case catch
      cbool cbyte cchar cdate cdec cdbl char cint class clng cobj compare const continue cshort csng cstr ctype currency date
      decimal declare default delegate dim do double each else elseif end enum erase error event exit explicit false finally
      for friend function get gettype global gosub goto handles if implement implements imports in inherits integer interface
      is let lib like load long loop lset me mid mod module mustinherit mustoverride mybase myclass namespace new next not
      nothing notinheritable notoverridable object on option optional or orelse overloads overridable overrides paramarray
      preserve private property protected public raiseevent readonly redim rem removehandler rset resume return select set
      shadows shared short single static step stop string structure sub synclock then throw to true try type typeof unload
      unicode until variant wend when while with withevents writeonly xor
    </keywords>
        
    <keywords name="Python keywords">
      and as assert break class continue def del elif else except exec finally for from global if import in is lambda None
      not or pass print raise return triple try while with yield
    </keywords>

    <keywords name="PHP keywords">
      and or xor __file__ __line__ array as break case cfunction class const continue declare default die do echo else elseif
      empty enddeclare endfor endforeach endif endswitch endwhile eval exit extends for foreach function global if include
      include_once isset list new old_function print require require_once return static switch unset use var while __function__
      __class__ php_version php_os default_include_path pear_install_dir pear_extension_dir php_extension_dir php_bindir php_libdir
      php_datadir php_sysconfdir php_localstatedir php_config_file_path php_output_handler_start php_output_handler_cont
      php_output_handler_end e_error e_warning e_parse e_notice e_core_error e_core_warning e_compile_error e_compile_warning
      e_user_error e_user_warning e_user_notice e_all true false bool boolean int integer float double real string array object
      resource null class extends parent stdclass directory __sleep __wakeup interface implements abstract public protected private
    </keywords>

    <style id="0" fore-color-light="#000000" /> <!-- SCE_HPHP_DEFAULT -->

    <style id="124" fore-color-light="#F77B09" /> <!-- SCE_HPHP_COMMENT -->
    <style id="125" fore-color-light="#F77B09" /> <!-- SCE_HPHP_COMMENTLINE -->
    
    <style id="121" fore-color-light="#C00000" bold="Yes"/> <!-- SCE_HPHP_WORD -->

    <!-- Various other styles -->
    <style id="34" back-color-light="#ffee55" back-color-dark="#ffee55" /> <!-- STYLE_BRACELIGHT -->
    <style id="35" back-color-light="#ff7855" back-color-dark="#ff7855" /> <!-- STYLE_BRACELBAD -->
  </language>

  <language name="SCLEX_CPP">
    <property name="styling.within.preprocessor" value="0" />
    <property name="lexer.cpp.allow.dollars" value="1" />
    <property name="lexer.cpp.track.preprocessor" value="1" />
    <property name="lexer.cpp.update.preprocessor" value="1" />
    <property name="lexer.cpp.triplequoted.strings" value="1" />
    <property name="lexer.cpp.hashquoted.strings" value="1" />
    <property name="fold.cpp.syntax.based" value="1" />
    <property name="fold.comment" value="1" />
    <property name="fold.cpp.comment.multiline" value="1" />
    <property name="fold.cpp.comment.explicit" value="1" />
    <property name="fold.preprocessor" value="1" />
    <property name="fold.at.else" value="1" />
    
    <!-- Editor settings -->
    <setting name="usetabs" value="0" />
    <setting name="tabwidth" value="2" />
    <setting name="indentation" value="2" />

    <keywords name="Primary keywords and identifiers">
      alignas alignof and and_eq asm auto bitand bitor bool break case catch char char16_t char32_t class compl const constexpr
      const_cast continue decltype default delete do double dynamic_cast else enum explicit export extern false float for friend
      goto if inline int long mutable namespace new noexcept not not_eq nullptr operator or or_eq private protected public register
      reinterpret_cast return short signed sizeof static static_assert static_cast struct switch template this thread_local throw
      true try typedef typeid typename union unsigned using virtual void volatile wchar_t while xor xor_eq 
    </keywords>

    <keywords name="Secondary keywords and identifiers">
    </keywords>

    <keywords name="Documentation comment keywords">
    </keywords>

    <keywords name="Global classes and typedefs">
    </keywords>

    <keywords name="Preprocessor definitions">
      if elseif else endif defined define import include
    </keywords>

    <style id="0" fore-color-light="#000000" /> <!-- SCE_C_DEFAULT -->
    <style id="1" fore-color-light="#097BF7" /> <!-- SCE_C_COMMENT -->
    <style id="2" fore-color-light="#097BF7" /> <!-- SCE_C_COMMENTLINE -->
    <style id="3" fore-color-light="#097BF7" /> <!-- SCE_C_COMMENTDOC -->
    <style id="4" fore-color-light="#FF0000" /> <!-- SCE_C_NUMBER -->
    <style id="5" fore-color-light="#000000"  bold="Yes"/> <!-- SCE_C_WORD -->
    <style id="6" fore-color-light="#3EAAFF" /> <!-- SCE_C_STRING -->
    <style id="7" fore-color-light="#3EAAFF" /> <!-- SCE_C_CHARACTER -->
    <style id="8" fore-color-light="#3EAAFF" /> <!-- SCE_C_UUID -->
    <style id="9" fore-color-light="#000000" /> <!-- SCE_C_PREPROCESSOR -->
    <style id="10" fore-color-light="#000000" /> <!-- SCE_C_OPERATOR -->
    <style id="11" fore-color-light="#000000" /> <!-- SCE_C_IDENTIFIER -->
    <style id="12" fore-color-light="#000000" /> <!-- SCE_C_STRINGEOL -->
    <style id="13" fore-color-light="#000000" /> <!-- SCE_C_VERBATIM -->
    <style id="14" fore-color-light="#000000" /> <!-- SCE_C_REGEX -->
    <style id="15" fore-color-light="#097BF7" /> <!-- SCE_C_COMMENTLINEDOC -->
    <style id="16" fore-color-light="#000000"  bold="Yes" /> <!-- SCE_C_WORD2 -->
    <style id="17" fore-color-light="#000000" /> <!-- SCE_C_COMMENTDOCKEYWORD -->
    <style id="18" fore-color-light="#000000" /> <!-- SCE_C_COMMENTDOCKEYWORDERROR -->
    <style id="19" fore-color-light="#000000"  bold="Yes" /> <!-- SCE_C_GLOBALCLASS -->
    <style id="20" fore-color-light="#3EAAFF" /> <!-- SCE_C_STRINGRAW -->
    <style id="21" fore-color-light="#000000" /> <!-- SCE_C_TRIPLEVERBATIM -->
    <style id="22" fore-color-light="#000000" /> <!-- SCE_C_HASHQUOTEDSTRING -->
    <style id="23" fore-color-light="#000000" /> <!-- SCE_C_PREPROCESSORCOMMENT -->
    <style id="24" fore-color-light="#000000" /> <!-- SCE_C_PREPROCESSORCOMMENTDOC -->

    <!-- Various other styles -->
    <style id="34" back-color-light="#ffee55" back-color-dark="#ffee55" /> <!-- STYLE_BRACELIGHT -->
    <style id="35" back-color-light="#ff7855" back-color-dark="#ff7855" /> <!-- STYLE_BRACELBAD -->
  </language>
	
	<language name="SCLEX_CPP_JS">

    <!-- Editor settings -->
    <property name="fold" value="1" />
    <property name="fold.compact" value="0" />
    <property name="fold.comment" value="1" />
    <property name="fold.at.else" value="1" />

    <setting name="usetabs" value="0" />
    <setting name="tabwidth" value="2" />
    <setting name="indentation" value="2" />

    <keywords name="Primary keywords and identifiers"> <!-- SCE_C_WORD -->
       abstract arguments
       boolean break byte
       case catch char class const continue
       debugger default delete do double
       else enum eval export extends
       false final finally float for function
       goto
       if implements import in instanceof int interface
       let long
       native new null
       package private protected public
       return
       short static super switch synchronized
       this throw throws transient true try typeof
       var void volatile
       while with
       yield
    </keywords>

    <keywords name="Secondary keywords and identifiers"> <!-- SCE_C_WORD2 -->
        add affectedRows all arrayAppend arrayDelete arrayInsert AS ASC
        bind buffer
        change close commit collections columnMetadata create createCollection createIndex createSchema
        defaultSchema delete DESC drop dropIndex dropSchema
        execute executeSql executionTime existInDatabase
        field fields find flush
        getAffectedRows getCollection getCollections getCollectionAsTable getCollectionNames getColumnMetadata getDefaultSchema
        getError getErrorCode getExecutionTime getHasData getInfo getLastInsertId getName getNodeSession getSchema getSchemas
        getSession getTable getTableNames getTables getUri getView getViewNames getViews getWarnings groupBy
        having hasData
        IndexUnique info insert
        lastInsertId limit
        merge modify
        name next nextDataSet nextResultSet
        orderBy offset
        remove rewind rollback
        schema schemas select session set setDefaultSchema setFetchWarnings skip sort sql startTransaction
        tables
        unset update uri
        values views
        warnings where
    </keywords>

    <keywords name="Documentation comment keywords">
    </keywords>

    <keywords name="Global classes and typedefs"> <!-- SCE_C_GLOBALCLASS -->
        mysqlx
    </keywords>

    <keywords name="Preprocessor definitions">
    </keywords>

    <style id="0" fore-color-light="#000000" /> <!-- SCE_C_DEFAULT -->
    <style id="1" fore-color-light="#3e7bd1" /> <!-- SCE_C_COMMENT -->
    <style id="2" fore-color-light="#3e7bd1" /> <!-- SCE_C_COMMENTLINE -->
    <style id="3" fore-color-light="#3e7bd1" /> <!-- SCE_C_COMMENTDOC -->
    <style id="4" fore-color-light="#a35f00" /> <!-- SCE_C_NUMBER -->
    <style id="5" fore-color-light="#000000" bold="Yes" /> <!-- SCE_C_WORD -->
    <style id="6" fore-color-light="#a35f00" /> <!-- SCE_C_STRING -->
    <style id="7" fore-color-light="#a35f00" /> <!-- SCE_C_CHARACTER -->
    <style id="8" fore-color-light="#a35f00" /> <!-- SCE_C_UUID -->
    <style id="9" fore-color-light="#000000" /> <!-- SCE_C_PREPROCESSOR -->
    <style id="10" fore-color-light="#000000" /> <!-- SCE_C_OPERATOR -->
    <style id="11" fore-color-light="#000000" /> <!-- SCE_C_IDENTIFIER -->
    <style id="12" fore-color-light="#000000" /> <!-- SCE_C_STRINGEOL -->
    <style id="13" fore-color-light="#000000" /> <!-- SCE_C_VERBATIM -->
    <style id="14" fore-color-light="#000000" /> <!-- SCE_C_REGEX -->
    <style id="15" fore-color-light="#3e7bd1" /> <!-- SCE_C_COMMENTLINEDOC -->
    <style id="16" fore-color-light="#6b298b"  bold="Yes" /> <!-- SCE_C_WORD2 -->
    <style id="17" fore-color-light="#000000" /> <!-- SCE_C_COMMENTDOCKEYWORD -->
    <style id="18" fore-color-light="#000000" /> <!-- SCE_C_COMMENTDOCKEYWORDERROR -->
    <style id="19" fore-color-light="#A0A0A0"  bold="Yes" /> <!-- SCE_C_GLOBALCLASS -->
    <style id="20" fore-color-light="#3EAAFF" /> <!-- SCE_C_STRINGRAW -->
    <style id="21" fore-color-light="#000000" /> <!-- SCE_C_TRIPLEVERBATIM -->
    <style id="22" fore-color-light="#000000" /> <!-- SCE_C_HASHQUOTEDSTRING -->
    <style id="23" fore-color-light="#000000" /> <!-- SCE_C_PREPROCESSORCOMMENT -->
    <style id="24" fore-color-light="#000000" /> <!-- SCE_C_PREPROCESSORCOMMENTDOC -->

    <!-- Various other styles -->
    <style id="34" back-color-light="#ffee55" back-color-dark="#ffee55" /> <!-- STYLE_BRACELIGHT -->
    <style id="35" back-color-light="#ff7855" back-color-dark="#ff7855" /> <!-- STYLE_BRACELBAD -->
  </language>

  <language name="SCLEX_CPP_JSON">
    <!-- Editor settings -->
    <setting name="usetabs" value="1" />
    <setting name="tabwidth" value="4" />
    <setting name="indentation" value="4" />

    <keywords name="Primary keywords and identifiers">
    </keywords>

    <keywords name="Secondary keywords and identifiers">
    </keywords>

    <keywords name="Documentation comment keywords">
    </keywords>

    <keywords name="Global classes and typedefs">
    </keywords>

    <keywords name="Preprocessor definitions">
    </keywords>

    <style id="4" fore-color-light="#ff3e5a" fore-color-dark="#ff3e5a" /> <!-- SCE_C_NUMBER -->
    <style id="6" fore-color-light="#ff9933" fore-color-dark="#ff9933" /> <!-- SCE_C_STRING -->
    <style id="10" fore-color-light="#000000" fore-color-dark="#FFFFFF" bold="YES" /> <!-- SCE_C_OPERATOR -->
    <style id="11" fore-color-light="#3133ff" fore-color-dark="#3133ff" /> <!-- SCE_C_IDENTIFIER -->
    <style id="20" fore-color-light="#ff5c33" fore-color-dark="#ff5c33" /> <!-- SCE_C_STRINGRAW -->

    <!-- Various other styles -->
    <style id="34" back-color-light="#ffee55" back-color-dark="#ffee55" /> <!-- STYLE_BRACELIGHT -->
    <style id="35" back-color-light="#ff7855" back-color-dark="#ff7855" /> <!-- STYLE_BRACELBAD -->
  </language>

</languages>
