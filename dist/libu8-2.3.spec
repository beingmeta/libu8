Name:           libu8
Version:        2.3.1
Release:        12%{?dist}
Summary:        utility/compatability for Unicode and other functions

Group:          System Environment/Libraries
License:        GNU LGPL
URL:            http://www.beingmeta.com/
Source0:        libu8-2.2.7.tar.gz
BuildRoot:      %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

BuildRequires:  openssl-devel doxygen
Requires:       openssl

%description
libu8 provides portable functions for manipulating unicode as well as
wrappers/implementations for various system-level functions such as
filesystem access, time primitives, mime handling, signature
functions, etc.

%package        devel
Summary:        Development files for %{name}
Group:          Development/Libraries
Requires:       %{name} = %{version}-%{release}

%description    devel
The %{name}-devel package contains libraries and header files for
developing applications that use %{name}.

%package        encodings
Summary:        Extra character set encodings for %{name}
Group:          Development/Libraries
Requires:       %{name} = %{version}-%{release}

%description    encodings
The %{name}-encodings package contains data files for handling
various character encodings, especially for Asian languages.

%package        static
Summary:        Static libraries for %{name}
Group:          Development/Libraries
Requires:       %{name}-devel = %{version}-%{release}

%description    static
The %{name}-static package contains static libraries for
developing statically linked applications that use %{name}.
You probably don't need it.

%prep
%setup -q


%build
%configure --prefix=/usr --without-sudo
make

%install
rm -rf $RPM_BUILD_ROOT
make install install-docs DESTDIR=$RPM_BUILD_ROOT
#find $RPM_BUILD_ROOT -name '*.la' -exec rm -f {} ';'


%clean
rm -rf $RPM_BUILD_ROOT


%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig


