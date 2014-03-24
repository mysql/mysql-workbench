import getopt, sys
import os.path
try:
    from pysqlite2 import dbapi2 as sqlite3
except ImportError:
    import sqlite3
import codecs
import re
import threading, Queue, time, datetime
import cgi
from BaseHTTPServer import BaseHTTPRequestHandler, HTTPServer
from urlparse import urlparse
from os import curdir, sep, makedirs
import zipfile

global_app_data_dir = "./"

# --------------------------------
# Log functions

def log_error(messagetext):
  """Log an error message."""
  print "Error: ", messagetext
    

def log_message(messagetext):
  """Log a general message."""
  print messagetext

def verbose_print(verbose, messagetext):
  """Output a general message if verbose is turned on."""
  if verbose:
    print messagetext

# --------------------------------
# Utility functions


def get_app_data_dir():
  if sys.platform.startswith("darwin"):
    path = os.path.expanduser("~/Library/Application Support/MySQL/DocLibrary/")
  elif sys.platform.startswith("win"):
    path = os.path.expandvars("${APPDATA}/MySQL/DocLibrary/")
  else:
    path = os.path.expanduser("~/.mysql/DocLibrary/")

  if not os.path.exists(path): 
    os.makedirs(path)

  return path

def get_lib_db_path():
  path = os.path.join(get_app_data_dir(), "mysqldoclib.sqlite")
  if os.path.exists(path):
    return path
  return os.path.join(global_app_data_dir, "mysqldoclib.sqlite")
	
  
def get_webui_db_path():
  path = os.path.join(get_app_data_dir(), "mysqldoclib_webui.sqlite")
  if os.path.exists(path):
    return path
  return os.path.join(global_app_data_dir, "mysqldoclib_webui.sqlite")

def get_user_db_path():
  path = os.path.join(get_app_data_dir(), "mysqldoclib_usr.sqlite")
  if not os.path.exists(path):
    try:
      # Connect to the database
      db_conn = sqlite3.connect(path)
      try:
        # Execute SQL
        try:
          c = db_conn.cursor()
          try:
            c.execute("""
              CREATE TABLE IF NOT EXISTS page_rating (
                id_page_rating INTEGER PRIMARY KEY AUTOINCREMENT,
                id_manual_type INTEGER NOT NULL,
                title TEXT NOT NULL,
                rating INTEGER
              )""")
            c.execute("CREATE INDEX IF NOT EXISTS idx_page_rating ON page_rating(title)")
            
            c.execute("""
              CREATE TABLE IF NOT EXISTS page_view (
                id_page_view INTEGER PRIMARY KEY AUTOINCREMENT,
                path TEXT NOT NULL,
                hits INTEGER
              )""")
            c.execute("CREATE INDEX IF NOT EXISTS idx_page_view ON page_view(path)")
            
            c.execute("""
              CREATE TABLE IF NOT EXISTS lib_search (
                id_lib_search INTEGER PRIMARY KEY AUTOINCREMENT,
                search_text TEXT NOT NULL,
                hits INTEGER
              )""")
            c.execute("CREATE INDEX IF NOT EXISTS idx_lib_search ON lib_search(search_text)")

            c.execute("""
              CREATE TABLE IF NOT EXISTS lib_status (
                id_lib_file INTEGER PRIMARY KEY AUTOINCREMENT,
                id_lib INTEGER,
                downloaded INTEGER,
                is_selected INTEGER
              )""")
            c.execute("INSERT INTO lib_status(id_lib, downloaded, is_selected) VALUES (1, 1, 1)")
            
            db_conn.commit()
          finally:
            c.close()
        except Exception, e:
          log_error("Error while creating the usr database. %r" % e)

      finally:
        db_conn.close();
    except Exception, e:
        log_error("An error occurred while creating the usr database at %s. %r" % (path, e))
        raise e
  return path

def get_module_installation_dir():
  return "./"

