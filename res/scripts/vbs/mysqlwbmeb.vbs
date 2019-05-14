
' Copyright (c) 2012, 2019 Oracle and/or its affiliates. All rights reserved.
'
' This program is free software; you can redistribute it and/or
' modify it under the terms of the GNU General Public License as
' published by the Free Software Foundation; version 2 of the
' License.
'
' This program is distributed in the hope that it will be useful,
' but WITHOUT ANY WARRANTY; without even the implied warranty of
' MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
' GNU General Public License for more details.
'
' You should have received a copy of the GNU General Public License
' along with this program; if not, write to the Free Software
' Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
' 02110-1301  USA

' VBS doesn't provide sorting out of the box
function qsort(arr, first, last)
    if first < last then
        p = qsort_partition(arr, first, last)
        call qsort(arr, first, p - 1)
        call qsort(arr, p + 1, last)
    end if
end function
function qsort_partition(arr, first, last)
    pivot = arr(last)

    i = first
    for j = first to last - 1
        if arr(j) <= pivot then
            ' swap
            tmp = arr(i)
            arr(i) = arr(j)
            arr(j) = tmp

            i = i + 1
        end if
    next

    ' swap
    tmp = arr(i)
    arr(i) = arr(last)
    arr(last) = tmp

    qsort_partition = i
end function

