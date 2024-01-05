#ifndef __CLIPP_DEFINES_HEADER__
#define __CLIPP_DEFINES_HEADER__

#define __namespace_CLIPP CLIPP
#define __CLIPP __namespace_CLIPP

#define __CLIPP_begin namespace __namespace_CLIPP {
#define __CLIPP_end }

#define SET_CLIPP_ALIAS(alias) namespace alias = __namespace_CLIPP

#endif //! __CLIPP_DEFINES_HEADER__