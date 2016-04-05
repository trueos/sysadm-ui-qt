//===========================================
//  PC-BSD source code
//  Copyright (c) 2016, PC-BSD Software/iXsystems
//  Available under the 3-clause BSD license
//  See the LICENSE file for full details
//===========================================
#include "page_pkg.h"

QStringList pkg_page::catsToText(QStringList cats){
  QStringList out;
  for(int i=0; i<cats.length(); i++){
    if(cats[i]=="accessibility"){ out << tr("Accessibility")+"::::"+cats[i]; }
    else if(cats[i]=="arabic"){ out << tr("Arabic")+"::::"+cats[i]; }
    else if(cats[i]=="archivers"){ out << tr("Archivers")+"::::"+cats[i]; }
    else if(cats[i]=="astro"){ out << tr("Astro")+"::::"+cats[i]; }
    else if(cats[i]=="audio"){ out << tr("Audio")+"::::"+cats[i]; }
    else if(cats[i]=="benchmarks"){ out << tr("Benchmarks")+"::::"+cats[i]; }
    else if(cats[i]=="biology"){ out << tr("Biology")+"::::"+cats[i]; }
    else if(cats[i]=="cad"){ out << tr("CAD")+"::::"+cats[i]; }
    else if(cats[i]=="chinese"){ out << tr("Chinese")+"::::"+cats[i]; }
    else if(cats[i]=="comms"){ out << tr("Comms")+"::::"+cats[i]; }
    else if(cats[i]=="converters"){ out << tr("Converters")+"::::"+cats[i]; }
    else if(cats[i]=="databases"){ out << tr("Databases")+"::::"+cats[i]; }
    else if(cats[i]=="deskutils"){ out << tr("Desktop Utilities")+"::::"+cats[i]; }
    else if(cats[i]=="devel"){ out << tr("Development")+"::::"+cats[i]; }
    else if(cats[i]=="dns"){ out << tr("DNS")+"::::"+cats[i]; }
    else if(cats[i]=="editors"){ out << tr("Editors")+"::::"+cats[i]; }
    else if(cats[i]=="emulators"){ out << tr("Emulators")+"::::"+cats[i]; }
    else if(cats[i]=="finance"){ out << tr("Finance")+"::::"+cats[i]; }
    else if(cats[i]=="french"){ out << tr("French")+"::::"+cats[i]; }
    else if(cats[i]=="ftp"){ out << tr("FTP")+"::::"+cats[i]; }
    else if(cats[i]=="games"){ out << tr("Games")+"::::"+cats[i]; }
    else if(cats[i]=="german"){ out << tr("German")+"::::"+cats[i]; }
    else if(cats[i]=="graphics"){ out << tr("Graphics")+"::::"+cats[i]; }
    else if(cats[i]=="hebrew"){ out << tr("Hebrew")+"::::"+cats[i]; }
    else if(cats[i]=="hungarian"){ out << tr("Hungarian")+"::::"+cats[i]; }
    else if(cats[i]=="irc"){ out << tr("IRC")+"::::"+cats[i]; }
    else if(cats[i]=="japanese"){ out << tr("Japanese")+"::::"+cats[i]; }
    else if(cats[i]=="java"){ out << tr("Java")+"::::"+cats[i]; }
    else if(cats[i]=="korean"){ out << tr("Korean")+"::::"+cats[i]; }
    else if(cats[i]=="lang"){ out << tr("Languages")+"::::"+cats[i]; }
    else if(cats[i]=="mail"){ out << tr("Mail")+"::::"+cats[i]; }
    else if(cats[i]=="math"){ out << tr("Math")+"::::"+cats[i]; }
    else if(cats[i]=="misc"){ out << tr("Miscellaneous")+"::::"+cats[i]; }
    else if(cats[i]=="multimedia"){ out << tr("Multimedia")+"::::"+cats[i]; }
    else if(cats[i]=="net"){ out << tr("Network")+"::::"+cats[i]; }
    else if(cats[i]=="net-im"){ out << tr("Network-IM")+"::::"+cats[i]; }
    else if(cats[i]=="net-mgmt"){ out << tr("Network-Management")+"::::"+cats[i]; }
    else if(cats[i]=="net-p2p"){ out << tr("Network-P2P")+"::::"+cats[i]; }
    else if(cats[i]=="news"){ out << tr("News")+"::::"+cats[i]; }
    else if(cats[i]=="palm"){ out << tr("Palm")+"::::"+cats[i]; }
    else if(cats[i]=="polish"){ out << tr("Polish")+"::::"+cats[i]; }
    else if(cats[i]=="ports-mgmt"){ out << tr("Ports Management")+"::::"+cats[i]; }
    else if(cats[i]=="portuguese"){ out << tr("Portuguese")+"::::"+cats[i]; }
    else if(cats[i]=="print"){ out << tr("Print")+"::::"+cats[i]; }
    else if(cats[i]=="russian"){ out << tr("Russian")+"::::"+cats[i]; }
    else if(cats[i]=="science"){ out << tr("Science")+"::::"+cats[i]; }
    else if(cats[i]=="security"){ out << tr("Security")+"::::"+cats[i]; }
    else if(cats[i]=="shells"){ out << tr("Shells")+"::::"+cats[i]; }
    else if(cats[i]=="sysutils"){ out << tr("System Utilities")+"::::"+cats[i]; }
    else if(cats[i]=="textproc"){ out << tr("Text Processing")+"::::"+cats[i]; }
    else if(cats[i]=="ukrainian"){ out << tr("Ukrainian")+"::::"+cats[i]; }
    else if(cats[i]=="vietnamese"){ out << tr("Vietnamese")+"::::"+cats[i]; }
    else if(cats[i]=="www"){ out << tr("WWW")+"::::"+cats[i]; }
    else if(cats[i]=="x11"){ out << tr("X11")+"::::"+cats[i]; }
    else if(cats[i]=="x11-clocks"){ out << tr("X11-Clocks")+"::::"+cats[i]; }
    else if(cats[i]=="x11-drivers"){ out << tr("X11-Drivers")+"::::"+cats[i]; }
    else if(cats[i]=="x11-fm"){ out << tr("X11-File Managers")+"::::"+cats[i]; }
    else if(cats[i]=="x11-fonts"){ out << tr("X11-Fonts")+"::::"+cats[i]; }
    else if(cats[i]=="x11-servers"){ out << tr("X11-Servers")+"::::"+cats[i]; }
    else if(cats[i]=="x11-themes"){ out << tr("X11-Themes")+"::::"+cats[i]; }
    else if(cats[i]=="x11-toolkits"){ out << tr("X11-Toolkits")+"::::"+cats[i]; }
    else if(cats[i]=="x11-wm"){ out << tr("X11-Window Managers")+"::::"+cats[i]; }
    else{ out << cats[i]+"::::"+cats[i]; } //no translation available
  }
  out.sort();
  return out;
}