' These variables will be retrieved from the backup profile
class JSONDumper
    public function json_object(element)
        element_type = TypeName(element)
        
        select case element_type
            case "Dictionary"
                fs = ""
                for each item in element.Keys
                    att = json_object(item)
                    val = json_object(element(item))
                    att_val = att & ":" & val
                    
                    if len(fs) > 0 then fs = fs & ","
                    
                    fs = fs & att_val
                next
                json_object = "{" & fs & "}"
            case "String"
                ' Holds the final string
                fs = ""
                for index = 1 to len (element)
                    next_char = mid(element, index, 1)
                    select case next_char
                        case """" fs = fs & "\"""
                        case "\" fs = fs & "\\"
                        case else fs = fs & next_char
                    end select
                next
                json_object = """" & fs & """"
            case "Boolean"
                if element then
                    json_object = "True"
                else
                    json_object = "False"
                end if
        end select
    end function
    
    
end class


class ConfigReader
    private my_file
    private my_file_handler
    public configuration
    
    private sub Class_Initialize()
        Set configuration = CreateObject("Scripting.Dictionary")
    end sub
    
    public sub load(file_name)
        set fso = CreateObject("Scripting.FileSystemObject")
        set my_file_handler = fso.OpenTextFile(file_name)
        set current_section = Nothing
        
        while not my_file_handler.AtEndOfStream
            strLine = my_file_handler.ReadLine()
            strLine = TRIM(strLine)
            lineLen = LEN(strLine)
            
            ' Handles a found section
            if Left(strLine, 1) = "[" and Right(strLine, 1) = "]" then
                section_name = Mid(strLine, 2, lineLen - 2)
                
                if configuration.exists(section_name) then
                    set current_section = configuration(section_name)
                else
                    set current_section = CreateObject("Scripting.Dictionary")
                    configuration.add section_name, current_section
                end if
            ' Ignores comment lines and processes the rest as items of the current section
            else 
                if Left(strLine,1) <> "#" then
                    index = INSTR(1, strLine, "=", 1)

                    if index > 1 then
                        attName  = LCase(Trim(Mid(strLine, 1, index - 1)))
                        attValue = Trim(Mid(strLine, index+1, lineLen - index))
                        
                        if not current_section is Nothing then
                            current_section.add attName, attValue
                        end if
                    end if
                end if
            end if
        wend
        
        my_file_handler.close()
    end sub
    
    public function get_value(section, item, default)
        found = false
        value = ""
        if configuration.exists(section) then
            set section_data = configuration(section)
            
            if section_data.exists(item) then
                value =  section_data(item)
                found = true
            end if
        end if
        
        if found then
            get_value = value
        else
            get_value = default
        end if
    end function
end class

class ConfigUpdater
    public config

    private sub Class_Initialize()
        Set config = new ConfigReader
    end sub
    
    public sub load_config(file_name)
        config.load(file_name)        
    end sub
    
    public sub update(file_name)
        set fso = CreateObject("Scripting.FileSystemObject")
        set my_source_handler = fso.OpenTextFile(file_name)
        set my_target_handler = fso.CreateTextFile(file_name & ".tmp", true)
        
        
        set current_section = Nothing
        while not my_source_handler.AtEndOfStream
            strLine = my_source_handler.ReadLine()
            strLine = TRIM(strLine)
            lineLen = LEN(strLine)
            
            ' Handles a found section
            if Left(strLine, 1) = "[" and Right(strLine, 1) = "]" then
                section_name = Mid(strLine, 2, lineLen - 2)
                
                if config.configuration.exists(section_name) then
                    set current_section = config.configuration(section_name)
                else
                    set current_section = CreateObject("Scripting.Dictionary")
                end if
            else 
                if Left(strLine,1) <> "#" then
                    index = INSTR(1, strLine, "=", 1)

                    if index > 1 then
                        attName  = LCase(Trim(Mid(strLine, 1, index - 1)))
                        attValue = Trim(Mid(strLine, index+1, lineLen - index))
                        
                        if current_section.exists(attName) then
                            strLine = attName & " = " & current_section(attName)
                        end if
                    end if
                end if
            end if
            
            my_target_handler.WriteLine(strLine)
        wend
        
        my_source_handler.close()
        my_target_handler.close()
        
        fso.DeleteFile file_name
        fso.MoveFile file_name & ".tmp", file_name
    end sub    
end class    

class Utilities
    private shell
    
    private sub Class_Initialize()
        set shell = WScript.CreateObject("WScript.Shell")
    end sub

    public function check_version_at_least(command, major, minor, revno)
        set exec = shell.Exec(command & " --version")
        output = ""
        do while exec.status = 0
            output = output & exec.StdErr.ReadAll()
        loop
        
        tokens = split(output, " ")
        
        version = ""
        found_major = 0
        found_minor = 0
        found_revno = 0
        
        for each token in tokens
            if token = "version" then
                version = "found"
            else 
                if version = "found" then
                    version = token
                    version_tokens = split(version, ".")
                    found_major = Int(version_tokens(0))
                    found_minor = Int(version_tokens(1))
                    found_revno = Int(version_tokens(2))
                end if
            end if
        next
        
        if found_major > major or _
           (found_major = major and found_minor > minor) or _
           (found_major = major and found_minor = minor and found_revno >= revno) then
           check_version_at_least = True
        else
            check_version_at_least = False
        end if
    end function

end class

class MEBCommandProcessor
    private help_needed
    private command_name
    
    private sub Class_Initialize()
        help_needed = false
        command_name = ""
    end sub
    
    private function read_params()
        ret_val = True
        if Wscript.Arguments.Count >= 1 then
            command_name = UCase(Wscript.Arguments.Item(0))
            if command_name = "HELP" then
                help_needed = true
                
                if Wscript.Arguments.Count >= 2 then
                    command_name = UCase(Wscript.Arguments.Item(1))
                end if
            end if
        end if


        if command_name = "" and not help_needed then
            Wscript.Echo "Error executing helper, use it as follows:"
            ret_val = false
        end if
        
        read_params = ret_val
    end function
    
    private sub print_usage()
        Wscript.Echo "mysqlwbmeb <command> <parameters>"
        Wscript.Echo
        Wscript.Echo "WHERE : <command>        : is one of HELP, VERSION, BACKUP, GET_PROFILES, UPDATE_SCHEDULING, PROPAGATE_DATA" 
        Wscript.Echo "        <parameters>     : are the parameters needed for each command."
        Wscript.Echo
        Wscript.Echo "You can also use as follows to get each command parameters:"
        Wscript.Echo
        Wscript.Echo "mysqlwbmeb help <command>"
        Wscript.Echo "WHERE : <command>        : is one of VERSION, BACKUP, GET_PROFILES, UPDATE_SCHEDULING, PROPAGATE_DATA"
    end sub
    
    public function execute()
        ret_val = 1
        
        if read_params() then
            set command = Nothing
            
            select case command_name
                case "VERSION"
                    set command = new MEBHelperVersion
                case "BACKUP"
                    set command = new MEBBackup
                case "GET_PROFILES"
                    set command = new MEBGetProfiles
                case "UPDATE_SCHEDULING"
                    set command = new MEBUpdateScheduling
                case "PROPAGATE_DATA"
                    set command = new MEBPropagateSettings
            end select    
            
            if command is Nothing then
                if not help_needed then Wscript.Echo "ERROR Executing mysqlwbmeb"
                print_usage()
            else
                if help_needed then
                    command.print_usage()
                    ret_val = 0
                else
                    ret_val = command.execute()
                end if
            end if
        else
            print_usage()
        end if
        
        execute = ret_val
    end function
end class        

class MEBHelperVersion
    private current
    
    private sub Class_Initialize()
        current = "10"
    end sub

    public function execute()
        Wscript.Echo current
        execute = 0
    end function
end class



class MEBBackup
    ' These will be received as parameters
    private profile_file
    private compress
    private compress_method
    private compress_level
    private incremental
    private to_single_file
    private report_progress
    private bkcommand
    
    ' These will be read from the profile file
    private command
    private backup_dir
    private inc_backup_dir
    private log_path
    private backups_home
    private target_folder
    private command_call
    private use_tts
    private skip_unused_pages
    private incremental_with_redo_log_only
    private use_encryption_password
    private encryption_password
    
    ' These are for internal use
    private shell
    private file_name
    private fso
    
    private sub Class_Initialize()
        file_name = ""
        set fso = CreateObject("Scripting.FileSystemObject")
        
        ' Sets the backups home to the parent folder of this script
        backups_home = fso.GetParentFolderName(Wscript.ScriptFullName)
        skip_unused_pages = false
    end sub
    
    private function read_params()
        if Wscript.Arguments.Count >= 7 then
            profile_file = Wscript.Arguments.Item(1)
            compress = Wscript.Arguments.Item(2)
            incremental = Wscript.Arguments.Item(3)
            to_single_file = Wscript.Arguments.Item(4)
            report_progress = Wscript.Arguments.Item(5)
            bkcommand = Wscript.Arguments.Item(6)
            
            if Wscript.Arguments.Count > 7 then
                file_name = Wscript.Arguments.Item(7)
            end if
            read_params = true
        else
            read_params = false
        end if
    end function
    
    sub read_profile_data()
        set profile = new ConfigReader
        profile.load(backups_home & "\" & profile_file)
        
        command = profile.get_value("meb_manager", "command", "")
        backup_dir = profile.get_value("mysqlbackup", "backup_dir", "")
        inc_backup_dir = profile.get_value("mysqlbackup", "incremental_backup_dir", "")
        use_tts = profile.get_value("meb_manager", "using_tts", "0")
        if profile.get_value("meb_manager", "compress", "False") = "True" then
            compress = true
        end if
        compress_method = profile.get_value("meb_manager", "compress_method", "lz4")
        compress_level = profile.get_value("meb_manager", "compress_level", "1")
        
        if profile.get_value("meb_manager", "skip_unused_pages", "False") = "True" then
            skip_unused_pages = true
        end if

        if profile.get_value("meb_manager", "incremental_with_redo_log_only", "False") = "True" then
            incremental_with_redo_log_only = true
        end if

        if profile.get_value("meb_manager", "use_encryption_password", "False") = "True" then
            use_encryption_password = true
        end if
        encryption_password = profile.get_value("meb_manager", "encryption_password", "")
        
    end sub
    
    ' Function used to create the target name in case it is a timestamp
    function get_tstamp_folder_name(ts)

      ts_short = FormatDateTime(ts, vbShortTime)

      ' Formats the elements to be 2 chars length with leading 0's
      ' if needed
      the_month = Right("0" & Month(ts), 2)
      the_day = Right("0" & Day(ts), 2)
      the_hour = Right("0" & MID(ts_short, 1, InStr(ts_short, ":") -1), 2)
      the_minute = Right("0" & Minute(ts), 2)
      the_second = Right("0" & Second(ts), 2)

      get_tstamp_folder_name = Year(ts) & "-" & the_month & "-" & the_day & "_" & the_hour & "-" & the_minute & "-" & the_second

    end function
    
    
    private sub set_backup_paths
        target_folder = ""

        ' Defines the target file/folder name for the backup
        if file_name <> "" then
            if to_single_file then
                if LCase(Right(file_name, 4)) <> ".mbi" then
                    file_name = file_name & ".mbi"
                end if

                ' On an image backup the backup dir will be the one
                ' Received as a parameter
                backup_dir = Left(file_name, Len(file_name) - 4 )
                log_path = replace(file_name, ".mbi", ".log")
            else
                ' If a file name is passed it is used as the backup folder
                target_folder = file_name
            end if
        else
            ' If no file name is passed, uses the timestamp to create one
            target_folder = get_tstamp_folder_name(Now())
        end if
            
        ' The full path is the target folder under the backups home for
        ' the profile
        if target_folder <> "" then
            backup_dir = backup_dir & "\" & target_folder
            log_path = backup_dir & ".log"
        end if
    end sub
        
    private function get_incremental_base_folder
        base_folder = ""
        
        base_tstamp = CDate("1900-01-01 00:00:00")
        
        lastest_full = find_lastest_backup(backup_dir, base_tstamp)
        
        if lastest_full > base_tstamp then
            base_folder = backup_dir
            base_tstamp = lastest_full
            
            lastest_inc = find_lastest_backup(inc_backup_dir, lastest_full)
            
            if lastest_inc > lastest_full then
                base_folder = inc_backup_dir
                base_tstamp = lastest_inc
            end if
        end if
        
        if base_folder <> "" then
            folder = get_tstamp_folder_name(base_tstamp)
            base_folder = base_folder & "\" & folder
        end if
        
        get_incremental_base_folder = base_folder
    end function

    private function get_folders_in_newest_to_oldest_order(path)

        ' Gets the folders on the target path
        set tgt_folder = fso.GetFolder(path)
        set folder_list = tgt_folder.SubFolders

        ' copy folder_list to a native array
        arr = Array()
        for each folder in folder_list
            last = UBound(arr)
            redim preserve arr(last + 1)
            arr(last + 1) = folder.path
        next

        ' sort folder names in reverse order
        arr_ub = UBound(arr)
        call qsort(arr, 0, arr_ub)
        redim sorted_folder_list(arr_ub + 1)
        for i = 0 to arr_ub
            sorted_folder_list(arr_ub - i) = arr(i)
        next

        get_folders_in_newest_to_oldest_order = sorted_folder_list

    end function

    private function find_lastest_backup(path, basetstamp)

        lastest = basetstamp

        ' Creates the regular expression to match the timestamo
        ' folder names used un meb
        set regExp = CreateObject("VBScript.RegExp")
        regExp.Global = true
        regExp.Pattern = "(\b[1-9]\d\d\d\b)-([0-1]\d)-([0-3]\d)_([0-2]\d)-([0-5]\d)-([0-5]\d)"

        folder_list = get_folders_in_newest_to_oldest_order(path)

        for each folder in folder_list
            set matches = regExp.Execute(folder)
            if matches.Count = 1 then
                if is_backup_dir_valid(folder) then

                    dim match, year, month, day, hour, minute, second
                    set match = matches(0).SubMatches

                    ' Gets the data for the matched folder
                    year = match(0)
                    month = match(1)
                    day = match(2)
                    hour = match(3)
                    minute = match(4)
                    second = match(5)

                    ' Compares the new tstamp with the base and if greather (more recent)
                    ' then replaces it
                    lastest = CDate(year & "-" & month & "-" & day & " " & hour & ":" & minute & ":" & second)

                    exit for
                end if
            end if
        next

        find_lastest_backup = lastest
    end function

    private function is_backup_dir_valid(folder)

        ' create regex for detecting backup success
        set backup_ok_flag_re = CreateObject("VBScript.RegExp")
        backup_ok_flag_re.Pattern = "^mysqlbackup completed OK!$"  ' according to http://dev.mysql.com/doc/mysql-enterprise-backup/3.10/en/mysqlbackup.html, we can rely on this appearing at the end of the backup log when the backup succeeds

        log_filename = folder & ".log"          ' each /path/to/backup/dir has a corresponding /path/to/backup/dir.log

        ' open log file
        on error resume next                    ' ensure the errors aren't fatal
        err.Clear
        set f = fso.OpenTextFile(log_filename)  ' on Windows, this may fail when opening the current log file due to file locking

        ' grep log file for the flag message
        is_backup_dir_valid = false
        if err.Number = 0 then                  ' if (opening file succeded)
            do until f.AtEndOfStream
                line = f.ReadLine
                if backup_ok_flag_re.Test(line) then
                    is_backup_dir_valid = true
                    exit do
                end if
            loop
            f.close
        else
            err.Clear
        end if
    end function

    private function prepare_command
        set my_utils = new Utilities
        ret_val = True
        command_call = """" & command & """ --defaults-file=""" & backups_home & "\" & profile_file & """"
        
        ' Adds the compress parameter if needed
        if compress then
            if my_utils.check_version_at_least(command, 3,10,0) then
                ' lz4 is the default so id it is selected only sets the --compress option
                if compress_method = "lz4" then
                    command_call = command_call & " --compress"
                    
                ' Otherwise using --compress-method makes --compress to be not needed
                else
                    command_call = command_call & " --compress-method=" & compress_method
                    
                    ' Level is only specified if not using the default value
                    if compress_level <> "1" then
                        command_call = command_call & " --compress-level=" & compress_level
                    end if
                end if
            else
                command_call = command_call & " --compress"
            end if
        end if
        

        ' Get the right path parameter, path and running type 
        path_param = " --backup-dir"

        ' If the backup is incremental
        if incremental then
            base_folder = get_incremental_base_folder()

            if base_folder <> "" then
                if incremental_with_redo_log_only then
                    command_call = command_call & " --incremental-with-redo-log-only --incremental-base=dir:" & """" & base_folder & """"
                else
                    command_call = command_call & " --incremental --incremental-base=dir:" & """" & base_folder & """"
                end if
                backup_dir = inc_backup_dir
                path_param = "  --incremental-backup-dir"
            else
                Wscript.Echo "ERROR: Unable to run incremental backup without a base folder."
                ret_val = false
            end if
        else
            if skip_unused_pages then
                command_call = command_call & " --skip-unused-pages"
            end if
        end if
                
        ' Sets the needed backup paths
        set_backup_paths()

        ' Adds the backup folder to the command
        command_call =  command_call & " " & path_param & "=""" & backup_dir & """"

        if to_single_file then command_call = command_call & " --backup-image=""" & file_name & """"
        
        if use_tts <> "0" then
            tts_value="with-minimum-locking"
            if use_tts = 2 then
                tts_value="with-full-locking"
            end if
            
            command_call = command_call & " --use-tts=" & tts_value
        end if
            
        if report_progress then command_call = command_call & " --show-progress=stdout"

        if use_encryption_password then
            command_call = command_call & " --encrypt-password=" & encryption_password
        end if

        command_call = command_call & " " & bkcommand & " > """ & log_path &  """  2>&1"

        prepare_command = ret_val
    end function
    
    public sub print_usage()
        Wscript.Echo "BACKUP <profile> <compress> <incremental> <to_single_file> <report_progress> <command>"
        Wscript.Echo "WHERE : <profile>        : is the UUID of the profile to be used for the backup process."
        Wscript.Echo "        <compress>       : indicates if the backup will be compressed. (1 or 0)"
        Wscript.Echo "        <incremental>    : indicates the backup should be incremental.  (1 or 0)"
        Wscript.Echo "        <to_single_file> : indicates the backup will be done to an image file.  (1 or 0)"
        Wscript.Echo "        <report_progress>: indicates the backup should be compressed.  (1 or 0)"
        Wscript.Echo "        <command>        : indicates the backup operation to be done, could be backup or backup-and-apply-log"
        Wscript.Echo "        [file_name]      : indicates the target name for the backup file or folder"
        Wscript.Echo
        Wscript.Echo
        
    end sub
    
    public function execute()
        ret_val = 1
        
        if read_params() then
                
            read_profile_data()
            
            ' Creates the scripting shell to execute the backup
            set shell = WScript.CreateObject("WScript.Shell")
            
            if prepare_command() then
                
                shell.Run "%comspec% /c """ & command_call & " & del *.running""", 0
                
                set dictionary = CreateObject("Scripting.Dictionary")
                dictionary.add "LOG_PATH", log_path
                
                set dumper = new JSONDumper
                WScript.Echo dumper.json_object(dictionary)
                
                ret_val = 0
            end if
        else
            print_usage()
        end if
        
        execute = ret_val
    end function
