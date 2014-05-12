
import java.io.*;

public class BisonHelper {

    public static void main(String[] args) throws IOException {
        File input = new File("5.0-sql_yacc.old.yy");
        InputStream is = new FileInputStream(input);
        byte[] content = new byte[(int) input.length()];
        is.read(content, 0, content.length);
        int cTextDepth = 0;
        boolean ignoreNext = false;
        File output = new File("5.0.sql_yacc.old.yy.txt");
        OutputStream os = new FileOutputStream(output);

        for (int i = 0; i < content.length; i++) {
            byte b = content[i];
            if((b == '\r') || (b == '\n')) {
                os.write(b);
            } else if((!ignoreNext) && (b == '{')) {
                cTextDepth++;
                if(cTextDepth == 1) {
                    os.write(b);
                }
            } else if((!ignoreNext) && (b == '}')) {
                cTextDepth--;
                if(cTextDepth == 0) {
                    os.write(b);
                }
            } else if(cTextDepth == 0) {
                os.write(b);
            }
            ignoreNext = (b == '\'');
        }

        is.close();
        os.close();
    }
}