def read_file_content(filename, encoding="utf-8"):
  """Read the contents of a text file"""
  try:
    # Open file in read mode using the correct encoding
    f = codecs.open(filename, "r", encoding)
    try:
      # Return the file contents
      return f.read()
    except Exception, e:
      log_error("An error occurred reading from the file %s." % filename)
    finally:
      f.close();
  except Exception, e:
    log_error("An error occurred opening the file %s." % filename)

def save_file_content(filename, content, encoding="utf-8"):
  """Read the contents of a text file"""
  try:
    # Open file in read mode using the correct encoding
    f = codecs.open(filename, "w", encoding)
    try:
      # Return the file contents
      f.write(content)
    except Exception, e:
      log_error("An error occurred reading from the file %s." % filename)
    finally:
      f.close();
  except Exception, e:
    log_error("An error occurred opening the file %s." % filename)

# --------------------------------
# Library creation functions

def execute_sql_script(db_conn, filename, encoding = "UTF-8"):
  """Executes a SQL script file"""
  
  # Read SQL file
  sql_commands = read_file_content(filename, encoding)
  if not sql_commands:
    return False

  try:
    c = db_conn.cursor()
    
    try:
      # Run the SQL Script to create the database
      c.executescript(sql_commands)
      
      db_conn.commit()
      
      return True
  
    except Exception, e:
      log_error("An error occurred while executing the SQL script. %r" % e);
      return False
    finally:
      c.close()
  except Exception, e:
    log_error("An error occurred aquiring a database cursor. %r" % e);
    return False

# --------------------------------
# Manual page caching functions

def html_remove_tags(data):
  """Removes all HTML tags from a given string"""
  # other expression: '<[^>]*?>'
  p = re.compile(r'<[^<]*?/?>')
  return p.sub(' ', data)

def html_remove_extra_spaces(data):
  """Removes all extra spaces from a given string"""
  p = re.compile(r'\s+')
  return p.sub(' ', data)

def html_get_page_title(data):
  """Returns the chapter and title of a manual html page string"""
  p = re.compile(r'\<title\>(?P<Chapter>(Chapter\s)?[A-Z]?[\d\.]+)\s*(?P<Title>.*)\<\/title\>')
  match = p.search(data)
  
  if match:
    return match.group('Chapter'), match.group('Title')
  else:
    p = re.compile(r'\<title\>(?P<Title>.*)\<\/title\>')
    match = p.search(data)
    if match:
      return "", match.group('Title')
    else:
      return "", ""

def html_apply_page_modifications(data):
  """Make required changes to the html"""
  p = re.compile(r'\<\/title\>')
  data = p.sub('</title><link rel="stylesheet" type="text/css" href="/webui/stylesheets/docs.css" />', data)
  
  p = re.compile(r'\starget=\"_top\"')
  data = p.sub('', data)
  return data

class ManualPageData():
  """Data class that holds information about a manual page"""
  filename = ""
  title = ""
  chapter = ""
  content = ""
  html_content = ""

class ScanManualPageThread(threading.Thread):
  filename_queue = 0
  manual_page_data_queue = 0
  lib_zip_file = ""
  def run(self):
    try:
      while True:
        # get a filename from the queue, do not block
        filename = self.filename_queue.get(False)
        #full_filename = os.path.join(self.path, filename)
        
        try:
          # Open HTML file as a utf-8 file
          #html_string = read_file_content(full_filename)
          #if not html_string:
            #continue
          
          html_file = self.lib_zip_file.open(filename)
          html_string = unicode(html_file.read(), "utf-8")
          if not html_string:
            continue
          
          # Make file modifications
          #save_file_content(full_filename, html_apply_page_modifications(html_string))
        
          # Add new page data object to the queue
          manual_page_data = ManualPageData()
          manual_page_data.filename = filename
          manual_page_data.chapter, manual_page_data.title = html_get_page_title(html_string)
          manual_page_data.content = html_remove_extra_spaces(html_remove_tags(html_string))
          manual_page_data.html_content = html_apply_page_modifications(html_string)
          
          self.manual_page_data_queue.put(manual_page_data)
        except Exception, e:
          log_error("An error processing the page. %r" % e)
          break
          
    except Queue.Empty:
      pass

