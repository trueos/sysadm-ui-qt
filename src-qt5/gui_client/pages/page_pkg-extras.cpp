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
    if(cats[i]=="accessibility"){ out << tr("")+":::"+cats[i]; }
    else if(cats[i]=="arabic"){ out << tr("")+":::"+cats[i]; }
    else if(cats[i]=="archivers"){ out << tr("")+":::"+cats[i]; }
    else if(cats[i]=="astro"){ out << tr("")+":::"+cats[i]; }
    else if(cats[i]=="audio"){ out << tr("")+":::"+cats[i]; }
    else if(cats[i]=="benchmarks"){ out << tr("")+":::"+cats[i]; }
    else if(cats[i]=="biology"){ out << tr("")+":::"+cats[i]; }
    else if(cats[i]=="cad"){ out << tr("")+":::"+cats[i]; }
    else if(cats[i]=="chinese"){ out << tr("")+":::"+cats[i]; }
    else if(cats[i]=="comms"){ out << tr("")+":::"+cats[i]; }
    else if(cats[i]=="converters"){ out << tr("")+":::"+cats[i]; }
    else if(cats[i]=="databases"){ out << tr("")+":::"+cats[i]; }
    else if(cats[i]=="deskutils"){ out << tr("")+":::"+cats[i]; }
    else if(cats[i]=="devel"){ out << tr("")+":::"+cats[i]; }
    else if(cats[i]=="dns"){ out << tr("")+":::"+cats[i]; }
    else if(cats[i]=="editors"){ out << tr("")+":::"+cats[i]; }
    else if(cats[i]=="emulators"){ out << tr("")+":::"+cats[i]; }
    else if(cats[i]=="finance"){ out << tr("")+":::"+cats[i]; }
    else if(cats[i]=="french"){ out << tr("")+":::"+cats[i]; }
    else if(cats[i]=="ftp"){ out << tr("")+":::"+cats[i]; }
    else if(cats[i]=="games"){ out << tr("")+":::"+cats[i]; }
    else if(cats[i]=="german"){ out << tr("")+":::"+cats[i]; }
    else if(cats[i]=="graphics"){ out << tr("")+":::"+cats[i]; }
    else if(cats[i]=="hebrew"){ out << tr("")+":::"+cats[i]; }
    else if(cats[i]=="hungarian"){ out << tr("")+":::"+cats[i]; }
    else if(cats[i]=="irc"){ out << tr("")+":::"+cats[i]; }
    else if(cats[i]=="japanese"){ out << tr("")+":::"+cats[i]; }
    else if(cats[i]=="java"){ out << tr("")+":::"+cats[i]; }
    else if(cats[i]=="korean"){ out << tr("")+":::"+cats[i]; }
    else if(cats[i]=="lang"){ out << tr("")+":::"+cats[i]; }
    else if(cats[i]=="mail"){ out << tr("")+":::"+cats[i]; }
    else if(cats[i]=="math"){ out << tr("")+":::"+cats[i]; }
    else if(cats[i]=="misc"){ out << tr("")+":::"+cats[i]; }
    else if(cats[i]=="multimedia"){ out << tr("")+":::"+cats[i]; }
    else if(cats[i]=="net"){ out << tr("")+":::"+cats[i]; }
    else if(cats[i]=="net-im"){ out << tr("")+":::"+cats[i]; }
    else if(cats[i]=="net-mgmt"){ out << tr("")+":::"+cats[i]; }
    else if(cats[i]=="net-p2p"){ out << tr("")+":::"+cats[i]; }
    else if(cats[i]=="news"){ out << tr("")+":::"+cats[i]; }
    else if(cats[i]=="palm"){ out << tr("")+":::"+cats[i]; }
    else if(cats[i]=="polish"){ out << tr("")+":::"+cats[i]; }
    else if(cats[i]=="ports-mgmt"){ out << tr("")+":::"+cats[i]; }
    else if(cats[i]=="portuguese"){ out << tr("")+":::"+cats[i]; }
    else if(cats[i]=="print"){ out << tr("")+":::"+cats[i]; }
    else if(cats[i]=="russian"){ out << tr("")+":::"+cats[i]; }
    else if(cats[i]=="science"){ out << tr("")+":::"+cats[i]; }
    else if(cats[i]=="security"){ out << tr("")+":::"+cats[i]; }
    else if(cats[i]=="shells"){ out << tr("")+":::"+cats[i]; }
    else if(cats[i]=="sysutils"){ out << tr("")+":::"+cats[i]; }
    else if(cats[i]=="textproc"){ out << tr("")+":::"+cats[i]; }
    else if(cats[i]=="ukrainian"){ out << tr("")+":::"+cats[i]; }
    else if(cats[i]=="vietnamese"){ out << tr("")+":::"+cats[i]; }
    else if(cats[i]=="www"){ out << tr("")+":::"+cats[i]; }
    else if(cats[i]=="x11"){ out << tr("")+":::"+cats[i]; }
    else if(cats[i]=="x11-clocks"){ out << tr("")+":::"+cats[i]; }
    else if(cats[i]=="x11-drivers"){ out << tr("")+":::"+cats[i]; }
    else if(cats[i]=="x11-fm"){ out << tr("")+":::"+cats[i]; }
    else if(cats[i]=="x11-fonts"){ out << tr("")+":::"+cats[i]; }
    else if(cats[i]=="x11-servers"){ out << tr("")+":::"+cats[i]; }
    else if(cats[i]=="x11-themes"){ out << tr("")+":::"+cats[i]; }
    else if(cats[i]=="x11-toolkits"){ out << tr("")+":::"+cats[i]; }
    else if(cats[i]=="x11-wm"){ out << tr("")+":::"+cats[i]; }
    else{ out << cats[i]+":::"+cats[i]; } //no translation available
  }
  out.sort();
  return out;
}