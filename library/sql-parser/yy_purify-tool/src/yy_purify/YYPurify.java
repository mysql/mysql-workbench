package yy_purify;
/*
 * YYPurify.java
 *
 * Created on 30 Май 2007 г., 20:38
 *
 * To change this template, choose Tools | Template Manager
 * and open the template in the editor.
 */

import java.io.*;

/**
 *
 * @author serg
 */
public class YYPurify {
    
    enum Mode {mUnknown, mCutSemantics, mPurifyGrammar};

    /**
     * Creates a new instance of YYPurify
     */
    public YYPurify() {
    }
    
    /**
     * @param args the command line arguments
     */
    public static void main(String[] args) throws IOException {
        if (3 != args.length) {
            System.err.println("Number of given parameters is inappropriate.\n" +
                    "Syntax:\n" +
                    "-(c|p) in_file out_file\n" +
                    ",where\n" +
                    "-c to cut semantics\n" +
                    "-p to purify grammar");
            return;
        }            

        String flag= args[0];
        String inFileName= args[1];
        String outFileName= args[2];
        
        Mode mode= Mode.mUnknown;
        if (flag.equals("-c"))
            mode= Mode.mCutSemantics;
        else if (flag.equals("-p"))
            mode= Mode.mPurifyGrammar;
        
        switch (mode) {
            case mCutSemantics: {
                System.out.print("Cut semantics: ");
                break;
            }
            case mPurifyGrammar: {
                System.out.print("Purify grammar: ");
                break;
            }
            case mUnknown: {
                System.err.println("Unknown flag given (" + flag + ").\n" +
                        "-c to cut semantics\n" +
                        "-p to purify grammar");
                return;
            }
        }
        
        System.out.print(args[1]);
        System.out.print(" -> ");
        System.out.println(args[2]);
        
        File input = new File(args[1]);
        InputStream is = new FileInputStream(input);
        File output = new File(args[2]);
        OutputStream os = new FileOutputStream(output);

        byte[] content = new byte[(int) input.length()];
        is.read(content, 0, content.length);

        switch (mode) {
            case mCutSemantics: {
                cutSemantics(is, os, content);
                break;
            }
            case mPurifyGrammar: {
                purifyGrammar(is, os, content);
                break;
            }
        }
        
        is.close();
        os.close();
    }
    
    public static void cutSemantics(InputStream is, OutputStream os, byte[] content) throws IOException {
        int textDepth = 0;
        boolean ignoreNext = false;

        for (int i = 0; i < content.length; i++) {
            byte b = content[i];
            
            if((b == '\r') || (b == '\n')) {
                os.write(b);
            } else if((!ignoreNext) && (b == '{')) {
                textDepth++;
                if(textDepth == 1)
                    os.write(b);
            } else if((!ignoreNext) && (b == '}')) {
                textDepth--;
                if(textDepth == 0)
                    os.write(b);
            } else if(textDepth == 0)
                os.write(b);
            
            ignoreNext = (b == '\''); //ignore literals '{', '}', etc.
        }
    }

    public static void purifyGrammar(InputStream is, OutputStream os, byte[] content) throws IOException {
        int textDepth= 0;
        boolean ignoreNext= false;
        byte lastSymbol= ' ';

        for (int i= 0; i < content.length; ++i) {
            byte b= content[i];
            
            switch (b) {
                case'{': {
                    if (!ignoreNext)
                        textDepth++;
                    break;
                }
                case'}': {
                    if (!ignoreNext)
                        textDepth--;
                    break;
                }
                default: {
                    if (0 == textDepth)
                        os.write(b);
                    break;
                }
            }

            if (('%' == lastSymbol) && ('%' == b)) {
                os.write('\n');
                os.write('\n');
                purifyRulesSection(is, os, content, i);
                break;
            }

            lastSymbol = b;
            ignoreNext = (b == '\''); //ignore literals '{', '}', etc.
        }
    }

    public static void purifyRulesSection(InputStream is, OutputStream os, byte[] content, int i) throws IOException {
        int textDepth= 0;
        boolean ignoreNext= false;
        boolean commentSection= false;
        boolean symbol_processed= false;
        byte lastSymbol= ' ';
        int maxI= content.length-1;
        
        for (++i; i < content.length; ++i) {
            byte b= content[i];

            if (!commentSection) {
                symbol_processed= !ignoreNext;
                if (!ignoreNext)
                    switch (b) {
                        case '{': {
                            textDepth++;
                            break;
                        }
                        case '}': {
                            textDepth--;
                            break;
                        }
                        case '/': {
                            if (i < maxI && ('*' == content[i+1])) {
                                commentSection= true;
                                ++i;
                                break;
                            }
                        }
                        default: {
                            symbol_processed= false;
                            break;
                        }
                    }

                if (!symbol_processed && (0 == textDepth)) {
                    symbol_processed= !ignoreNext;
                    if (!ignoreNext)
                        switch (b) {
                            case ';': {
                                os.write(b);
                                os.write('\n');
                                os.write('\n');
                                break;
                            }
                            case ':': {
                                os.write(b);
                                os.write('\n');
                                os.write(' ');
                                break;
                            }
                            case '|': {
                                os.write('\n');
                                os.write(b);
                                break;
                            }
                            default: {
                                symbol_processed= false;
                                break;
                            }
                        }

                    if (!symbol_processed)
                        switch (b) {
                            case '\n':
                            case '\r':
                            case '\t':
                            case ' ': {
                                break;
                            }
                            default: {
                                if (('\t' == lastSymbol) || (' ' == lastSymbol))
                                    os.write(' ');
                                os.write(b);
                                break;
                            }
                        }
                }
                lastSymbol= b;
                ignoreNext= ('\'' == b); //ignore literals '{', '}', etc.
            } else {
                if ('*' == b) {
                    if (i > 0 && ('/' == content[i+1])) {
                        commentSection= false;
                        ++i;
                    }
                }
            }
        }
    }
}