def process_manual_page_data_queue(db_conn, lib_zip_file, path, id_manual, file_nr, file_count, manual_page_data_queue):
  try:
    # Get database cursor
    c = db_conn.cursor()
    files_processed = 0
    
    # Check if there are manual_page_data objects in the queue and if so, process them
    while True:
      try:
        # Fetch manual_page_data object from the queue if available, do not block
        manual_page_data = manual_page_data_queue.get(False)
        
        try:
          # Insert HTML page
          c.execute("INSERT OR REPLACE INTO web_object(path, content_type, content, allow_embedded_code_execution) VALUES(?, ?, ?, ?)",
            ["/" + path + "/" + os.path.basename(manual_page_data.filename), "text/html", manual_page_data.html_content, 0])

          # Insert manual page and content
          c.execute("INSERT INTO page(id_manual, id_web_object, title, chapter) VALUES (?, ?, ?, ?)", 
            [id_manual, c.lastrowid, manual_page_data.title, manual_page_data.chapter])
          c.execute("INSERT INTO page_content(id_page, title, content) VALUES (?, ?, ?)", 
            [c.lastrowid, manual_page_data.title, manual_page_data.content])
 
          files_processed += 1
          if (files_processed % 100 == 0):
            log_message("%d file(s) of %d processed..." % (file_nr + files_processed, file_count));

        except Exception, e:
          log_error("An error occurred while inserting the page values for %s. %r" % (manual_page_data.filename, e))
      except Queue.Empty:
        break
    
    return files_processed
  except Exception, e:
      log_error("An error occurred aquiring a database cursor. %r" % e)


def cache_pages(db_conn, manual_ids):
  try:
    # Get database cursor
    c = db_conn.cursor()
    try:
      # Get all available manuals versions
      c.execute("""-- Select all manuals
        SELECT m.id_manual, m.directory, m.description
        FROM manual m
        ORDER BY m.id_manual""")
      rows = c.fetchall()

    except Exception, e:
      log_error("An error occurred while executing the SQL commands. %r" % e)
    finally:
      c.close()

    # Loop over all manuals and cache the contents of the file directory
    for id_manual, directory, description in rows:
    
      # if the number of manuals has been limited
      if manual_ids:
        # only include the given manual
        if not str(id_manual) in manual_ids:
          log_message("Skipping manual %s." % description)
          continue
      
      zip_file = directory + ".zip"
      
      # Locate the zip file, first in the user app dir
      # zip_file_path = os.path.join(os.path.join(get_app_data_dir(), 'repository'), zip_file)
      # if not os.path.exists(zip_file_path):
      # then in the ./repository dir
      zip_file_path = os.path.join(os.path.join('.', 'repository'), zip_file)
      if not os.path.exists(zip_file_path):
        log_error("The zip file %s cannot be found." % zip_file_path)
        continue
      
      log_message("Processing %s ..." % zip_file_path)
      
      lib_zip_file = zipfile.ZipFile(zip_file_path, 'r')
      try:
        #path = os.path.join('./', directory)
        #files = [file for file in os.listdir(path) if file.lower().endswith(".html")]
        files = [file for file in lib_zip_file.namelist() if file.lower().endswith(".html")]
        file_count = len(files)
        file_nr = 0
        
        log_message("Caching manual %s, processing %d file(s) ..." % (description, file_count))
        
        # Generate synchronization objects
        filename_queue = Queue.Queue() 
        manual_page_data_queue = Queue.Queue() 
        
        # Fill filename queue
        for f in files: #[:1]:
          filename_queue.put(f)
        
        time_start = datetime.datetime.now()
        
        # Start threads
        Pool = []
        for i in range(1):
          thread = ScanManualPageThread()
          thread.filename_queue = filename_queue
          thread.manual_page_data_queue = manual_page_data_queue
          #thread.path = path
          thread.lib_zip_file = lib_zip_file
          
          Pool.append(thread)
          thread.start()
  
        # Wait for threads to complete
        while Pool:
          # Process all objects in queue
          file_nr += process_manual_page_data_queue(db_conn, lib_zip_file, directory, id_manual, file_nr, file_count, manual_page_data_queue)
          
          # Check if there are still threads that are alive
          for index, the_thread in enumerate(Pool):
            if the_thread.isAlive():
              continue
            else:
              del Pool[index]
            break
          
        # Process all objects still left in queue after the threads have all been closed
        file_nr += process_manual_page_data_queue(db_conn, lib_zip_file, directory, id_manual, file_nr, file_count, manual_page_data_queue)
        
        # Get database cursor
        c = db_conn.cursor()
        try:
          # Update manual to be installed
          generation_date = datetime.datetime.now().strftime("%Y-%m-%d")
          c.execute("UPDATE manual SET installed=1, generation_date=? WHERE id_manual=?", (generation_date, id_manual))
    
        except Exception, e:
          log_error("An error occurred while updating the manual entry. %r" % e)
        finally:
          c.close()
  
        db_conn.commit()
      
        time_duration = datetime.datetime.now() - time_start
      
        log_message("%d file(s) of %d processed. Duration %d.%d seconds." % (file_nr, file_count, 
          time_duration.seconds, time_duration.microseconds))
          
        
        # Add the images as web_objects  
        files = [file for file in lib_zip_file.namelist() if file.lower().endswith(".png")]

        log_message("Processing %d image file(s) ..." % len(files))
        
        for filename in files:
          try:
            # Get database cursor
            c = db_conn.cursor()
            
            try:
              image_file = lib_zip_file.open(filename)
              image_file_string = image_file.read()
              if not image_file_string:
                continue
            
              # Insert HTML page
              c.execute("INSERT OR REPLACE INTO web_object(path, content_type, content, allow_embedded_code_execution) VALUES(?, ?, ?, ?)",
                ["/" + directory + "/images/" + os.path.basename(filename), "image/png", sqlite3.Binary(image_file_string), 0])
                
            except Exception, e:
              log_error("An error occurred while inserting the image file %s. %r" % (filename, e))
          except Exception, e:
              log_error("An error occurred aquiring a database cursor. %r" % e)
        
        db_conn.commit()
      except Exception, e:
        log_error("An error occurred while executing the SQL commands. %r" % e)
      finally:
        lib_zip_file.close()
        
  except Exception, e:
      log_error("An error occurred aquiring a database cursor. %r" % e)


