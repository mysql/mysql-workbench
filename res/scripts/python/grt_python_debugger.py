import os
import sys
import bdb
import wbpdb
import traceback
import tempfile
import time


STOP_REASON_STEP = 0
STOP_REASON_BREAKPOINT = 1
STOP_REASON_EXCEPTION = 2

STOP_REASON_HEARTBEAT = 5
STOP_REASON_PAUSE = 6

HEARTBEAT_INTERVAL = 0.5

# Notes on how this works:
# 
# The Python debugger framework supports executing a callback (user_line) every time a 
# condition (breakpoint, stepping enabled, exception etc) is met.
# Normally a program would only stop on a breakpoint or when you're stepping through the 
# code. Then when the user clicks continue, the program would continue the normal execution.
# But that has 2 problems, if the program freezes (infinite loop for example), the whole app
# will freeze. And if the program is simply slow, the program also freezes during that time.
# There's no way to stop or pause the program from the debugger, since the UI is frozen
# and also because there seems to be no other hooks to stop the program in the middle of its
# execution (if I'm wrong, tell me!)
# To workaround that, we always run the program in stepping mode, which makes user_line
# get called after every instruction. We then make user_line call back to the UI for refresh
# and user actions, which lets us pause or stop the program.
#

class PersistentBreakpoint:
    def __init__(self, owner, file, line, cond=None, funcname=None):
        self.owner = owner
        self.file = file
        self.line = line
        self.cond = cond
        self.funcname = None
        self.active = False

    def set_condition(self, cond):
        self.cond = cond
        if self.active:
            self.deactivate()
            self.activate()


    def activate(self):
        f = self.owner.set_break(self.owner.canonic(self.file), self.line, cond=self.cond)
        if f is None:
            self.active = True
        else:
            self.active = False
        return f


    def deactivate(self):
        self.owner.clear_break(self.owner.canonic(self.file), self.line)



