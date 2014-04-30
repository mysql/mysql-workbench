/*
 * Copyright (c) 2008, 2013, Oracle and/or its affiliates. All rights reserved.
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


#import <Cocoa/Cocoa.h>

#include <signal.h>

int main(int argc, char *argv[])
{
  // ignore SIGPIPE, which can be thrown by child processes (like when we launch the cmdline client and it dies while we're writing to it)
  signal(SIGPIPE, SIG_IGN);
    
  return NSApplicationMain(argc,  (const char **) argv);
}
