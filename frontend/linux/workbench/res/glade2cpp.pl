#!/usr/bin/perl
use strict;
use File::Basename;

open (FH, $ARGV[0]) || die ("Cant open $ARGV[1]");

my $fname=basename($ARGV[0]);

$fname =~ s/\./_/;
my $header=$fname.".h";
my $source=$fname.".cpp";
my $size = 0;

open SRC, ">", $source || die ("Can't create header file %s");

printf SRC "\n//This file is auto-generated. Do not modify.\n\n\nextern const char *modelXml = ";
foreach my $line (<FH>)
{
  $size += length($line);
  $line =~ s/\"/\\"/g;
  chomp $line;
  #print '"'."$line".'"';
  printf SRC '"';
  printf SRC $line;
  printf SRC "\\n\"\n";
}
print SRC "\"\";\n";
close(SRC);

open HDR, ">", $header || die ("Can't create header file %s");
printf HDR "#ifndef __".$fname."_h__\n#define __"."$fname"."_h__\n\nextern const char *modelXml;\nenum {modelXmlSize=$size};\n#endif\n";
close(HDR);

close(FH);
