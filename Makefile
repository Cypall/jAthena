# $Id: Makefile,v 1.1 2003/06/20 05:30:43 lemit Exp $

CC = gcc -pipe
PACKETDEF = -DPACKETVER=6 -DNEW_006b
#PACKETDEF = -DPACKETVER=5 -DNEW_006b
#PACKETDEF = -DPACKETVER=4 -DNEW_006b
#PACKETDEF = -DPACKETVER=3 -DNEW_006b
#PACKETDEF = -DPACKETVER=2 -DNEW_006b
#PACKETDEF = -DPACKETVER=1 -DNEW_006b

PLATFORM = $(shell uname)

MYSQL_INCLUDE = /usr/local/include/mysql
MYSQL_LIBS = /usr/local/lib/mysql

ifeq ($(findstring CYGWIN,$(PLATFORM)), CYGWIN)
OS_TYPE = -DCYGWIN -DFD_SETSIZE=4096

else
OS_TYPE =
endif

ifeq ($(findstring FreeBSD,$(PLATFORM)), FreeBSD)
MAKE = gmake
else
MAKE = make
endif

CFLAGS = -D_XOPEN_SOURCE -D_BSD_SOURCE -Wall -Wno-sign-compare -I../common $(PACKETDEF) $(OS_TYPE)

ifdef SQLFLAG
	CFLAGS += -I$(MYSQL_INCLUDE)
else
	CFLAGS += -DTXT_ONLY
endif

#Link Zlib(NOTrecommended)
#CFLAGS += -DLOCALZLIB

#debug(recommended)
CFLAGS += -g

#-------Low CPU-----
#optimize(recommended)
#CFLAGS += -O2

#-------High CPU----
#optimize(recommended)
CFLAGS += -O3

# change authfifo comparing data
#CFLAGS += -DCMP_AUTHFIFO_IP
#CFLAGS += -DCMP_AUTHFIFO_LOGIN2

# disable httpd
#CFLAGS += -DNO_HTTPD

# disable httpd-external-CGI
CFLAGS += -DNO_HTTPD_CGI

# disable csvdb
#CFLAGS += -DNO_CSVDB

# disable csvdb via script
#CFLAGS += -DNO_CSVDB_SCRIPT

# usable TKSGSL
#CFLAGS += -DTKSGSL
#CFLAGS += -DTKSGSLGSNJ

# enable dynamic memory allocation of status change data
#CFLAGS += -DDYNAMIC_SC_DATA

# enable journal system (text mode only)
#CFLAGS += -DTXT_ONLY -DTXT_JOURNAL

# account regist MailAddress
#CFLAGS += -DAC_MAIL

#-----------------CPU MARCH-------------------
#GCC 3.2.x~

#i386 (Intel)
#CFLAGS +=-march=i386 -fomit-frame-pointer

#i486 (Intel)
#CFLAGS +=-march=i486 -fomit-frame-pointer

#Pentium 1 (Intel)
#CFLAGS +=-march=pentium -fomit-frame-pointer

#Pentium MMX (Intel)
#CFLAGS +=-march=pentium-mmx -fomit-frame-pointer

#Pentium PRO (Intel)
#CFLAGS +=-march=pentiumpro -fomit-frame-pointer

#Pentium II (Intel)
#CFLAGS +=-march=pentium2 -fomit-frame-pointer

#Celeron (Mendocino), aka Celeron1 (Intel)
#CFLAGS +=-march=pentium2 -fomit-frame-pointer

#Pentium III (Intel)
#CFLAGS +=-march=pentium3 -fomit-frame-pointer

#Pentium III (Intel)
#CFLAGS +=-march=pentium3 -O3 -fomit-frame-pointer
#   -fforce-addr -falign-functions=4 -fprefetch-loop-arrays

#Celeron (Coppermine) aka Celeron2 (Intel)
#CFLAGS +=-march=pentium3 -fomit-frame-pointer

#Celeron (Willamette?) (Intel)
#CFLAGS +=-march=pentium4 -fomit-frame-pointer

#Pentium 4 (Intel)
#CFLAGS +=-march=pentium4 -fomit-frame-pointer -mfpmath=sse -msse -msse2

#optimize for pentium3
#CFLAGS += -march=i686 -mcpu=pentium3 -mfpmath=sse -mmmx -msse

#optimize for pentium4
#CFLAGS += -march=pentium4 -mfpmath=sse -msse -msse2

#Eden C3/Ezra (Via)
#CFLAGS +=-march=i586 -m3dnow -fomit-frame-pointer

#K6 (AMD)
#CFLAGS +=-march=k6 -fomit-frame-pointer

#K6-2 (AMD)
#CFLAGS +=-march=k6-2 -fomit-frame-pointer

#K6-3 (AMD)
#CFLAGS +=-march=k6-3 -fomit-frame-pointer

#Athlon (AMD)
#CFLAGS +=-march=athlon -fomit-frame-pointer

#Athlon-tbird, aka K7 (AMD)
#CFLAGS +=-march=athlon-tbird -fomit-frame-pointer

#Athlon-tbird XP (AMD)
#CFLAGS +=-march=athlon-xp -pipe -msse -mfpmath=sse -mmmx -fomit-frame-pointer

#Athlon 4(AMD)
#CFLAGS +=-march=athlon-4 -m3dnow -fomit-frame-pointer

