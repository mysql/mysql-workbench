#ifndef _SQL_PARSER_PUBLIC_INTERFACE_H_
#define _SQL_PARSER_PUBLIC_INTERFACE_H_

#if defined(_WIN32)

#ifdef SQL_PARSER_EXPORTS
#define SQL_PARSER_PUBLIC_FUNC __declspec(dllexport)
#else
#define SQL_PARSER_PUBLIC_FUNC __declspec(dllimport)
#endif

#else
#define SQL_PARSER_PUBLIC_FUNC
#endif

#endif // _SQL_PARSER_PUBLIC_INTERFACE_H_