def rebuild_lib(verbose, doclib_db_name, manual_ids):
  try:
    # Check which database name to use
    if not doclib_db_name or doclib_db_name == "":
      doclib_db_name = get_lib_db_path()
      
    # Connect to the database
    db_conn = sqlite3.connect(doclib_db_name)
    
    try:
      log_message("Creating the documentation library structure...")
      
      if execute_sql_script(db_conn, './mysqldoclib.sql'):
        log_message("Documentation library created successfully.")
        
        cache_pages(db_conn, manual_ids)
        
        log_message( "Documentation library cached has been filled.")
      else:
        log_message("The documentation library structure has not been created.")

    finally:
      db_conn.close();
  except Exception, e:
      log_error("An error occurred while opening the database connection. %r" % e)


def rebuild_webui(verbose, webui_db_name):
  try:
    # Check which database name to use
    if not webui_db_name or webui_db_name == "":
      webui_db_name = get_lib_db_path()
      
    # Connect to the database
    db_conn = sqlite3.connect(webui_db_name)
    try:
      log_message("Creating the documentation library webui structure...")
      
      execute_sql_script(db_conn, './mysqldoclib_webui.sql')
      
      c = db_conn.cursor()
      try:
        for path, dirs, files in os.walk("webui"):
          for name in files:
            if name.endswith(".html") or name.endswith(".wbp") or name.endswith(".css"):
              # Open HTML file as a utf-8 file
              html_string = read_file_content(os.path.join(path, name))
              if not html_string:
                continue
                
              log_message("Path: %s, File: %s" % (path, name) )
              
              c.execute("INSERT OR REPLACE INTO web_object(path, content_type, content, allow_embedded_code_execution) VALUES(?, ?, ?, ?)",
                ["/" + path + "/" + os.path.basename(name), "text/html", html_string, 1])
            else:
              content_type = ""
              if name.endswith(".png"):
                content_type = "image/png"
              elif name.endswith(".gif"):
                content_type = "image/gif"
              elif name.endswith(".ico"):
                content_type = "image/vnd.microsoft.icon"
                
              img_file = open(os.path.join(path, name), "rb")

              log_message("Path: %s, File: %s" % (path, name) )
              
              c.execute("INSERT OR REPLACE INTO web_object(path, content_type, content, allow_embedded_code_execution) VALUES(?, ?, ?, ?)",
                ["/" + path + "/" + os.path.basename(name), content_type, sqlite3.Binary(img_file.read()), 0])
      finally:
        c.close()
  
      db_conn.commit()
    finally:
      db_conn.close();
  except Exception, e:
      log_error("An error occurred while opening the database connection. %r" % e)