%files
%defattr(-,root,root,-)
%doc
%{_libdir}/*.so.*
%{_datadir}/libu8/encodings/CP1125
%{_datadir}/libu8/encodings/CP1251
%{_datadir}/libu8/encodings/CP1252
%{_datadir}/libu8/encodings/CP1258
%{_datadir}/libu8/encodings/GREEK7
%{_datadir}/libu8/encodings/ISO_6937
%{_datadir}/libu8/encodings/ISO88591
%{_datadir}/libu8/encodings/ISO885910
%{_datadir}/libu8/encodings/ISO885911
%{_datadir}/libu8/encodings/ISO885913
%{_datadir}/libu8/encodings/ISO885914
%{_datadir}/libu8/encodings/ISO885915
%{_datadir}/libu8/encodings/ISO885916
%{_datadir}/libu8/encodings/ISO88592
%{_datadir}/libu8/encodings/ISO88593
%{_datadir}/libu8/encodings/ISO88594
%{_datadir}/libu8/encodings/ISO88595
%{_datadir}/libu8/encodings/ISO88597
%{_datadir}/libu8/encodings/ISO88598
%{_datadir}/libu8/encodings/ISO88599
%{_datadir}/libu8/encodings/KOI8
%{_datadir}/libu8/encodings/KOI8R
%{_datadir}/libu8/encodings/MACINTOSH


%files encodings
%defattr(-,root,root,-)
%doc
%{_datadir}/libu8/encodings/BIG5
%{_datadir}/libu8/encodings/EUCJP
%{_datadir}/libu8/encodings/EUCKR
%{_datadir}/libu8/encodings/EUCTW
%{_datadir}/libu8/encodings/GB2312
%{_datadir}/libu8/encodings/GBK
%{_datadir}/libu8/encodings/SHIFT_JIS
%{_datadir}/libu8/encodings/SHIFT_JISX0213
%{_datadir}/libu8/encodings/ARABIC
%{_datadir}/libu8/encodings/ASMO708
%{_datadir}/libu8/encodings/BIGFIVE
%{_datadir}/libu8/encodings/CHINESE
%{_datadir}/libu8/encodings/CN-BIG5
%{_datadir}/libu8/encodings/CNS11643
%{_datadir}/libu8/encodings/CP1253
%{_datadir}/libu8/encodings/CP1254
%{_datadir}/libu8/encodings/CP1255
%{_datadir}/libu8/encodings/CP1256
%{_datadir}/libu8/encodings/CP1257
%{_datadir}/libu8/encodings/CP819
%{_datadir}/libu8/encodings/CP936
%{_datadir}/libu8/encodings/CSBIG5
%{_datadir}/libu8/encodings/CSEUCJP
%{_datadir}/libu8/encodings/CSEUCKR
%{_datadir}/libu8/encodings/CSGB2312
%{_datadir}/libu8/encodings/CSISO58GB231280,ISOIR58
%{_datadir}/libu8/encodings/CSISOLATIN
%{_datadir}/libu8/encodings/CSISOLATIN2
%{_datadir}/libu8/encodings/CSISOLATIN3
%{_datadir}/libu8/encodings/CSISOLATIN4
%{_datadir}/libu8/encodings/CSISOLATIN5
%{_datadir}/libu8/encodings/CSISOLATINARABIC
%{_datadir}/libu8/encodings/CSISOLATINCYRILLIC
%{_datadir}/libu8/encodings/CSISOLATINGREEK
%{_datadir}/libu8/encodings/CSISOLATINHEBREW
%{_datadir}/libu8/encodings/CSKOI8R
%{_datadir}/libu8/encodings/CSMACINTOSH
%{_datadir}/libu8/encodings/CSSHIFTJIS
%{_datadir}/libu8/encodings/CYRILLIC
%{_datadir}/libu8/encodings/ECMA114
%{_datadir}/libu8/encodings/ECMA118
%{_datadir}/libu8/encodings/ELOT928
%{_datadir}/libu8/encodings/GREEK
%{_datadir}/libu8/encodings/GREEK8
%{_datadir}/libu8/encodings/HEBREW
%{_datadir}/libu8/encodings/IBM819
%{_datadir}/libu8/encodings/ISOIR100
%{_datadir}/libu8/encodings/ISOIR101
%{_datadir}/libu8/encodings/ISOIR109
%{_datadir}/libu8/encodings/ISOIR110
%{_datadir}/libu8/encodings/ISOIR126
%{_datadir}/libu8/encodings/ISOIR127
%{_datadir}/libu8/encodings/ISOIR138
%{_datadir}/libu8/encodings/ISOIR144
%{_datadir}/libu8/encodings/ISOIR148
%{_datadir}/libu8/encodings/L1
%{_datadir}/libu8/encodings/L2
%{_datadir}/libu8/encodings/L3
%{_datadir}/libu8/encodings/L4
%{_datadir}/libu8/encodings/L5
%{_datadir}/libu8/encodings/LATIN1
%{_datadir}/libu8/encodings/LATIN2
%{_datadir}/libu8/encodings/LATIN3
%{_datadir}/libu8/encodings/LATIN4
%{_datadir}/libu8/encodings/LATIN5
%{_datadir}/libu8/encodings/MAC
%{_datadir}/libu8/encodings/MACCENTRALEUROPE
%{_datadir}/libu8/encodings/MACCYRILLIC
%{_datadir}/libu8/encodings/MACROMAN
%{_datadir}/libu8/encodings/MS1252
%{_datadir}/libu8/encodings/MS1254
%{_datadir}/libu8/encodings/MS1255
%{_datadir}/libu8/encodings/MS1256
%{_datadir}/libu8/encodings/MS1257
%{_datadir}/libu8/encodings/MS1258
%{_datadir}/libu8/encodings/MS936
%{_datadir}/libu8/encodings/MSANSI
%{_datadir}/libu8/encodings/MSARAB
%{_datadir}/libu8/encodings/MSCYRL
%{_datadir}/libu8/encodings/MSEE
%{_datadir}/libu8/encodings/MSGREEK
%{_datadir}/libu8/encodings/MSHEBR
%{_datadir}/libu8/encodings/MSKANJI
%{_datadir}/libu8/encodings/MSTURK
%{_datadir}/libu8/encodings/SJIS
%{_datadir}/libu8/encodings/TSCII
%{_datadir}/libu8/encodings/UJIS
%{_datadir}/libu8/encodings/VISCII
%{_datadir}/libu8/encodings/WINBALTRIM
%{_datadir}/libu8/encodings/WINDOWS1250
%{_datadir}/libu8/encodings/WINDOWS1251
%{_datadir}/libu8/encodings/WINDOWS1252
%{_datadir}/libu8/encodings/WINDOWS1253
%{_datadir}/libu8/encodings/WINDOWS1254
%{_datadir}/libu8/encodings/WINDOWS1255
%{_datadir}/libu8/encodings/WINDOWS1256
%{_datadir}/libu8/encodings/WINDOWS1257
%{_datadir}/libu8/encodings/WINDOWS1258
%{_datadir}/libu8/encodings/WINDOWS936
%{_datadir}/libu8/encodings/XEUCJP

%files devel
%defattr(-,root,root,-)
%doc %{_mandir}/man3/*
%{_includedir}/*
%{_libdir}/*.so


%files static
%defattr(-,root,root,-)
%doc
%{_libdir}/*.a

%changelog
* Sun Aug 19 2012 Ken Haase <kh@beingmeta.com> 2.2
Support for more asynchronous servers.
Numerous API extensions