class PyDebugger(bdb.Bdb):
    def __init__(self, ui):
        bdb.Bdb.__init__(self)

        self.ui = ui
        self.main_file = None
        self.current_stack = None
        self.top_stack_index = None
        self.started_stepping = False
        self.persistent_breakpoints = []
        self.last_heartbeat = 0
        self.is_stepping = False
        self.pause_pending = False
    
    
    def find_pbreakpoint(self, file, line):
        for pb in self.persistent_breakpoints:
            if pb.file == file and pb.line == line:
                return pb
        return None

    
    def enable_breakpoints(self):
        self.clear_all_breaks()
        for pb in self.persistent_breakpoints:
            s = pb.activate()
            if s:
                self.ui_print("Error activating breakpoint: %s\n" % s)


    def show_stack(self, stack):
        import linecache, repr
        self.ui_clear_stack()
        # index 0 is in bdb.py and index 1 is the execfile() command from wdb_run()
        for frame, line in reversed(stack[2:]):
            show_args = True
            if frame.f_code.co_name:
                location = frame.f_code.co_name
                if location == "<module>":
                    show_args = False
            else:
                location = "<lambda>"
            args = frame.f_code.co_varnames[:frame.f_code.co_argcount]
            if show_args:
                if args:
                    location = location+ "(%s)"%", ".join(args)
                else:
                    location = location+"()"

            self.ui_add_stack(location, self.canonic(frame.f_code.co_filename), line)


    def uncaught_exception(self, tb):
        self.handle_program_stop(tb.tb_frame, STOP_REASON_EXCEPTION)


    def handle_program_stop(self, frame, reason):
        filename = frame.f_code.co_filename
        line = frame.f_lineno

        self.current_stack, self.top_stack_index = self.get_stack(frame, None)
        if len(self.current_stack) == 2:
            # the 1st time we're called it's in the entry point of the debugger itself
            # (ie execfile('file')), which we don't want to bug the user about
            self.set_step()
            return

        self.current_frame = self.current_stack[self.top_stack_index][0]
        
        self.show_stack(self.current_stack)
        self.wdb_refresh_variables(self.top_stack_index)
        
        # call back into UI, which will block until the user clicks continue, stop or some other action button
        next_command_name = self.ui_program_stopped(filename, line, reason)
        if not next_command_name:
            next_command_name = "abort"
        
        # perform the next action clicked by the user
        next_command = getattr(self, "wdb_"+next_command_name)
        if reason == STOP_REASON_HEARTBEAT and next_command_name == "continue":
            next_command(quiet=True)
        else:
            next_command()


    ### Entry point methods for the C++ code
    def wdb_run(self, filename, stepping):
        self.pause_pending = False
        if stepping:
            self.started_stepping = True
            self.ui_print("> step\n")
        else:
            self.started_stepping = False
            self.ui_print("> run\n")
        try:
            self.enable_breakpoints()
        except Exception, exc:
            self.ui_print("Error activating breakpoints: %s\n" % exc)
            self.ui_print(traceback.format_exc()+"\n")
            return
        
        self.main_file = self.canonic(filename)
        
        try:
            self.run('execfile(r"%s")' % self.main_file)
        except:
            self.ui_print("Uncaught exception while executing %s:\n" % filename)
            e, v, t = sys.exc_info()
            
            # print stack, except for 1st 3 entries (this function and the execfile() command)
            stack = traceback.extract_tb(t)
            self.ui_print("".join(traceback.format_list(stack[3:])))
            self.ui_print("".join(traceback.format_exception_only(e, v))+"\n")
            while t.tb_next is not None:
                t = t.tb_next

            self.uncaught_exception(t)


    def wdb_stop(self):
        self.ui_print("> stop\n")
        self.set_quit()
        self.current_frame = None
        self.current_stack = None
        self.top_stack_index = None


    def wdb_abort(self):
        self.ui_print("> abort\n")
        self.set_quit()
        self.current_frame = None
        self.current_stack = None
        self.top_stack_index = None

    def wdb_pause(self):
        self.ui_print("> pause\n")
        self.set_step()
        self.pause_pending = True

    def wdb_continue(self, quiet=False):
        if not quiet:
            self.ui_print("> continue\n")
        #self.set_continue()
        self.set_step()
        self.current_frame = None
        self.current_stack = None
        self.top_stack_index = None
        self.is_stepping = False


    def wdb_step(self):
        self.ui_print("> step\n")
        self.set_next(self.current_frame)
        self.current_frame = None
        self.current_stack = None
        self.top_stack_index = None
        self.is_stepping = True


    def wdb_step_into(self):
        self.ui_print("> step into\n")
        self.set_step()
        self.current_frame = None
        self.current_stack = None
        self.top_stack_index = None
        self.is_stepping = True


    def wdb_step_out(self):
        self.ui_print("> step out\n")
        self.set_return(self.current_frame)
        self.current_frame = None
        self.current_stack = None
        self.top_stack_index = None
        self.is_stepping = True

    def wdb_refresh_breakpoints(self):
        self.ui_clear_breakpoints()
        for pb in self.persistent_breakpoints:
            self.ui_add_breakpoint(pb.active, pb.file, pb.line, pb.cond)


    def wdb_set_bp_condition(self, bp_index, cond):
        if bp_index >= 0 and bp_index < len(self.persistent_breakpoints):
            self.persistent_breakpoints[bp_index].set_condition(cond)
            return True
        return False
        

    def wdb_refresh_variables(self, frame_index):
        if not self.current_stack:
            self.ui_clear_variables()
            return
        
        if frame_index == 0:
            # toplevel frame index
            frame_index = 2
        
        frame = self.current_stack[frame_index][0]
    
        self.ui_clear_variables()
        toplevel = False
        if frame_index == self.top_stack_index or (frame_index < 0 and -1*frame_index+1 == self.top_stack_index):
            toplevel = True

        if not toplevel:
            self.ui_add_variable("Arguments", "")
            for varname in frame.f_code.co_varnames[:frame.f_code.co_argcount]:
                if varname in frame.f_locals:
                    self.ui_add_variable("    "+varname, repr(frame.f_locals[varname]))
                else:
                    self.ui_add_variable("    "+varname, "")

            self.ui_add_variable("Locals", "")
            for varname in frame.f_code.co_varnames[frame.f_code.co_argcount:]:
                if varname in frame.f_locals:
                    self.ui_add_variable("    "+varname, repr(frame.f_locals[varname]))
                else:
                    self.ui_add_variable("    "+varname, "")
        else:
            self.ui_add_variable("Globals", "")
            for varname, value in sorted(frame.f_globals.items()):
                self.ui_add_variable("    "+varname, repr(value))


    def wdb_toggle_breakpoint(self, file, line):
        pb = self.find_pbreakpoint(file, line)
        if pb: # remove bp
            self.persistent_breakpoints.remove(pb)
            pb.deactivate()
            self.wdb_refresh_breakpoints();
            return False
        else: # add bp
            pb = PersistentBreakpoint(self, file, line)
            self.persistent_breakpoints.append(pb)
            pb.activate()
            self.ui_add_breakpoint(pb.active, pb.file, pb.line, pb.cond)
            return True


    def wdb_update_breakpoint(self, file, line, delta):
        # go through all breakpoints for the given file and update their line position
        for bp in self.persistent_breakpoints:
            if bp.file != file:
                continue
            if bp.line > line or (delta > 0 and bp.line == line):
                if bp.active:
                    bp.deactivate()
                    bp.line += delta
                    bp.activate()
                else:
                    bp.line += delta

        self.wdb_refresh_breakpoints()


    def wdb_reload_module_for_file(self, file):
        path = os.path.splitext(self.canonic(file))[0]
        # find out what module the file corresponds to and reload it
        for module in sys.modules.values():
            mpath = getattr(module, "__file__", None)
            if mpath:
                mpath = os.path.splitext(mpath)[0]
                if mpath == path:
                    self.ui_print("Reloading module %s..."%file)
                    try:
                        reload(module)
                    except:
                        self.ui_print("There was an error reloading %s" % file)
                        import traceback
                        traceback.print_exc()
                        raise
                    break


    ### Wrappers for UI functions

    def ui_print(self, msg):
        wbpdb.ui_print(self.ui, msg)


    def ui_clear_breakpoints(self):
        wbpdb.ui_clear_breakpoints(self.ui)


    def ui_add_breakpoint(self, active, file, line, cond):
        wbpdb.ui_add_breakpoint(self.ui, active, file, line, cond)


    def ui_program_stopped(self, filename, line, reason):
        return wbpdb.ui_program_stopped(self.ui, filename, line, reason)


    def ui_clear_stack(self):
        wbpdb.ui_clear_stack(self.ui)


    def ui_add_stack(self, location, file, line):
        wbpdb.ui_add_stack(self.ui, location, file, line)


    def ui_clear_variables(self):
        wbpdb.ui_clear_variables(self.ui)


    def ui_add_variable(self, variable, value):
        wbpdb.ui_add_variable(self.ui, variable, value)

    ### Debugger stuff

    def user_call(self, frame, argument_list):
        """This method is called from dispatch_call() when there is the possibility that a break might be necessary anywhere inside the called function."""
        #self.ui_print("enter: %s, %s\n" % (frame.f_code.co_name, argument_list))
        pass

    def user_line(self, frame):
        """This method is called from dispatch_line() when either stop_here() or break_here() yields True."""

        ## In the WB Debugger, this is called for every line, since we're always stepping through the program
        ## But when the stepping is not requested by the user, we don't do anything except if the heartbeat
        ## timeout occurs, in that case, the program stops to give the UI some time to refresh and check
        ## if the program should be paused

        if self.pause_pending:
            self.pause_pending = False
            reason = STOP_REASON_PAUSE
        elif self.is_stepping:
            reason = STOP_REASON_STEP
        elif self.break_here(frame):
            self.ui_print("Breakpoint hit\n")
            reason = STOP_REASON_BREAKPOINT
        else:
            t = time.time()
            if t - self.last_heartbeat < HEARTBEAT_INTERVAL:
                return 
            reason = STOP_REASON_HEARTBEAT

        self.is_stepping = False
        self.handle_program_stop(frame, reason)
        self.last_heartbeat = time.time()

    
    def user_return(self, frame, return_value):
        """This method is called from dispatch_return() when stop_here() yields True."""
        #self.ui_print("leave: %s, %s\n" % (frame.f_code.co_name, return_value))
        pass

    def user_exception(self, frame, exc_info):
        """This method is called from dispatch_exception() when stop_here() yields True."""
        #self.ui_print("user_exception: %s, %s\n" % (frame, exc_info))
        #XXX show exception in UI
        pass
        

    def do_clear(self, bp_number):
        """Handle how a breakpoint must be removed when it is a temporary one."""
        #self.ui_print("user_clear: %s\n" % arg)
        pass # clear temporary breakpoint 