# --------------------------------
# HTTP server functions

def open_lib_db():
  # Connect to the database
  db_conn = sqlite3.connect(get_lib_db_path())
  # Attach webui and usr database
  try:
    c = db_conn.cursor()
    try:
      c.execute("ATTACH DATABASE ? AS webui", (get_webui_db_path(),))
      c.execute("ATTACH DATABASE ? AS usr", (get_user_db_path(),))
    finally:
      c.close()
  except Exception, e:
    log_error("Could not attach webui or usr database. %r" % e)

  return db_conn

def build_search_result_page(search_string, manual_type):
  # Escape search string
  html_escape_table = {
    "&": "&amp;",
    '"': "&quot;",
    "'": "&apos;",
    ">": "&gt;",
    "<": "&lt;",
  }
  
  search_string_html = "".join(html_escape_table.get(c,c) for c in search_string)
  search_string = search_string.replace(";", "\\;").replace("'", "\\'")

  html = '''<html>
<head>
  <meta http-equiv="Content-Type" content="text/html; charset=utf-8">
  <link rel="stylesheet" type="text/css" href="stylesheets/ui.css" />
  <title>MySQL Workbench Documentation Library Search Result</title>
</head>
<body class="search">'''

  try:
    # Connect to the database
    db_conn = open_lib_db()
    try:
      # Get database cursor
      c = db_conn.cursor()

      # Log search
      c.execute("SELECT hits FROM usr.lib_search WHERE search_text = ?", (search_string,))
      rows = c.fetchall()
      if rows:
        c.execute("UPDATE usr.lib_search SET hits = hits + 1 WHERE search_text = ?", (search_string,))
      else:
        c.execute("INSERT INTO usr.lib_search (search_text, hits) VALUES(?, 1)", (search_string,))
        
      db_conn.commit()
 
      # Do the search
      sql_select = """
        SELECT p.chapter, p.title, wpo.path, pr.rating, m.directory,
            offsets(page_content) as fs_offsets, snippet(page_content) as snippet
        FROM page_content pc
          JOIN page p ON p.id_page = pc.id_page
          JOIN manual m ON m.id_manual = p.id_manual
          JOIN web_object wpo ON p.id_web_object = wpo.id_web_object
          LEFT OUTER JOIN page_rating pr ON p.title = pr.title
          LEFT OUTER JOIN usr.page_rating pru ON p.title = pru.title
        WHERE page_content MATCH '""" + search_string + "'"
      if int(manual_type) > 0:
        sql_select += " AND m.id_manual = " + str(manual_type) + " "

      sql_select += """
        ORDER BY pru.rating DESC, substr(fs_offsets, 1, 1), pr.rating DESC
        LIMIT 0, 51"""
      
      try:
        # Get search result
        c.execute(sql_select)
        
        rows = c.fetchall()
        
        html += "<h2>MySQL Document Manual Search</h2><hr/><p class='search_header'>"
        html += "Search Result for <b>`%s`</b> returned " % search_string_html
        
        if len(rows) > 50:
          html += "more than <b>50</b> matches.<br>Only the first 50 matches are displayed."
        else:
          html += "<b>%d</b> matches." % len(rows)
          
        html += "</p><br>"
        
        for chapter, title, path, rating, directory, offsets, snippet in rows:
          html += "<p class='search'><a href='" + path + "'>"
          html += chapter + " <b>" + title + "</b></a></p>"
          html += "<p class='search_snippet'>" + snippet + "</p><br>"
          
      except Exception, e:
        log_error("An error occurred while executing the SQL command. %r" % e)
        html += "<br>An error occurred while executing the SQL command.<br>%r" % str(e)
      finally:
        c.close()
  
    finally:
      db_conn.close();
  except Exception, e:
      log_error("An error occurred while opening the database connection. %r" % e)
      
  html += '''</body>
</html>
'''
  return html

