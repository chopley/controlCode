#!/usr/bin/python

import re,string,sys

# one last gasp before we exit
def die( message ):
    sys.stderr.write( message + "\n" )
    sys.exit(1)

def usage():
    die("usage: code2html [filename]\n");


if len(sys.argv)!=2: usage()
filename= sys.argv[1]

try:
    fin= open(filename)
    code= fin.read()
    fin.close()

except:
    sys.stderr.write("Could not find file " + filename + "\n\n");
    usage()

lines= string.split(code,"\n")

count=0;
maxcount= len(lines)

pattern= re.compile(r'{{(\w+)}}')
    
while count<maxcount:
    line= lines[count]
    count= count + 1
    fields= string.split(line)
    if len(fields)>=2 and fields[0]=="HTML" and fields[1]=="BEGIN":
        
        # generate 1 web page at a time
        
        page= []
        pagename= None
        while count< maxcount:
            
            line= lines[count]
            fields= string.split(line)

            if len(fields)>=2 and fields[0]=="HTML" and fields[1]=="END": break

            if pagename==None and len(fields):
                pagename= fields[0] 
                line= "<BLUEBOLD>" + line + "</BLUEBOLD>"

                page.append("<HEAD><TITLE>The " + pagename + " Scripting Command")
                page.append("</TITLE></HEAD>")
                page.append("<BODY bgcolor=#add8e6><center><a href=index.html>Index</a></center><HR>")
                
            if len(fields)>=1 and fields[0] == pagename:
                line= line.replace(pagename,"<code>"+pagename) + "</code>"

            #line= line.replace("<[A-Z]>","<a href=protocol.html>protocol</a>")

            #m= pattern.match(line)
            #if m:
            #    print "here's a match!"
            #    print m.group(0)
            #    print m.group(1)
            #    print m.group(2)
            
            line= line.replace("\t"," "*8)
            line= line.replace(" ","&nbsp;")
            line= line.replace("<BLUEBOLD>","<font color=blue><code><b>")
            line= line.replace("</BLUEBOLD>","</b></code></font>")
            line= line.replace("{{protocol}}","<a href=protocol.pdf>protocol</a>")
            line= pattern.sub('<a href=\\1.html>\\1</a>',line)

            page.append(line)
                
            count= count + 1

        page.append("<br><br>\n")
        page.append( "<HR><i><font size=-1>Auto-generated from " +
                     "gcp/control/code/unix/control_src/bicep/specificscript.c" +
                     "</font></i></BODY>" )

        fout= open("html/" + pagename + ".html", 'w')
        fout.write(string.join(page,"<br>\n"))
        fout.close()