end class


class MEBUpdateScheduling
    ' These will be received as parameters
    private change
    private profile_file
    private old_label
    private old_fb_schedule
    private old_ib_schedule
    
    ' These will be read from the profile file
    private profile
    private uuid
    private new_label
    private new_fb_schedule
    private new_ib_schedule
    
    ' These are for internal use
    private fso
    private backups_home
    private frequency
    private month_day
    private week_day
    private hour
    private minute
    
    private compress
    private apply_log
    
    
    private sub Class_Initialize()
        file_name = ""
        set fso = CreateObject("Scripting.FileSystemObject")
        
        ' Sets the backups home to the parent folder of this script
        backups_home = fso.GetParentFolderName(Wscript.ScriptFullName)
        
        ' Initializes these to False so they serve their purpose
        ' in the case of a DELETE
        new_fb_schedule = "False"
        new_ib_schedule = "False"
    end sub
    
    private function read_params()
        read_params = false
        
        if Wscript.Arguments.Count > 2 then
            change = Wscript.Arguments.Item(1)
            if change = "NEW" then
                param_count = 3
            else
                param_count = 6
            end if
            
            if Wscript.Arguments.Count = param_count then
                
                profile_file = Wscript.Arguments.Item(2)
                
                if change <> "NEW" then
                    old_label = Wscript.Arguments.Item(3)
                    old_fb_schedule = Wscript.Arguments.Item(4)
                    old_ib_schedule = Wscript.Arguments.Item(5)
                end if
                
                read_params = true
            end if
        end if
    end function
    
    sub init_profile()
        set profile = new ConfigReader
        file_name = backups_home & "\" & profile_file & ".cnf"
        profile.load(file_name)
        
        new_label = profile.get_value("meb_manager", "label", "")
        uuid = profile.get_value("meb_manager", "uuid", "")
        new_fb_schedule = profile.get_value("meb_manager", "full_backups_enabled", "")
        new_ib_schedule = profile.get_value("meb_manager", "inc_backups_enabled", "")
        compress = profile.get_value("meb_manager", "compress", "")
        apply_log = profile.get_value("meb_manager", "apply_log", "")
    end sub
    
    sub read_profile_data(backup_type)
        frequency = profile.get_value("meb_manager", backup_type & "_backups_frequency", "")
        month_day = profile.get_value("meb_manager", backup_type & "_backups_month_day", "")
        week_day = profile.get_value("meb_manager", backup_type & "_backups_week_days", "")
        hour = profile.get_value("meb_manager", backup_type & "_backups_hour", "")
        minute = profile.get_value("meb_manager", backup_type & "_backups_minute", "")
    end sub
    
    
    private function create_backup_command_call(backup_type)
        command = "cscript /nologo \""" & Wscript.ScriptFullName & "\"""
        command = command & " BACKUP " & uuid & ".cnf"
        real_compress = "0"
        real_incremental = "0"
        
        if backup_type = "full" then
            if compress = "True" and apply_log = "False" then
                real_compress = "1"
            end if
        else
            real_incremental = "1"
        end if
        
        command = command & " " & real_compress
        command = command & " " & real_incremental
        command = command & " 0" ' To single file
        command = command & " 1" ' Report Progress
        
        if apply_log = "True" and backup_type = "full" then
            command = command & " backup-and-apply-log"
        else
            command = command & " backup"
        end if
        
        create_backup_command_call = command
        
    end function
    
    private function get_unschedule_command(backup_type)
        get_unschedule_command = "schtasks /Delete /TN ""MySQLBackup\" & uuid & "-" & old_label & "-" & backup_type & """ /F" 
    end function

    private function get_schedule_command(backup_type)
        command = "schtasks /Create /TN ""MySQLBackup\" & uuid & "-" & new_label & "-" & backup_type & """ /SC " 
        
        hour = right("00" & hour, 2)
        minute = right("00" & minute, 2)
        
        select case frequency
            case "0" 'hourly
                command = command & "HOURLY /ST 00:" & minute
            case "1" 'Daily
                command = command & "DAILY /ST " & hour & ":" & minute
            case "2" 'Weekly
                weekdays = array("SUN", "MON","TUE","WED","THU","FRI","SAT")
                wd_string = ""
                wd_numbers = split(week_day, ",")

                for each day_number in wd_numbers
                    day_index = Int(day_number)
                    wd_string = wd_string & weekdays(day_index) & ","
                next
                wd_string= left(wd_string, len(wd_string) - 1)
                command = command & "WEEKLY /D " & wd_string & " /ST " & hour & ":" & minute
            case "3" 'Monthly
                command = command & "MONTHLY /D " & month_day & " /ST " & hour & ":" & minute
        end select
        
        get_schedule_command = command & " /RU SYSTEM /TR """ & create_backup_command_call(backup_type) & """ /F"
    end function
    
    public sub print_usage()
        Wscript.Echo "UPDATE_SCHEDULING <profile> <old_label> <old_full> <old_incremental>"
        Wscript.Echo "WHERE : <change>           : Indicates the operation being done with the profile: NEW, UPDATE, DELETE."
        Wscript.Echo "        <profile>          : is the UUID of the profile to be used for the scheduling."
        Wscript.Echo "        [<old_label>]      : indicates the label under which the jobs were scheduled. Applies on UPDATE and DELETE changes."
        Wscript.Echo "        [<old_full>]       : indicates if the profile was scheduled for full backup. (0 or 1). Applies on UPDATE and DELETE changes."
        Wscript.Echo "        [<old_incremental>]: indicates if the profile was scheduled for incremental backup. (0 or 1). Applies on UPDATE and DELETE changes."
        Wscript.Echo
        Wscript.Echo
    end sub
    
    public function exec_command(shell, command)
        set exec = shell.Exec(command)
        do while exec.status = 0
            WScript.Echo exec.StdOut.ReadAll()
            WScript.Echo exec.StdErr.ReadAll()
        loop
        
        exec_command = exec.exitcode
    end function
    
    public function execute()
        ret_val = 1
        if read_params() then
            ret_val = 0
            set shell = WScript.CreateObject("WScript.Shell")
        
            ' Profile data would NOT be read on DELETE actions.
            if change <> "DELETE" then
                init_profile()
            end if
            
            ' Unscheduling would NOT occur on NEW profiles
            if change <> "NEW" then
                if old_label <> new_label  or old_fb_schedule = "1" and new_fb_schedule = "False" then
                    command = get_unschedule_command("full")
                    ret_val = ret_val + exec_command(shell, command)
                end if

                if old_label <> new_label  or (old_ib_schedule = "1" and new_ib_schedule = "False") then
                    command = get_unschedule_command("inc")
                    ret_val = ret_val + exec_command(shell, command)
                end if
            end if
            
            if new_fb_schedule = "True" then
                read_profile_data("full")
                command = get_schedule_command("full")
                ret_val = ret_val + exec_command(shell, command)
            end if
            
            if new_ib_schedule = "True" then
                read_profile_data("inc")
                command = get_schedule_command("inc")
                ret_val = ret_val + exec_command(shell, command)
            end if
        else
            print_usage()
        end if
        
        execute = ret_val
    end function
end class

class MEBPropagateSettings
    private datadir
    private datafile
    
    private fso
    private backups_home
    
    
    
    private sub Class_Initialize()
        datadir = ""
        set fso = CreateObject("Scripting.FileSystemObject")
        
        ' Sets the backups home to the parent folder of this script
        backups_home = fso.GetParentFolderName(Wscript.ScriptFullName)
    end sub    
    
    public function read_params()
        ret_val = false

        if Wscript.Arguments.Count = 2 then
            datafile = Wscript.Arguments.Item(1)
            ret_val = true
        end if

        read_params = ret_val
    end function

    public sub print_usage()
        Wscript.Echo "PROPAGATE_DATA <datadir>"
        Wscript.Echo 
        Wscript.Echo "WHERE : <datadir> : is the path to the datadir of the server instance for which the profiles are"
        Wscript.Echo "                    being loaded. (There could be more than one instance on the same box)."
        Wscript.Echo
    end sub
        

    public function execute()
        ret_val = 1
        if read_params() then
            ret_val = 0
            ' Loads the information to be propagated
            set updater = new ConfigUpdater
            source_config_file = backups_home & "\" & datafile 
            updater.load_config(source_config_file)
        
            datadir = updater.config.get_value("target", "datadir", "")
            
            if datadir <> "" then
                Set master_data = CreateObject("Scripting.Dictionary")

                Set backups_home_folder = fso.GetFolder(backups_home)
                Set all_files = backups_home_folder.Files
                
                for each file in all_files
                    profile_issues = 0
                    if ucase(fso.GetExtensionName(file.name)) = "CNF" then
                        set profile = new ConfigReader
                        profile.load(backups_home & "\" & file.name)

                        ' Verifies the datadir to ensure it belongs to the requested instance
                        command = profile.get_value("meb_manager", "command", "")
                        
                        profile_datadir = profile.get_value("mysqlbackup", "datadir",  "")
                        if UCase(profile_datadir) = UCase(datadir) then
                            updater.update(backups_home & "\" & file.name)
                        end if
                    end if
                next
            else
                WScript.Echo("Data propagation file is missing the target datadir.")
            end if
            
            ' Deletes the temporary file
            fso.DeleteFile source_config_file
        else
            print_usage()
        end if
        
    end function
end class
                    

class MEBGetProfiles
    private datadir
    
    private fso
    private backups_home
    private meb_command
    
    private sub Class_Initialize()
        datadir = ""
        set fso = CreateObject("Scripting.FileSystemObject")
        
        ' Sets the backups home to the parent folder of this script
        backups_home = fso.GetParentFolderName(Wscript.ScriptFullName)
    end sub    
    
    public function read_params()
        ret_val = false

        if Wscript.Arguments.Count = 3 then
            meb_command = Wscript.Arguments.Item(1)
            datadir = Wscript.Arguments.Item(2)
            if Right(datadir,1) <> "\" then datadir = datadir & "\"
            ret_val = true
        end if

        read_params = ret_val
    end function

    public sub print_usage()
        Wscript.Echo "GET_PROFILES <meb_command> <datadir>"
        Wscript.Echo 
        Wscript.Echo "WHERE : <meb_command> : is the path to a valid MEB executable"
        Wscript.Echo "        <datadir> : is the path to the datadir of the server instance for which the profiles are"
        Wscript.Echo "                    being loaded. (There could be more than one instance on the same box)."
        Wscript.Echo
    end sub
        

    public function execute()
        if read_params() then
            Set master_data = CreateObject("Scripting.Dictionary")

            Set backups_home_folder = fso.GetFolder(backups_home)
            Set all_files = backups_home_folder.Files
            
            for each file in all_files
                profile_issues = 0
                if ucase(fso.GetExtensionName(file.name)) = "CNF" then
                    set profile = new ConfigReader
                    profile.load(backups_home & "\" & file.name)

                    ' Verifies the datadir to ensure it belongs to the requested instance
                    profile_command = profile.get_value("meb_manager", "command", "")
                    
                    ' Checks that the profile command matches the one received as parameter
                    if profile_command <> meb_command then
                        profile_issues = profile_issues or 16
                    end if
                    
                    profile_datadir = profile.get_value("mysqlbackup", "datadir",  "")
                    if UCase(profile_datadir) = UCase(datadir) then

                        set data = CreateObject("Scripting.Dictionary")
                        
                        data.add "LABEL", profile.get_value("meb_manager", "label", "")
                        data.add "PARTIAL", profile.get_value("meb_manager", "partial", "")
                        data.add "USING_TTS", profile.get_value("meb_manager", "using_tts", "0")
                        data.add "BACKUP_DIR", profile.get_value("mysqlbackup", "backup_dir", "")

                        ' Gets the available space
                        data.add "AVAILABLE", get_available_space(data("BACKUP_DIR"))

                        ' Validates the backups folder for write permission
                        if not is_dir_writable(data("BACKUP_DIR")) then
                            profile_issues = profile_issues or 1
                        end if

                        ' Gets the full schedule data
                        e = profile.get_value("meb_manager", "full_backups_enabled", "")
                        f = profile.get_value("meb_manager", "full_backups_frequency", "")
                        md = profile.get_value("meb_manager", "full_backups_month_day", "")
                        wd = profile.get_value("meb_manager", "full_backups_week_days", "")
                        h = profile.get_value("meb_manager", "full_backups_hour", "")
                        m = profile.get_value("meb_manager", "full_backups_minute", "")
                        data.add "FSCHEDULE", e & "-" & f & "-" & md & "-" & wd & "-" & h & "-" & m

                        ' Gets the incremental schedule data
                        e = profile.get_value("meb_manager", "inc_backups_enabled", "")
                        f = profile.get_value("meb_manager", "inc_backups_frequency", "")
                        md = profile.get_value("meb_manager", "inc_backups_month_day", "")
                        wd = profile.get_value("meb_manager", "inc_backups_week_days", "")
                        h = profile.get_value("meb_manager", "inc_backups_hour", "")
                        m = profile.get_value("meb_manager", "inc_backups_minute", "")
                        data.add "ISCHEDULE", e & "-" & f & "-" & md & "-" & wd & "-" & h & "-" & m
                        
                        ' Validates Partial Backup Options
                        set my_utils = new Utilities
                        
                        ' Profiles with version 0 could have an UUID as the value for the "include" field
                        ' It was used to make all of the InnoDB tables NOT matching the selection criteria on initial 
                        ' versions of MEB.
                        ' On MEB 3.9.0 this became invalid as an error would be generated so
                        ' If found in a profile, we need to report the invalid configuration on the UI.
                        profile_version = CInt(profile.get_value("meb_manager", "version", "0"))
                        if profile_version = "0" and my_utils.check_version_at_least(meb_command, 3,9,0) then
                            include = profile.get_value("mysqlbackup", "include", "")
                            if include <> "" then
                                set my_reg_exp = New RegExp
                                my_reg_exp.Pattern = "^[\dA-Fa-f]{8}-([\dA-Fa-f]{4}-){3}[\dA-Fa-f]{12}$"
                                set my_matches = my_reg_exp.Execute(include)
                                
                                if my_matches.count > 0 then
                                    profile_issues = profile_issues or 2
                                end if
                            end if
                        end if
                        
                        if my_utils.check_version_at_least(meb_command, 3,10,0) then
                            include = profile.get_value("mysqlbackup", "include", "")
                            databases = profile.get_value("mysqlbackup", "include", "")
                            
                            if len(include) > 0 or len(databases) > 0 then
                                profile_issues = profile_issues or 8
                            end if
                        else
                            include = profile.get_value("mysqlbackup", "include_tables", "")
                            exclude = profile.get_value("mysqlbackup", "exclude_tables", "")
                            
                            if len(include) > 0 or len(exclude) > 0 then
                                profile_issues = profile_issues or 4
                            end if
                        end if
                        
                        ' The VALID item will contain a numeric valid describing the issues encountered on the profile
                        ' Validation. Each issue should be assigned a value of 2^x so the different issues can be joined
                        ' using bitwise operations
                        ' 1 : Indicates the backup folder is not valid to store the backups.
                        ' 2 : Indicates a partial backup profile using a regular expression on the include parameter.
                        ' 
                        data.add "VALID", CStr(profile_issues)
                        master_data.add backups_home & "\" & file.name, data
                    end if
                end if
            next
            
            ' If any, prints the profile data
            if master_data.count > 0 then
                set dumper = new JSONDumper
                Wscript.Echo dumper.json_object(master_data)
                execute = 0
            end if
        else:
            print_usage()
            execute = 1
        end if
    end function
            
    private function is_dir_writable(path)
        ret_val = "False"
        if fso.FolderExists(path) then
            set folder = fso.GetFolder(path)
            set file = folder.CreateTextFile("test.txt")
            if not file is Nothing then
                ret_val = "True"
                file.Close()
                fso.DeleteFile(path & "\" & "test.txt")
            end if
        end if
        
        is_dir_writable = ret_val
    end function

    private function get_available_space(path):
        suffixes = Array("B", "KB", "MB", "GB", "TB", "PB", "EB")
        
        set drive = fso.GetDrive(fso.GetDriveName(path)) 
        total = drive.TotalSize
        available = drive.AvailableSpace
        
        limit = 1024
        index = 0
        while total > limit
            total = total/limit
            available =  available/limit
            index = index + 1
        wend

        get_available_space = CStr(FormatNumber(available, 2)) & CStr(suffixes(index)) & " of " & CStr(FormatNumber(total, 2)) & CStr(suffixes(index)) & " available."
        
    end function
    
end class



set processor = new MEBCommandProcessor
WScript.Quit(processor.execute())