class DocsLibHandler(BaseHTTPRequestHandler):
   
  def do_GET(self):
    try:
      url_full = self.path
      
      if url_full == "/":
        self.send_response(301)
        self.send_header("Location", "/webui/index.wbp")
        self.end_headers()
        return
      
      url_parsed = urlparse(url_full)
      url = url_parsed.path
      
      content_type = ""
      
      if url.endswith(".html"):
        content_type = "text/html"
      elif url.endswith(".css"):
        content_type = "text/css"
      elif url.endswith(".png"):
        content_type = "image/png"
      elif url.endswith(".gif"):
        content_type = "image/gif"
      elif url.endswith(".ico"):
        content_type = "image/vnd.microsoft.icon"
      elif url.endswith(".wbp"):
        content_type = "text/html"
      
      if len(content_type) > 0:
        if url.endswith("search.wbp"):
          log_message("Query: %s" % url_parsed.query)
          
          d = dict([(k,v) for k,junk,v in [line.partition("=") for line in url_parsed.query.split("&")]])
        
          search_string = d["search"].replace("+", " ").replace("%22", "\"")
          manual_type = d["manual_type"].strip()
  
          log_message("Search started, search_string: %s, manual_type: %d" % (search_string, int(manual_type)))
          
          self.send_response(200)
          self.send_header("Content-type", content_type)
          self.end_headers()
          self.wfile.write(build_search_result_page(search_string, int(manual_type)).encode("utf-8"))
          
        else:
          # check for web object in database
          # Get database cursor
          try:
            c = self.server.db_conn.cursor()
            
            try:
              database = ""
              if url.startswith("/webui"):
                database = "webui."
              
              # Get search result
              c.execute("""
                SELECT content_type, content, allow_embedded_code_execution 
                FROM """ + database + """web_object
                WHERE path = ?""", [url])
              
              rows = c.fetchall()
              
              if rows:
                for wo_content_type, wo_content, wo_allow_embedded_code_execution in rows:
                  self.send_response(200)
                  self.send_header("Content-type", wo_content_type)
                  self.end_headers()
                  if wo_content_type.startswith("text/"):
                    self.wfile.write(wo_content.encode("utf-8"))
                  else:
                    self.wfile.write(wo_content)
                  
                  # Count hits
                  c.execute("SELECT hits FROM usr.page_view WHERE path = ?", (url,))
                  rows = c.fetchall()
                  if rows:
                    c.execute("UPDATE usr.page_view SET hits = hits + 1 WHERE path = ?", (url,))
                  else:
                    c.execute("INSERT INTO usr.page_view (path, hits) VALUES(?, 1)", (url,))
                    
                  self.server.db_conn.commit()
              else:
                try:
                  f = open(curdir + sep + url)
                  self.send_response(200)
                  self.send_header("Content-type", content_type)
                  self.end_headers()
                  self.wfile.write(f.read())
                  f.close()
                except Exception, e:
                  self.send_error(404, "File Not Found: %s" % url)
                  log_error("File not found. %r" % e)
                  
            except Exception, e:
              self.send_error(404, "An error occurred. %r" % e)
              log_error("An error occurred while executing the SQL command. %r" % e)

            finally:
              c.close()
          except Exception, e:
            log_error("An error occurred while opening the database connection. %r" % e)
          


    except IOError:
      self.send_error(404, "File Not Found: %s" % url)
  
  def do_POST(self):
    try:
      ctype, pdict = cgi.parse_header(self.headers.getheader("content-type"))
      if ctype == "text/plain":
        log_message("Header Items: %s" % self.headers.items())
      
        # Get submitted values
        values = ""
        if self.headers.has_key('content-length'):
          length = int( self.headers['content-length'] )
          values = self.rfile.read(length)
        
        # AppleWebKit uses & as separators between values
        if self.headers.has_key('user-agent'):
          if "AppleWebKit" in self.headers['user-agent']:
            values = values.replace("&", "\n")

        d = dict([(k,v) for k,junk,v in [line.partition("=") for line in values.split("\n")]])
        
        search_string = d["search"].strip()
        manual_type = d["manual_type"].strip()

        log_message("Search started, search_string: %s, manual_type: %d" % (search_string, int(manual_type)))
        
        self.send_response(301)
        
        self.end_headers()
        
        self.wfile.write(build_search_result_page(search_string, int(manual_type)).encode("utf-8"))

      else: 
        self.send_error(404, "Wrong content-type" % ctype)
        self.end_headers()
        self.wfile.write("<HTML>Wrong content-type<BR><BR>")
      
    except Exception, e:
      verbose_print(self.server.verbose, "An Exception was raised while processing the POST handler. %r" % e)

  def log_message(self, message, *args):
    verbose_print(self.server.verbose, message % args)
  
  def log_request(self, code='-', size='-'):
    verbose_print(self.server.verbose, '"%s" %s %s' % (self.requestline, str(code), str(size)))
 
  def log_error(self, message, *args):
    verbose_print(self.server.verbose, message % args)