#Athlon XP (AMD)
#CFLAGS +=-march=athlon-xp -m3dnow -msse -mfpmath=sse -mmmx -fomit-frame-pointer -m3dnow -msse -mfpmath=sse -mmmx

#Athlon MP (AMD)
#CFLAGS +=-march=athlon-mp -m3dnow -msse -mfpmath=sse -mmmx -fomit-frame-pointer

#Athlon XP XX00+
#CFLAGS +=-march=athlon-xp -m3dnow -msse -mfpmath=sse -mmmx 
#-fforce-addr -fomit-frame-pointer -funroll-loops -frerun-cse-after-loop
#-frerun-loop-opt -falign-functions=4 -maccumulate-outgoing-args -ffast-math
#-fprefetch-loop-arrays

#optimize for Athlon-4(mobile Athlon)
#CFLAGS += -march=athlon -m3dnow -msse -mcpu=athlon-4 -mfpmath=sse

#optimize for Athlon-mp
#CFLAGS += -march=athlon -m3dnow -msse -mcpu=athlon-mp -mfpmath=sse

#optimize for Athlon-xp
#CFLAGS += -march=athlon -m3dnow -msse -mcpu=athlon-xp -mfpmath=sse

#603 (PowerPC / Kuro-Box)
#CFLAGS +=-pipe

#603e (PowerPC / Kuro-Box)
#CFLAGS +=-pipe

#604 (PowerPC / Kuro-Box)
#CFLAGS +=-pipe

#604e (PowerPC / Kuro-Box)
#CFLAGS +=-pipe

#750 aka as G3 (PowerPC)
#CFLAGS +=-mcpu=750 -mpowerpc-gfxopt

#7400, aka G4 (PowerPC)
#CFLAGS +=-mcpu=7400 -maltivec
#	-mabi=altivec -mpowerpc-gfxopt
#	-maltivec -mabi=altivec -mpowerpc-gfxopt

#7450, aka G4 second generation (PowerPC)
#CFLAGS +=-mcpu=7450 -pipe
#	-maltivec -mabi=altivec -mpowerpc-gfxopt

#PowerPC (If you don't know which one)
#CFLAGS +=-mpowerpc-gfxopt

#Sparc
#CFLAGS +=-fomit-frame-pointer

#Sparc 64
#CFLAGS +=-fomit-frame-pointer

#Linux Zaurus (SL-C7xx)
#CFLAGS +=-pipe -fomit-frame-pointer -Wall -Wstrict-prototypes

#---------------------------------------------------

MKDEF = CC="$(CC)" CFLAGS="$(CFLAGS)" MYSQL_LIBS="$(MYSQL_LIBS)"

all clean: src/common/zlib/GNUmakefile src/common/GNUmakefile src/login/GNUmakefile src/char/GNUmakefile src/map/GNUmakefile src/converter/GNUmakefile
	cd src ; cd common ; $(MAKE) $(MKDEF) $@ ; cd ..
	cd src ; cd common ; cd zlib ; $(MAKE) $(MKDEF) $@ ; cd ..
	cd src ; cd login ; $(MAKE) $(MKDEF) $@ ; cd ..
	cd src ; cd char ; $(MAKE) $(MKDEF) $@ ; cd ..
	cd src ; cd converter ; $(MAKE) $(MKDEF) $@ ; cd ..
	cd src ; cd map ; $(MAKE) $(MKDEF) $@ ; cd ..

ifdef SQLFLAG
sql: src/common/zlib/GNUmakefile src/common/GNUmakefile src/login/GNUmakefile src/char/GNUmakefile src/map/GNUmakefile src/converter/GNUmakefile
	cd src ; cd common ; $(MAKE) $(MKDEF) $@ ; cd ..
	cd src ; cd common ; cd zlib ; $(MAKE) $(MKDEF) $@ ; cd ..
	cd src ; cd login ; $(MAKE) $(MKDEF) $@ SQLFLAG=1; cd ..
	cd src ; cd char ; $(MAKE) $(MKDEF) $@ SQLFLAG=1; cd ..
	cd src ; cd converter ; $(MAKE) $(MKDEF) $@ SQLFLAG=1; cd ..
	cd src ; cd map ; $(MAKE) $(MKDEF) $@ ; cd ..
else
sql:
	$(MAKE) CC="$(CC)" OPT="$(OPT)" SQLFLAG=1
endif

src/common/GNUmakefile: src/common/Makefile
	sed -e 's/$$>/$$^/' src/common/Makefile > src/common/GNUmakefile
src/common/zlib/GNUmakefile: src/common/zlib/Makefile
	sed -e 's/$$>/$$^/' src/common/zlib/Makefile > src/common/zlib/GNUmakefile
src/login/GNUmakefile: src/login/Makefile
	sed -e 's/$$>/$$^/' src/login/Makefile > src/login/GNUmakefile
src/char/GNUmakefile: src/char/Makefile
	sed -e 's/$$>/$$^/' src/char/Makefile > src/char/GNUmakefile
src/map/GNUmakefile: src/map/Makefile
	sed -e 's/$$>/$$^/' src/map/Makefile > src/map/GNUmakefile
src/converter/GNUmakefile: src/converter/Makefile
	sed -e 's/$$>/$$^/' src/converter/Makefile > src/converter/GNUmakefile