def serve_docs(port = 8080, verbose = 1, datadir= "./", ready_event=None, bind=''):
  global global_app_data_dir
  global_app_data_dir = datadir
  try:
    try:
      # Connect to the database
      db_conn = open_lib_db()
      try:
        server = HTTPServer((bind, port), DocsLibHandler)
        server.verbose = verbose
        server.db_conn = db_conn
        
        verbose_print(verbose, "Started HTTP server on port %d." % port)
        if ready_event:
            ready_event.set()
        server.serve_forever()
      finally:
        db_conn.close();
    except Exception, e:
        log_error("An error occurred while opening the database connection. %r" % e)
        raise e
  except KeyboardInterrupt:
    verbose_print(verbose, "Keyboard interrupt received, shutting down HTTP server.")
    server.socket.close()


# --------------------------------
# Main application

def usage():
  print """MySQL Document Library Standalone Application - mysqldoclib.py
Usage: wbdocs.py -h -pPort -v [build-lib | rebuild-lib | build-webui | rebuild-webui | serve-docs]

This applications serves and maintains a documentation library for MySQL products.

build-lib | rebuild-lib
  These commands create the documentation library repository.

build-webui | rebuild-webui
  These commands rebuild the webui repository that is used to server the web pages.

serve-docs
  This argument launches a web server to server the documentation library"""

def main(argv):
  try:
    opts, args = getopt.getopt(sys.argv[1:], "hp:d:v", ["help", "port=", "db="])
  except getopt.GetoptError, e:
    print "Invalid option passed to module. ", str(e)
    usage()
    sys.exit(2)
    
  verbose = False
  port = 8080
  db_name = ""
  timestamp = ""
  
  for o, a in opts:
    if o == "-v":
        verbose = True
    elif o in ("-h", "--help"):
        usage()
        sys.exit()
    elif o in ("-p", "--port"):
      if a.isdigit():
        port = int(a)
      else:
        assert False, "The specified port must be a number."
    elif o in ("-d", "--db"):
      db_name = a
    else:
        assert False, "Unhandled option."
        
  if args:
    if args[0] in ("build-lib", "rebuild-lib"):
      rebuild_lib(verbose, db_name, args[1:])
    if args[0] in ("build-webui", "rebuild-webui"):
      rebuild_webui(verbose, db_name)
    elif args[0] == "serve-docs":
      serve_docs(port, verbose)
  else:
    usage()

if __name__ == "__main__":
  main(sys.argv[1:])
  

  